# Компилятор C-подобного языка

## Описание проекта
Учебный проект компилятора для упрощенного C-подобного языка.

## Команда
Студент группы ИСБ-123 Савченко С.Ю.

## Требования
- C++17 (разработка на gcc 15.2.1)
- Make (разработка на GNU Make 4.4.1)
- NASM (разработка на NASM 3.01)
- ld (GNU Binutils 2.46)
- lcov (разработка на LCOV version 2.4-0)
- graphviz (разработка на graphviz version 14.1.2)

## Сборка проекта

```bash
# Сборка проекта
make build

# Очистка
make clean

# Запуск всех тестов
make test

# Запуск только тестов лексера
make test-lexer

# Запуск только тестов препроцессора
make test-preproc

# Запуск только тестов парсера
make test-parser

# Запуск семантических тестов
make test-semantic

# Запуск тестов промежуточного представления
make test-ir

# Запуск тестов кодогенерации
make test-codegen

# Запуск интеграционных тестов
make test-integration

# Отчет о покрытии кода (coverage)
make coverage
```

## Использование

### Команды лексера и препроцессора
```bash
# Запуск лексера
./compiler lex --input <файл> --output <файл>

# Запуск только препроцессора
./compiler preprocess --input <файл> --output <файл>
```

### Команды парсера
```bash
# Запуск парсера
./compiler parse --input <файл> --output <файл> [опции]
```

#### Опции парсера

| Опция | Описание |
|-------|----------|
| `--format txt/dot/json` | Формат вывода AST (по умолчанию - txt) |
| `--verbose` | Подробный вывод процесса разбора |
| `--max-errors <число>` | Максимальное количество ошибок (по умолчанию - 100) |

### Команды семантического анализа
```bash
# Запуск семантического анализа
./compiler check --input <файл> --output <файл> [опции]
```

#### Опции семантического анализа

| Опция | Описание |
|-------|----------|
| `--format txt/dot/json` | Формат вывода AST (по умолчанию - txt) |
| `--dump-symbols` | Вывести таблицу символов |
| `--show-types` | Показать аннотации типов в AST |
| `--symbol-format text/json` | Формат вывода таблицы символов (по умолчанию - txt) |
| `--verbose` | Подробный вывод (включает отчёт о валидации) |
| `--max-errors <число>` | Максимальное количество ошибок (по умолчанию - 100) |

### Команды генерации промежуточного представления
```bash
# Запуск генерации промежуточного представления
./compiler ir --input <файл> --output <файл> [опции]
```

#### Опции генерации промежуточного представления

| Опция | Описание |
|-------|----------|
| `--ir-format text/dot/json` | Формат вывода IR (по умолчанию - txt) |
| `--optimize` | Включить оптимизации |
| `--stats` | Показать статистику IR |
| `--verbose` | Подробный вывод |

#### Форматы вывода промежуточного представления

- **text** — человекочитаемый текстовый формат
- **dot** — формат Graphviz для визуализации графа потока управления (CFG)
- **json** — машиночитаемый JSON формат

### Команды генерации x86-64 ассемблера
```bash
# Генерация x86-64 ассемблера с полной оптимизацией (LSRA + Peephole)
./compiler compile --input <файл> --output <файл>

# Только с оптимизацией LSRA (без оконной оптимизации)
./compiler compile --input <файл> --output <файл> --no-peephole

# Без оптимизаций (чистый код)
./compiler compile --input <файл> --output <файл> --no-lsra
```

#### Опции кодогенерации

| Опция | Описание |
|-------|----------|
| `--target <архитектура>` | Целевая архитектура (по умолчанию - x86_64) |
| `--no-lsra` | Отключить LSRA и оконную оптимизацию |
| `--no-peephole` | Отключить только оконную оптимизацию (LSRA остаётся) |
| `--verbose` | Подробный вывод |

#### Ассемблирование и линковка

```bash
# Генерация ассемблера
./compiler compile --input program.src --output program.asm

# Ассемблирование
nasm -f elf64 program.asm -o program.o
nasm -f elf64 src/runtime/runtime.asm -o runtime.o

# Линковка
ld -o program runtime.o program.o

# Запуск
./program
echo $?  # Код возврата
```

## Архитектура компилятора

Компилятор работает в несколько этапов:

1. **Препроцессор** — обработка директив `#define`, `#ifdef`, удаление комментариев
2. **Лексер** — разбиение исходного кода на токены
3. **Парсер** — построение AST (Abstract Syntax Tree)
4. **Семантический анализ** — проверка типов, построение таблицы символов
5. **IR-генератор** — трансляция AST в промежуточное представление (SSA-форма) с оптимизациями: пропуск избыточной инициализации `i = 0` в циклах `for`, прямая подстановка выражения в `RETURN`
6. **Оптимизатор IR** — свёртка констант, алгебраические упрощения, удаление мёртвого кода
7. **Генератор кода** — трансляция IR в x86-64 ассемблер с распределением регистров (Linear Scan) и оконной оптимизацией (Peephole)

## System V AMD64 ABI

Компилятор следует соглашениям System V AMD64 ABI:

- Первые 6 целочисленных аргументов передаются через регистры: `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`
- Аргументы с плавающей точкой: `xmm0`-`xmm7`
- Возвращаемое значение (целое): `rax`
- Стек выравнивается по 16 байтам перед вызовом функций
- Сохраняются callee-saved регистры: `rbx`, `r12`-`r15`

Подробнее в docs/assembler.md.

## Примеры

### Лексер и препроцессор
```bash
# Лексический анализ
./compiler lex --input examples/hello.src --output tokens.txt

# Только препроцессор
./compiler preprocess --input examples/hello.src --output hello_srs.txt
```

### Парсер
```bash
# Парсер выводит AST в текстовом формате
./compiler parse --input examples/factorial.src --output ast.txt

# Парсер выводит AST в DOT формате
./compiler parse --input examples/factorial.src --format dot --output ast.dot

# Визуализация AST с помощью Graphviz
dot -Tpng ast.dot -o ast.png

# Парсер выводит AST в JSON формате
./compiler parse --input examples/factorial.src --format json --output ast.json

# Подробный вывод с ограничением ошибок
./compiler parse --input examples/hello.src --verbose --max-errors 50 --output ast.txt
```

### Семантический анализ
```bash
# Семантический анализ
./compiler check --input examples/factorial.src --output result.txt

# Анализ с таблицей символов
./compiler check --input examples/factorial.src --output result.txt --dump-symbols

# Анализ с аннотациями типов
./compiler check --input examples/factorial.src --output result.txt --show-types

# Полный вывод
./compiler check --input examples/factorial.src --output result.txt --dump-symbols --show-types --verbose

# Таблица символов в JSON формате
./compiler check --input examples/factorial.src --dump-symbols --symbol-format json --output symbols.json
```

### Генерация промежуточного представления

**Пример исходного кода:**
```c
fn main() -> int {
    int n = 5;
    int result = 1;

    while (n > 0) {
        result = result * n;
        n = n - 1;
    }

    return result;
}
```

**Сгенерированное промежуточное представление:**
```assembly
function main: int ()
  entry:
      n_1 = 5
      result_1 = 1
      JUMP L1
  L1:
      n_2 = PHI (n_1, entry), (n_3, L2)
      result_2 = PHI (result_1, entry), (result_3, L2)
      t1 = CMP_GT n_2, 0
      JUMP_IF_NOT t1, L3
      JUMP L2
  L2:
      t2 = MUL result_2, n_2
      result_3 = t2
      t3 = SUB n_2, 1
      n_3 = t3
      JUMP L1
  L3:
      RETURN result_2
```

**Команды для работы с IR:**
```bash
# Генерация IR в текстовом формате
./compiler ir --input examples/factorial.src --output factorial.ir

# Генерация IR с оптимизациями
./compiler ir --input examples/factorial.src --output factorial_opt.ir --optimize

# Генерация IR со статистикой
./compiler ir --input examples/factorial.src --output factorial_stats.ir --stats

# Генерация IR в формате DOT для визуализации CFG
./compiler ir --input examples/factorial.src --output factorial.dot --ir-format dot

# Визуализация CFG с помощью Graphviz
dot -Tpng factorial.dot -o factorial_cfg.png

# Генерация IR в формате JSON
./compiler ir --input examples/factorial.src --output factorial.json --ir-format json
```

### Генерация x86-64 ассемблера

**Пример исходного кода (`add.src`):**
```c
fn add(int a, int b) -> int {
    return a + b;
}

fn main() -> int {
    return add(10, 20);
}
```

**Сгенерированный ассемблер с полной оптимизацией (`add.asm`):**
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

**Пример с режимами оптимизации:**
```bash
# Полная оптимизация (LSRA + Peephole) — по умолчанию
./compiler compile --input examples/add.src --output add_full.asm

# Только LSRA
./compiler compile --input examples/add.src --output add_lsra.asm --no-peephole

# Без оптимизаций
./compiler compile --input examples/add.src --output add_no_opt.asm --no-lsra
```

**Полный цикл компиляции и запуска:**
```bash
# Генерация ассемблера
./compiler compile --input examples/add.src --output add.asm

# Ассемблирование
nasm -f elf64 add.asm -o add.o
nasm -f elf64 src/runtime/runtime.asm -o runtime.o

# Линковка
ld -o add_program runtime.o add.o

# Запуск
./add_program
echo $?  # Выведет: 30
```

## Рантайм-библиотека

Компилятор включает минимальную рантайм-библиотеку (`src/runtime/runtime.asm`):

| Функция | Описание |
|---------|----------|
| `print_int` | Вывод целого числа в stdout |
| `print_string` | Вывод строки в stdout |
| `read_int` | Чтение целого числа из stdin |
| `exit` | Завершение программы с указанным кодом |
| `_start` | Точка входа (вызывает `main`) |

## Документация

- [Спецификация языка](docs/language_spec.md)
- [Грамматика языка](docs/grammar.md)
- [Семантический анализ](docs/semantic.md)
- [Промежуточное представление](docs/ir.md)
- [Генерация x86-64 ассемблера](docs/assembler.md)

## Структура проекта

```
compiler-project/
├── docs # Документация
│   ├── assembler.md
│   ├── grammar.md
│   ├── ir.md
│   ├── language_spec.md
│   └── semantic.md
├── examples # Примеры исходного кода
│   ├── factorial.src
│   ├── fibonacci.src
│   └── hello.src
├── Makefile # Makefile
├── README.md # Readme
├── src
│   ├── codegen # Генерация x86-64 ассемблера
│   │   ├── abi.cpp
│   │   ├── abi.hpp
│   │   ├── peephole_optimizer.cpp
│   │   ├── peephole_optimizer.hpp
│   │   ├── register_allocator.cpp
│   │   ├── register_allocator.hpp
│   │   ├── stack_frame.cpp
│   │   ├── stack_frame.hpp
│   │   ├── x86_generator.cpp
│   │   └── x86_generator.hpp
│   ├── ir # Промежуточное представление
│   │   ├── ir.cpp
│   │   ├── ir_generator.cpp
│   │   ├── ir_generator.hpp
│   │   ├── ir.hpp
│   │   ├── ir_optimizer.cpp
│   │   ├── ir_optimizer.hpp
│   │   ├── ir_printer.cpp
│   │   └── ir_printer.hpp
│   ├── lexer # Лексер
│   │   ├── lexer.cpp
│   │   ├── lexer.hpp
│   │   └── tokens.hpp
│   ├── main.cpp
│   ├── parser # Парсер
│   │   ├── ast.cpp
│   │   ├── ast.hpp
│   │   ├── ast_visitor.cpp
│   │   ├── ast_visitor.hpp
│   │   ├── ast_visualizer.cpp
│   │   ├── ast_visualizer.hpp
│   │   ├── error.hpp
│   │   ├── parser.cpp
│   │   └── parser.hpp
│   ├── preprocessor # Препроцессор
│   │   ├── preprocessor.cpp
│   │   └── preprocessor.hpp
│   ├── runtime # Рантайм-библиотека
│   │   └── runtime.asm
│   ├── semantic # Семантический анализ
│   │   ├── semantic_analyzer.cpp
│   │   ├── semantic_analyzer.hpp
│   │   ├── semantic_error.hpp
│   │   ├── symbol.hpp
│   │   ├── symbol_table.hpp
│   │   ├── type_checker.hpp
│   │   └── type.hpp
│   └── utils # Вспомогательные утилиты
│       ├── cli.cpp
│       ├── cli.hpp
│       ├── file_io.cpp
│       └── file_io.hpp
└── tests
    ├── codegen # Тесты кодогенерации
    │   ├── invalid
    │   │   ├── assembly_errors
    │   │   │   └── undeclared_func.src
    │   │   └── expected
    │   │       └── undeclared_func.txt
    │   └── valid
    │       ├── arithmetic_ops
    │       │   ├── complex_expression.src
    │       │   ├── divide.src
    │       │   ├── modulo.src
    │       │   ├── multiply.src
    │       │   ├── simple_add.src
    │       │   └── subtract.src
    │       ├── control_flow
    │       │   ├── for_loop.src
    │       │   ├── simple_if.src
    │       │   └── while_loop.src
    │       ├── expected
    │       │   ├── complex_expression.txt
    │       │   ├── conditional.txt
    │       │   ├── divide.txt
    │       │   ├── factorial.txt
    │       │   ├── fibonacci.txt
    │       │   ├── for_loop.txt
    │       │   ├── modulo.txt
    │       │   ├── multiple_params.txt
    │       │   ├── multiply.txt
    │       │   ├── nested_call.txt
    │       │   ├── simple_add.txt
    │       │   ├── simple_call.txt
    │       │   ├── simple_if.txt
    │       │   ├── subtract.txt
    │       │   └── while_loop.txt
    │       ├── function_calls
    │       │   ├── multiple_params.src
    │       │   ├── nested_call.src
    │       │   └── simple_call.src
    │       └── integration
    │           ├── conditional.src
    │           ├── factorial.src
    │           └── fibonacci.src
    ├── ir # Тесты промежуточного представления
    │   ├── generation
    │   │   ├── arithmetic.src
    │   │   ├── comparisons.src
    │   │   ├── expected
    │   │   │   ├── arithmetic.txt
    │   │   │   ├── comparisons.txt
    │   │   │   ├── factorial.txt
    │   │   │   ├── for.txt
    │   │   │   ├── functions.txt
    │   │   │   ├── if_else.txt
    │   │   │   ├── logical.txt
    │   │   │   └── while.txt
    │   │   ├── factorial.src
    │   │   ├── for.src
    │   │   ├── functions.src
    │   │   ├── if_else.src
    │   │   ├── logical.src
    │   │   └── while.src
    │   ├── optimization
    │   │   ├── algebraic_simplify.src
    │   │   ├── constant_folding.src
    │   │   ├── dead_code.src
    │   │   └── expected
    │   │       ├── algebraic_simplify.txt
    │   │       ├── constant_folding.txt
    │   │       └── dead_code.txt
    │   └── validation
    │       ├── expected
    │       │   ├── type_mismatch.txt
    │       │   ├── undeclared_variable.txt
    │       │   └── wrong_operator_types.txt
    │       ├── type_mismatch.src
    │       ├── undeclared_variable.src
    │       └── wrong_operator_types.src
    ├── lexer # Тесты лексера
    │   ├── invalid
    │   │   ├── expected
    │   │   │   ├── test_backslash.txt
    │   │   │   ├── test_invalid_char.txt
    │   │   │   ├── test_invalid_utf8.txt
    │   │   │   ├── test_malformed_number.txt
    │   │   │   ├── test_mixed_invalid_chars.txt
    │   │   │   ├── test_special_chars.txt
    │   │   │   ├── test_unclosed_char.txt
    │   │   │   ├── test_unclosed_string.txt
    │   │   │   ├── test_unicode_invalid.txt
    │   │   │   └── test_unterminated_string.txt
    │   │   ├── test_backslash.src
    │   │   ├── test_invalid_char.src
    │   │   ├── test_invalid_utf8.src
    │   │   ├── test_malformed_number.src
    │   │   ├── test_mixed_invalid_chars.src
    │   │   ├── test_special_chars.src
    │   │   ├── test_unclosed_char.src
    │   │   ├── test_unclosed_string.src
    │   │   ├── test_unicode_invalid.src
    │   │   └── test_unterminated_string.src
    │   └── valid
    │       ├── expected
    │       │   ├── test_all_delimiters.txt
    │       │   ├── test_array_ops.txt
    │       │   ├── test_boolean_ops.txt
    │       │   ├── test_comments.txt
    │       │   ├── test_comparisons_chain.txt
    │       │   ├── test_complex_expression.txt
    │       │   ├── test_edge_values.txt
    │       │   ├── test_function_decl.txt
    │       │   ├── test_identifiers.txt
    │       │   ├── test_keywords.txt
    │       │   ├── test_long_identifier.txt
    │       │   ├── test_mixed.txt
    │       │   ├── test_multiple_stmts.txt
    │       │   ├── test_negative_numbers.txt
    │       │   ├── test_nested_parens.txt
    │       │   ├── test_numbers.txt
    │       │   ├── test_operators.txt
    │       │   ├── test_simple_math.txt
    │       │   ├── test_string_escapes.txt
    │       │   └── test_ternary_op.txt
    │       ├── test_all_delimiters.src
    │       ├── test_array_ops.src
    │       ├── test_boolean_ops.src
    │       ├── test_comments.src
    │       ├── test_comparisons_chain.src
    │       ├── test_complex_expression.src
    │       ├── test_edge_values.src
    │       ├── test_function_decl.src
    │       ├── test_identifiers.src
    │       ├── test_keywords.src
    │       ├── test_long_identifier.src
    │       ├── test_mixed.src
    │       ├── test_multiple_stmts.src
    │       ├── test_negative_numbers.src
    │       ├── test_nested_parens.src
    │       ├── test_numbers.src
    │       ├── test_operators.src
    │       ├── test_simple_math.src
    │       ├── test_string_escapes.src
    │       └── test_ternary_op.src
    ├── parser # Тесты парсера
    │   ├── invalid
    │   │   ├── bad_expression.src
    │   │   ├── expected
    │   │   │   ├── bad_expression.txt
    │   │   │   ├── invalid_var_name.txt
    │   │   │   ├── mismatched_parens.txt
    │   │   │   ├── missing_brace.txt
    │   │   │   ├── missing_function_name.txt
    │   │   │   └── missing_semicolon.txt
    │   │   ├── invalid_var_name.src
    │   │   ├── mismatched_parens.src
    │   │   ├── missing_brace.src
    │   │   ├── missing_function_name.src
    │   │   └── missing_semicolon.src
    │   └── valid
    │       ├── arithmetic.src
    │       ├── comparison.src
    │       ├── complex_priority.src
    │       ├── double_semicolon.src
    │       ├── expected
    │       │   ├── arithmetic.txt
    │       │   ├── comparison.txt
    │       │   ├── complex_priority.txt
    │       │   ├── double_semicolon.txt
    │       │   ├── for.txt
    │       │   ├── if_else.txt
    │       │   ├── logical.txt
    │       │   ├── missing_return_type.txt
    │       │   ├── roundtrip.txt
    │       │   ├── simple.txt
    │       │   ├── struct.txt
    │       │   ├── variables.txt
    │       │   └── while.txt
    │       ├── for.src
    │       ├── if_else.src
    │       ├── logical.src
    │       ├── missing_return_type.src
    │       ├── roundtrip.src
    │       ├── simple.src
    │       ├── struct.src
    │       ├── variables.src
    │       └── while.src
    ├── preprocessor # Тесты препроцессора
    │   ├── invalid
    │   │   ├── expected
    │   │   │   └── test_unterminated_comment.txt
    │   │   └── test_unterminated_comment.src
    │   └── valid
    │       ├── expected
    │       │   └── test_define.txt
    │       └── test_define.src
    ├── scripts
    │   ├── check_coverage.sh
    │   ├── run_all_tests.sh
    │   ├── test_codegen.sh
    │   ├── test_integration.sh
    │   ├── test_ir.sh
    │   ├── test_lexer.sh
    │   ├── test_parser.sh
    │   ├── test_preprocessor.sh
    │   └── test_semantic.sh
    └── semantic # Тесты семантического анализа
        ├── invalid
        │   ├── argument_count_mismatch.src
        │   ├── argument_type_mismatch.src
        │   ├── duplicate_function.src
        │   ├── duplicate_variable.src
        │   ├── expected
        │   │   ├── argument_count_mismatch.txt
        │   │   ├── argument_type_mismatch.txt
        │   │   ├── duplicate_function.txt
        │   │   ├── duplicate_variable.txt
        │   │   ├── return_in_void.txt
        │   │   ├── scope_error.txt
        │   │   ├── struct_duplicate_field.txt
        │   │   ├── struct_self_reference.txt
        │   │   ├── struct_undeclared_type.txt
        │   │   ├── type_mismatch_assign.txt
        │   │   ├── type_mismatch_return.txt
        │   │   ├── undeclared_variable.txt
        │   │   ├── void_variable.txt
        │   │   └── wrong_operator_types.txt
        │   ├── return_in_void.src
        │   ├── scope_error.src
        │   ├── struct_duplicate_field.src
        │   ├── struct_self_reference.src
        │   ├── struct_undeclared_type.src
        │   ├── type_mismatch_assign.src
        │   ├── type_mismatch_return.src
        │   ├── undeclared_variable.src
        │   ├── void_variable.src
        │   └── wrong_operator_types.src
        └── valid
            ├── complex_expressions.src
            ├── control_flow.src
            ├── expected
            │   ├── complex_expressions.txt
            │   ├── control_flow.txt
            │   ├── function_calls.txt
            │   ├── nested_scopes.txt
            │   ├── return_statements.txt
            │   └── type_compatibility.txt
            ├── function_calls.src
            ├── nested_scopes.src
            ├── return_statements.src
            └── type_compatibility.src
```