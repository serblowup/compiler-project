#!/bin/bash

TEST_DIR=$1
EXPECTED_DIR=$2
CATEGORY=$3

if [ -z "$TEST_DIR" ] || [ -z "$EXPECTED_DIR" ]; then
    echo "Использование: $0 <директория_тестов> <директория_ожидаемых_результатов> [категория]"
    echo "Пример: $0 tests/parser/valid tests/parser/valid/expected PARSER-VALID"
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

echo "Тестирование $CATEGORY"

for test_file in "$TEST_DIR"/*.src; do
    if [ ! -f "$test_file" ]; then
        continue
    fi
    
    if [[ "$(basename "$test_file")" == "roundtrip.src" ]]; then
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
    
    ./compiler parse --input "$test_file" --output "$output_file" 2> "$error_file"
    
    if [[ "$CATEGORY" == *"INVALID"* ]]; then
        if [ -s "$error_file" ]; then
            if diff -u "$expected_file" "$error_file" > /dev/null 2>&1; then
                echo "  [${CATEGORY}] ${filename}: успешно (ошибка обнаружена)"
                PASSED=$((PASSED + 1))
            else
                echo "  [${CATEGORY}] ${filename}: неудача - неверный вывод ошибки"
                echo "    Различия:"
                diff -u "$expected_file" "$error_file" | sed 's/^/      /'
                FAILED=$((FAILED + 1))
            fi
        elif grep -q "Ошибка синтаксиса" "$output_file" 2>/dev/null; then
            grep "Ошибка синтаксиса" "$output_file" > "${output_file}.errors"
            if diff -u "$expected_file" "${output_file}.errors" > /dev/null 2>&1; then
                echo "  [${CATEGORY}] ${filename}: успешно (ошибка обнаружена)"
                PASSED=$((PASSED + 1))
            else
                echo "  [${CATEGORY}] ${filename}: неудача - неверный вывод ошибки"
                echo "    Различия:"
                diff -u "$expected_file" "${output_file}.errors" | sed 's/^/      /'
                FAILED=$((FAILED + 1))
            fi
        else
            echo "  [${CATEGORY}] ${filename}: неудача - ошибка не обнаружена"
            FAILED=$((FAILED + 1))
        fi
    else
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
            echo "  [${CATEGORY}] ${filename}: успешно!"
            PASSED=$((PASSED + 1))
        else
            echo "  [${CATEGORY}] ${filename}: неудача!!!"
            echo "    Различия:"
            diff -u "$expected_file" "$output_file" | sed 's/^/      /'
            FAILED=$((FAILED + 1))
        fi
    fi
done

if [[ "$CATEGORY" == *"VALID"* ]]; then
    echo "  Запуск round-trip теста"
    
    roundtrip_file="$TEST_DIR/roundtrip.src"
    if [ -f "$roundtrip_file" ]; then
        TOTAL=$((TOTAL + 1))
        
        roundtrip_ast1="test_output/roundtrip_ast1.txt"
        roundtrip_ast2="test_output/roundtrip_ast2.txt"
        roundtrip_error="test_output/roundtrip.err"
        
        echo "    Шаг 1: Парсинг исходного файла -> AST1"
        ./compiler parse --input "$roundtrip_file" --output "$roundtrip_ast1" 2> "$roundtrip_error"
        
        if [ $? -eq 0 ] && [ -f "$roundtrip_ast1" ]; then
            echo "    Шаг 2: Повторный парсинг того же файла -> AST2"
            ./compiler parse --input "$roundtrip_file" --output "$roundtrip_ast2" 2>> "$roundtrip_error"
            
            if [ $? -eq 0 ] && [ -f "$roundtrip_ast2" ]; then
                echo "    Шаг 3: Сравнение AST1 и AST2"
                if diff -u "$roundtrip_ast1" "$roundtrip_ast2" > /dev/null; then
                    echo "  [${CATEGORY}] roundtrip: успешно! (AST эквивалентны)"
                    PASSED=$((PASSED + 1))
                else
                    echo "  [${CATEGORY}] roundtrip: неудача - AST различаются!!!"
                    echo "    Различия:"
                    diff -u "$roundtrip_ast1" "$roundtrip_ast2" | sed 's/^/      /'
                    FAILED=$((FAILED + 1))
                fi
            else
                echo "  [${CATEGORY}] roundtrip: неудача - ошибка при повторном парсинге"
                if [ -s "$roundtrip_error" ]; then
                    echo "    Сообщение об ошибке:"
                    cat "$roundtrip_error" | sed 's/^/      /'
                fi
                FAILED=$((FAILED + 1))
            fi
        else
            echo "  [${CATEGORY}] roundtrip: неудача - ошибка при первом парсинге"
            if [ -s "$roundtrip_error" ]; then
                echo "    Сообщение об ошибке:"
                cat "$roundtrip_error" | sed 's/^/      /'
            fi
            FAILED=$((FAILED + 1))
        fi
    fi
fi

echo "Результаты тестирования $CATEGORY"
echo "  Всего: $TOTAL"
echo "  Успешно: $PASSED"
echo "  Провалено: $FAILED"

if [ $FAILED -gt 0 ]; then
    exit 1
else
    exit 0
fi
