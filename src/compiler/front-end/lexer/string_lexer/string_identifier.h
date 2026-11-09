#ifndef STRING_IDENTIFIER_H
#define STRING_IDENTIFIER_H

#include "string_common.h"

typedef struct {
    const char* prefixes_start;
    int prefixes_len;
    const char* quote_start;
    int quote_type;
    char quote_char;
    bool is_raw;
    bool is_format;
    bool is_char_literal;
    const char* postfix_start;  // Start of postfix section (after closing quote)
    int postfix_len;            // Length of postfix section (0 if none)
} StringDescriptor;

StringDescriptor identify_string_literal(const char* ptr) {
    StringDescriptor desc = {NULL, 0, NULL, 0, 0, false, false, false, NULL, 0};
    
    const char* start = ptr;
    const char* prefix_end = ptr;
    
    // Parse prefixes
    while (1) {
        // Check for multichar prefix first
        int multi_len = check_multichar_prefix(prefix_end);
        if (multi_len > 0) {
            prefix_end += multi_len;
            continue;
        }
        // Check for single char prefix
        else if (is_prefix_char(*prefix_end)) {
            prefix_end++;
            continue;
        }
        break;
    }
    
    int prefix_len = prefix_end - start;
    if (!is_valid_prefix(start, prefix_end)) {
        desc.prefixes_len = 0;
        prefix_end = start;
        prefix_len = 0;
    } else {
        desc.prefixes_start = start;
        desc.prefixes_len = prefix_len;
        desc.is_raw = has_prefix(desc.prefixes_start, prefix_end, 'r');
        desc.is_format = has_prefix(desc.prefixes_start, prefix_end, 'f');
    }
    
    // Get quote type
    desc.quote_type = get_quote_type(prefix_end);
    if (desc.quote_type == 0) {
        return desc;
    }
    
    desc.quote_start = prefix_end;
    desc.quote_char = *prefix_end;
    desc.is_char_literal = (desc.quote_char == '\'' && desc.quote_type == 1);
    
    return desc;
}

// Helper function to update postfix info after parsing the string
void update_postfix_info(StringDescriptor* desc, const char* postfix_start, int postfix_len) {
    desc->postfix_start = postfix_start;
    desc->postfix_len = postfix_len;
}

#endif