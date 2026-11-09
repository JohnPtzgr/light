#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Token Types
typedef enum {
  // Special
  TOKEN_CTRLEOF,
  TOKEN_CTRLERROR,

  // Single-character tokens
  TOKEN_SCLPAREN,      // (
  TOKEN_SCRPAREN,      // )
  TOKEN_SCLBRACE,      // {
  TOKEN_SCRBRACE,      // }
  TOKEN_SCLBRACKET,    // [
  TOKEN_SCRBRACKET,    // ]
  TOKEN_SCCOMMA,       // ,
  TOKEN_SCSEMICOLON,   // ;
  TOKEN_SCCOLON,       // :
  TOKEN_SCDOT,         // .
  TOKEN_SCHASH,        // #
  TOKEN_SCAT,          // @
  TOKEN_SCQUESTION,    // ?
  TOKEN_SCBANG,        // !
  TOKEN_SCCARET,       // ^
  TOKEN_SCPIPE,        // |
  TOKEN_SCAMP,         // &
  TOKEN_SCQUOTE,       // '
  TOKEN_SCDQUOTE,      // "
  TOKEN_SCDOLLAR,      // $
  TOKEN_SCTILDE,       // ~
  TOKEN_SCPLUS,        // +
  TOKEN_SCMINUS,       // -
  TOKEN_SCSTAR,        // *
  TOKEN_SCSLASH,       // /
  TOKEN_SCPERCENT,     // %
  TOKEN_SCASSIGN,      // =
  TOKEN_SCLESS,        // <
  TOKEN_SCGREATER,     // >

  // Whitespace and comments
  TOKEN_WHITESPACE,
  TOKEN_COMMENT,

  // Keywords
  TOKEN_LET,        // let
  TOKEN_FN,         // fn
  TOKEN_IF,         // if
  TOKEN_ELSE,       // else
  TOKEN_ELIF,       // elif
  TOKEN_WHILE,      // while
  TOKEN_DO,         // do
  TOKEN_FOR,        // for
  TOKEN_LOOP,       // loop
  TOKEN_BREAK,      // break
  TOKEN_CONTINUE,      // continue
  TOKEN_TRY,        // try
  TOKEN_CATCH,      // catch
  TOKEN_FINALLY,    // finally
  TOKEN_DEFER,      // defer
  TOKEN_USKIP,      // _
  TOKEN_RETURN,     // return
  TOKEN_STRUCT,     // struct
  TOKEN_CLASS,      // class
  TOKEN_ENUM,       // enum
  TOKEN_UNION,      // union
  TOKEN_DATA,       // data
  TOKEN_STATE,      // state
  TOKEN_EVENT,      // event
  TOKEN_IMPL,       // impl
  TOKEN_TRAIT,      // trait
  TOKEN_MOD,        // mod
  TOKEN_MATCH,      // match
  TOKEN_TRUE,       // true
  TOKEN_FALSE,      // false
  TOKEN_NIL,        // nil
  TOKEN_ERROR,      // error
  TOKEN_VOID,       // void
  TOKEN_VAR,        // var
  TOKEN_TYPE,       // type
  TOKEN_BOOL,       // bool
  TOKEN_OBJECT,     // object
  TOKEN_PTR,        // ptr
  TOKEN_AS,         // as
  TOKEN_IS,         // is
  TOKEN_WHERE,      // where
  TOKEN_PUB,        // pub
  TOKEN_PRIV,       // priv
  TOKEN_PROT,       // prot
  TOKEN_STATIC,     // static
  TOKEN_UNIQUE,     // unique
  TOKEN_SHARED,     // shared
  TOKEN_AUTOREF,    // autoref
  TOKEN_ARENA,      // arena
  TOKEN_ATOMIC,     // atomic
  TOKEN_CONST,      // const
  TOKEN_MUT,        // mut
  TOKEN_DYN,        // dyn
  TOKEN_RESTRICT,   // restrict
  TOKEN_VOLATILE,   // volatile
  TOKEN_INLINE,     // inline
  TOKEN_CONSTEXPR,  // constexpr
  TOKEN_EXPR,       // expr
  TOKEN_THREADLOCAL,// threadlocal


  // Special keywords
  TOKEN_EXTERN,     // extern
  TOKEN_MACRO,      // macro
  TOKEN_QUOTE,      // quote
  TOKEN_SAFE,       // safe
  TOKEN_UNSAFE,     // unsafe
  TOKEN_ASYNC,      // async
  TOKEN_AWAIT,      // await
  TOKEN_YIELD,      // yield
  TOKEN_PURE,       // pure
  TOKEN_IMPORT,     // import
  TOKEN_EMBED,      // embed
  TOKEN_DEF,        // def
  TOKEN_UNDEF,      // undef
  TOKEN_IFDEF,      // ifdef
  TOKEN_IFNDEF,     // ifndef
  TOKEN_LINE,       // __LINE__
  TOKEN_FILE,       // __FILE__
  TOKEN_FUNCTION,   // __FUNCTION__

  // Placeholder for future phases
  TOKEN_IDENTIFIER,
  TOKEN_NUMBER,
  TOKEN_CHAR,
  TOKEN_LIFETIMEREF,
  TOKEN_DECORATOR,
  
  // String lexer tokens
  TOKEN_STRING_LITERAL,      // Complete string literal (simple case)
  TOKEN_STRING_PART,         // Part of complex string (for incremental parsing)
  
  // For string lexer callbacks
  TOKEN_STRING_BEGIN,
  TOKEN_STRING_CONTENT,
  TOKEN_STRING_ESCAPE,
  TOKEN_STRING_END,
  TOKEN_STRING_INTERP_START,
  TOKEN_STRING_INTERP_END,
  TOKEN_STRING_POSTFIX_SEP,
  TOKEN_STRING_POSTFIX_IDENT,

  TOKEN_COUNT
} TokenType;

// Token structure
typedef struct {
  TokenType type;
  const char* start;
  size_t length;
  int line;
  int column;
} Token;

// Lexer context
typedef struct Lexer {
  const char* source;
  size_t source_len;
  const char* current;
  int line;
  int column;
  const char* filename;
  
  // For error reporting
  char error_msg[256];
  bool had_error;
} Lexer;

Token lexer_skip_line_comment(Lexer* lexer);
Token lexer_skip_block_comment(Lexer* lexer);
void lexer_init(Lexer* lexer, const char* source, size_t len, const char* filename);
Token lexer_next(Lexer* lexer);
const char* token_type_name(TokenType type);
void lexer_print_error(Lexer* lexer, const char* context);
Token lexer_identifier(Lexer* lexer);
TokenType lexer_check_keyword(const char* start, size_t length);

typedef enum {
  STRING_NORMAL,     // "hello"
  STRING_CUSTOM,     // "hello"custom (user-defined)
  STRING_RAW,        // "hello"r  (raw, no escapes)
  STRING_BYTE,       // "hello"b  (byte array)
  STRING_C,          // "hello"c  (C string, null-terminated)
  STRING_WIDE,       // "hello"w  (wide string)
  STRING_FORMAT,     // "hello {name}"f (format string)
  STRING_UTF,        // u"hello"  default utf-X format
  STRING_UTF8,       // u8"hello"  utf-8
  STRING_UTF16,      // u16"hello"  utf-16
  STRING_UTF32,      // u32"hello"  utf-32
  STRING_MULTILINE,  // """hello""" (no suffix needed)
} StringType;

typedef struct {
  const char* name;      // e.g., "json", "regex", "sql"
  size_t length;         // length of the name
  bool is_standard;      // true for built-in (r,b,c,w,f)
} StringSuffixInfo;

typedef struct {
  const char* start;
  size_t length;
  StringType type;
  const char* suffix_start;  // Points to the suffix in source
  size_t suffix_length;
  bool has_escapes;  // For normal strings
} StringLiteral;

typedef struct {
  const char* suffix;
  size_t len;
  StringType type;
} StringSuffixEntry;

typedef struct {
  const char* prefix;
  size_t len;
  StringType type;
} StringPrefixEntry;

typedef enum {
  ESCAPE_SUCCESS,
  ESCAPE_INVALID,
  ESCAPE_TRUNCATED
} EscapeResult;

Token lexer_string(Lexer* lexer);
Token lexer_char(Lexer* lexer);
Token lexer_multiline_string(Lexer* lexer);
// bool lexer_check_string_suffix(Lexer* lexer, StringType* out_type, size_t* out_len);

// And make sure lexer_number is declared before lexer_next
Token lexer_number(Lexer* lexer);  // Forward declaration

#include "./tokens_single.h"
#include "./tokens_multi.h"

CommentType lexer_check_comment(Lexer* lexer);

#endif // !LEXER_H