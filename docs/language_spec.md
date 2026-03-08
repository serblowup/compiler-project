# Спецификация языка программирования

## 1. Лексическая грамматика (EBNF)

### 1.1. Набор символов
```ebnf
(Кодировка: UTF-8)
letter = "A" | "B" | ... | "Z" | "a" | "b" | ... | "z" ;
digit = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9" ;
whitespace = " " | "\t" | "\n" | "\r" ;
```
### 1.2. Идентификаторы и ключевые слова
```ebnf
identifier = letter, { letter | digit | "_" } ;
(Максимальная длина: 255 символов)

keyword = "if" | "else" | "while" | "for" | "int" | "float" | "bool" | "return" | "true" | "false" | "void" | "struct" | "fn" ;
```

### 1.3. Литералы
```ebnf
integer_literal = digit, { digit } ;
(Диапазон: [-2³¹, 2³¹-1])

float_literal = digit, { digit }, ".", digit, { digit } ;

string_literal = '"', { character - '"' }, '"' ;

boolean_literal = "true" | "false" ;
```

### 1.4. Операторы
```ebnf
(Арифметические)
operator_add = "+" ;
operator_sub = "-" ;
operator_mul = "*" ;
operator_div = "/" ;
operator_mod = "%" ;

(Отношения)
operator_eq = "==" ;
operator_neq = "!=" ;
operator_lt = "<" ;
operator_gt = ">" ;
operator_lte = "<=" ;
operator_gte = ">=" ;

(Логические)
operator_and = "&&" ;
operator_or = "||" ;
operator_not = "!" ;

(Присваивания)
assign = "=" ;
assign_add = "+=" ;
assign_sub = "-=" ;
assign_mul = "*=" ;
assign_div = "/=" ;
assign_mod = "%=" ;

(Инкремент/Декремент)
increment = "++" ;
decrement = "--" ;

```

### 1.5. Разделители
```ebnf
delimiter = ";" | "," | "." | ":" | "(" | ")" | "{" | "}" | "[" | "]" ;
```

### 1.6. Комментарии
```ebnf
single_line_comment = "//", { character - "\n" }, "\n" ;
multi_line_comment = "/*", { character }, "*/" ;
```

#### 2. Примеры

Корректные идентификаторы:
```text
counter
x
```
Некорректные идентификаторы:
```text
a@b
```

Числовые литералы:
```text
42
3.14159
0
-10
```

Строковые литералы:
```text
"Hello, World!"
""
```

##### 3. Обработка ошибок

Лексер сообщает об ошибках в формате:
```text
[Строка:Колонка] ERROR "описание ошибки"
```

Типы ошибок:
- Недопустимый символ;
- Незавершенный строковый литерал;
- Незавершенный многострочный комментарий;
- Некорректное числовое значение.
