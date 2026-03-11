# Формальная LL(1)-грамматика C-подобного языка

## Обозначения
- `*` - 0 или более повторений
- `|` - или (альтернатива)
- `[ ]` - опционально
- Терминалы в кавычках "if"

## Терминалы
```
Ключевые слова:
  "if" | "else" | "while" | "for" | "int" | "float" | "bool" | "return"
  | "true" | "false" | "void" | "struct" | "fn"

Идентификаторы и литералы:
  identifier   : буква {буква|цифра|"_"}
  int_literal  : цифра {цифра}
  float_literal: цифра {цифра} "." цифра {цифра}
  string_literal: '"' {символ-'"'} '"'

Операторы:
  "="  | "+=" | "-=" | "*=" | "/=" | "%="  (присваивания)
  "+"  | "-"  | "*"  | "/"  | "%"          (арифметические)
  "==" | "!=" | "<"  | ">"  | "<=" | ">="  (отношения)
  "&&" | "||" | "!"                        (логические)
  "++" | "--"                              (инкремент/декремент)
  "->" | "."                               (возвращаемый тип функции, доступ к полю)

Разделители:
  ";" | "," | ":" | "(" | ")" | "{" | "}" | "[" | "]"

EOF : конец файла
```

## Таблица приоритетов операторов
| Уровень | Категория операторов | Операторы | Ассоциативность |
|---------|----------------------|-----------|-----------------|
| 1 | Первичные выражения | литералы, идентификаторы, `(expr)` | - |
| 2 | Постфиксные | `()` (вызов), `.` (доступ к полю), `++` `--` | Левая |
| 3 | Унарные (префиксные) | `-` `!` `++` `--` | Правая |
| 4 | Мультипликативные | `*` `/` `%` | Левая |
| 5 | Аддитивные | `+` `-` | Левая |
| 6 | Отношения | `<` `>` `<=` `>=` | Левая |
| 7 | Равенство | `==` `!=` | Левая |
| 8 | Логическое И | `&&` | Левая |
| 9 | Логическое ИЛИ | `||` | Левая |
| 10 | Присваивание | `=` `+=` `-=` `*=` `/=` `%=` | Правая |

## LL(1)-грамматика

### Программа
```
Program ::= DeclarationList EOF

DeclarationList ::= Declaration DeclarationList | ε
```

### Объявления
```
Declaration ::= FunctionDecl | StructDecl | VarDecl

FunctionDecl ::= "fn" identifier "(" ParameterList ")" FunctionReturn Block

FunctionReturn ::= "->" Type | ε

ParameterList ::= Parameter ParametersRest | ε

ParametersRest ::= "," Parameter ParametersRest | ε

Parameter ::= Type identifier

StructDecl ::= "struct" identifier "{" FieldList "}"

FieldList ::= Field FieldList | ε

Field ::= Type identifier ";"

VarDecl ::= Type identifier VarInitializer ";"

VarInitializer ::= "=" Expression | ε
```

### Типы
```
Type ::= "int" | "float" | "bool" | "void" | "struct" identifier | identifier
```

### Инструкции 
```
Statement ::= BlockStmt | IfStmt | WhileStmt | ForStmt | ReturnStmt | ExprStmt | VarDecl | EmptyStmt

BlockStmt ::= "{" StatementList "}"

IfStmt ::= "if" "(" Expression ")" Statement ElsePart

ElsePart ::= "else" Statement | ε

WhileStmt ::= "while" "(" Expression ")" Statement

ForStmt ::= "for" "(" ForInit ";" ForCondition ";" ForUpdate ")" Statement

ForInit ::= VarDecl | ExprStmt | ";"

ForCondition ::= Expression | ε

ForUpdate ::= Expression | ε

ReturnStmt ::= "return" ReturnExpr ";"

ReturnExpr ::= Expression | ε

ExprStmt ::= Expression ";"

EmptyStmt ::= ";"

StatementList ::= Statement StatementList | ε
```

### Выражения 
```
Expression ::= AssignmentExpr

AssignmentExpr ::= LogicalOrExpr AssignmentRest

AssignmentRest ::= AssignmentOperator AssignmentExpr | ε

AssignmentOperator ::= "=" | "+=" | "-=" | "*=" | "/=" | "%="

LogicalOrExpr ::= LogicalAndExpr LogicalOrRest

LogicalOrRest ::= "||" LogicalAndExpr LogicalOrRest | ε

LogicalAndExpr ::= EqualityExpr LogicalAndRest

LogicalAndRest ::= "&&" EqualityExpr LogicalAndRest | ε

EqualityExpr ::= RelationalExpr EqualityRest

EqualityRest ::= EqualityOperator RelationalExpr EqualityRest | ε

EqualityOperator ::= "==" | "!="

RelationalExpr ::= AdditiveExpr RelationalRest

RelationalRest ::= RelationalOperator AdditiveExpr RelationalRest | ε

RelationalOperator ::= "<" | ">" | "<=" | ">="

AdditiveExpr ::= MultiplicativeExpr AdditiveRest

AdditiveRest ::= AdditiveOperator MultiplicativeExpr AdditiveRest | ε

AdditiveOperator ::= "+" | "-"

MultiplicativeExpr ::= UnaryExpr MultiplicativeRest

MultiplicativeRest ::= MultiplicativeOperator UnaryExpr MultiplicativeRest | ε

MultiplicativeOperator ::= "*" | "/" | "%"

UnaryExpr ::= UnaryOperator UnaryExpr | PostfixExpr

UnaryOperator ::= "-" | "!" | "++" | "--"

PostfixExpr ::= PrimaryExpr PostfixRest

PostfixRest ::= PostfixOperator PostfixRest | ε

PostfixOperator ::= CallSuffix | DotSuffix | IncrementSuffix | DecrementSuffix

CallSuffix ::= "(" ArgumentList ")"

DotSuffix ::= "." identifier

IncrementSuffix ::= "++"

DecrementSuffix ::= "--"

PrimaryExpr ::= Literal | identifier | "(" Expression ")"

Literal ::= int_literal | float_literal | string_literal | "true" | "false"

ArgumentList ::= Expression ArgumentRest | ε

ArgumentRest ::= "," Expression ArgumentRest | ε
```

## Множества FIRST и FOLLOW

### Множества FIRST
```
FIRST(Program) = { "fn", "struct", "int", "float", "bool", "void", identifier, ε }
FIRST(Declaration) = { "fn", "struct", "int", "float", "bool", "void", identifier }
FIRST(FunctionDecl) = { "fn" }
FIRST(StructDecl) = { "struct" }
FIRST(VarDecl) = { "int", "float", "bool", "void", identifier, "struct" }
FIRST(Statement) = { "{", "if", "while", "for", "return", identifier, 
                     int_literal, float_literal, string_literal, "true", "false", 
                     "(", "++", "--", "!", "-", "int", "float", "bool", "void", "struct", ";" }
FIRST(BlockStmt) = { "{" }
FIRST(IfStmt) = { "if" }
FIRST(WhileStmt) = { "while" }
FIRST(ForStmt) = { "for" }
FIRST(ReturnStmt) = { "return" }
FIRST(ExprStmt) = { identifier, int_literal, float_literal, string_literal, 
                    "true", "false", "(", "++", "--", "!", "-" }
FIRST(EmptyStmt) = { ";" }
FIRST(Expression) = { identifier, int_literal, float_literal, string_literal, 
                      "true", "false", "(", "++", "--", "!", "-" }
```

### Множества FOLLOW
```
FOLLOW(Program) = { EOF }
FOLLOW(Declaration) = { "fn", "struct", "int", "float", "bool", "void", identifier, EOF }
FOLLOW(Statement) = { ";", "}", "else", EOF }
FOLLOW(BlockStmt) = { ";", "}", "else", EOF }
FOLLOW(IfStmt) = { ";", "}", "else", EOF }
FOLLOW(WhileStmt) = { ";", "}", "else", EOF }
FOLLOW(ForStmt) = { ";", "}", "else", EOF }
FOLLOW(ReturnStmt) = { ";", "}", "else", EOF }
FOLLOW(ExprStmt) = { ";", "}", "else", EOF }
FOLLOW(EmptyStmt) = { ";", "}", "else", EOF }
FOLLOW(Expression) = { ";", ")", ",", "}", "]", ".",
                       "=", "+=", "-=", "*=", "/=", "%=",
                       "||", "&&", "==", "!=", "<", ">", "<=", ">=",
                       "+", "-", "*", "/", "%", "else" }
```