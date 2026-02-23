#!/bin/bash

# Функции для печати
print_green() { echo "$1"; }
print_red() { echo "$1"; }
print_yellow() { echo "$1"; }
print_blue() { echo "$1"; }

echo "Запуск всех тестов"

# Компилируем проект
echo "Компиляция проекта..."
make build > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Ошибка: не удалось скомпилировать проект!!!"
    exit 1
fi
echo "Компиляция завершена"
echo ""

TOTAL_PASSED=0
TOTAL_FAILED=0
TOTAL_TESTS=0

# Функция для запуска тестов и подсчета результатов
run_test_suite() {
    local script=$1
    local test_dir=$2
    local expected_dir=$3
    local category=$4
    
    if [ -d "$test_dir" ]; then
        $script "$test_dir" "$expected_dir" "$category"
        local result=$?
        
        # Подсчитываем количество тестов в директории
        local count=$(ls -1 "$test_dir"/*.src 2>/dev/null | wc -l)
        TOTAL_TESTS=$((TOTAL_TESTS + count))
        
        if [ $result -eq 0 ]; then
            TOTAL_PASSED=$((TOTAL_PASSED + count))
        else
            TOTAL_FAILED=$((TOTAL_FAILED + count))
        fi
    fi
}

# Делаем скрипты исполняемыми
chmod +x tests/scripts/*.sh 2>/dev/null || true

# Тесты лексера
run_test_suite "./tests/scripts/test_lexer.sh" "tests/lexer/valid" "tests/lexer/valid/expected" "LEXER-VALID"
run_test_suite "./tests/scripts/test_lexer.sh" "tests/lexer/invalid" "tests/lexer/invalid/expected" "LEXER-INVALID"

# Тесты препроцессора
run_test_suite "./tests/scripts/test_preprocessor.sh" "tests/preprocessor/valid" "tests/preprocessor/valid/expected" "PREPROC-VALID"
run_test_suite "./tests/scripts/test_preprocessor.sh" "tests/preprocessor/invalid" "tests/preprocessor/invalid/expected" "PREPROC-INVALID"

echo "Итоговый результат"
echo "Всего тестов: $TOTAL_TESTS"
print_green "Успешно: $TOTAL_PASSED"
print_red "Провалено: $TOTAL_FAILED"
echo ""

if [ $TOTAL_FAILED -gt 0 ]; then
    exit 1
else
    echo "Все тесты пройдены!"
    exit 0
fi