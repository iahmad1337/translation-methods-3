# Лабораторная 3: Bison & ANTLR
Вариант: 3 (Перевод с Python на Си)

## Грамматика
В грамматику входят обычные арифметические выражения (в том числе вызов функций), присваивания, `for` и `if`
блоки.


Замечание: питону
[не важны пустые строчки](https://stackoverflow.com/questions/60143061/meaning-of-an-empty-line-in-python-source-code-file)
поэтому их можно игнорировать

```
%start file;

file:
    statements
;

statements:
    statement statements
    | %empty
;

%nterm <std::shared_ptr<TTree>> statement;
statement:
    compound_stmt
    | simple_stmt
;

simple_stmt:
    expr LF
;

compound_stmt:
    "if" expr ":" LF INDENT statements DEDENT
    | "for" ID "in" expr ":" LF INDENT statements DEDENT
;

%right "=";
%left "or";
%left "and";
%precedence "not";
%nonassoc "<" ">" "==" "!=";
%left "-" "+";
%left "*";

expr:
    NUMBER
    | STRING
    | ID
    | ID "=" expr
    | ID "(" arglist ")"
    | "(" expr ")"
    | expr "or" expr
    | expr "and" expr
    | "not" expr
    | expr "==" expr
    | expr "!=" expr
    | expr "<" expr
    | expr ">" expr
    | expr "-" expr
    | expr "+" expr
    | expr "*" expr
;

arglist:
    expr arglist-cont
    | %empty
;

arglist-cont:
    %empty
    | "," expr arglist-cont
;

```


## Лексический анализатор (токенайзер)
Для определения вложенности я решил ввести необычный токен - `DEDENT`. Его можно
увидеть в грамматике из оффициальной доки питона: https://docs.python.org/3/reference/grammar.html

Ещё больше информации о DEDENT здесь: https://docs.python.org/3/reference/lexical_analysis.html#indentation

Другого решения для определения вложенности я не могу придумать, кажется что без
этого никак.

Все отступы по умолчанию равны четырём пробелам, я решил не мудрить с разными
величинами отступов. Табы тоже не принимаются за отступы.

Список лексем (токенов):
```
%token <std::string> ID;
%token <std::string> STRING;
%token <std::string> NUMBER;
%token INDENT;
%token DEDENT;
%token LF;
%token EQ "==";
%token NEQ "!=";
%token LESS "<";
%token GREATER ">";
%token PLUS "+";
%token MINUS "-";
%token ASTERISK "*";
%token ASS "=";
%token LPAREN "(";
%token RPAREN ")";
%token COLON ":";
%token IF "if";
%token FOR "for";
%token IN "in";
%token AND "and";
%token OR "or";
%token NOT "not";
%token COMMA ",";
```

