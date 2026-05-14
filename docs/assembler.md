# Генерация x86-64 ассемблера

## Обзор

Модуль кодогенерации транслирует промежуточное представление (IR) в ассемблерный код x86-64, следуя соглашениям System V AMD64 ABI. Результатом является файл в синтаксисе NASM, который можно ассемблировать и слинковать с рантайм-библиотекой для получения исполняемого файла.

Включает алгоритм распределения регистров **Linear Scan** для эффективного использования регистров общего назначения, а также **Peephole-оптимизации** для улучшения сгенерированного кода.

Поддерживаются три режима генерации кода:
- **По умолчанию** — LSRA + Peephole 
- **`--no-peephole`** — только LSRA, без оконной оптимизации
- **`--no-lsra`** — без оптимизаций

## Архитектура модуля

```
src/codegen/
├── abi.hpp / abi.cpp # Константы System V AMD64 ABI
├── register_allocator.hpp / register_allocator.cpp # Алгоритм Linear Scan
├── stack_frame.hpp / stack_frame.cpp # Управление стековым кадром
├── peephole_optimizer.hpp / peephole_optimizer.cpp # Оконная оптимизация
├── label_manager.hpp / label_manager.cpp # Управление метками
├── control_flow_generator.hpp / control_flow_generator.cpp # Генерация переходов
├── expression_generator.hpp / expression_generator.cpp # Генерация выражений
├── x86_generator.hpp / x86_generator.cpp # Генератор ассемблерного кода
src/runtime/
└── runtime.asm # Рантайм-библиотека
```

## System V AMD64 ABI

### Соглашение о вызовах

| Параметр | Регистр |
|----------|--------|
| 1-й целочисленный аргумент | `rdi` |
| 2-й целочисленный аргумент | `rsi` |
| 3-й целочисленный аргумент | `rdx` |
| 4-й целочисленный аргумент | `rcx` |
| 5-й целочисленный аргумент | `r8` |
| 6-й целочисленный аргумент | `r9` |
| 1-8 аргументы с плавающей точкой | `xmm0`-`xmm7` |
| Аргументы сверх 6 (целые) | Стек (справа налево) |
| Возвращаемое значение (целое) | `rax` |
| Возвращаемое значение (float) | `xmm0` |

### Регистры

| Категория | Регистры |
|-----------|----------|
| **Caller-saved** (сохраняет вызывающий) | `rax`, `rcx`, `rdx`, `rsi`, `rdi`, `r8`, `r9`, `r10`, `r11` |
| **Callee-saved** (сохраняет вызываемый) | `rbx`, `rsp`, `rbp`, `r12`, `r13`, `r14`, `r15` |
| **Специальные** | `rsp` (указатель стека), `rbp` (указатель кадра), `rip` (указатель инструкций) |

### Стековый кадр

```
Высокие адреса
+-----------------+
| Аргументы > 6   |  [rbp + 16 + N*8]
+-----------------+
| Return Address  |  [rbp + 8]
+-----------------+
| Сохранённый rbp |  [rbp]  ← rbp указывает сюда
+-----------------+
| Локальная пер. 1|  [rbp - 8]
+-----------------+
| Локальная пер. 2|  [rbp - 16]
+-----------------+
| ...             |
+-----------------+
| Spill-слоты     |  (для вытесненных регистров)
+-----------------+
| Красная зона    |  [rsp - 128] ... [rsp] (128 байт)
+-----------------+
Низкие адреса    ← rsp указывает сюда
```

### Выравнивание стека

- Стек должен быть выровнен по **16 байтам** перед инструкцией `call`.
- При входе в функцию стек выровнен по 8 байт (return address на вершине).
- Для выравнивания используется `sub rsp, N` где N кратно 16.
- Минимальный размер стекового кадра - 16 байт (для пустых функций).

### Красная зона (Red Zone)

- 128 байт ниже `rsp` зарезервированы для использования **листовыми функциями** (теми, которые не делают вызовов).
- В нелистовых функциях красная зона не используется, так как `call` может перезаписать её.

## Этапы кодогенерации

### 1. Анализ стекового кадра (`StackFrameAnalyzer`)

Для каждой IR-функции вычисляется:
- Размер каждой локальной переменной и параметра (в байтах)
- Необходимое выравнивание
- Общий размер стекового кадра (с учётом spill-слотов)
- Список сохраняемых callee-saved регистров

```cpp
StackFrame frame = StackFrameAnalyzer::analyze(func);
```

### 2. Распределение регистров (`RegisterAllocator`)

Алгоритм **Linear Scan** распределяет доступные регистры (`eax`, `ecx`, `edx`, `esi`, `edi`, `r8d`-`r11d`) между переменными на основе их диапазонов жизни (live ranges):

- Сбор диапазонов жизни из IR-инструкций
- Группировка SSA-версий одной переменной в общий стековый слот
- Многоверсионные переменные (пользовательские) всегда размещаются в стеке
- Одиночные временные переменные (`t1`, `t2`, ...) распределяются по регистрам
- При нехватке регистров вытесняется интервал с наибольшим временем жизни (farthest)

```cpp
reg_alloc.collectLiveRanges(func);
reg_alloc.allocateRegisters(func);
frame.setSpillSlotsSize(reg_alloc.getTotalSpillSize());
```

### 3. Генерация пролога

```asm
push rbp                ; Сохранить базовый указатель вызывающего
mov rbp, rsp            ; Установить новый базовый указатель
sub rsp, N              ; Выделить N байт для локальных переменных и spill-слотов
push rbx                ; Сохранить callee-saved регистры (если нужно)
push r12
push r13
mov [rbp-4], edi        ; Сохранить параметр a в локальный слот
mov [rbp-8], esi        ; Сохранить параметр b в локальный слот
```

Параметры функции сохраняются из регистров ABI в соответствующие слоты стекового кадра. Для каждого параметра также мапятся все его SSA-версии (например, `a_1`, `a_2` → `[rbp-4]`).

### 4. Генерация инструкций

Каждая IR-инструкция отображается на одну или несколько инструкций x86-64. Генератор применяет ряд оптимизаций на лету:

- **Прямая проверка регистров** — для `JUMP_IF`/`JUMP_IF_NOT`, если операнд уже в регистре, используется `test reg, reg` вместо `mov eax, reg; test eax, eax`
- **Прямая пересылка регистр-регистр** — если оба операнда `MOVE` в регистрах, используется `mov reg1, reg2` вместо `mov eax, reg2; mov reg1, eax`
- **Прямой возврат после CALL** — если `RETURN` идёт сразу после `CALL` и возвращается та же временная переменная, значение остаётся в `eax` без лишних пересылок

| IR инструкция | x86-64 |
|---------------|--------|
| `MOVE dest, const` | `mov dest, const` |
| `ADD dest, src1, src2` | `mov eax, src1` → `add eax, src2` → `mov dest, eax` |
| `SUB dest, src1, src2` | `mov eax, src1` → `sub eax, src2` → `mov dest, eax` |
| `MUL dest, src1, src2` | `mov eax, src1` → `imul eax, src2` → `mov dest, eax` |
| `DIV dest, src1, src2` | `mov eax, src1` → `cdq` → `mov ecx, src2` → `idiv ecx` → `mov dest, eax` |
| `MOD dest, src1, src2` | `mov eax, src1` → `cdq` → `mov ecx, src2` → `idiv ecx` → `mov dest, edx` |
| `NEG dest, src1` | `neg eax` → `mov dest, eax` |
| `CMP_EQ dest, src1, src2` | `mov eax, src1` → `cmp eax, src2` → `sete al` → `movzx eax, al` → `mov dest, eax` |
| `CMP_NE dest, src1, src2` | `mov eax, src1` → `cmp eax, src2` → `setne al` → `movzx eax, al` → `mov dest, eax` |
| `CMP_LT dest, src1, src2` | `mov eax, src1` → `cmp eax, src2` → `setl al` → `movzx eax, al` → `mov dest, eax` |
| `CMP_LE dest, src1, src2` | `mov eax, src1` → `cmp eax, src2` → `setle al` → `movzx eax, al` → `mov dest, eax` |
| `CMP_GT dest, src1, src2` | `mov eax, src1` → `cmp eax, src2` → `setg al` → `movzx eax, al` → `mov dest, eax` |
| `CMP_GE dest, src1, src2` | `mov eax, src1` → `cmp eax, src2` → `setge al` → `movzx eax, al` → `mov dest, eax` |
| `AND dest, src1, src2` | `mov eax, src1` → `and eax, src2` → `mov dest, eax` |
| `OR dest, src1, src2` | `mov eax, src1` → `or eax, src2` → `mov dest, eax` |
| `NOT dest, src1` | `mov eax, src1` → `xor eax, 1` → `mov dest, eax` |
| `JUMP label` | `jmp label` |
| `JUMP_IF cond, label` | `test cond, cond` → `jnz label` (если в регистре) |
| `JUMP_IF_NOT cond, label` | `test cond, cond` → `jz label` (если в регистре) |
| `CALL dest, func, args` | Загрузка аргументов в RDI, RSI, ... → `call func` → `mov dest, eax` |
| `RETURN value` | `mov eax, value` (если не в eax) → эпилог → `ret` |
| `PHI dest, (v1, b1), (v2, b2)` | Не генерирует код — разрешается через маппинг имён |
| `PARAM index, dest` | Не генерирует код — обрабатывается в прологе |
| `LOAD dest, src` | `mov eax, src` → `mov dest, eax` |
| `STORE src, dest` | `mov eax, src` → `mov dest, eax` |

### 5. Генерация эпилога

```asm
pop r13                 ; Восстановить callee-saved регистры (в обратном порядке)
pop r12
pop rbx
mov rsp, rbp            ; Восстановить указатель стека
pop rbp                 ; Восстановить базовый указатель
ret                     ; Возврат к вызывающему
```

Callee-saved регистры восстанавливаются в порядке, обратном сохранению.

### 6. Peephole-оптимизация (`PeepholeOptimizer`)

После генерации кода применяется оконная оптимизация для улучшения качества ассемблера:

- **Удаление точных дубликатов** — `mov r1, r2; mov r1, r2` → удаление второй инструкции
- **Удаление избыточных загрузок** — `mov [mem], eax; mov eax, [mem]` → удаление загрузки
- **Замена add/sub на inc/dec** — `add reg, 1` → `inc reg`, `sub reg, 1` → `dec reg`
- **Замена mov на xor** — `mov reg, 0` → `xor reg, reg`
- **Удаление test перед jump** — `add eax, ebx; test eax, eax; jnz L` → `add eax, ebx; jnz L`
- **Замена умножения на степень двойки** — `imul reg, 2` → `shl reg, 1`
- **Удаление бесполезных переходов** — `jmp L1; L1:` → удаление `jmp`
- **Удаление самоприсваиваний** — `mov reg, reg` → удаление

### 7. Генерация управляющих конструкций

Генератор кода поддерживает все основные управляющие конструкции:

**Условные операторы (if/else):**
```asm
    cmp eax, 0
    jle .Lelse
    ; then-блок
    jmp .Lendif
.Lelse:
    ; else-блок
.Lendif:
```

**Циклы (while/for):**
```asm
.Lloop_cond:
    cmp eax, 10
    jge .Lloop_end
    ; тело цикла
    jmp .Lloop_cond
.Lloop_end:
```

**Логические операторы с короткой схемой (&&, ||):**
- `&&` — если левый операнд ложен, переход к false-ветке без вычисления правого
- `||` — если левый операнд истинен, переход к true-ветке без вычисления правого

**Break/Continue:**
- `break` — безусловный переход к метке выхода из цикла
- `continue` — безусловный переход к метке обновления/проверки условия

### 8. Обработка SSA-переменных

SSA-переменные (с суффиксами `_1`, `_2`...) отображаются на их базовые имена:

- `a_1`, `a_2` → `a` (один слот в стеке)
- `t1`, `t2` → временные слоты в стеке или регистры

Аллокатор группирует все SSA-версии одной переменной в общий стековый слот, а Phi-функции разрешаются через маппинг имён без генерации дополнительного кода.

### 9. Разрешение операндов

Генератор кода использует `var_mapping` для отслеживания соответствия между IR-переменными и их физическим расположением (регистр или слот в стеке). Для каждой переменной приоритет поиска:

1. Точное совпадение в `var_mapping`
2. Базовое имя в `var_mapping` (для SSA-версий)
3. Регистровый аллокатор (`getReg` / `getMem`)
4. `StackFrame` (`fallbackGetOperand`)

Это гарантирует, что каждая переменная всегда ссылается на правильное смещение в стеке, независимо от количества SSA-версий.

## Рантайм-библиотека (`runtime.asm`)

### Функции

| Функция | Сигнатура | Описание |
|---------|-----------|----------|
| `print_int` | `void print_int(int)` | Вывод целого числа в stdout |
| `print_string` | `void print_string(char*)` | Вывод строки в stdout |
| `read_int` | `int read_int()` | Чтение целого числа из stdin |
| `exit` | `void exit(int)` | Завершение программы |
| `_start` | `void _start()` | Точка входа, вызывает `main` |

### Системные вызовы Linux

| Вызов | Номер (rax) | Аргументы |
|-------|-------------|-----------|
| `read` | 0 | `rdi`=fd, `rsi`=buf, `rdx`=count |
| `write` | 1 | `rdi`=fd, `rsi`=buf, `rdx`=count |
| `exit` | 60 | `rdi`=code |

## Использование

### Компиляция в ассемблер

```bash
# Полная оптимизация (по умолчанию) - LSRA + Peephole
./compiler compile --input program.src --output program.asm

# Только LSRA, без оконной оптимизации
./compiler compile --input program.src --output program.asm --no-peephole

# Без оптимизаций 
./compiler compile --input program.src --output program.asm --no-lsra
```

### Ассемблирование

```bash
nasm -f elf64 program.asm -o program.o
```

### Компоновка с рантаймом

```bash
nasm -f elf64 src/runtime/runtime.asm -o runtime.o
ld -o program runtime.o program.o
```

### Запуск

```bash
./program
echo $?  # Код возврата
```

## Пример полного цикла

### Сложение двух чисел (`add.src`)

**Исходный код:**
```c
fn add(int a, int b) -> int {
    return a + b;
}

fn main() -> int {
    return add(10, 20);
}
```

**Сгенерированный ассемблер (с полной оптимизацией):**
```asm
; x86-64 Assembly generated by MiniCompiler
; System V AMD64 ABI compliant
; Register allocation: Linear Scan
; Peephole optimization: enabled

section .text

global add
add:
    ; Function prologue: add
    push rbp
    mov rbp, rsp
    sub rsp, 16
    ; Allocated 16 bytes for locals
    mov [rbp-4], edi
    mov [rbp-8], esi
    mov eax, dword [rbp-4]
    add eax, dword [rbp-8]
    ; Function epilogue: add
    mov rsp, rbp
    pop rbp
    ret

global main
main:
    ; Function prologue: main
    push rbp
    mov rbp, rsp
    sub rsp, 16
    ; Allocated 16 bytes for locals
    mov edi, 10
    mov esi, 20
    call add
    ; Function epilogue: main
    mov rsp, rbp
    pop rbp
    ret
```

**Запуск:**
```bash
$ ./compiler compile --input add.src --output add.asm
$ nasm -f elf64 add.asm -o add.o
$ nasm -f elf64 src/runtime/runtime.asm -o runtime.o
$ ld -o add_program runtime.o add.o
$ ./add_program
$ echo $?
30
```