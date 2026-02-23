#!/bin/bash

TEST_DIR=$1
EXPECTED_DIR=$2
CATEGORY=$3

if [ -z "$TEST_DIR" ] || [ -z "$EXPECTED_DIR" ]; then
    echo "Использование: $0 <директория_тестов> <директория_ожидаемых_результатов> [категория]"
    echo "Пример: $0 tests/lexer/valid tests/lexer/valid/expected LEXER-VALID"
    exit 1
fi

if [ ! -d "$TEST_DIR" ]; then
    echo "Ошибка: Директория $TEST_DIR не существует"
    exit 1
fi

if [ ! -d "$EXPECTED_DIR" ]; then
    echo "Ошибка: Директория $EXPECTED_DIR не существует"
    exit 1
fi

# Создаем директорию для выходных файлов
mkdir -p test_output

TOTAL=0
PASSED=0
FAILED=0

echo -e "\n Тестирование $CATEGORY \n"

for test_file in "$TEST_DIR"/*.src; do
    if [ ! -f "$test_file" ]; then
        continue
    fi
    
    TOTAL=$((TOTAL + 1))
    
    filename=$(basename "$test_file")
    basename="${filename%.*}"
    
    expected_file="$EXPECTED_DIR/$basename.txt"
    output_file="test_output/${basename}_lexer.out"
    error_file="test_output/${basename}_lexer.err"
    
    if [ ! -f "$expected_file" ]; then
        echo "  [${CATEGORY}] ${filename}: ОШИБКА - отсутствует ожидаемый файл $expected_file"
        FAILED=$((FAILED + 1))
        continue
    fi
    
    # Запускаем лексер
    ./compiler lex --input "$test_file" --output "$output_file" > /dev/null 2> "$error_file"
    
    # Проверяем, создался ли выходной файл
    if [ ! -f "$output_file" ]; then
        echo "  [${CATEGORY}] ${filename}: ОШИБКА - выходной файл не создан"
        if [ -s "$error_file" ]; then
            echo "    Сообщение об ошибке:"
            cat "$error_file" | sed 's/^/      /'
        fi
        FAILED=$((FAILED + 1))
        continue
    fi
    
    # Для invalid тестов проверяем наличие ошибок в выводе
    if [[ "$CATEGORY" == *"INVALID"* ]]; then
        # Проверяем, есть ли токены ERROR в выходном файле
        if grep -q "ERROR" "$output_file"; then
            # Сравниваем полный вывод с ожидаемым
            if diff -u "$expected_file" "$output_file" > /dev/null; then
                echo "  [${CATEGORY}] ${filename}: УСПЕШНО"
                PASSED=$((PASSED + 1))
            else
                echo "  [${CATEGORY}] ${filename}: НЕУДАЧА"
                echo "    Различия:"
                diff -u "$expected_file" "$output_file" | sed 's/^/      /'
                FAILED=$((FAILED + 1))
            fi
        else
            echo "  [${CATEGORY}] ${filename}: НЕУДАЧА - ошибка не обнаружена"
            FAILED=$((FAILED + 1))
        fi
    else
        # Для valid тестов просто сравниваем вывод
        if diff -u "$expected_file" "$output_file" > /dev/null; then
            echo "  [${CATEGORY}] ${filename}: УСПЕШНО"
            PASSED=$((PASSED + 1))
        else
            echo "  [${CATEGORY}] ${filename}: НЕУДАЧА"
            echo "    Различия:"
            diff -u "$expected_file" "$output_file" | sed 's/^/      /'
            FAILED=$((FAILED + 1))
        fi
    fi
done

echo -e "\n Результаты тестирования $CATEGORY "
echo "  Всего: $TOTAL"
echo "  Успешно: $PASSED"
echo "  Провалено: $FAILED"
echo ""

if [ $FAILED -gt 0 ]; then
    exit 1
else
    exit 0
fi