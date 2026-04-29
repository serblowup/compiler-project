#!/bin/bash

TEST_DIR=$1
EXPECTED_DIR=$2
CATEGORY=$3

if [ -z "$TEST_DIR" ] || [ -z "$EXPECTED_DIR" ]; then
    echo "Использование: $0 <директория_тестов> <директория_ожидаемых_результатов> [категория]"
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

# Проверяем наличие необходимых инструментов
if ! command -v nasm >/dev/null 2>&1; then
    echo "Ошибка: nasm не установлен"
    exit 1
fi

if ! command -v ld >/dev/null 2>&1; then
    echo "Ошибка: ld не установлен"
    exit 1
fi

# Создаем директорию для выходных файлов
mkdir -p test_output/codegen

TOTAL=0
PASSED=0
FAILED=0

echo -e "\n Тестирование $CATEGORY \n"

# Собираем рантайм один раз
RUNTIME_OBJ="test_output/codegen/runtime.o"
nasm -f elf64 src/runtime/runtime.asm -o "$RUNTIME_OBJ" 2>/dev/null
if [ $? -ne 0 ]; then
    echo "Ошибка: не удалось ассемблировать runtime.asm"
    exit 1
fi

for test_file in "$TEST_DIR"/*.src; do
    if [ ! -f "$test_file" ]; then
        continue
    fi
    
    TOTAL=$((TOTAL + 1))
    
    filename=$(basename "$test_file")
    basename="${filename%.*}"
    
    expected_file="$EXPECTED_DIR/$basename.txt"
    asm_file="test_output/codegen/${basename}.asm"
    obj_file="test_output/codegen/${basename}.o"
    exe_file="test_output/codegen/${basename}"
    
    if [ ! -f "$expected_file" ]; then
        echo "  [${CATEGORY}] ${filename}: ОШИБКА - отсутствует ожидаемый файл $expected_file"
        FAILED=$((FAILED + 1))
        continue
    fi
    
    # Генерируем ассемблер
    ./compiler compile --input "$test_file" --output "$asm_file" 2>/dev/null
    
    if [ ! -f "$asm_file" ]; then
        if [[ "$CATEGORY" == *"INVALID"* ]]; then
            echo "  [${CATEGORY}] ${filename}: УСПЕШНО (ошибка обнаружена)"
            PASSED=$((PASSED + 1))
        else
            echo "  [${CATEGORY}] ${filename}: ОШИБКА - ассемблер не сгенерирован"
            FAILED=$((FAILED + 1))
        fi
        continue
    fi
    
    # Проверяем, ожидается ли ошибка
    if [[ "$CATEGORY" == *"INVALID"* ]]; then
        echo "  [${CATEGORY}] ${filename}: НЕУДАЧА - ожидалась ошибка, но файл создан"
        FAILED=$((FAILED + 1))
        continue
    fi
    
    # Ассемблируем
    nasm -f elf64 "$asm_file" -o "$obj_file" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "  [${CATEGORY}] ${filename}: ОШИБКА - ошибка ассемблирования"
        FAILED=$((FAILED + 1))
        continue
    fi
    
    # Линкуем
    ld -o "$exe_file" "$RUNTIME_OBJ" "$obj_file" 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "  [${CATEGORY}] ${filename}: ОШИБКА - ошибка линковки"
        FAILED=$((FAILED + 1))
        continue
    fi
    
    # Запускаем и проверяем результат
    "$exe_file" 2>/dev/null
    exit_code=$?
    
    # Извлекаем ожидаемый код возврата из файла
    expected_exit=$(grep -oP 'EXIT_CODE=\K[0-9]+' "$expected_file" || echo "0")
    
    if [ "$exit_code" -eq "$expected_exit" ]; then
        echo "  [${CATEGORY}] ${filename}: УСПЕШНО (exit: $exit_code)"
        PASSED=$((PASSED + 1))
    else
        echo "  [${CATEGORY}] ${filename}: НЕУДАЧА (expected: $expected_exit, got: $exit_code)"
        echo "    Сгенерированный ассемблер сохранен в: $asm_file"
        FAILED=$((FAILED + 1))
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