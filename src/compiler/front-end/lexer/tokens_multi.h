#ifndef TOKENS_MULTI_H
#define TOKENS_MULTI_H
#ifndef LEXER_H
#include "./lexer.h"
#endif // !LEXER_H



// Char classifier for multi-char tokens
typedef struct CharClassifier {
  bool (*is_digit)(char c);
  bool (*is_alpha)(char c);
  bool (*is_alnum)(char c);
  bool (*is_whitespace)(char c);
  bool (*is_operator_start)(char c);
  bool (*is_identifier_start)(char c);
  bool (*is_identifier_continue)(char c);
} CharClassifier;

extern const CharClassifier ASCII_CLASSIFIER;

// Comment token
typedef enum {
  COMMENT_NONE,
  COMMENT_LINE,     // /// comment
  COMMENT_BLOCK,    // /* comment */
} CommentType;

typedef struct {
    const char* keyword;
    TokenType token_type;
} KeywordEntry;

extern const KeywordEntry KEYWORDS[];
extern const size_t KEYWORD_COUNT;

#endif // !TOKENS_MULTI_H
