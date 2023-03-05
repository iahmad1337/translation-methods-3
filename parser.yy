%require "3.8"
%debug
%language "c++"
/* This option only works when we use variant/union */
%define api.token.constructor
%define api.value.type variant
%define api.value.automove
%define parse.assert
%define parse.error detailed
%define parse.trace
%locations

/* See 3.7.15 for %code directive details */
%code requires // *.hh
{
#include <memory> // std::unique_ptr
#include <string>
#include <vector>

#include "ast.hh"

  using string_uptr = std::unique_ptr<std::string>;
  using string_uptrs = std::vector<string_uptr>;

}

%code // *.cc
{
#include <climits>  // INT_MIN, INT_MAX
#include <iostream>
#include <sstream>

  namespace yy
  {
    // Prototype of the yylex function providing subsequent tokens.
    static parser::symbol_type yylex ();

    // Print a vector of strings.
    std::ostream&
    operator<< (std::ostream& o, const string_uptrs& ss)
    {
      o << '{';
      const char *sep = "";
      for (const auto& s: ss)
        {
          o << sep << *s;
          sep = ", ";
        }
      return o << '}';
    }
  }

  template <typename... Args>
  string_uptr
  make_string_uptr (Args&&... args)
  {
    // std::make_unique is C++14.
    return string_uptr (new std::string{std::forward<Args> (args)...});
  }


}

%nterm <std::shared_ptr<TTree>> file;
%nterm <std::shared_ptr<TTree>> statements;
%nterm <std::shared_ptr<TTree>> statement;
%nterm <std::shared_ptr<TTree>> compound_stmt;
%nterm <std::shared_ptr<TTree>> simple_stmt;
%nterm <std::shared_ptr<TNode>> expr;
%nterm <std::shared_ptr<TTree>> arglist;

/* https://www.gnu.org/software/bison/manual/html_node/Token-Decl.html */
%token <std::string> ID;
%token <std::string> STRING;
%token <int> NUMBER;
%token EOF;
%token INDENT;
%token DEDENT;
%token EQ "==";
%token NEQ "!=";
%token LESS "<";
%token GREATER ">";
%token PLUS "+";
%token ASTERISK "*";
%token ASS "=";
%token LPAREN "(";
%token RPAREN ")";
%token COLON ":";

%printer { yyo << '(' << &$$ << ") " << $$; } <*>;
%printer { yyo << *$$; } <string_uptr>;

%%

file:
    statements EOF {
        $$ = std::make_shared<TTree>(
            "file",
            $1
        );
    }
;

statements:
    statement statements {
        $$ = $2
        auto tptr = dynamic_cast<TTree*>($$.get());
        assert(tptr != nullptr);
        assert(tptr->name == "statements");
        tptr->children.insert(tptr->children.begin(), $1);
    }
    | %empty {
        $$ = std::make_shared<TTree>("statements");
    }
;

statement:
    compound_stmt {
        $$ = std::make_shared<TTree>(
            "compound-statement",
            $1
        );
    }
    | simple_stmt {
        $$ = $1;
    }
;

simple_stmt:
    expr {
        $$ = std::make_shared<TTree>(
            "simple-statement",
            $1
        );
    }
;

// expr in `if` must be of type int
// expr in `for` must be of special `range` type
compound_stmt:
    "if" expr ":" INDENT statements DEDENT {
        $$ = std::make_shared<TTree>(
            "if",
            $2,
            $5
        );
    }
    | "for" ID "in" expr ":" INDENT statements DEDENT {
        $$ = std::make_shared<TTree>(
            "for-loop",
            $2,
            $4,
            $7
        );
    }
;

/* all expressions will be typed and the types will be inferred during semantic */
/* analysis */

/* Precedence: https://www.gnu.org/software/bison/manual/html_node/Precedence.html */
/* The order of lines determines the precedence. */
/* There is a caveat for unary operators: */
/* https://www.gnu.org/software/bison/manual/html_node/Contextual-Precedence.html */
%right "="
%left "or"
%left "and"
%precedence NOT
%nonassoc "<" ">" "==" "!="
%left "+"
%left "*"

/* TODO: use union in generated C code for values (it's safe in C) */
expr:
    NUMBER {
        $$ = std::make_shared<TNumber>($1);
    }
    | STRING {
        $$ = std::make_shared<TString>($1);
    }
    | ID {
        $$ = std::make_shared<TId>($1);
    }
    | ID "=" expr {
        $$ = std::make_shared<TTree>("assign", std::make_shared<TId>($1), $3);
    }
    | ID "(" arglist ")" {
        $$ = std::make_shared<TTree>("invoke", $1, $3);
    }
    | "(" expr ")" { $$ = $2; }
    | expr "or" expr { $$ = std::make_shared<TTree>("or", $1, $3); }
    | expr "and" expr { $$ = std::make_shared<TTree>("and", $1, $3); }
    | "not" expr %prec NOT { $$ = std::make_shared<TTree>("not", $2); }
    | expr "==" expr { $$ = std::make_shared<TTree>("eq", $1, $3); }
    | expr "!=" expr { $$ = std::make_shared<TTree>("neq", $1, $3); }
    | expr "<" expr { $$ = std::make_shared<TTree>("less", $1, $3); }
    | expr ">" expr { $$ = std::make_shared<TTree>("greater", $1, $3); }
    | expr "+" expr { $$ = std::make_shared<TTree>("add", $1, $3); }
    | expr "*" expr { $$ = std::make_shared<TTree>("multiply", $1, $3); }
;

arglist:
    arglist "," expr {
        $$ = $1;
        auto tptr = dynamic_cast<TTree*>($$.get());
        assert(tptr != nullptr);
        $$->children.push_back($3);
    }
    | expr {
        $$ = std::make_shared<TTree>("arglist", $1);
    }
    | %empty {
        $$ = std::make_shared<TTree>("arglist");
    }
;

%%

// The last number return by the scanner is max - 1.
int max = 4;

namespace yy
{
  // The yylex function providing subsequent tokens:
  // TEXT         "I have three numbers for you."
  // NUMBER       1
  // NUMBER       2
  // NUMBER       ...
  // NUMBER       max - 1
  // TEXT         "And that's all!"
  // END_OF_FILE

  auto yylex(value_type* yyval, location_type* yyloc) -> parser::symbol_type {
    static bool isEof = false;
    static int count = 0;
    const int stage = count;

    static int indentLevel = 0;
    static bool isStartOfLine = true;

    if (isEof) {
      std::cerr << "WHAT THE FUCK!!!!!!" << std::endl;
      std::exit(228);
    }
    ++count;
    auto loc = parser::location_type{nullptr, stage + 1, stage + 1};

    char c = std::cin.get();
    switch (c) {
        case '(':
            return make_LPAREN(std::move(loc));
            break;
        case ')':
            return make_RPAREN(std::move(loc));
            return;
        case '=':
            c = std::cin.get();
            if (

    }
    if (stage == 0)
      return parser::make_TEXT (make_string_uptr ("I have numbers for you."), std::move (loc));
    else if (stage < max)
      return parser::make_NUMBER (stage, std::move (loc));
    else if (stage == max)
      return parser::make_TEXT (make_string_uptr ("And that's all!"), std::move (loc));
    else
      return parser::make_END_OF_FILE (std::move (loc));
  }

  // Mandatory error function
  void parser::error (const parser::location_type& loc, const std::string& msg) {
    std::cerr << loc << ": " << msg << '\n';
  }
}

int main (int argc, const char *argv[]) {
  if (2 <= argc && isdigit (static_cast<unsigned char> (*argv[1])))
    {
      auto maxl = strtol (argv[1], nullptr, 10);
      max = INT_MIN <= maxl && maxl <= INT_MAX ? int(maxl) : 4;
    }
  auto&& p = yy::parser{};
  p.set_debug_level (!!getenv ("YYDEBUG"));
  return p.parse ();
}

// Local Variables:
// mode: C++
// End:
