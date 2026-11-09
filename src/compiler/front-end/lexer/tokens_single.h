#ifndef TOKENS_SINGLE_H
#define TOKENS_SINGLE_H
#ifndef LEXER_H
#include "./lexer.h"
#endif // !LEXER_H

// Single char token
typedef struct {
    char character;
    TokenType token_type;
    const char* description;
} SingleCharTokenEntry;

extern const SingleCharTokenEntry SINGLE_CHAR_TOKENS[];
extern const size_t SINGLE_CHAR_TOKEN_COUNT;
#endif // !TOKENS_SINGLE_H
