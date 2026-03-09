# Формальная LL(1)-грамматика C-подобного языка

## Обозначения
- `*` - 0 или более повторений
- `|` - или (альтернатива)
- `[ ]` - опционально
- Терминалы в кавычках "if"

## Терминалы

```
KW_IF        : "if"
KW_ELSE      : "else"
KW_WHILE     : "while"
KW_FOR       : "for"
KW_INT       : "int"
KW_FLOAT     : "float"
KW_BOOL      : "bool"
KW_RETURN    : "return"
KW_TRUE      : "true"
KW_FALSE     : "false"
KW_VOID      : "void"
KW_STRUCT    : "struct"
KW_FN        : "fn"

IDENTIFIER   : буква {буква|цифра|"_"}
INT_LITERAL  : цифра {цифра}
FLOAT_LITERAL: цифра {цифра} "." цифра {цифра}
STRING_LITERAL: '"' {символ-'"'} '"'

OPERATOR_ASSIGN          : "="
OPERATOR_ADD_ASSIGN      : "+="
OPERATOR_SUB_ASSIGN      : "-="
OPERATOR_MUL_ASSIGN      : "*="
OPERATOR_DIV_ASSIGN      : "/="
OPERATOR_MOD_ASSIGN      : "%="
OPERATOR_PLUS            : "+"
OPERATOR_MINUS           : "-"
OPERATOR_MUL             : "*"
OPERATOR_DIV             : "/"
OPERATOR_MOD             : "%"
OPERATOR_EQ              : "=="
OPERATOR_NEQ             : "!="
OPERATOR_LT              : "<"
OPERATOR_GT              : ">"
OPERATOR_LTE             : "<="
OPERATOR_GTE             : ">="
OPERATOR_AND             : "&&"
OPERATOR_OR              : "||"
OPERATOR_NOT             : "!"
OPERATOR_INCREMENT       : "++"
OPERATOR_DECREMENT       : "--"

SEMICOLON    : ";"
COMMA        : ","
DOT          : "."
COLON        : ":"
LPAREN       : "("
RPAREN       : ")"
LBRACE       : "{"
RBRACE       : "}"
LBRACKET     : "["
RBRACKET     : "]"
ARROW        : "->"

EOF          : конец файла
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

FunctionDecl ::= KW_FN IDENTIFIER LPAREN ParameterList RPAREN FunctionReturn Block

FunctionReturn ::= ARROW Type | ε

ParameterList ::= Parameter ParametersRest | ε

ParametersRest ::= COMMA Parameter ParametersRest | ε

Parameter ::= Type IDENTIFIER

StructDecl ::= KW_STRUCT IDENTIFIER LBRACE FieldList RBRACE

FieldList ::= Field FieldList | ε

Field ::= Type IDENTIFIER SEMICOLON

VarDecl ::= Type IDENTIFIER VarInitializer SEMICOLON

VarInitializer ::= OPERATOR_ASSIGN Expression | ε
```

### Типы
```
Type ::= KW_INT | KW_FLOAT | KW_BOOL | KW_VOID | KW_STRUCT IDENTIFIER | IDENTIFIER
```

### Инструкции 
```
Statement ::= BlockStmt | IfStmt | WhileStmt | ForStmt | ReturnStmt | ExprStmt | VarDecl | EmptyStmt

BlockStmt ::= LBRACE StatementList RBRACE

IfStmt ::= KW_IF LPAREN Expression RPAREN Statement ElsePart

ElsePart ::= KW_ELSE Statement | ε

WhileStmt ::= KW_WHILE LPAREN Expression RPAREN Statement

ForStmt ::= KW_FOR LPAREN ForInit SEMICOLON ForCondition SEMICOLON ForUpdate RPAREN Statement

ForInit ::= VarDecl | ExprStmt | SEMICOLON

ForCondition ::= Expression | ε

ForUpdate ::= Expression | ε

ReturnStmt ::= KW_RETURN ReturnExpr SEMICOLON

ReturnExpr ::= Expression | ε

ExprStmt ::= Expression SEMICOLON

EmptyStmt ::= SEMICOLON

StatementList ::= Statement StatementList | ε
```

### Выражения 
```
Expression ::= AssignmentExpr

AssignmentExpr ::= LogicalOrExpr AssignmentRest

AssignmentRest ::= AssignmentOperator AssignmentExpr | ε

AssignmentOperator ::= OPERATOR_ASSIGN | OPERATOR_ADD_ASSIGN | OPERATOR_SUB_ASSIGN | OPERATOR_MUL_ASSIGN | OPERATOR_DIV_ASSIGN | OPERATOR_MOD_ASSIGN

LogicalOrExpr ::= LogicalAndExpr LogicalOrRest

LogicalOrRest ::= OPERATOR_OR LogicalAndExpr LogicalOrRest | ε

LogicalAndExpr ::= EqualityExpr LogicalAndRest

LogicalAndRest ::= OPERATOR_AND EqualityExpr LogicalAndRest | ε

EqualityExpr ::= RelationalExpr EqualityRest

EqualityRest ::= EqualityOperator RelationalExpr EqualityRest | ε

EqualityOperator ::= OPERATOR_EQ | OPERATOR_NEQ

RelationalExpr ::= AdditiveExpr RelationalRest

RelationalRest ::= RelationalOperator AdditiveExpr RelationalRest | ε

RelationalOperator ::= OPERATOR_LT | OPERATOR_GT | OPERATOR_LTE | OPERATOR_GTE

AdditiveExpr ::= MultiplicativeExpr AdditiveRest

AdditiveRest ::= AdditiveOperator MultiplicativeExpr AdditiveRest | ε

AdditiveOperator ::= OPERATOR_PLUS | OPERATOR_MINUS

MultiplicativeExpr ::= UnaryExpr MultiplicativeRest

MultiplicativeRest ::= MultiplicativeOperator UnaryExpr MultiplicativeRest | ε

MultiplicativeOperator ::= OPERATOR_MUL | OPERATOR_DIV | OPERATOR_MOD

UnaryExpr ::= UnaryOperator UnaryExpr | PostfixExpr

UnaryOperator ::= OPERATOR_MINUS | OPERATOR_NOT | OPERATOR_INCREMENT | OPERATOR_DECREMENT

PostfixExpr ::= PrimaryExpr PostfixRest

PostfixRest ::= PostfixOperator PostfixRest | ε

PostfixOperator ::= CallSuffix | DotSuffix | IncrementSuffix | DecrementSuffix

CallSuffix ::= LPAREN ArgumentList RPAREN

DotSuffix ::= DOT IDENTIFIER

IncrementSuffix ::= OPERATOR_INCREMENT

DecrementSuffix ::= OPERATOR_DECREMENT

PrimaryExpr ::= Literal | IDENTIFIER | LPAREN Expression RPAREN

Literal ::= INT_LITERAL | FLOAT_LITERAL | STRING_LITERAL | KW_TRUE | KW_FALSE

ArgumentList ::= Expression ArgumentRest | ε

ArgumentRest ::= COMMA Expression ArgumentRest | ε
```

## Множества FIRST и FOLLOW

### Множества FIRST
```
FIRST(Program) = { KW_FN, KW_STRUCT, KW_INT, KW_FLOAT, KW_BOOL, KW_VOID, IDENTIFIER, ε }
FIRST(Declaration) = { KW_FN, KW_STRUCT, KW_INT, KW_FLOAT, KW_BOOL, KW_VOID, IDENTIFIER }
FIRST(FunctionDecl) = { KW_FN }
FIRST(StructDecl) = { KW_STRUCT }
FIRST(VarDecl) = { KW_INT, KW_FLOAT, KW_BOOL, KW_VOID, IDENTIFIER, KW_STRUCT }
FIRST(Statement) = { LBRACE, KW_IF, KW_WHILE, KW_FOR, KW_RETURN, IDENTIFIER, 
                     INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL, KW_TRUE, KW_FALSE, 
                     LPAREN, OPERATOR_INCREMENT, OPERATOR_DECREMENT, OPERATOR_NOT, 
                     OPERATOR_MINUS, KW_INT, KW_FLOAT, KW_BOOL, KW_VOID, KW_STRUCT, SEMICOLON }
FIRST(BlockStmt) = { LBRACE }
FIRST(IfStmt) = { KW_IF }
FIRST(WhileStmt) = { KW_WHILE }
FIRST(ForStmt) = { KW_FOR }
FIRST(ReturnStmt) = { KW_RETURN }
FIRST(ExprStmt) = { IDENTIFIER, INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL, 
                    KW_TRUE, KW_FALSE, LPAREN, OPERATOR_INCREMENT, OPERATOR_DECREMENT, 
                    OPERATOR_NOT, OPERATOR_MINUS }
FIRST(EmptyStmt) = { SEMICOLON }
FIRST(Expression) = { IDENTIFIER, INT_LITERAL, FLOAT_LITERAL, STRING_LITERAL, 
                      KW_TRUE, KW_FALSE, LPAREN, OPERATOR_INCREMENT, OPERATOR_DECREMENT, 
                      OPERATOR_NOT, OPERATOR_MINUS }
```

### Множества FOLLOW
```
FOLLOW(Program) = { EOF }
FOLLOW(Declaration) = { KW_FN, KW_STRUCT, KW_INT, KW_FLOAT, KW_BOOL, KW_VOID, IDENTIFIER, EOF }
FOLLOW(Statement) = { SEMICOLON, RBRACE, KW_ELSE, EOF }
FOLLOW(BlockStmt) = { SEMICOLON, RBRACE, KW_ELSE, EOF }
FOLLOW(IfStmt) = { SEMICOLON, RBRACE, KW_ELSE, EOF }
FOLLOW(WhileStmt) = { SEMICOLON, RBRACE, KW_ELSE, EOF }
FOLLOW(ForStmt) = { SEMICOLON, RBRACE, KW_ELSE, EOF }
FOLLOW(ReturnStmt) = { SEMICOLON, RBRACE, KW_ELSE, EOF }
FOLLOW(ExprStmt) = { SEMICOLON, RBRACE, KW_ELSE, EOF }
FOLLOW(EmptyStmt) = { SEMICOLON, RBRACE, KW_ELSE, EOF }
FOLLOW(Expression) = { SEMICOLON, RPAREN, COMMA, RBRACE, RBRACKET, DOT,
                       OPERATOR_ASSIGN, OPERATOR_ADD_ASSIGN, OPERATOR_SUB_ASSIGN, 
                       OPERATOR_MUL_ASSIGN, OPERATOR_DIV_ASSIGN, OPERATOR_MOD_ASSIGN,
                       OPERATOR_OR, OPERATOR_AND, OPERATOR_EQ, OPERATOR_NEQ,
                       OPERATOR_LT, OPERATOR_GT, OPERATOR_LTE, OPERATOR_GTE,
                       OPERATOR_PLUS, OPERATOR_MINUS, OPERATOR_MUL, OPERATOR_DIV, 
                       OPERATOR_MOD, KW_ELSE }
```
