#ifndef STRING_REGULAR_H
#define STRING_REGULAR_H

#include "string_common.h"
#include "string_identifier.h"
#include "string_postfix.h"

// Escape sequence length calculator (same as before)
int get_escape_length(const char* ptr) {
    if (*ptr != '\\') return 0;
    
    ptr++;
    if (!*ptr) return 1;
    
    switch (*ptr) {
        case '\'': case '"': case '?': case '\\':
        case 'a': case 'b': case 'f': case 'n': case 'r': case 't': case 'v':
            return 2;
        case 'x': {
            ptr++;
            int len = 2;
            if (isxdigit(*ptr)) { ptr++; len++; }
            if (isxdigit(*ptr)) { ptr++; len++; }
            return len;
        }
        case 'u': {
            ptr++;
            int len = 2;
            for (int i = 0; i < 4 && *ptr && isxdigit(*ptr); i++) {
                ptr++; len++;
            }
            return len;
        }
        case 'U': {
            ptr++;
            int len = 2;
            for (int i = 0; i < 8 && *ptr && isxdigit(*ptr); i++) {
                ptr++; len++;
            }
            return len;
        }
        case '0': case '1': case '2': case '3':
        case '4': case '5': case '6': case '7': {
            int len = 2;
            ptr++;
            for (int i = 0; i < 2 && *ptr >= '0' && *ptr <= '7'; i++) {
                ptr++; len++;
            }
            return len;
        }
        default:
            return 2;
    }
}

// Emit content token (same as before)
void emit_content_with_lines(const char* start, const char* end, 
                              StringTokenType type, int line, int col,
                              void (*emit_token)(StringToken)) {
    if (start >= end) return;
    
    const char* line_start = start;
    const char* ptr = start;
    int current_line = line;
    int current_col = col;
    
    while (ptr < end) {
        if (*ptr == '\n') {
            if (ptr > line_start) {
                StringToken token = {
                    .type = type,
                    .start = line_start,
                    .end = ptr,
                    .line = current_line,
                    .column = current_col
                };
                emit_token(token);
            }
            line_start = ptr + 1;
            current_line++;
            current_col = 0;
        }
        ptr++;
    }
    
    if (ptr > line_start) {
        StringToken token = {
            .type = type,
            .start = line_start,
            .end = ptr,
            .line = current_line,
            .column = current_col
        };
        emit_token(token);
    }
}

// Parse regular string with proper escape token emission
const char* parse_regular_string(const char* full_input, StringDescriptor* desc, 
                                  void (*emit_token)(StringToken), int line, int col) {
    const char* quote_start = desc->quote_start;
    int quote_type = desc->quote_type;
    char quote_char = desc->quote_char;
    
    // Determine token types
    StringTokenType begin_type, content_type, escape_type, end_type;
    if (desc->is_char_literal) {
        begin_type = TOKEN_CHAR_BEGIN;
        content_type = TOKEN_CHAR_CONTENT;
        escape_type = TOKEN_CHAR_ESCAPE;
        end_type = TOKEN_CHAR_END;
    } else if (quote_type == 3) {
        begin_type = TOKEN_MULTILINE_BEGIN;
        content_type = TOKEN_MULTILINE_CONTENT;
        escape_type = TOKEN_MULTILINE_ESCAPE;
        end_type = TOKEN_MULTILINE_END;
    } else {
        begin_type = TOKEN_STRING_BEGIN;
        content_type = TOKEN_STRING_CONTENT;
        escape_type = TOKEN_STRING_ESCAPE;
        end_type = TOKEN_STRING_END;
    }
    
    // Emit BEGIN token
    StringToken begin_token = {
        .type = begin_type,
        .start = quote_start,
        .end = quote_start + quote_type,
        .line = line,
        .column = col + (quote_start - full_input)
    };
    emit_token(begin_token);
    
    const char* ptr = quote_start + quote_type;
    const char* content_start = ptr;
    int current_line = line;
    int current_col = col + (ptr - full_input);
    
    while (*ptr) {
        if (*ptr == '\\') {
            // Emit content before escape
            if (ptr > content_start) {
                StringToken content_token = {
                    .type = content_type,
                    .start = content_start,
                    .end = ptr,
                    .line = current_line,
                    .column = current_col
                };
                emit_token(content_token);
                
                // Update position
                for (const char* p = content_start; p < ptr; p++) {
                    if (*p == '\n') {
                        current_line++;
                        current_col = 0;
                    } else {
                        current_col++;
                    }
                }
            }
            
            // Emit escape sequence as ESCAPE token
            int esc_len = get_escape_length(ptr);
            if (esc_len > 0) {
                StringToken escape_token = {
                    .type = escape_type,
                    .start = ptr,
                    .end = ptr + esc_len,
                    .line = current_line,
                    .column = current_col
                };
                emit_token(escape_token);
                
                // Update position for escape
                for (int i = 0; i < esc_len; i++) {
                    if (ptr[i] == '\n') {
                        current_line++;
                        current_col = 0;
                    } else {
                        current_col++;
                    }
                }
                
                content_start = ptr + esc_len;
                ptr += esc_len;
                continue;
            }
        }
        
        // Check for closing quote
        if (*ptr == quote_char) {
            // Check for triple quotes
            if (quote_type == 3 && ptr[1] == quote_char && ptr[2] == quote_char) {
                // Emit remaining content
                if (ptr > content_start) {
                    StringToken content_token = {
                        .type = content_type,
                        .start = content_start,
                        .end = ptr,
                        .line = current_line,
                        .column = current_col
                    };
                    emit_token(content_token);
                }
                
                // Emit END token
                StringToken end_token = {
                    .type = end_type,
                    .start = ptr,
                    .end = ptr + 3,
                    .line = current_line,
                    .column = current_col
                };
                emit_token(end_token);
                
                // Parse postfix
                const char* postfix_start = ptr + 3;
                const char* postfix_end = parse_postfix(postfix_start, emit_token, 
                                                         current_line, current_col);
                desc->postfix_start = postfix_start;
                desc->postfix_len = postfix_end - postfix_start;
                
                return postfix_end;
            } else if (quote_type == 1) {
                // Emit remaining content
                if (ptr > content_start) {
                    StringToken content_token = {
                        .type = content_type,
                        .start = content_start,
                        .end = ptr,
                        .line = current_line,
                        .column = current_col
                    };
                    emit_token(content_token);
                }
                
                // Emit END token
                StringToken end_token = {
                    .type = end_type,
                    .start = ptr,
                    .end = ptr + 1,
                    .line = current_line,
                    .column = current_col
                };
                emit_token(end_token);
                
                // Parse postfix
                const char* postfix_start = ptr + 1;
                const char* postfix_end = parse_postfix(postfix_start, emit_token, 
                                                         current_line, current_col);
                desc->postfix_start = postfix_start;
                desc->postfix_len = postfix_end - postfix_start;
                
                return postfix_end;
            }
        }
        
        ptr++;
    }
    
    // Unterminated string
    StringToken error_token = {
        .type = TOKEN_ERROR,
        .start = quote_start,
        .end = ptr,
        .line = line,
        .column = col + (quote_start - full_input)
    };
    emit_token(error_token);
    
    return ptr;
}

#endif