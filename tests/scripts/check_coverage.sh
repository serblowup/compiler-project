#!/bin/bash

COVERAGE_DIR="tests/coverage"
MIN_LINES=70
MIN_FUNCTIONS=70
MIN_BRANCHES=50

echo "Проверка покрытия кода тестами"

if [ ! -f "$COVERAGE_DIR/coverage_filtered.info" ]; then
    echo "Ошибка: Сначала запустите make coverage"
    exit 1
fi

# Извлекаем проценты покрытия
LINES=$(lcov --summary $COVERAGE_DIR/coverage_filtered.info 2>&1 | grep "lines" | awk '{print $2}' | sed 's/%//')
FUNCTIONS=$(lcov --summary $COVERAGE_DIR/coverage_filtered.info 2>&1 | grep "functions" | awk '{print $2}' | sed 's/%//')
BRANCHES=$(lcov --summary $COVERAGE_DIR/coverage_filtered.info 2>&1 | grep "branches" | awk '{print $2}' | sed 's/%//')

echo "Текущее покрытие:"
echo "  Строки:    ${LINES}% (минимум ${MIN_LINES}%)"
echo "  Функции:   ${FUNCTIONS}% (минимум ${MIN_FUNCTIONS}%)"
echo "  Ветвления: ${BRANCHES}% (минимум ${MIN_BRANCHES}%)"
echo ""

FAILED=0

if (( $(echo "$LINES < $MIN_LINES" | bc -l) )); then
    echo "Покрытие строк ниже минимума: ${LINES}% < ${MIN_LINES}%"
    FAILED=1
else
    echo "Покрытие строк: ${LINES}% >= ${MIN_LINES}%"
fi

if (( $(echo "$FUNCTIONS < $MIN_FUNCTIONS" | bc -l) )); then
    echo "Покрытие функций ниже минимума: ${FUNCTIONS}% < ${MIN_FUNCTIONS}%"
    FAILED=1
else
    echo "Покрытие функций: ${FUNCTIONS}% >= ${MIN_FUNCTIONS}%"
fi

if (( $(echo "$BRANCHES < $MIN_BRANCHES" | bc -l) )); then
    echo "Покрытие ветвлений ниже минимума: ${BRANCHES}% < ${MIN_BRANCHES}%"
    FAILED=1
else
    echo "Покрытие ветвлений: ${BRANCHES}% >= ${MIN_BRANCHES}%"
fi

echo ""
if [ $FAILED -eq 0 ]; then
    echo "Все требования по покрытию выполнены!"
    exit 0
else
    echo "Требования по покрытию не выполнены!"
    exit 1
fi
