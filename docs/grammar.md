# Формальная LL(1)-грамматика C-подобного языка

## Обозначения
- `*` - 0 или более повторений
- `|` - или (альтернатива)
- `[ ]` - опционально
- Терминалы в кавычках "if"

## Терминалы
```
keyword:
  "if" | "else" | "while" | "for" | "int" | "float" | "bool" | "return"
  | "true" | "false" | "void" | "struct" | "fn"

Идентификаторы и литералы:
  identifier        : letter, { letter | digit | "_" }
  integer_literal   : digit, { digit }
  float_literal     : digit, { digit }, ".", digit, { digit }
  string_literal    : '"', { character - '"' }, '"'
  boolean_literal   : "true" | "false"

Арифметические операторы:
  operator_add = "+"
  operator_sub = "-"
  operator_mul = "*"
  operator_div = "/"
  operator_mod = "%"

Операторы отношения:
  operator_eq  = "=="
  operator_neq = "!="
  operator_lt  = "<"
  operator_gt  = ">"
  operator_lte = "<="
  operator_gte = ">="

Логические операторы:
  operator_and = "&&"
  operator_or  = "||"
  operator_not = "!"

Операторы присваивания:
  assign      = "="
  assign_add  = "+="
  assign_sub  = "-="
  assign_mul  = "*="
  assign_div  = "/="
  assign_mod  = "%="

Инкремент/Декремент:
  increment = "++"
  decrement = "--"

Возвращаемый тип функции (стрелка):
  arrow = "->"

Разделители:
  delimiter = ";" | "," | "." | ":" | "(" | ")" | "{" | "}" | "[" | "]"

EOF : конец файла
```

## Таблица приоритетов операторов
| Уровень | Категория операторов | Операторы | Ассоциативность |
|---------|----------------------|-----------|-----------------|
| 1 | Первичные выражения | литералы, идентификаторы, `(expr)` | - |
| 2 | Постфиксные | `()`, `.`, `++` `--` | Левая |
| 3 | Унарные (префиксные) | `-` `!` `++` `--` | Правая |
| 4 | Мультипликативные | `*` `/` `%` | Левая |
| 5 | Аддитивные | `+` `-` | Левая |
| 6 | Отношения | `<` `>` `<=` `>=` | Левая |
| 7 | Равенство | `==` `!=` | Левая |
| 8 | Логическое И | `&&` | Левая |
| 9 | Логическое ИЛИ | `\|\|` | Левая |
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

FunctionReturn ::= arrow Type | ε

ParameterList ::= Parameter ParametersRest | ε

ParametersRest ::= "," Parameter ParametersRest | ε

Parameter ::= Type identifier

StructDecl ::= "struct" identifier "{" FieldList "}"

FieldList ::= Field FieldList | ε

Field ::= Type identifier ";"

VarDecl ::= Type identifier VarInitializer ";"

VarInitializer ::= assign Expression | ε
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

AssignmentOperator ::= assign | assign_add | assign_sub | assign_mul | assign_div | assign_mod

LogicalOrExpr ::= LogicalAndExpr LogicalOrRest

LogicalOrRest ::= operator_or LogicalAndExpr LogicalOrRest | ε

LogicalAndExpr ::= EqualityExpr LogicalAndRest

LogicalAndRest ::= operator_and EqualityExpr LogicalAndRest | ε

EqualityExpr ::= RelationalExpr EqualityRest

EqualityRest ::= EqualityOperator RelationalExpr EqualityRest | ε

EqualityOperator ::= operator_eq | operator_neq

RelationalExpr ::= AdditiveExpr RelationalRest

RelationalRest ::= RelationalOperator AdditiveExpr RelationalRest | ε

RelationalOperator ::= operator_lt | operator_gt | operator_lte | operator_gte

AdditiveExpr ::= MultiplicativeExpr AdditiveRest

AdditiveRest ::= AdditiveOperator MultiplicativeExpr AdditiveRest | ε

AdditiveOperator ::= operator_add | operator_sub

MultiplicativeExpr ::= UnaryExpr MultiplicativeRest

MultiplicativeRest ::= MultiplicativeOperator UnaryExpr MultiplicativeRest | ε

MultiplicativeOperator ::= operator_mul | operator_div | operator_mod

UnaryExpr ::= UnaryOperator UnaryExpr | PostfixExpr

UnaryOperator ::= operator_sub | operator_not | increment | decrement

PostfixExpr ::= PrimaryExpr PostfixRest

PostfixRest ::= PostfixOperator PostfixRest | ε

PostfixOperator ::= CallSuffix | DotSuffix | IncrementSuffix | DecrementSuffix

CallSuffix ::= "(" ArgumentList ")"

DotSuffix ::= "." identifier

IncrementSuffix ::= increment

DecrementSuffix ::= decrement

PrimaryExpr ::= Literal | identifier | "(" Expression ")"

Literal ::= integer_literal | float_literal | string_literal | boolean_literal

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
                     integer_literal, float_literal, string_literal, boolean_literal, 
                     "(", increment, decrement, operator_not, operator_sub, 
                     "int", "float", "bool", "void", "struct", ";" }
FIRST(BlockStmt) = { "{" }
FIRST(IfStmt) = { "if" }
FIRST(WhileStmt) = { "while" }
FIRST(ForStmt) = { "for" }
FIRST(ReturnStmt) = { "return" }
FIRST(ExprStmt) = { identifier, integer_literal, float_literal, string_literal, 
                    boolean_literal, "(", increment, decrement, operator_not, operator_sub }
FIRST(EmptyStmt) = { ";" }
FIRST(Expression) = { identifier, integer_literal, float_literal, string_literal, 
                      boolean_literal, "(", increment, decrement, operator_not, operator_sub }
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
                       assign, assign_add, assign_sub, assign_mul, assign_div, assign_mod,
                       operator_or, operator_and, operator_eq, operator_neq,
                       operator_lt, operator_gt, operator_lte, operator_gte,
                       operator_add, operator_sub, operator_mul, operator_div, 
                       operator_mod, "else" }
```
