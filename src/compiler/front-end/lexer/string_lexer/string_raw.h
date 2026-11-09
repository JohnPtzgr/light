#ifndef STRING_RAW_H
#define STRING_RAW_H

#include "string_common.h"
#include "string_identifier.h"
#include "string_postfix.h"

// Parse raw string with proper quote escaping rules
const char* parse_raw_string(const char* full_input, StringDescriptor* desc,
                              void (*emit_token)(StringToken), int line, int col) {
    const char* quote_start = desc->quote_start;
    int quote_type = desc->quote_type;
    char quote_char = desc->quote_char;
    
    // Determine token types
    StringTokenType begin_type, content_type, end_type;
    if (desc->is_char_literal) {
        begin_type = TOKEN_CHAR_BEGIN;
        content_type = TOKEN_CHAR_CONTENT;
        end_type = TOKEN_CHAR_END;
    } else if (quote_type == 3) {
        begin_type = TOKEN_MULTILINE_BEGIN;
        content_type = TOKEN_MULTILINE_CONTENT;
        end_type = TOKEN_MULTILINE_END;
    } else {
        begin_type = TOKEN_STRING_BEGIN;
        content_type = TOKEN_STRING_CONTENT;
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
    
    // Parse raw string content
    while (*ptr) {
        // In raw strings, backslashes are literal EXCEPT when escaping the quote
        // But note: r"\" is invalid because the backslash escapes the quote
        if (*ptr == '\\' && ptr[1] == quote_char) {
            // In raw string, backslash+quote is two literal characters
            // But we need to handle the closing quote correctly
            ptr += 2;
            continue;
        }
        
        // Check for closing quote
        if (*ptr == quote_char) {
            // Check if this is a valid closing quote
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
                // For single quotes, we need to check if the quote was escaped
                // Look back to see if there's an odd number of backslashes
                const char* check = ptr - 1;
                int backslash_count = 0;
                while (check >= content_start && *check == '\\') {
                    backslash_count++;
                    check--;
                }
                
                if (backslash_count % 2 == 1) {
                    // Odd number of backslashes means this quote is escaped
                    // Continue without closing
                    ptr++;
                    continue;
                }
                
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
        
        // Track newlines for line counting
        if (*ptr == '\n') {
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
            content_start = ptr + 1;
            current_line++;
            current_col = 0;
        } else {
            current_col++;
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