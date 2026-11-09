#ifndef STRING_POSTFIX_H
#define STRING_POSTFIX_H

#include "string_common.h"

// Parse postfix tokens after a string literal
const char* parse_postfix(const char* ptr, void (*emit_token)(StringToken), 
                          int line, int col) {
    while (*ptr) {
        // Check for namespace separator "::"
        if (ptr[0] == ':' && ptr[1] == ':') {
            StringToken sep_token = {
                .type = TOKEN_POSTFIX_SEP,
                .start = ptr,
                .end = ptr + 2,
                .line = line,
                .column = col
            };
            emit_token(sep_token);
            ptr += 2;
            col += 2;
            continue;
        }
        
        // Check for identifier
        if (is_ident_start(*ptr)) {
            const char* ident_start = ptr;
            while (is_ident_char(*ptr)) {
                ptr++;
            }
            
            StringToken ident_token = {
                .type = TOKEN_POSTFIX_IDENT,
                .start = ident_start,
                .end = ptr,
                .line = line,
                .column = col
            };
            emit_token(ident_token);
            col += (ptr - ident_start);
            continue;
        }
        
        // Stop postfix parsing at whitespace or delimiters
        if (isspace(*ptr) || *ptr == ';' || *ptr == ',' || *ptr == ')' || 
            *ptr == ']' || *ptr == '}' || *ptr == '+' || *ptr == '-' || 
            *ptr == '*' || *ptr == '/' || *ptr == '=') {
            break;
        }
        
        // Invalid character in postfix
        StringToken error_token = {
            .type = TOKEN_ERROR,
            .start = ptr,
            .end = ptr + 1,
            .line = line,
            .column = col
        };
        emit_token(error_token);
        return ptr + 1;
    }
    
    return ptr;
}

#endif