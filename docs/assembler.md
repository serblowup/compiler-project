# Генерация x86-64 ассемблера

## Обзор

Модуль кодогенерации транслирует промежуточное представление (IR) в ассемблерный код x86-64, следуя соглашениям System V AMD64 ABI. Результатом является файл в синтаксисе NASM, который можно ассемблировать и слинковать с рантайм-библиотекой для получения исполняемого файла.

Включает алгоритм распределения регистров **Linear Scan** для эффективного использования регистров общего назначения.

## Архитектура модуля

```
src/codegen/
├── abi.hpp / abi.cpp # Константы ABI и распределение регистров (Linear Scan)
├── stack_frame.hpp / stack_frame.cpp # Управление стековым кадром и spill-слотами
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
- Распределение регистров с вытеснением (spill) при нехватке

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

### 4. Генерация инструкций

Каждая IR-инструкция отображается на одну или несколько инструкций x86-64:

| IR инструкция | x86-64 |
|---------------|--------|
| `MOVE dest, const` | `mov dest, const` |
| `ADD dest, src1, src2` | `mov eax, src1` → `add eax, src2` → `mov dest, eax` |
| `SUB dest, src1, src2` | `mov eax, src1` → `sub eax, src2` → `mov dest, eax` |
| `MUL dest, src1, src2` | `mov eax, src1` → `imul eax, src2` → `mov dest, eax` |
| `DIV dest, src1, src2` | `mov eax, src1` → `cdq` → `idiv src2` → `mov dest, eax` |
| `MOD dest, src1, src2` | `mov eax, src1` → `cdq` → `idiv src2` → `mov dest, edx` |
| `CMP_EQ dest, src1, src2` | `mov eax, src1` → `cmp eax, src2` → `sete al` → `movzx eax, al` → `mov dest, eax` |
| `JUMP label` | `jmp label` |
| `JUMP_IF cond, label` | `test eax, eax` → `jnz label` |
| `JUMP_IF_NOT cond, label` | `test eax, eax` → `jz label` |
| `CALL dest, func, args` | Загрузка аргументов в RDI, RSI, ... → `call func` → `mov dest, eax` |
| `RETURN value` | `mov eax, value` → эпилог → `ret` |
| `PHI dest, (v1, b1), (v2, b2)` | Не генерирует код — разрешается через маппинг имён |

### 5. Генерация эпилога

```asm
pop r13                 ; Восстановить callee-saved регистры (в обратном порядке)
pop r12
pop rbx
mov rsp, rbp            ; Восстановить указатель стека
pop rbp                 ; Восстановить базовый указатель
ret                     ; Возврат к вызывающему
```

### 6. Обработка SSA-переменных

SSA-переменные (с суффиксами `_1`, `_2`...) отображаются на их базовые имена:

- `a_1`, `a_2` → `a` (один слот в стеке)
- `t1`, `t2` → временные слоты в стеке или регистры

Аллокатор группирует все SSA-версии одной переменной в общий стековый слот, а Phi-функции разрешаются через маппинг имён без генерации дополнительного кода.

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
./compiler compile --input program.src --output program.asm
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

### Исходный код (`add.src`)

```c
fn add(int a, int b) -> int {
    return a + b;
}

fn main() -> int {
    return add(10, 20);
}
```

### Сгенерированный ассемблер (`add.asm`)

```asm
; x86-64 Assembly generated by MiniCompiler
; System V AMD64 ABI compliant
; Register allocation: Linear Scan

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
    mov dword [rbp-4], eax
    mov eax, dword [rbp-4]
    ; Function epilogue: main
    mov rsp, rbp
    pop rbp
    ret
```
