#!/bin/bash

TEST_DIR=$1
EXPECTED_DIR=$2
CATEGORY=$3

if [ -z "$TEST_DIR" ] || [ -z "$EXPECTED_DIR" ]; then
    echo "Использование: $0 <директория_тестов> <директория_ожидаемых_результатов> [категория]"
    echo "Пример: $0 tests/semantic/valid tests/semantic/valid/expected SEMANTIC-VALID"
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

mkdir -p test_output

TOTAL=0
PASSED=0
FAILED=0

echo -e "\n Тестирование $CATEGORY\n"

for test_file in "$TEST_DIR"/*.src; do
    if [ ! -f "$test_file" ]; then
        continue
    fi
    
    TOTAL=$((TOTAL + 1))
    
    filename=$(basename "$test_file")
    basename="${filename%.*}"
    
    expected_file="$EXPECTED_DIR/$basename.txt"
    output_file="test_output/${basename}.txt"
    error_file="test_output/${basename}.err"
    
    if [ ! -f "$expected_file" ]; then
        echo "  [${CATEGORY}] ${filename}: ошибка - отсутствует ожидаемый файл $expected_file"
        FAILED=$((FAILED + 1))
        continue
    fi
    
    if [[ "$CATEGORY" == *"INVALID"* ]]; then
        ./compiler check --input "$test_file" --output /dev/null 2> "$error_file"
        
        if [ -s "$error_file" ]; then
            rm -f "$output_file" 2>/dev/null
            
            if diff -u "$expected_file" "$error_file" > /dev/null 2>&1; then
                echo "  [${CATEGORY}] ${filename}: успешно (ошибка обнаружена)"
                PASSED=$((PASSED + 1))
            else
                echo "  [${CATEGORY}] ${filename}: неудача - неверный вывод ошибки"
                echo "    Различия:"
                diff -u "$expected_file" "$error_file" | sed 's/^/      /'
                FAILED=$((FAILED + 1))
            fi
        else
            echo "  [${CATEGORY}] ${filename}: неудача - ошибка не обнаружена"
            FAILED=$((FAILED + 1))
        fi
    else
        ./compiler check --input "$test_file" --dump-symbols --show-types --output "$output_file" 2> "$error_file"
        
        if [ ! -f "$output_file" ]; then
            echo "  [${CATEGORY}] ${filename}: ошибка - выходной файл не создан"
            if [ -s "$error_file" ]; then
                echo "    Сообщение об ошибке:"
                cat "$error_file" | sed 's/^/      /'
            fi
            FAILED=$((FAILED + 1))
            continue
        fi
        
        if [ -s "$error_file" ]; then
            echo "  [${CATEGORY}] ${filename}: неудача - есть сообщения об ошибках"
            echo "    Сообщения:"
            cat "$error_file" | sed 's/^/      /'
            FAILED=$((FAILED + 1))
        elif diff -u "$expected_file" "$output_file" > /dev/null; then
            echo "  [${CATEGORY}] ${filename}: успешно"
            PASSED=$((PASSED + 1))
        else
            echo "  [${CATEGORY}] ${filename}: неудача"
            echo "    Различия:"
            diff -u "$expected_file" "$output_file" | sed 's/^/      /'
            FAILED=$((FAILED + 1))
        fi
    fi
done

echo -e "\n Результаты тестирования $CATEGORY"
echo "  Всего: $TOTAL"
echo "  Успешно: $PASSED"
echo "  Провалено: $FAILED"

if [ $FAILED -gt 0 ]; then
    exit 1
else
    exit 0
fi