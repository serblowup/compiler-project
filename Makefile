CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -g -I$(SRCDIR)
CXXFLAGS_COV = -std=c++17 -Wall -Wextra -Wpedantic -g -fprofile-arcs -ftest-coverage -O0 -I$(SRCDIR)
LDFLAGS = 
LDFLAGS_COV = -lgcov -fprofile-arcs -ftest-coverage
TARGET = compiler
TARGET_COV = compiler_cov
SRCDIR = src
BUILDDIR = build
BUILDDIR_COV = build_cov
COVERAGE_DIR = tests/coverage

# Все исходные файлы
SOURCES = $(wildcard $(SRCDIR)/*.cpp) \
          $(wildcard $(SRCDIR)/lexer/*.cpp) \
          $(wildcard $(SRCDIR)/parser/*.cpp) \
          $(wildcard $(SRCDIR)/preprocessor/*.cpp) \
          $(wildcard $(SRCDIR)/semantic/*.cpp) \
          $(wildcard $(SRCDIR)/utils/*.cpp)

ifeq ($(wildcard $(SRCDIR)/semantic/*.cpp),)
    SOURCES += $(SRCDIR)/semantic/semantic_analyzer.cpp
endif

OBJECTS = $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(SOURCES))
OBJECTS_COV = $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR_COV)/%.o,$(SOURCES))

.PHONY: all clean build test scripts coverage coverage-simple \
        test-lexer test-preproc test-parser test-parser-valid test-parser-invalid test-roundtrip test-integration \
        test-semantic lexer preprocess parse check

all: $(TARGET)

# Обычная сборка
$(TARGET): $(OBJECTS)
	$(CXX) $^ -o $@ $(CXXFLAGS) $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Сборка с coverage
$(TARGET_COV): $(OBJECTS_COV)
	$(CXX) $^ -o $@ $(CXXFLAGS_COV) $(LDFLAGS_COV)

$(BUILDDIR_COV)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS_COV) -c $< -o $@

build: $(TARGET)

build-cov: $(TARGET_COV)

# Делаем скрипты исполняемыми
scripts:
	@chmod +x tests/scripts/*.sh 2>/dev/null || true

# Запуск всех тестов
test: scripts $(TARGET)
	@./tests/scripts/run_all_tests.sh

# Запуск только тестов лексера
test-lexer: scripts $(TARGET)
	@./tests/scripts/test_lexer.sh tests/lexer/valid tests/lexer/valid/expected "LEXER-VALID"
	@./tests/scripts/test_lexer.sh tests/lexer/invalid tests/lexer/invalid/expected "LEXER-INVALID"

# Запуск только тестов препроцессора
test-preproc: scripts $(TARGET)
	@if [ -d "tests/preprocessor/valid" ]; then \
		./tests/scripts/test_preprocessor.sh tests/preprocessor/valid tests/preprocessor/valid/expected "PREPROC-VALID"; \
	fi
	@if [ -d "tests/preprocessor/invalid" ]; then \
		./tests/scripts/test_preprocessor.sh tests/preprocessor/invalid tests/preprocessor/invalid/expected "PREPROC-INVALID"; \
	fi

# Запуск тестов парсера
test-parser: scripts $(TARGET)
	@./tests/scripts/test_parser.sh tests/parser/valid tests/parser/valid/expected "PARSER-VALID"
	@./tests/scripts/test_parser.sh tests/parser/invalid tests/parser/invalid/expected "PARSER-INVALID"

# Запуск интеграционных тестов
test-integration: scripts $(TARGET)
	@./tests/scripts/test_integration.sh

# Запуск семантических тестов
test-semantic: scripts $(TARGET)
	@if [ -d "tests/semantic/valid" ]; then \
		./tests/scripts/test_semantic.sh tests/semantic/valid tests/semantic/valid/expected "SEMANTIC-VALID"; \
	fi
	@if [ -d "tests/semantic/invalid" ]; then \
		./tests/scripts/test_semantic.sh tests/semantic/invalid tests/semantic/invalid/expected "SEMANTIC-INVALID"; \
	fi

# Запуск тестов с coverage
coverage: scripts build-cov
	@echo "Запуск тестов с coverage..."
	@rm -rf $(COVERAGE_DIR) || true
	@mkdir -p $(COVERAGE_DIR) || true
	@mkdir -p $(COVERAGE_DIR)/data || true

	@for test_file in tests/lexer/valid/*.src; do \
		if [ -f "$$test_file" ]; then \
			./$(TARGET_COV) lex --input "$$test_file" --output "$(COVERAGE_DIR)/data/$$(basename $$test_file .src).out" > /dev/null 2>&1 || true; \
		fi \
	done

	@for test_file in tests/lexer/invalid/*.src; do \
		if [ -f "$$test_file" ]; then \
			./$(TARGET_COV) lex --input "$$test_file" --output "$(COVERAGE_DIR)/data/$$(basename $$test_file .src).out" > /dev/null 2>&1 || true; \
		fi \
	done

	@if [ -d "tests/preprocessor/valid" ]; then \
		for test_file in tests/preprocessor/valid/*.src; do \
			if [ -f "$$test_file" ]; then \
				./$(TARGET_COV) preprocess --input "$$test_file" --output "$(COVERAGE_DIR)/data/$$(basename $$test_file .src).out" > /dev/null 2>&1 || true; \
			fi \
		done \
	fi

	@if [ -d "tests/preprocessor/invalid" ]; then \
		for test_file in tests/preprocessor/invalid/*.src; do \
			if [ -f "$$test_file" ]; then \
				./$(TARGET_COV) preprocess --input "$$test_file" --output "$(COVERAGE_DIR)/data/$$(basename $$test_file .src).out" > /dev/null 2>&1 || true; \
			fi \
		done \
	fi

	@if [ -d "tests/parser/valid" ]; then \
		for test_file in tests/parser/valid/*.src; do \
			if [ -f "$$test_file" ]; then \
				echo "Запуск $$test_file"; \
				./$(TARGET_COV) parse --input "$$test_file" --output "$(COVERAGE_DIR)/data/$$(basename $$test_file .src).out" > /dev/null 2>&1 || true; \
			fi \
		done \
	fi

	@if [ -d "tests/parser/invalid" ]; then \
		for test_file in tests/parser/invalid/*.src; do \
			if [ -f "$$test_file" ]; then \
				echo "Запуск $$test_file"; \
				./$(TARGET_COV) parse --input "$$test_file" --output "$(COVERAGE_DIR)/data/$$(basename $$test_file .src).out" > /dev/null 2>&1 || true; \
			fi \
		done \
	fi

	@if [ -d "tests/semantic/valid" ]; then \
		for test_file in tests/semantic/valid/*.src; do \
			if [ -f "$$test_file" ]; then \
				echo "Запуск $$test_file"; \
				./$(TARGET_COV) check --input "$$test_file" --output "$(COVERAGE_DIR)/data/$$(basename $$test_file .src).out" > /dev/null 2>&1 || true; \
			fi \
		done \
	fi

	@if [ -d "tests/semantic/invalid" ]; then \
		for test_file in tests/semantic/invalid/*.src; do \
			if [ -f "$$test_file" ]; then \
				echo "Запуск $$test_file"; \
				./$(TARGET_COV) check --input "$$test_file" --output "$(COVERAGE_DIR)/data/$$(basename $$test_file .src).out" > /dev/null 2>&1 || true; \
			fi \
		done \
	fi

	@for test_file in examples/*.src; do \
		if [ -f "$$test_file" ]; then \
			echo "Запуск $$test_file"; \
			./$(TARGET_COV) check --input "$$test_file" --output "$(COVERAGE_DIR)/data/$$(basename $$test_file .src).out" > /dev/null 2>&1 || true; \
		fi \
	done

	# Создаем отчет
	@echo "Генерация отчета о покрытии..."
	@echo "=============================================" > $(COVERAGE_DIR)/coverage_report.txt || true
	@echo "Отчёт о покрытии" >> $(COVERAGE_DIR)/coverage_report.txt || true
	@echo "=============================================" >> $(COVERAGE_DIR)/coverage_report.txt || true
	@echo "" >> $(COVERAGE_DIR)/coverage_report.txt || true
	
	# Пытаемся получить данные coverage, но игнорируем ошибки
	@if command -v lcov >/dev/null 2>&1; then \
		lcov --directory $(BUILDDIR_COV) --capture --rc lcov_branch_coverage=1 --output-file $(COVERAGE_DIR)/coverage.info --ignore-errors mismatch,negative,empty 2>/dev/null || true; \
		if [ -f $(COVERAGE_DIR)/coverage.info ]; then \
			lcov --remove $(COVERAGE_DIR)/coverage.info '/usr/*' '*/googletest/*' '*/test/*' --output-file $(COVERAGE_DIR)/coverage_filtered.info --ignore-errors mismatch,negative,empty 2>/dev/null || true; \
			if [ -f $(COVERAGE_DIR)/coverage_filtered.info ]; then \
				echo "Общее покрытие:" >> $(COVERAGE_DIR)/coverage_report.txt; \
				echo "---------------------------------------------" >> $(COVERAGE_DIR)/coverage_report.txt; \
				lcov --summary $(COVERAGE_DIR)/coverage_filtered.info 2>&1 | grep -E "lines|functions|branches" >> $(COVERAGE_DIR)/coverage_report.txt || true; \
				echo "---------------------------------------------" >> $(COVERAGE_DIR)/coverage_report.txt; \
			else \
				echo "Не удалось отфильтровать данные покрытия" >> $(COVERAGE_DIR)/coverage_report.txt; \
			fi \
		else \
			echo "Не удалось собрать данные покрытия" >> $(COVERAGE_DIR)/coverage_report.txt; \
		fi \
	else \
		echo "lcov не установлен. Установите lcov для получения отчета о покрытии." >> $(COVERAGE_DIR)/coverage_report.txt; \
	fi
	
	@echo "Отчет сохранен в: $(COVERAGE_DIR)/coverage_report.txt"

# Простой отчет без lcov
coverage-simple:
	@echo "Запуск тестов с coverage (простой отчет)..."
	@$(MAKE) build-cov > /dev/null 2>&1 || true

	@for test_file in tests/lexer/valid/*.src; do \
		if [ -f "$$test_file" ]; then \
			./$(TARGET_COV) lex --input "$$test_file" --output /dev/null > /dev/null 2>&1 || true; \
		fi \
	done
	@for test_file in tests/lexer/invalid/*.src; do \
		if [ -f "$$test_file" ]; then \
			./$(TARGET_COV) lex --input "$$test_file" --output /dev/null > /dev/null 2>&1 || true; \
		fi \
	done

	@if [ -d "tests/preprocessor/valid" ]; then \
		for test_file in tests/preprocessor/valid/*.src; do \
			if [ -f "$$test_file" ]; then \
				./$(TARGET_COV) preprocess --input "$$test_file" --output /dev/null > /dev/null 2>&1 || true; \
			fi \
		done \
	fi
	@if [ -d "tests/preprocessor/invalid" ]; then \
		for test_file in tests/preprocessor/invalid/*.src; do \
			if [ -f "$$test_file" ]; then \
				./$(TARGET_COV) preprocess --input "$$test_file" --output /dev/null > /dev/null 2>&1 || true; \
			fi \
		done \
	fi

	@if [ -d "tests/parser/valid" ]; then \
		for test_file in tests/parser/valid/*.src; do \
			if [ -f "$$test_file" ]; then \
				./$(TARGET_COV) parse --input "$$test_file" --output /dev/null > /dev/null 2>&1 || true; \
			fi \
		done \
	fi
	@if [ -d "tests/parser/invalid" ]; then \
		for test_file in tests/parser/invalid/*.src; do \
			if [ -f "$$test_file" ]; then \
				./$(TARGET_COV) parse --input "$$test_file" --output /dev/null > /dev/null 2>&1 || true; \
			fi \
		done \
	fi

	@if [ -d "tests/semantic/valid" ]; then \
		for test_file in tests/semantic/valid/*.src; do \
			if [ -f "$$test_file" ]; then \
				./$(TARGET_COV) check --input "$$test_file" --output /dev/null > /dev/null 2>&1 || true; \
			fi \
		done \
	fi
	@if [ -d "tests/semantic/invalid" ]; then \
		for test_file in tests/semantic/invalid/*.src; do \
			if [ -f "$$test_file" ]; then \
				./$(TARGET_COV) check --input "$$test_file" --output /dev/null > /dev/null 2>&1 || true; \
			fi \
		done \
	fi

	@for test_file in examples/*.src; do \
		if [ -f "$$test_file" ]; then \
			./$(TARGET_COV) check --input "$$test_file" --output /dev/null > /dev/null 2>&1 || true; \
		fi \
	done

	@echo "Покрытие по файлам"
	@for src_file in $(SOURCES); do \
		echo "  $$(basename $$src_file):"; \
		gcov -n $$src_file 2>/dev/null | grep -E "Lines executed|Branches executed" | sed 's/^/    /' || echo "    Нет данных"; \
		echo ""; \
	done

# Очистка
clean:
	rm -rf $(BUILDDIR) $(BUILDDIR_COV) $(TARGET) $(TARGET_COV) $(COVERAGE_DIR) test_output
	find . -name "*.gcda" -delete
	find . -name "*.gcno" -delete
	find . -name "*.gcov" -delete

# Запуск лексера на отдельном файле
lexer: $(TARGET)
	@if [ -z "$(INPUT)" ] || [ -z "$(OUTPUT)" ]; then \
		echo "Использование: make lexer INPUT=<файл> OUTPUT=<файл>"; \
		exit 1; \
	fi
	./$(TARGET) lex --input $(INPUT) --output $(OUTPUT)

# Запуск препроцессора на отдельном файле
preprocess: $(TARGET)
	@if [ -z "$(INPUT)" ] || [ -z "$(OUTPUT)" ]; then \
		echo "Использование: make preprocess INPUT=<файл> OUTPUT=<файл>"; \
		exit 1; \
	fi
	./$(TARGET) preprocess --input $(INPUT) --output $(OUTPUT)

# Запуск парсера на отдельном файле
parse: $(TARGET)
	@if [ -z "$(INPUT)" ] || [ -z "$(OUTPUT)" ]; then \
		echo "Использование: make parse INPUT=<файл> OUTPUT=<файл> [FORMAT=text|dot|json]"; \
		echo "Пример: make parse INPUT=examples/factorial.src OUTPUT=ast.txt FORMAT=text"; \
		exit 1; \
	fi
	@if [ -z "$(FORMAT)" ]; then \
		./$(TARGET) parse --input $(INPUT) --output $(OUTPUT); \
	else \
		./$(TARGET) parse --input $(INPUT) --output $(OUTPUT) --format $(FORMAT); \
	fi

# Запуск семантического анализа на отдельном файле
check: $(TARGET)
	@if [ -z "$(INPUT)" ] || [ -z "$(OUTPUT)" ]; then \
		echo "Использование: make check INPUT=<файл> OUTPUT=<файл> [DUMP_SYMBOLS=1] [SHOW_TYPES=1] [FORMAT=text|dot|json]"; \
		echo "Пример: make check INPUT=examples/factorial.src OUTPUT=result.txt DUMP_SYMBOLS=1 SHOW_TYPES=1"; \
		exit 1; \
	fi
	@CMD="./$(TARGET) check --input $(INPUT) --output $(OUTPUT)"
	@if [ -n "$(FORMAT)" ]; then \
		CMD="$$CMD --format $(FORMAT)"; \
	fi
	@if [ -n "$(DUMP_SYMBOLS)" ] && [ "$(DUMP_SYMBOLS)" = "1" ]; then \
		CMD="$$CMD --dump-symbols"; \
	fi
	@if [ -n "$(SHOW_TYPES)" ] && [ "$(SHOW_TYPES)" = "1" ]; then \
		CMD="$$CMD --show-types"; \
	fi
	@if [ -n "$(SYMBOL_FORMAT)" ]; then \
		CMD="$$CMD --symbol-format $(SYMBOL_FORMAT)"; \
	fi
	@if [ -n "$(VERBOSE)" ] && [ "$(VERBOSE)" = "1" ]; then \
		CMD="$$CMD --verbose"; \
	fi
	@echo "Выполнение: $$CMD"
	@$$CMD