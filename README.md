# Компилятор C-подобного языка

## Описание проекта
Учебный проект компилятора для упрощенного C-подобного языка.

## Команда
Студент группы ИСБ-123 Савченко С.Ю.

## Требования
- C++17 (разработка на gcc 15.2.1)
- Make (разработка на GNU Make 4.4.1)
- lcov (разработка на LCOV version 2.4-0)

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

# Отчет о покрытии кода (coverage)
make coverage
```

### Использование
```bash
# Запуск лексера
./compiler lex --input <файл> --output <файл>

# Запуск только препроцессора
./compiler preprocess --input <файл> --output <файл>
```

#### Примеры
```bash
# Лексический анализ
./compiler lex --input examples/hello.src --output tokens.txt

# Только препроцессор
./compiler preprocess --input examples/hello.src --output hello_srs.txt
```

#### Спецификация языка

Лексическая грамматика описана в файле docs/language_spec.md.

##### Структура проекта

```
compiler_project
├── docs # Документация
│   └── language_spec.md
├── examples # Примеры исходного кода
│   └── hello.src
├── Makefile # Makefile
├── README.md # Readme
├── src
│   ├── lexer  # Лексер
│   │   ├── lexer.cpp
│   │   ├── lexer.hpp
│   │   └── tokens.hpp
│   ├── main.cpp
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
        ├── check_coverage.sh # Запуск тестов с покрытием кода
        ├── run_all_tests.sh # Запуск всех тестов
        ├── test_lexer.sh  # Тесты лексера
        └── test_preprocessor.sh  # Тесты препроцессора
```
