%require "3.8"
%debug
%language "c++"
%define api.token.constructor
%define api.value.type {std::shared_ptr<TTree>}
%define api.value.automove
%define parse.assert
%locations

%code requires // *.hh
{
#include <memory> // std::unique_ptr
#include <string>
#include <vector>

  using string_uptr = std::unique_ptr<std::string>;
  using string_uptrs = std::vector<string_uptr>;

  using TPtr = std::shared_ptr<TTree>;

  struct TTree {
    std::string name;
    std::vector<TPtr> children;
  };
  struct TTraverser {
  public:
    TTraverser() = delete;
    TTraverser(std::ostream& os_, const char* indent = "    ")
      : os{os_}, indent{indent_}, indent_level{0} {}
    TTraverser(const TTraverser&) = default;
    TTraverser& operator=(const TTraverser&) = default;

    void traverse(const TTree* t) {
      for (int i=0; i < indent_level; i++) {
        os << indent;
      }
      os << name << std::endl;
      indent_level++;
      for (const auto& c : t->children) {
        assert(c.use_count() > 0);
        traverse(c.get());
      }
      indent_level--;
    }
  private:
    int indent_level = 0;
    const char* indent;
    std::ostream &os;
  };

  template<typename ...Args>
  TPtr MakePtr(Args&&... args) {
    return std::make_shared<TTree>(std::forward<Args>(args)...);
  }

  TPtr MakeInvocation(std::string identifier, std::shared_ptr<TTree> arglist) {
    return std::make_shared<TTree>{
      "invocation",
      { std::make_shared<TTree>(
    };
  }
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

%token <std::string> STRING;
%token <int> NUMBER;
%printer { yyo << '(' << &$$ << ") " << $$; } <*>;
%printer { yyo << *$$; } <string_uptr>;
%token END_OF_FILE 0;

%%

file:
    statements EOF

statements:
    statement statements
    | EPS

statement:
    compound_stmt
    | simple_stmt

simple_stmt:
    expr

// expr in `if` must be of type int
// expr in `for` must be of special `range` type
compound_stmt:
    "if" expr ":" INDENT statements DEDENT
    | "for" IDENTIFIER "in" expr ":" INDENT statements DEDENT

// all expressions will be typed and the types will be inferred during semantic
// analysis

// Precedence: https://www.gnu.org/software/bison/manual/html_node/Precedence.html
// The order of lines determines the precedence.
// There is a caveat for unary operators:
// https://www.gnu.org/software/bison/manual/html_node/Contextual-Precedence.html
%right "="
%left "or"
%left "and"
%precedence NOT
%nonassoc "<" ">" "==" "!="
%left "+"
%left "*"

// TODO: use union in generated C code for values (it's safe in C)
expr:
    NUMBER
    | STRING
    | IDENTIFIER
    | IDENTIFIER "=" expr
    | IDENTIFIER "(" arglist ")" { $$ = std::make_shared<TTree>{"invoke", {std::vector{$1}, $3}}'; }
    | "(" expr ")" { $$ = $2; }
    | expr "or" expr { $$ = std::make_shared<TTree>{"or", {$1, $3}}'; }
    | expr "and" expr { $$ = std::make_shared<TTree>{"and", {$1, $3}}'; }
    | "not" expr %prec NOT { $$ = std::make_shared<TTree>{"not", {$2}}'; }
    | expr "==" expr { $$ = std::make_shared<TTree>{"eq", {$1, $3}}'; }
    | expr "!=" expr { $$ = std::make_shared<TTree>{"neq", {$1, $3}}'; }
    | expr "<" expr { $$ = std::make_shared<TTree>{"less", {$1, $3}}'; }
    | expr ">" expr { $$ = std::make_shared<TTree>{"greater", {$1, $3}}'; }
    | expr "+" expr { $$ = std::make_shared<TTree>{"add", {$1, $3}}'; }
    | expr "*" expr { $$ = std::make_shared<TTree>{"multiply", {$1, $3}}'; }
;

arglist:
    | arglist "," expr { $$ = std::move($1); $$->push_back($3); }
    | expr { $$ = std::make_shared<TTree>(std::vector<parser::value_type>{$1}); }
    | %empty
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

  static
  parser::symbol_type
  yylex (value_type* yyval, location_type* yyloc)
  {
    static int count = 0;
    const int stage = count;
    ++count;
    auto loc = parser::location_type{nullptr, stage + 1, stage + 1};
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
  void
  parser::error (const parser::location_type& loc, const std::string& msg)
  {
    std::cerr << loc << ": " << msg << '\n';
  }
}

int
main (int argc, const char *argv[])
{
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
