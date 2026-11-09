#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

// Forward declarations
typedef struct LiteralResult LiteralResult;
LiteralResult recognize_literal(const char* current_ptr);

// Structure to hold literal result
struct LiteralResult {
  const char* end;
  bool found;
  const char* prefixes;
  int prefix_len;
  const char* quote_start;
  char quote_char;
  const char* content_start;
  const char* content_end;
  const char* postfixes;
  int postfix_len;
  bool is_raw;
  bool is_fstring;
};

bool is_prefix_char(char c) {
  return (c == 'u' || c == 'r' || c == 'c' || c == 'f' || c == 'b');
}

int check_multichar_prefix(const char* start) {
  if (strncmp(start, "u8", 2) == 0) return 2;
  if (strncmp(start, "u16", 3) == 0) return 3;
  if (strncmp(start, "u32", 3) == 0) return 3;
  return 0;
}

bool has_raw_prefix(const char* start, const char* end) {
  for (const char* p = start; p < end; p++) {
    if (*p == 'r') return true;
  }
  return false;
}

bool has_f_prefix(const char* start, const char* end) {
  for (const char* p = start; p < end; p++) {
    if (*p == 'f') return true;
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

int escape_sequence_length(const char* ptr, bool is_raw) {
  if (!is_raw && *ptr == '\\') {
    ptr++;
    if (!*ptr) return 1;
    
    switch (*ptr) {
      case '\'': case '"': case '?': case '\\':
      case 'a': case 'b': case 'f': case 'n': case 'r': case 't': case 'v':
        return 2;
      case 'x':
        ptr++;
        if (!isxdigit(*ptr)) return 2;
        ptr++;
        if (isxdigit(*ptr)) ptr++;
        return ptr - (ptr - 2);
      case 'u':
        ptr++;
        for (int i = 0; i < 4 && *ptr && isxdigit(*ptr); i++) ptr++;
        return ptr - (ptr - 2);
      case 'U':
        ptr++;
        for (int i = 0; i < 8 && *ptr && isxdigit(*ptr); i++) ptr++;
        return ptr - (ptr - 2);
      case '0': case '1': case '2': case '3':
      case '4': case '5': case '6': case '7':
        for (int i = 0; i < 3 && *ptr >= '0' && *ptr <= '7'; i++) ptr++;
        return ptr - (ptr - 2);
      default:
        return 2;
    }
  }
  return 0;
}

// Parse an expression inside f-string braces
const char* parse_fstring_expression(const char* ptr, char string_quote_char, bool is_raw, int* brace_count) {
  int paren_depth = 0;
  int bracket_depth = 0;
  
  while (*ptr) {
    // Handle escape sequences
    if (*ptr == '\\' && !is_raw) {
      int esc_len = escape_sequence_length(ptr, false);
      if (esc_len > 0) {
        ptr += esc_len;
        continue;
      }
    }
    
    // Handle string literals within the expression
    if (*ptr == '"' || *ptr == '\'') {
      LiteralResult nested = recognize_literal(ptr);
      if (nested.found) {
        ptr = nested.end;
        continue;
      }
    }
    
    // Track nesting levels
    if (*ptr == '(') paren_depth++;
    else if (*ptr == ')') {
      if (paren_depth == 0) return NULL;
      paren_depth--;
    }
    else if (*ptr == '[') bracket_depth++;
    else if (*ptr == ']') {
      if (bracket_depth == 0) return NULL;
      bracket_depth--;
    }
    else if (*ptr == '{') {
      // This is a nested brace - could be dict/set or nested f-string
      (*brace_count)++;
      const char* next = ptr + 1;
      while (isspace(*next)) next++;
      
      // Check if this starts a nested f-string
      const char* temp = next;
      while (is_prefix_char(*temp) || *temp == '8') temp++;
      
      if (*temp == '"' || *temp == '\'') {
        LiteralResult nested = recognize_literal(next);
        if (nested.found && nested.is_fstring) {
          // Nested f-string consumed
          ptr = nested.end;
          continue;
        }
      }
      
      // Regular brace - just increment depth
      ptr++;
      continue;
    }
    else if (*ptr == '}') {
      (*brace_count)--;
      if (*brace_count == 0) {
        // Found the closing brace for our f-string expression
        return ptr;
      }
    }
    
    ptr++;
  }
  
  return NULL;
}

// Find closing quote for f-strings
const char* find_closing_quote_fstring(const char* start, char quote_char, bool is_raw) {
  const char* ptr = start + 1;
  int brace_count = 0;
  
  while (*ptr) {
    // Handle escape sequences
    if (*ptr == '\\') {
      if (is_raw) {
        if (ptr[1] == quote_char) {
          ptr += 2;
          continue;
        }
        ptr++;
        continue;
      } else {
        int esc_len = escape_sequence_length(ptr, false);
        if (esc_len > 0) {
          ptr += esc_len;
          continue;
        }
      }
    }
    
    // Handle f-string expressions
    if (*ptr == '{') {
      brace_count++;
      const char* next = ptr + 1;
      while (isspace(*next)) next++;
      
      // Check if this is a nested f-string
      const char* temp = next;
      while (is_prefix_char(*temp) || *temp == '8') temp++;
      
      if (*temp == '"' || *temp == '\'') {
        LiteralResult nested = recognize_literal(next);
        if (nested.found && nested.is_fstring) {
          // Skip the entire nested f-string
          ptr = nested.end;
          continue;
        }
      }
      
      // Parse regular expression
      ptr = parse_fstring_expression(ptr + 1, quote_char, is_raw, &brace_count);
      if (!ptr) return NULL;
      ptr++; // Skip the closing '}'
      continue;
    }
    
    // Check for closing quote
    if (*ptr == quote_char && brace_count == 0) {
      return ptr;
    }
    
    ptr++;
  }
  return NULL;
}

// Find closing quote for normal strings
const char* find_closing_quote_normal(const char* start, char quote_char, bool is_raw) {
  const char* ptr = start + 1;
  
  while (*ptr) {
    if (*ptr == '\\') {
      if (is_raw) {
        if (ptr[1] == quote_char) {
          ptr += 2;
          continue;
        }
        ptr++;
        continue;
      } else {
        int esc_len = escape_sequence_length(ptr, false);
        if (esc_len > 0) {
          ptr += esc_len;
          continue;
        }
      }
    }
    
    if (*ptr == quote_char) {
      return ptr;
    }
    
    ptr++;
  }
  return NULL;
}

// Main recognition function
LiteralResult recognize_literal(const char* current_ptr) {
  LiteralResult result = {NULL, false, NULL, 0, NULL, 0, NULL, NULL, NULL, 0, false, false};
  
  if (!current_ptr || !*current_ptr) return result;
  
  const char* ptr = current_ptr;
  const char* prefix_start = ptr;
  
  while (is_prefix_char(*ptr) || *ptr == '8') {
    ptr++;
  }
  
  int prefix_len = ptr - prefix_start;
  if (!is_valid_prefix(prefix_start, ptr)) {
    ptr = current_ptr;
    prefix_len = 0;
  }
  
  if (!(*ptr == '"' || *ptr == '\'')) {
    return result;
  }
  
  char quote_char = *ptr;
  const char* quote_start = ptr;
  
  bool is_raw = has_raw_prefix(prefix_start, ptr);
  bool is_fstring = has_f_prefix(prefix_start, ptr);
  
  const char* closing_quote = NULL;
  if (is_fstring) {
    closing_quote = find_closing_quote_fstring(ptr, quote_char, is_raw);
  } else {
    closing_quote = find_closing_quote_normal(ptr, quote_char, is_raw);
  }
  
  if (!closing_quote) {
    return result;
  }
  
  const char* postfix_start = closing_quote + 1;
  const char* postfix_end = postfix_start;
  
  while (*postfix_end && !isspace(*postfix_end) && 
       *postfix_end != ';' && *postfix_end != ',' && 
       *postfix_end != ')' && *postfix_end != ']' && *postfix_end != '}') {
    postfix_end++;
  }
  
  result.found = true;
  result.end = postfix_end;
  result.prefixes = (prefix_len > 0) ? prefix_start : NULL;
  result.prefix_len = prefix_len;
  result.quote_start = quote_start;
  result.quote_char = quote_char;
  result.content_start = quote_start + 1;
  result.content_end = closing_quote;
  result.postfixes = (postfix_end > postfix_start) ? postfix_start : NULL;
  result.postfix_len = postfix_end - postfix_start;
  result.is_raw = is_raw;
  result.is_fstring = is_fstring;
  
  return result;
}

void print_escaped(const char* start, const char* end) {
  for (const char* p = start; p < end; p++) {
    if (*p == '\\') printf("\\\\");
    else if (*p == '"') printf("\\\"");
    else if (*p == '\'') printf("\\'");
    else putchar(*p);
  }
}

void print_literal_result(const LiteralResult* result, const char* input) {
  if (!result->found) {
    printf("Not a valid literal\n");
    return;
  }
  
  printf("Found literal:\n");
  printf("  Prefixes: ");
  if (result->prefixes && result->prefix_len > 0) {
    printf("'%.*s' (length %d)\n", result->prefix_len, result->prefixes, result->prefix_len);
  } else {
    printf("none\n");
  }
  
  printf("  Type: %s%s\n", 
       result->is_raw ? "raw " : "",
       result->is_fstring ? "f-string" : "string");
  printf("  Quote: '%c'\n", result->quote_char);
  printf("  Content: '");
  print_escaped(result->content_start, result->content_end);
  printf("' (length %ld)\n", result->content_end - result->content_start);
  
  if (result->postfixes && result->postfix_len > 0) {
    printf("  Postfixes: '");
    print_escaped(result->postfixes, result->postfixes + result->postfix_len);
    printf("' (length %d)\n", result->postfix_len);
  } else {
    printf("  Postfixes: none\n");
  }
  
  printf("  Full literal: ");
  const char* start = (result->prefixes && result->prefix_len > 0) ? result->prefixes : result->quote_start;
  for (const char* p = start; p < result->end; p++) putchar(*p);
  printf("\n");
  printf("  Ends at position: %ld\n", result->end - input);
}

void test_literal(const char* input) {
  printf("\n========================================\n");
  printf("Input: \"%s\"\n", input);
  printf("----------------------------------------\n");
  LiteralResult result = recognize_literal(input);
  print_literal_result(&result, input);
}

int main() {
  printf("=== ADVANCED F-STRING LEXER WITH NESTED EXPRESSIONS ===\n\n");
  
  // The critical cases you asked about
  test_literal("r\"\\\"");
  test_literal("r\"\\\\\"");
  test_literal("r\"\\\"\"");
  test_literal("r\"\\\\\"\"");

  // Additional raw string tests
  test_literal("r\"Raw string with \\n literal backslash-n\"");
  test_literal("r\"Raw string with \"quotes\" inside\"");
  test_literal("r\"Raw string with \\\"escaped quotes\\\" inside\"");
  test_literal("r\"C:\\Program Files\\MyApp\"");
  test_literal("r\"\"");
  test_literal("r\"\""); // Empty raw string

  // Normal string tests for comparison
  test_literal("\"Normal string with \\n newline\"");
  test_literal("\"Normal string with \\\"escaped quotes\\\"\"");
  test_literal("\'\\xff\\xaa\'");

  // Raw string with prefixes
  test_literal("ur\"Unicode raw string\"");
  test_literal("u8r\"UTF-8 raw string\"");
  test_literal("fu8r\"UTF-8 raw string with {\"\\\"\"}\"");

  // Raw strings with postfixes
  test_literal("r\"raw\"_postfix");

  // Invalid cases
  test_literal("r\"unterminated raw string");
  test_literal("r\"\\\""); // Still unterminated because \ doesn't escape "
  
  // Test cases
  test_literal("f\"{some_f(fr\"ads\")}\"");
  test_literal("f\"Outer {f\"Inner {value}\"} Outer\"");
  test_literal("fr\"Raw f-string with {func(f\"nested\")\"");
  test_literal("f\"{{literal brace}}\"");
  test_literal("f\"{f\"{f\"deep\"}\"}\"");
  test_literal("f\"{f\"{\"{{{{{{{{{\"}\"}\" adsasda");
  test_literal("fu8r\"UTF-8 raw string with {\"\\\"\"}\"");
  test_literal("\"normal string\"");
  test_literal("r\"raw string\"");
  
  return 0;
}