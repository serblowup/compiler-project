#!/bin/bash

if [ ! -d "tests/ir/generation" ] && [ ! -d "tests/ir/optimization" ] && [ ! -d "tests/ir/validation" ]; then
    echo "Ошибка: Директории с тестами IR не найдены"
    exit 1
fi

TOTAL=0
PASSED=0
FAILED=0

run_tests() {
    local TEST_DIR=$1
    local EXPECTED_DIR=$2
    local CATEGORY=$3
    local OPTIMIZE_FLAG=$4
    local CHECK_ERRORS=$5
    
    if [ ! -d "$TEST_DIR" ]; then
        return
    fi
    
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
        
        if [ "$CHECK_ERRORS" = "true" ]; then
            ./compiler ir --input "$test_file" --output /dev/null 2> "$error_file"
            
            if diff -u "$expected_file" "$error_file" > /dev/null; then
                echo "  [${CATEGORY}] ${filename}: Успешно"
                PASSED=$((PASSED + 1))
            else
                echo "  [${CATEGORY}] ${filename}: Неудача"
                echo "    Различия:"
                diff -u "$expected_file" "$error_file" | sed 's/^/      /'
                FAILED=$((FAILED + 1))
            fi
        else
            if [ "$OPTIMIZE_FLAG" = "true" ]; then
                ./compiler ir --input "$test_file" --output "$output_file" --optimize 2> "$error_file"
            else
                ./compiler ir --input "$test_file" --output "$output_file" 2> "$error_file"
            fi
            
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
                echo "  [${CATEGORY}] ${filename}: Успешно"
                PASSED=$((PASSED + 1))
            else
                echo "  [${CATEGORY}] ${filename}: Неудача"
                echo "    Различия:"
                diff -u "$expected_file" "$output_file" | sed 's/^/      /'
                FAILED=$((FAILED + 1))
            fi
        fi
    done
}

mkdir -p test_output

echo "Запуск тестов IR"
echo ""

run_tests "tests/ir/generation" "tests/ir/generation/expected" "IR-GENERATION" "false" "false"
run_tests "tests/ir/optimization" "tests/ir/optimization/expected" "IR-OPTIMIZATION" "true" "false"
run_tests "tests/ir/validation" "tests/ir/validation/expected" "IR-VALIDATION" "false" "true"

echo ""
echo "Результаты тестирования IR"
echo "  Всего: $TOTAL"
echo "  Успешно: $PASSED"
echo "  Провалено: $FAILED"

if [ $FAILED -gt 0 ]; then
    exit 1
else
    exit 0
fi
