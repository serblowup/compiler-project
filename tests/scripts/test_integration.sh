#!/bin/bash

echo "Запуск интеграционных тестов (примеры из examples/)"

INTEGRATION_DIR="examples"
TOTAL=0
PASSED=0
FAILED=0

mkdir -p test_output/integration

for test_file in "$INTEGRATION_DIR"/*.src; do
    if [ ! -f "$test_file" ]; then
        continue
    fi
    
    TOTAL=$((TOTAL + 1))
    
    filename=$(basename "$test_file")
    basename="${filename%.*}"
    
    echo "Тестирование $filename"
    
    output_file="test_output/integration/${basename}_ast.txt"
    error_file="test_output/integration/${basename}.err"
    
    ./compiler parse --input "$test_file" --output "$output_file" 2> "$error_file"
    
    if [ -s "$error_file" ]; then
        echo "  Ошибка при парсинге $filename"
        echo "    Сообщение об ошибке:"
        cat "$error_file" | sed 's/^/      /'
        FAILED=$((FAILED + 1))
    elif [ ! -f "$output_file" ]; then
        echo "  AST не создан для $filename"
        FAILED=$((FAILED + 1))
    else
        echo "  Успешно: AST сохранен в $output_file"
        PASSED=$((PASSED + 1))
    fi
done

echo "Результаты интеграционных тестов"
echo "  Всего: $TOTAL"
echo "  Успешно: $PASSED"
echo "  Провалено: $FAILED"

if [ $FAILED -gt 0 ]; then
    exit 1
else
    exit 0
fi
