#include "./lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static inline char lexer_peek(Lexer* lexer) {
  return *lexer->current;
}

static inline char lexer_peek_next(Lexer* lexer) {
  if (*(lexer->current + 1) == '\0') return '\0';
  return *(lexer->current + 1);
}

static inline char lexer_peek_n(Lexer* lexer, int n) {
  if (*(lexer->current + n) == '\0') return '\0';
  return *(lexer->current + n);
}

static inline char lexer_advance(Lexer* lexer) {
  char c = *lexer->current++;
  if (c == '\n') {
    lexer->line++;
    lexer->column = 1;
  } else {
    lexer->column++;
  }
  return c;
}

static inline bool lexer_is_at_end(Lexer* lexer) {
  return *lexer->current == '\0';
}

static inline bool lexer_match(Lexer* lexer, char expected) {
  if (lexer_is_at_end(lexer)) return false;
  if (*lexer->current != expected) return false;
  lexer_advance(lexer);
  return true;
}

static inline void lexer_skip_whitespace(Lexer* lexer) {
  while (!lexer_is_at_end(lexer)) {
    char c = lexer_peek(lexer);
    if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
      lexer_advance(lexer);
    } else {
      break;
    }
  }
}

CommentType lexer_check_comment(Lexer* lexer) {
  if (lexer_peek(lexer) != '/') return COMMENT_NONE;
  
  // Check for line comment: ///
  if (lexer_peek_next(lexer) == '/' && 
    lexer_peek_next(lexer) == '/') {
    return COMMENT_LINE;
  }
  
  // Check for block comment: /*
  if (lexer_peek_next(lexer) == '*') {
    return COMMENT_BLOCK;
  }
  
  return COMMENT_NONE;
}

Token lexer_skip_line_comment(Lexer* lexer) {
  const char* start = lexer->current;
  int start_line = lexer->line;
  int start_col = lexer->column;
  
  // Skip the three slashes
  lexer_advance(lexer); // first '/'
  lexer_advance(lexer); // second '/'
  lexer_advance(lexer); // third '/'
  
  // Read until newline or EOF
  while (!lexer_is_at_end(lexer) && lexer_peek(lexer) != '\n') {
    lexer_advance(lexer);
  }
  
  Token token;
  token.type = TOKEN_COMMENT;
  token.start = start;
  token.length = lexer->current - start;
  token.line = start_line;
  token.column = start_col;
  return token;
}

Token lexer_skip_block_comment(Lexer* lexer) {
  const char* start = lexer->current;
  int start_line = lexer->line;
  int start_col = lexer->column;
  
  // Skip the /*
  lexer_advance(lexer); // '/'
  lexer_advance(lexer); // '*'
  
  int depth = 1;
  
  while (!lexer_is_at_end(lexer) && depth > 0) {
    char c = lexer_peek(lexer);
    
    if (c == '*' && lexer_peek_next(lexer) == '/') {
      lexer_advance(lexer); // '*'
      lexer_advance(lexer); // '/'
      depth--;
    } else if (c == '/' && lexer_peek_next(lexer) == '*') {
      // Nested block comment
      lexer_advance(lexer); // '/'
      lexer_advance(lexer); // '*'
      depth++;
    } else {
      lexer_advance(lexer);
    }
  }
  
  Token token;
  token.type = TOKEN_COMMENT;
  token.start = start;
  token.length = lexer->current - start;
  token.line = start_line;
  token.column = start_col;
  return token;
}

void lexer_init(Lexer* lexer, const char* source, size_t len, const char* filename) {
  lexer->source = source;
  lexer->source_len = len;
  lexer->current = source;
  lexer->line = 1;
  lexer->column = 1;
  lexer->filename = filename;
  lexer->had_error = false;
  lexer->error_msg[0] = '\0';
}

static Token make_token(Lexer* lexer, TokenType type) {
  Token token;
  token.type = type;
  token.start = lexer->current - 1;
  token.length = 1;
  token.line = lexer->line;
  token.column = lexer->column - 1;
  return token;
}

static Token make_token_length(Lexer* lexer, TokenType type, size_t len) {
  Token token;
  token.type = type;
  token.start = lexer->current - len;
  token.length = len;
  token.line = lexer->line;
  token.column = lexer->column - len;
  return token;
}

static Token error_token(Lexer* lexer, const char* message) {
  Token token;
  token.type = TOKEN_CTRLERROR;
  token.start = lexer->current;
  token.length = (int)strlen(message);
  token.line = lexer->line;
  token.column = lexer->column;
  
  lexer->had_error = true;
  strncpy(lexer->error_msg, message, sizeof(lexer->error_msg) - 1);
  
  return token;
}

static Token lexer_single_char_token(Lexer* lexer, char c) {
  // Look up in token table
  for (size_t i = 0; i < SINGLE_CHAR_TOKEN_COUNT; i++) {
    if (SINGLE_CHAR_TOKENS[i].character == c) {
      return make_token(lexer, SINGLE_CHAR_TOKENS[i].token_type);
    }
  }
  return error_token(lexer, "Unexpected character");
}

static void skip_whitespace(Lexer* lexer) {
  while (!lexer_is_at_end(lexer) && ASCII_CLASSIFIER.is_whitespace(lexer_peek(lexer))) {
    lexer_advance(lexer);
  }
}

Token lexer_lifetimeref(Lexer* lexer) {
  Token token = (Token){.column = lexer->column, .line=lexer->line, .start=lexer->current};
  char current = lexer_peek(lexer);
  char next = lexer_peek_next(lexer);
  if (current == '&' && next == '\'') {
    token.length = 2;
    token.type = TOKEN_LIFETIMEREF;
    lexer_advance(lexer);
    lexer_advance(lexer);
    return token;
  }
  token.length = 1;
  token.type = TOKEN_SCAMP;
  lexer_advance(lexer);
  return token;
}

bool is_at_complex_string_literal(Lexer* lexer) {
  switch (*lexer->current) {
   case 'b':
   case 'c':
   case 'f':
   case 'r':
   case 'u':
    break;
   default:
    return false;
  }
  size_t i = 0;
  while (lexer->current[i] && (lexer->current[i] != '\"' && lexer->current[i] != '\'') && (i < 8)) {
    if (lexer->current[i] != 'b' ||
        lexer->current[i] != 'c' ||
        lexer->current[i] != 'f' ||
        lexer->current[i] != 'r' ||
        lexer->current[i] != 'u' ||
        lexer->current[i] != '1' ||
        lexer->current[i] != '2' ||
        lexer->current[i] != '3' ||
        lexer->current[i] != '6' ||
        lexer->current[i] != '8') {
      return false;
    }
  }
  if (lexer->current[i] && (lexer->current[i] == '\"' || lexer->current[i] == '\'')) {
    return true;
  }
  return false;
}

Token lexer_next(Lexer* lexer) {
  lexer_skip_whitespace(lexer);
  
  if (lexer_is_at_end(lexer)) {
    return (Token){TOKEN_CTRLEOF, lexer->current, 0, lexer->line, lexer->column};
  }


  // Complex string/char handler
  if (is_at_complex_string_literal(lexer)) {
    // Complex string/char lexer goes here
  }
  

  // switch ((((int)lexer_peek(lexer)) << 8) + lexer_peek_next(lexer)) {
  //   case 0:
  //     break;
  //   const char* s = "";
  //   default:
  //     break;
  // }
  
  // // Check for multiline string first (triple quote)
  // if (lexer_peek(lexer) == '"' && 
  //     lexer_peek_next(lexer) == '"' && 
  //     lexer_peek_n(lexer, 2) == '"') {
  //   return lexer_multiline_string(lexer);
  // }
  
  // Check for comments
  CommentType comment_type = lexer_check_comment(lexer);
  if (comment_type == COMMENT_LINE) {
    return lexer_skip_line_comment(lexer);
  }
  if (comment_type == COMMENT_BLOCK) {
    return lexer_skip_block_comment(lexer);
  }
  
  char c = lexer_peek(lexer);
  
  // Handle simple string literals
  if (c == '"') {
    return lexer_string(lexer);
  }

  if (c == '&') {
    return lexer_lifetimeref(lexer);
  }
  
  // Handle simple character literals
  if (c == '\'') {
    lexer_advance(lexer); // consume the quote
    return lexer_char(lexer);
  }
  
  lexer_advance(lexer);
  
  // Check for identifier start
  if (ASCII_CLASSIFIER.is_identifier_start(c)) {
    return lexer_identifier(lexer);
  }
  
  // Check for numbers
  if (ASCII_CLASSIFIER.is_digit(c)) {
    return lexer_number(lexer);
  }
  
  // Single character token
  return lexer_single_char_token(lexer, c);
}

const char* token_type_name(TokenType type) {
  static const char* names[TOKEN_COUNT] = {
    "EOF", "ERROR",
    
    // Single-character tokens
    "LPAREN", "RPAREN", "LBRACE", "RBRACE", 
    "LBRACKET", "RBRACKET", "COMMA", "SEMICOLON",
    "COLON", "DOT", "HASH", "AT", "QUESTION", "BANG", "CARET",
    "PIPE", "AMP", "QUOTE", "DQUOTE", "DOLLAR", "TILDE",
    
    // Operators
    "PLUS", "MINUS", "STAR", "SLASH", "PERCENT", "ASSIGN", "LESS", "GREATER",
    
    // Whitespace and comments
    "WHITESPACE", "COMMENT",

    "LET", "FN", "IF", "ELSE", "ELIF", "WHILE", "DO", "FOR", "LOOP", "BREAK", "CONTINUE",
    "TRY", "CATCH", "FINALLY", "DEFER", "UNDERSCORE", "RETURN",
    "STRUCT", "CLASS", "ENUM", "UNION", "DATA", "STATE", "EVENT",
    "IMPL", "TRAIT", "MOD", "MATCH",
    "TRUE", "FALSE", "NIL", "ERROR", "VOID", "VAR", "TYPE", "BOOL", "OBJECT", "PTR",
    "AS", "IS", "WHERE",
    "PUB", "PRIV", "PROT",
    "STATIC", "UNIQUE", "SHARED", "AUTOREF", "ARENA", "ATOMIC",
    "CONST", "MUT", "DYN", "RESTRICT", "VOLATILE", "INLINE", "CONSTEXPR", "EXPR",
    "THREADLOCAL", "EXTERN", "MACRO", "QUOTE", "SAFE", "UNSAFE",
    "ASYNC", "AWAIT", "YIELD", "PURE", "IMPORT", "EMBED", "DEF", "UNDEF",
    "IFDEF", "IFNDEF", "LINE", "FILE", "FUNCTION",
    
    "IDENTIFIER", "NUMBER", "CHAR", 
    "LIFETIMEREF", "DECORATOR",
      // String lexer tokens
    "TOKEN_STRING_LITERAL",      // Complete string literal (simple case)
    "TOKEN_STRING_PART",         // Part of complex string (for incremental parsing)
    
    // For string lexer callbacks
    "TOKEN_STRING_BEGIN",
    "TOKEN_STRING_CONTENT",
    "TOKEN_STRING_ESCAPE",
    "TOKEN_STRING_END",
    "TOKEN_STRING_INTERP_START",
    "TOKEN_STRING_INTERP_END",
    "TOKEN_STRING_POSTFIX_SEP",
    "TOKEN_STRING_POSTFIX_IDENT",
  };
  
  if (type >= 0 && type < TOKEN_COUNT) {
    return names[type];
  }
  return "UNKNOWN";
}

// Simple perfect hash for keywords (can be optimized later)
TokenType lexer_check_keyword(const char* start, size_t length) {
  for (size_t i = 0; i < KEYWORD_COUNT; i++) {
    if (strlen(KEYWORDS[i].keyword) == length &&
        strncmp(KEYWORDS[i].keyword, start, length) == 0) {
      return KEYWORDS[i].token_type;
    }
  }
  return TOKEN_IDENTIFIER;
}

Token lexer_identifier(Lexer* lexer) {
  const char* start = lexer->current - 1;  // include first character
  int start_line = lexer->line;
  int start_col = lexer->column - 1;
  
  // Read identifier (letters, digits, underscore)
  while (!lexer_is_at_end(lexer) && 
        (ASCII_CLASSIFIER.is_alnum(lexer_peek(lexer)) || 
        lexer_peek(lexer) == '_')) {
    lexer_advance(lexer);
  }
  
  size_t length = lexer->current - start;
  
  // Check if it's a keyword
  TokenType type = lexer_check_keyword(start, length);
  
  Token token;
  token.type = type;
  token.start = start;
  token.length = length;
  token.line = start_line;
  token.column = start_col;
  return token;
}

void lexer_print_error(Lexer* lexer, const char* context) {
  fprintf(stderr, "%s:%d:%d: error: %s\n", 
          lexer->filename, lexer->line, lexer->column, 
          lexer->error_msg[0] ? lexer->error_msg : "unknown error");
  
  if (context) {
    fprintf(stderr, "  while: %s\n", context);
  }
  
  // Print the line with a caret
  const char* line_start = lexer->current;
  while (line_start > lexer->source && *(line_start - 1) != '\n') {
    line_start--;
  }
  
  const char* line_end = lexer->current;
  while (*line_end != '\0' && *line_end != '\n') {
    line_end++;
  }
  
  fprintf(stderr, "  ");
  for (const char* p = line_start; p < line_end; p++) {
    fputc(*p, stderr);
  }
  fputc('\n', stderr);
  
  fprintf(stderr, "  ");
  for (int i = 0; i < lexer->column - 1; i++) {
    fputc(' ', stderr);
  }
  fprintf(stderr, "^\n");
}

static const StringSuffixEntry STRING_SUFFIXES[] = {
  {"raw", 3, STRING_RAW},      // Alternative longer forms
  {"byte", 4, STRING_BYTE},
  {"cstr", 4, STRING_C},
  {"wide", 4, STRING_WIDE},
  {"fmt", 3, STRING_FORMAT},
};

static const StringPrefixEntry STRING_PREFIXES[] = {
  {"r", 1, STRING_RAW},
  {"b", 1, STRING_BYTE},
  {"c", 1, STRING_C},
  {"w", 1, STRING_WIDE},
  {"f", 1, STRING_FORMAT},
  {"u", 1, STRING_UTF},
  {"u8", 2, STRING_UTF8},
  {"u16", 3, STRING_UTF16},
  {"u32", 3, STRING_UTF32},
};

static const size_t STRING_SUFFIX_COUNT = sizeof(STRING_SUFFIXES) / sizeof(STRING_SUFFIXES[0]);

static const size_t STRING_PREFIX_COUNT = sizeof(STRING_SUFFIXES) / sizeof(STRING_SUFFIXES[0]);

static bool lexer_is_hex_digit(char c) {
  return (c >= '0' && c <= '9') ||
         (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}

static bool lexer_is_binary_digit(char c) {
  return c == '0' || c == '1';
}

static bool lexer_is_octal_digit(char c) {
  return c >= '0' && c <= '7';
}

static bool lexer_is_digit_separator(char c) {
  return c == '_';  // Allow 1_000_000 for readability
}

static bool lexer_check_string_suffix(Lexer* lexer, StringType* out_type, 
                                       StringSuffixInfo* out_info) {
  if (lexer_is_at_end(lexer)) return false;
  
  const char* start = lexer->current;
  const char* p = start;
  
  // Suffix can be letters or underscores (for custom suffixes)
  while (!lexer_is_at_end(lexer) && 
         (ASCII_CLASSIFIER.is_alpha(lexer_peek(lexer)) || 
          lexer_peek(lexer) == '_')) {
    lexer_advance(lexer);
    p++;
  }
  
  size_t len = p - start;
  if (len == 0) return false;
  
  // Copy the suffix name
  char suffix_name[64];
  if (len >= 64) return false;  // Too long
  strncpy(suffix_name, start, len);
  suffix_name[len] = '\0';
  
  // Check against built-in suffixes
  for (size_t i = 0; i < STRING_SUFFIX_COUNT; i++) {
    if (len == STRING_SUFFIXES[i].len &&
        strncmp(start, STRING_SUFFIXES[i].suffix, len) == 0) {
      
      *out_type = STRING_SUFFIXES[i].type;
      if (out_info) {
        out_info->name = STRING_SUFFIXES[i].suffix;
        out_info->length = len;
        out_info->is_standard = true;
      }
      return true;
    }
  }
  
  // Custom suffix (e.g., "abc"a, "data"json, "query"sql)
  *out_type = STRING_CUSTOM;
  if (out_info) {
    out_info->name = start;  // Points into source
    out_info->length = len;
    out_info->is_standard = false;
  }
  
  // We've already advanced past the suffix in the loop above
  return true;
}

static bool lexer_is_multiline_string(Lexer* lexer) {
  return lexer_peek(lexer) == '"' && 
         lexer_peek_next(lexer) == '"' && 
         lexer_peek_n(lexer, 2) == '"';
}

static EscapeResult lexer_parse_escape(Lexer* lexer, char* out_char, bool in_string) {
  if (lexer_peek(lexer) != '\\') return ESCAPE_INVALID;
  lexer_advance(lexer); // consume '\'
  
  if (lexer_is_at_end(lexer)) {
    return ESCAPE_TRUNCATED;
  }
  
  char c = lexer_peek(lexer);
  lexer_advance(lexer);
  
  // Simple escapes
  switch (c) {
    case 'n': *out_char = '\n'; return ESCAPE_SUCCESS;
    case 't': *out_char = '\t'; return ESCAPE_SUCCESS;
    case 'r': *out_char = '\r'; return ESCAPE_SUCCESS;
    case '0': *out_char = '\0'; return ESCAPE_SUCCESS;
    case '\\': *out_char = '\\'; return ESCAPE_SUCCESS;
    case '\'': *out_char = '\''; return ESCAPE_SUCCESS;
    case '"': *out_char = '"'; return ESCAPE_SUCCESS;
    
    // Hex escape: \x7F
    case 'x': {
      if (lexer_is_at_end(lexer)) return ESCAPE_TRUNCATED;
      if (!lexer_is_hex_digit(lexer_peek(lexer))) return ESCAPE_INVALID;
      
      char hex[3] = {lexer_peek(lexer), 0, 0};
      lexer_advance(lexer);
      
      if (!lexer_is_at_end(lexer) && lexer_is_hex_digit(lexer_peek(lexer))) {
        hex[1] = lexer_peek(lexer);
        lexer_advance(lexer);
        hex[2] = '\0';
      } else {
        hex[1] = '\0';
      }
      
      *out_char = (char)strtol(hex, NULL, 16);
      return ESCAPE_SUCCESS;
    }
    
    // Octal escape: \o177
    case 'o': {
      if (lexer_is_at_end(lexer)) return ESCAPE_TRUNCATED;
      
      char octal[4] = {0};
      int i = 0;
      
      while (i < 3 && !lexer_is_at_end(lexer) && 
             lexer_is_octal_digit(lexer_peek(lexer))) {
        octal[i++] = lexer_peek(lexer);
        lexer_advance(lexer);
      }
      
      if (i == 0) return ESCAPE_INVALID;
      
      *out_char = (char)strtol(octal, NULL, 8);
      return ESCAPE_SUCCESS;
    }
    
    // Binary escape: \b01111111
    case 'b': {
      if (lexer_is_at_end(lexer)) return ESCAPE_TRUNCATED;
      
      char binary[9] = {0};
      int i = 0;
      
      while (i < 8 && !lexer_is_at_end(lexer) && 
             lexer_is_binary_digit(lexer_peek(lexer))) {
        binary[i++] = lexer_peek(lexer);
        lexer_advance(lexer);
      }
      
      if (i == 0) return ESCAPE_INVALID;
      
      *out_char = (char)strtol(binary, NULL, 2);
      return ESCAPE_SUCCESS;
    }
    
    default:
      return ESCAPE_INVALID;
  }
}

Token lexer_char(Lexer* lexer) {
  const char* start = lexer->current - 1;
  int start_line = lexer->line;
  int start_col = lexer->column - 1;
  
  // Check for empty char literal: ''
  if (lexer_peek(lexer) == '\'') {
    lexer_advance(lexer);
    return error_token(lexer, "Empty character literal");
  }
  
  // Parse the character (could be escaped)
  int char_count = 0;
  char char_value = 0;
  
  if (lexer_peek(lexer) == '\\') {
    EscapeResult result = lexer_parse_escape(lexer, &char_value, false);
    if (result != ESCAPE_SUCCESS) {
      return error_token(lexer, "Invalid escape sequence in character literal");
    }
    char_count = 1;
  } else {
    char_value = lexer_peek(lexer);
    lexer_advance(lexer);
    char_count = 1;
  }
  
  // Check for multi-character literal
  if (!lexer_is_at_end(lexer) && lexer_peek(lexer) != '\'') {
    return error_token(lexer, "Multi-character literal not allowed (use string instead)");
  }
  
  // Expect closing quote
  if (lexer_is_at_end(lexer) || lexer_peek(lexer) != '\'') {
    return error_token(lexer, "Unterminated character literal");
  }
  lexer_advance(lexer); // consume closing quote
  
  // Check for suffix (like 'a'u8, 'x'custom)
  StringType suffix_type = STRING_NORMAL;
  StringSuffixInfo suffix_info = {0};
  bool has_suffix = lexer_check_string_suffix(lexer, &suffix_type, &suffix_info);
  
  Token token;
  token.type = TOKEN_CHAR;
  token.start = start;
  token.length = lexer->current - start;
  token.line = start_line;
  token.column = start_col;
  
  return token;
}

Token lexer_multiline_string(Lexer* lexer) {
  const char* start = lexer->current;  // points to first quote
  int start_line = lexer->line;
  int start_col = lexer->column;
  
  // Skip the three opening quotes
  lexer_advance(lexer); // first '"'
  lexer_advance(lexer); // second '"'
  lexer_advance(lexer); // third '"'
  
  // Read until closing triple quote
  int quote_count = 0;
  while (!lexer_is_at_end(lexer)) {
    if (lexer_peek(lexer) == '"') {
      quote_count++;
      lexer_advance(lexer);
      if (quote_count == 3) {
        break;
      }
    } else {
      quote_count = 0;
      lexer_advance(lexer);
    }
  }
  
  Token token;
  token.type = TOKEN_STRING_LITERAL;
  token.start = start;
  token.length = lexer->current - start;
  token.line = start_line;
  token.column = start_col;
  return token;
}

Token lexer_string(Lexer* lexer) {
  const char* start = lexer->current;
  int start_line = lexer->line;
  int start_col = lexer->column;
  
  char quote_char = lexer_peek(lexer);
  lexer_advance(lexer); // consume opening quote
  
  bool has_escapes = false;
  
  while (!lexer_is_at_end(lexer)) {
    char c = lexer_peek(lexer);
    
    if (c == quote_char) {
      lexer_advance(lexer); // consume closing quote
      break;
    }
    
    if (c == '\\') {
      has_escapes = true;
      char dummy;
      EscapeResult result = lexer_parse_escape(lexer, &dummy, true);
      if (result != ESCAPE_SUCCESS) {
        return error_token(lexer, "Invalid escape sequence in string");
      }
      continue;
    }
    
    lexer_advance(lexer);
  }
  
  if (lexer_is_at_end(lexer)) {
    return error_token(lexer, "Unterminated string literal");
  }
  
  // Check for suffix
  StringType suffix_type = STRING_NORMAL;
  StringSuffixInfo suffix_info = {0};
  bool has_suffix = lexer_check_string_suffix(lexer, &suffix_type, &suffix_info);
  
  Token token;
  token.type = TOKEN_STRING_LITERAL;
  token.start = start;
  token.length = lexer->current - start;
  token.line = start_line;
  token.column = start_col;
  
  // Store suffix info for later stages (parser will need this)
  // For now, we just return the token
  
  return token;
}

Token lexer_number(Lexer* lexer) {
  const char* start = lexer->current - 1;
  int start_line = lexer->line;
  int start_col = lexer->column - 1;
  
  // Check for hex/octal/binary prefix
  if (start[0] == '0' && !lexer_is_at_end(lexer)) {
    char next = lexer_peek(lexer);
    if (next == 'x' || next == 'X') {
      // Hexadecimal
      lexer_advance(lexer); // consume 'x'
      while (!lexer_is_at_end(lexer) && 
             (lexer_is_hex_digit(lexer_peek(lexer)) || 
              lexer_is_digit_separator(lexer_peek(lexer)))) {
        lexer_advance(lexer);
      }
    } else if (next == 'b' || next == 'B') {
      // Binary
      lexer_advance(lexer); // consume 'b'
      while (!lexer_is_at_end(lexer) && 
             (lexer_is_binary_digit(lexer_peek(lexer)) || 
              lexer_is_digit_separator(lexer_peek(lexer)))) {
        lexer_advance(lexer);
      }
    } else if (next >= '0' && next <= '7') {
      // Octal (starts with 0)
      while (!lexer_is_at_end(lexer) && 
             (lexer_is_octal_digit(lexer_peek(lexer)) || 
              lexer_is_digit_separator(lexer_peek(lexer)))) {
        lexer_advance(lexer);
      }
    } else {
      // Just a decimal 0
    }
  } else {
    // Decimal or float
    bool has_dot = false;
    bool has_exponent = false;
    
    // Integer part
    while (!lexer_is_at_end(lexer) && 
           (ASCII_CLASSIFIER.is_digit(lexer_peek(lexer)) || 
            lexer_is_digit_separator(lexer_peek(lexer)))) {
      lexer_advance(lexer);
    }
    
    // Fractional part
    if (!lexer_is_at_end(lexer) && lexer_peek(lexer) == '.') {
      has_dot = true;
      lexer_advance(lexer);
      
      while (!lexer_is_at_end(lexer) && 
             (ASCII_CLASSIFIER.is_digit(lexer_peek(lexer)) || 
              lexer_is_digit_separator(lexer_peek(lexer)))) {
        lexer_advance(lexer);
      }
    }
    
    // Exponent part
    if (!lexer_is_at_end(lexer) && 
        (lexer_peek(lexer) == 'e' || lexer_peek(lexer) == 'E')) {
      has_exponent = true;
      lexer_advance(lexer);
      
      // Optional sign
      if (!lexer_is_at_end(lexer) && 
          (lexer_peek(lexer) == '+' || lexer_peek(lexer) == '-')) {
        lexer_advance(lexer);
      }
      
      // Exponent digits
      while (!lexer_is_at_end(lexer) && 
             (ASCII_CLASSIFIER.is_digit(lexer_peek(lexer)) || 
              lexer_is_digit_separator(lexer_peek(lexer)))) {
        lexer_advance(lexer);
      }
    }
  }
  
  // Look for suffix (like 'u32', 'f64', 'i8', etc.)
  const char* suffix_start = lexer->current;
  size_t suffix_length = 0;
  
  if (!lexer_is_at_end(lexer) && ASCII_CLASSIFIER.is_alpha(lexer_peek(lexer))) {
    while (!lexer_is_at_end(lexer) && 
           (ASCII_CLASSIFIER.is_alnum(lexer_peek(lexer)) || 
            lexer_peek(lexer) == '_')) {
      lexer_advance(lexer);
      suffix_length++;
    }
  }
  
  Token token;
  token.type = TOKEN_NUMBER;
  token.start = start;
  token.length = lexer->current - start;
  token.line = start_line;
  token.column = start_col;
  return token;
}
