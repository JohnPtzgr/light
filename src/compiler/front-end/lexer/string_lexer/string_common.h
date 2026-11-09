#ifndef STRING_COMMON_H
#define STRING_COMMON_H

#include <stdbool.h>
#include <ctype.h>
#include <string.h>

typedef enum {
    TOKEN_CHAR_BEGIN,
    TOKEN_CHAR_CONTENT,
    TOKEN_CHAR_ESCAPE,
    TOKEN_CHAR_END,
    TOKEN_STRING_BEGIN,
    TOKEN_STRING_CONTENT,
    TOKEN_STRING_ESCAPE,
    TOKEN_STRING_END,
    TOKEN_MULTILINE_BEGIN,
    TOKEN_MULTILINE_CONTENT,
    TOKEN_MULTILINE_ESCAPE,
    TOKEN_MULTILINE_END,
    TOKEN_POSTFIX_SEP,
    TOKEN_POSTFIX_IDENT,
    TOKEN_ERROR
} StringTokenType;

typedef struct {
    StringTokenType type;
    const char* start;
    const char* end;
    int line;
    int column;
} StringToken;

// Prefix checking functions
bool is_prefix_char(char c) {
    return (c == 'u' || c == 'r' || c == 'c' || c == 'f' || c == 'b');
}

int check_multichar_prefix(const char* start) {
    if (strncmp(start, "u8", 2) == 0) return 2;
    if (strncmp(start, "u16", 3) == 0) return 3;
    if (strncmp(start, "u32", 3) == 0) return 3;
    return 0;
}

bool has_prefix(const char* start, const char* end, char prefix_char) {
    for (const char* p = start; p < end; p++) {
        if (*p == prefix_char) return true;
    }
    return false;
}

bool is_valid_prefix(const char* start, const char* end) {
    int len = end - start;
    if (len == 0) return true;
    
    const char* ptr = start;
    bool found_u_variant = false;
    
    while (ptr < end) {
        int u_prefix_len = check_multichar_prefix(ptr);
        if (u_prefix_len > 0) {
            if (found_u_variant) return false;
            found_u_variant = true;
            ptr += u_prefix_len;
        }
        else if (is_prefix_char(*ptr)) {
            if (*ptr == 'u' && found_u_variant) return false;
            if (*ptr == 'u') found_u_variant = true;
            ptr++;
        }
        else {
            return false;
        }
    }
    return true;
}

// Get quote type (single, double, or triple)
int get_quote_type(const char* ptr) {
    if (ptr[0] == '"' && ptr[1] == '"' && ptr[2] == '"') return 3;
    if (ptr[0] == '\'' && ptr[1] == '\'' && ptr[2] == '\'') return 3;
    if (ptr[0] == '"') return 1;
    if (ptr[0] == '\'') return 1;
    return 0;
}

// Check if character is valid for identifier
bool is_ident_start(char c) {
    return (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

bool is_ident_char(char c) {
    return (c == '_' || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'));
}

#endif