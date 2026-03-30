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
    error_file="test_output/integration/${basename}_parse.err"
    
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
        echo "  Парсер: успешно (AST сохранен в $output_file)"
        
        sem_output_file="test_output/integration/${basename}_semantic.txt"
        sem_error_file="test_output/integration/${basename}_semantic.err"
        
        ./compiler check --input "$test_file" --dump-symbols --show-types --output "$sem_output_file" 2> "$sem_error_file"
        
        if [ -s "$sem_error_file" ]; then
            echo "  Ошибка при семантическом анализе $filename"
            echo "    Сообщение об ошибке:"
            cat "$sem_error_file" | sed 's/^/      /'
            FAILED=$((FAILED + 1))
        elif [ ! -f "$sem_output_file" ]; then
            echo "  Семантический анализ не выполнен для $filename"
            FAILED=$((FAILED + 1))
        else
            echo "  Семантический анализ: успешно (AST с типами сохранен в $sem_output_file)"
            PASSED=$((PASSED + 1))
        fi
    fi
done

echo ""
echo "Результаты интеграционных тестов"
echo "  Всего: $TOTAL"
echo "  Успешно: $PASSED"
echo "  Провалено: $FAILED"

if [ $FAILED -gt 0 ]; then
    exit 1
else
    exit 0
fi
