# Компилятор C-подобного языка

## Описание проекта
Учебный проект компилятора для упрощенного C-подобного языка.

## Команда
Студент группы ИСБ-123 Савченко С.Ю.

## Требования
- C++17 (разработка на gcc 15.2.1)
- Make (разработка на GNU Make 4.4.1)
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

# Запуск интеграционных тестов (спринт 2)
make test-integration

# Отчет о покрытии кода (coverage)
make coverage
```

### Использование
```bash
# Запуск лексера
./compiler lex --input <файл> --output <файл>

# Запуск только препроцессора
./compiler preprocess --input <файл> --output <файл>

# Запуск парсера
./compiler parse --input <файл> --output <файл> [опции]
```

#### Опции парсера

| Опция | Описание |
|-------|----------|
| `--format txt|dot|json` | Формат вывода AST (по умолчанию - txt) |
| `--verbose` | Подробный вывод процесса разбора |
| `--max-errors <число>` | Максимальное количество ошибок (по умолчанию - 100) |

#### Примеры
```bash
# Лексический анализ
./compiler lex --input examples/hello.src --output tokens.txt

# Только препроцессор
./compiler preprocess --input examples/hello.src --output hello_srs.txt

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

#### Спецификация языка

Лексическая грамматика описана в файле docs/language_spec.md.

#### Грамматика языка

LL(1)-грамматика языка описана в файле docs/grammar.md.

##### Структура проекта

```
compiler_project/
├── docs # Документация
│   ├── grammar.md
│   └── language_spec.md
├── examples # Примеры исходного кода
│   ├── factorial.src
│   └── hello.src
├── Makefile # Makefile
├── README.md # Readme
├── src
│   ├── lexer # Лексер
│   │   ├── lexer.cpp
│   │   ├── lexer.hpp
│   │   └── tokens.hpp
│   ├── main.cpp
│   ├── parser # Парсер
│   │   ├── ast.cpp
│   │   ├── ast.hpp
│   │   ├── ast_visitor.cpp
│   │   ├── ast_visitor.hpp
│   │   ├── ast_visualizer.cpp
│   │   ├── ast_visualizer.hpp
│   │   ├── error.hpp
│   │   ├── parser.cpp
│   │   └── parser.hpp
│   ├── preprocessor # Препроцессор
│   │   ├── preprocessor.cpp
│   │   └── preprocessor.hpp
│   └── utils # Вспомогательные утилиты
│       ├── cli.cpp
│       ├── cli.hpp
│       ├── file_io.cpp
│       └── file_io.hpp
└── tests
    ├── lexer # Тесты лексера
    │   ├── invalid
    │   │   ├── expected
    │   │   │   ├── test_backslash.txt
    │   │   │   ├── test_invalid_char.txt
    │   │   │   ├── test_invalid_utf8.txt
    │   │   │   ├── test_malformed_number.txt
    │   │   │   ├── test_mixed_invalid_chars.txt
    │   │   │   ├── test_special_chars.txt
    │   │   │   ├── test_unclosed_char.txt
    │   │   │   ├── test_unclosed_string.txt
    │   │   │   ├── test_unicode_invalid.txt
    │   │   │   └── test_unterminated_string.txt
    │   │   ├── test_backslash.src
    │   │   ├── test_invalid_char.src
    │   │   ├── test_invalid_utf8.src
    │   │   ├── test_malformed_number.src
    │   │   ├── test_mixed_invalid_chars.src
    │   │   ├── test_special_chars.src
    │   │   ├── test_unclosed_char.src
    │   │   ├── test_unclosed_string.src
    │   │   ├── test_unicode_invalid.src
    │   │   └── test_unterminated_string.src
    │   └── valid
    │       ├── expected
    │       │   ├── test_all_delimiters.txt
    │       │   ├── test_array_ops.txt
    │       │   ├── test_boolean_ops.txt
    │       │   ├── test_comments.txt
    │       │   ├── test_comparisons_chain.txt
    │       │   ├── test_complex_expression.txt
    │       │   ├── test_edge_values.txt
    │       │   ├── test_function_decl.txt
    │       │   ├── test_identifiers.txt
    │       │   ├── test_keywords.txt
    │       │   ├── test_long_identifier.txt
    │       │   ├── test_mixed.txt
    │       │   ├── test_multiple_stmts.txt
    │       │   ├── test_negative_numbers.txt
    │       │   ├── test_nested_parens.txt
    │       │   ├── test_numbers.txt
    │       │   ├── test_operators.txt
    │       │   ├── test_simple_math.txt
    │       │   ├── test_string_escapes.txt
    │       │   └── test_ternary_op.txt
    │       ├── test_all_delimiters.src
    │       ├── test_array_ops.src
    │       ├── test_boolean_ops.src
    │       ├── test_comments.src
    │       ├── test_comparisons_chain.src
    │       ├── test_complex_expression.src
    │       ├── test_edge_values.src
    │       ├── test_function_decl.src
    │       ├── test_identifiers.src
    │       ├── test_keywords.src
    │       ├── test_long_identifier.src
    │       ├── test_mixed.src
    │       ├── test_multiple_stmts.src
    │       ├── test_negative_numbers.src
    │       ├── test_nested_parens.src
    │       ├── test_numbers.src
    │       ├── test_operators.src
    │       ├── test_simple_math.src
    │       ├── test_string_escapes.src
    │       └── test_ternary_op.src
    ├── parser # Тесты парсера
    │   ├── invalid
    │   │   ├── bad_expression.src
    │   │   ├── expected
    │   │   │   ├── bad_expression.txt
    │   │   │   ├── invalid_var_name.txt
    │   │   │   ├── mismatched_parens.txt
    │   │   │   ├── missing_brace.txt
    │   │   │   ├── missing_function_name.txt
    │   │   │   └── missing_semicolon.txt
    │   │   ├── invalid_var_name.src
    │   │   ├── mismatched_parens.src
    │   │   ├── missing_brace.src
    │   │   ├── missing_function_name.src
    │   │   └── missing_semicolon.src
    │   └── valid
    │       ├── arithmetic.src
    │       ├── comparison.src
    │       ├── complex_priority.src
    │       ├── double_semicolon.src
    │       ├── expected
    │       │   ├── arithmetic.txt
    │       │   ├── comparison.txt
    │       │   ├── complex_priority.txt
    │       │   ├── double_semicolon.txt
    │       │   ├── for.txt
    │       │   ├── if_else.txt
    │       │   ├── logical.txt
    │       │   ├── missing_return_type.txt
    │       │   ├── roundtrip.txt
    │       │   ├── simple.txt
    │       │   ├── struct.txt
    │       │   ├── variables.txt
    │       │   └── while.txt
    │       ├── for.src
    │       ├── if_else.src
    │       ├── logical.src
    │       ├── missing_return_type.src
    │       ├── roundtrip.src
    │       ├── simple.src
    │       ├── struct.src
    │       ├── variables.src
    │       └── while.src
    ├── preprocessor # Тесты препроцессора
    │   ├── invalid
    │   │   ├── expected
    │   │   │   └── test_unterminated_comment.txt
    │   │   └── test_unterminated_comment.src
    │   └── valid
    │       ├── expected
    │       │   └── test_define.txt
    │       └── test_define.src
    └── scripts
        ├── check_coverage.sh
        ├── run_all_tests.sh
        ├── test_integration.sh
        ├── test_lexer.sh
        ├── test_parser.sh
        └── test_preprocessor.sh
```
