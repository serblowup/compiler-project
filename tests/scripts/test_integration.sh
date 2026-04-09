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
            
            ir_output_file="test_output/integration/${basename}_ir.txt"
            ir_error_file="test_output/integration/${basename}_ir.err"
            
            ./compiler ir --input "$test_file" --output "$ir_output_file" 2> "$ir_error_file"
            
            if [ -s "$ir_error_file" ]; then
                echo "  Ошибка при генерации IR для $filename"
                echo "    Сообщение об ошибке:"
                cat "$ir_error_file" | sed 's/^/      /'
                FAILED=$((FAILED + 1))
            elif [ ! -f "$ir_output_file" ]; then
                echo "  IR не создан для $filename"
                FAILED=$((FAILED + 1))
            else
                echo "  IR: успешно (IR сохранен в $ir_output_file)"
                
                ir_opt_output_file="test_output/integration/${basename}_ir_opt.txt"
                ir_opt_error_file="test_output/integration/${basename}_ir_opt.err"
                
                ./compiler ir --input "$test_file" --output "$ir_opt_output_file" --optimize 2> "$ir_opt_error_file"
                
                if [ -s "$ir_opt_error_file" ]; then
                    echo "  Ошибка при генерации IR с оптимизациями для $filename"
                    echo "    Сообщение об ошибке:"
                    cat "$ir_opt_error_file" | sed 's/^/      /'
                    FAILED=$((FAILED + 1))
                elif [ ! -f "$ir_opt_output_file" ]; then
                    echo "  IR с оптимизациями не создан для $filename"
                    FAILED=$((FAILED + 1))
                else
                    echo "  IR с оптимизациями: успешно (IR сохранен в $ir_opt_output_file)"
                    PASSED=$((PASSED + 1))
                fi
            fi
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
