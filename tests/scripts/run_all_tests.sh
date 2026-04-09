#!/bin/bash

echo "Запуск всех тестов"

# Компилируем проект
echo -n "Компиляция проекта... "
make build > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Ошибка: не удалось скомпилировать проект!!!"
    exit 1
fi
echo "Готово"

TOTAL_PASSED=0
TOTAL_FAILED=0
TOTAL_TESTS=0

run_test_suite() {
    local script=$1
    local test_dir=$2
    local expected_dir=$3
    local category=$4
    
    if [ -d "$test_dir" ]; then
        $script "$test_dir" "$expected_dir" "$category"
        local result=$?
        
        local count=$(ls -1 "$test_dir"/*.src 2>/dev/null | wc -l)
        TOTAL_TESTS=$((TOTAL_TESTS + count))
        
        if [ $result -eq 0 ]; then
            TOTAL_PASSED=$((TOTAL_PASSED + count))
        else
            TOTAL_FAILED=$((TOTAL_FAILED + count))
        fi
        echo ""
    fi
}

run_integration_tests() {
    local script=$1
    $script
    local result=$?
    
    local count=$(ls -1 examples/*.src 2>/dev/null | wc -l)
    TOTAL_TESTS=$((TOTAL_TESTS + count))
    
    if [ $result -eq 0 ]; then
        TOTAL_PASSED=$((TOTAL_PASSED + count))
    else
        TOTAL_FAILED=$((TOTAL_FAILED + count))
    fi
    echo ""
}

# Делаем скрипты исполняемыми
chmod +x tests/scripts/*.sh 2>/dev/null || true

# Тесты лексера
echo "Тесты лексера"
run_test_suite "./tests/scripts/test_lexer.sh" "tests/lexer/valid" "tests/lexer/valid/expected" "LEXER-VALID"
run_test_suite "./tests/scripts/test_lexer.sh" "tests/lexer/invalid" "tests/lexer/invalid/expected" "LEXER-INVALID"

# Тесты препроцессора
echo "Тесты препроцессора"
run_test_suite "./tests/scripts/test_preprocessor.sh" "tests/preprocessor/valid" "tests/preprocessor/valid/expected" "PREPROC-VALID"
run_test_suite "./tests/scripts/test_preprocessor.sh" "tests/preprocessor/invalid" "tests/preprocessor/invalid/expected" "PREPROC-INVALID"

# Тесты парсера
echo "Тесты парсера"
run_test_suite "./tests/scripts/test_parser.sh" "tests/parser/valid" "tests/parser/valid/expected" "PARSER-VALID"
run_test_suite "./tests/scripts/test_parser.sh" "tests/parser/invalid" "tests/parser/invalid/expected" "PARSER-INVALID"

# Тесты семантического анализа
echo "Тесты семантического анализа"
run_test_suite "./tests/scripts/test_semantic.sh" "tests/semantic/valid" "tests/semantic/valid/expected" "SEMANTIC-VALID"
run_test_suite "./tests/scripts/test_semantic.sh" "tests/semantic/invalid" "tests/semantic/invalid/expected" "SEMANTIC-INVALID"

# Тесты IR (генерация и оптимизации в одном скрипте)
echo "Тесты IR"
run_test_suite "./tests/scripts/test_ir.sh" "tests/ir/generation" "tests/ir/generation/expected" "IR-GENERATION"
run_test_suite "./tests/scripts/test_ir.sh" "tests/ir/optimization" "tests/ir/optimization/expected" "IR-OPTIMIZATION"

# Интеграционные тесты
echo "Интеграционные тесты"
run_integration_tests "./tests/scripts/test_integration.sh"

echo ""
echo "[Итоговый результат]"
echo "Всего тестов: $TOTAL_TESTS"
echo "Успешно: $TOTAL_PASSED"
if [ $TOTAL_FAILED -gt 0 ]; then
    echo "Провалено: $TOTAL_FAILED"
else
    echo "Провалено: $TOTAL_FAILED"
fi

if [ $TOTAL_FAILED -gt 0 ]; then
    exit 1
else
    echo "Все тесты пройдены!"
    exit 0
fi