#include "./lexer.h"
#include <stdio.h>
#include <string.h>

void print_token(Token* token) {
    printf("Token: %-12s '%.*s' at %d:%d\n",
           token_type_name(token->type),
           (int)token->length, token->start,
           token->line, token->column);
}

void test_lexer(const char* source, const char* filename) {
    printf("Testing: '%s'\n", source);
    printf("========================================\n");
    
    Lexer lexer;
    lexer_init(&lexer, source, strlen(source), filename);
    
    while (1) {
        Token token = lexer_next(&lexer);
        print_token(&token);
        
        if (token.type == TOKEN_CTRLEOF || token.type == TOKEN_CTRLERROR) {
            if (token.type == TOKEN_CTRLERROR) {
                lexer_print_error(&lexer, "test");
            }
            break;
        }
    }
    printf("\n");
}

int main() {
    // Test all single-character tokens
    test_lexer("(){}[].,;: #@?!^|&\'a\'\"a\"$~ +-*/%=", 
               "test_single_chars.light");
    // Test line comments
    test_lexer("/// This is a line comment\n"
               "let x = 42i32;\n"
               "let x = 1.9e2;\n"
               "/// Another comment\n"
               "let y = 24;",
               "test_line_comments.light");
    
    // Test block comments
    test_lexer("/* Simple block comment */\n"
               "let x = 42;\n"
               "/* Nested /* block */ comment */\n"
               "let y = 24;",
               "test_block_comments.light");
    
    // Test mixed
    test_lexer("/// Line comment\n"
               "class Base {};\n"
               "bool a = 1; \n"
               "let x = 42; /* inline block */\n"
               "/* Multi-line\n"
               "   block comment\n"
               "   with /// nested line comment (doesn't count)\n"
               "*/ let y = 24;\n"
               "let s == \"abc\"a;\n"
               "let! cdfsdfsdf := \'a\'a;",
               "test_mixed.light");
    
    return 0;
}