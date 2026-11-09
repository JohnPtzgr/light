#include "./lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static inline bool ascii_is_digit(char c) {
  return c >= '0' && c <= '9';
}

static inline bool ascii_is_alpha(char c) { 
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline bool ascii_is_alnum(char c) {
  return ascii_is_digit(c) || ascii_is_alpha(c);
}

static inline bool ascii_is_whitespace(char c) { 
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static inline bool ascii_is_operator_start(char c) {
  return c == '=' || c == '!' || c == '<' || c == '>' || 
         c == '&' || c == '|' || c == '+' || c == '-' ||
         c == '*' || c == '/' || c == '%' || c == ':' ||
         c == '.' || c == '?';
}

static inline bool ascii_is_identifier_start(char c) {
    return ascii_is_alpha(c) || c == '_';
}

static inline bool ascii_is_identifier_continue(char c) {
  return ascii_is_alnum(c) || c == '_';
}

const CharClassifier ASCII_CLASSIFIER = {
  .is_digit = ascii_is_digit,
  .is_alpha = ascii_is_alpha,
  .is_alnum = ascii_is_alnum,
  .is_whitespace = ascii_is_whitespace,
  .is_operator_start = ascii_is_operator_start,
  .is_identifier_start = ascii_is_identifier_start,
  .is_identifier_continue = ascii_is_identifier_continue
};

const KeywordEntry KEYWORDS[] = {
  // Core keywords
  {"let", TOKEN_LET},
  {"fn", TOKEN_FN},
  {"if", TOKEN_IF},
  {"else", TOKEN_ELSE},
  {"elif", TOKEN_ELIF},
  {"while", TOKEN_WHILE},
  {"do", TOKEN_DO},
  {"for", TOKEN_FOR},
  {"loop", TOKEN_LOOP},
  {"break", TOKEN_BREAK},
  {"continue", TOKEN_CONTINUE},
  {"try", TOKEN_TRY},
  {"catch", TOKEN_CATCH},
  {"finally", TOKEN_FINALLY},
  {"defer", TOKEN_DEFER},
  {"_", TOKEN_USKIP},
  {"return", TOKEN_RETURN},
  
  // Type definition keywords
  {"struct", TOKEN_STRUCT},
  {"class", TOKEN_CLASS},
  {"enum", TOKEN_ENUM},
  {"union", TOKEN_UNION},
  {"data", TOKEN_DATA},
  {"state", TOKEN_STATE},
  {"event", TOKEN_EVENT},
  {"impl", TOKEN_IMPL},
  {"trait", TOKEN_TRAIT},
  {"mod", TOKEN_MOD},
  
  // Control flow
  {"match", TOKEN_MATCH},
  
  // Literals
  {"true", TOKEN_TRUE},
  {"false", TOKEN_FALSE},
  {"nil", TOKEN_NIL},
  {"error", TOKEN_ERROR},
  {"void", TOKEN_VOID},
  {"var", TOKEN_VAR},
  {"type", TOKEN_TYPE},
  {"bool", TOKEN_BOOL},
  {"object", TOKEN_OBJECT},
  {"ptr", TOKEN_PTR},
  
  // Operators as keywords
  {"as", TOKEN_AS},
  {"is", TOKEN_IS},
  {"where", TOKEN_WHERE},
  
  // Visibility and storage
  {"pub", TOKEN_PUB},
  {"priv", TOKEN_PRIV},
  {"prot", TOKEN_PROT},
  {"static", TOKEN_STATIC},
  {"unique", TOKEN_UNIQUE},
  {"shared", TOKEN_SHARED},
  {"autoref", TOKEN_AUTOREF},
  {"arena", TOKEN_ARENA},
  {"atomic", TOKEN_ATOMIC},
  
  // Mutability
  {"const", TOKEN_CONST},
  {"mut", TOKEN_MUT},
  {"dyn", TOKEN_DYN},
  {"restrict", TOKEN_RESTRICT},
  {"volatile", TOKEN_VOLATILE},
  {"inline", TOKEN_INLINE},
  {"constexpr", TOKEN_CONSTEXPR},
  {"expr", TOKEN_EXPR},
  {"threadlocal", TOKEN_THREADLOCAL},
  
  // FFI and macros
  {"extern", TOKEN_EXTERN},
  {"macro", TOKEN_MACRO},
  {"quote", TOKEN_QUOTE},

  // Other special keywords
  {"safe", TOKEN_SAFE},
  {"unsafe", TOKEN_UNSAFE},
  {"async", TOKEN_ASYNC},
  {"await", TOKEN_AWAIT},
  {"yield", TOKEN_YIELD},
  {"pure", TOKEN_PURE},
  {"import", TOKEN_IMPORT},
  {"embed", TOKEN_EMBED},
  {"def", TOKEN_DEF},
  {"undef", TOKEN_UNDEF},
  {"ifdef", TOKEN_IFDEF},
  {"ifndef", TOKEN_IFNDEF},
  {"__LINE__", TOKEN_LINE},
  {"__FILE__", TOKEN_FILE},
  {"__FUNCTION__", TOKEN_FUNCTION},
};

const size_t KEYWORD_COUNT = sizeof(KEYWORDS) / sizeof(KEYWORDS[0]);


