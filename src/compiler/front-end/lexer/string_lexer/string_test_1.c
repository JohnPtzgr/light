#include <stdio.h>
#include <string.h>
#include "string_regular.h"
#include "string_raw.h"

// Global test counter
int test_passed = 0;
int test_failed = 0;

void emit_token(StringToken token) {
    const char* type_str = "";
    switch(token.type) {
        case TOKEN_CHAR_BEGIN: type_str = "CHAR_BEGIN"; break;
        case TOKEN_CHAR_CONTENT: type_str = "CHAR_CONTENT"; break;
        case TOKEN_CHAR_ESCAPE: type_str = "CHAR_ESCAPE"; break;
        case TOKEN_CHAR_END: type_str = "CHAR_END"; break;
        case TOKEN_STRING_BEGIN: type_str = "STRING_BEGIN"; break;
        case TOKEN_STRING_CONTENT: type_str = "STRING_CONTENT"; break;
        case TOKEN_STRING_ESCAPE: type_str = "STRING_ESCAPE"; break;
        case TOKEN_STRING_END: type_str = "STRING_END"; break;
        case TOKEN_MULTILINE_BEGIN: type_str = "MULTILINE_BEGIN"; break;
        case TOKEN_MULTILINE_CONTENT: type_str = "MULTILINE_CONTENT"; break;
        case TOKEN_MULTILINE_ESCAPE: type_str = "MULTILINE_ESCAPE"; break;
        case TOKEN_MULTILINE_END: type_str = "MULTILINE_END"; break;
        case TOKEN_POSTFIX_SEP: type_str = "POSTFIX_SEP"; break;
        case TOKEN_POSTFIX_IDENT: type_str = "POSTFIX_IDENT"; break;
        case TOKEN_ERROR: type_str = "ERROR"; break;
    }
    printf("  %s (line %d, col %d): '%.*s'\n", 
           type_str, token.line, token.column,
           (int)(token.end - token.start), token.start);
}

void run_test(const char* input, const char* test_name) {
    printf("\n========================================\n");
    printf("TEST: %s\n", test_name);
    printf("Input: %s\n", input);
    printf("----------------------------------------\n");
    
    StringDescriptor desc = identify_string_literal(input);
    
    if (desc.quote_type == 0) {
        printf("  Not a string literal\n");
        test_failed++;
        return;
    }
    
    printf("  Prefixes: '%.*s', Raw: %d, Format: %d, Char: %d, Quote type: %d\n",
           desc.prefixes_len, desc.prefixes_start ? desc.prefixes_start : "",
           desc.is_raw, desc.is_format, desc.is_char_literal, desc.quote_type);
    
    const char* end;
    if (desc.is_raw) {
        end = parse_raw_string(input, &desc, emit_token, 1, 0);
    } else {
        end = parse_regular_string(input, &desc, emit_token, 1, 0);
    }
    
    if (desc.postfix_start && desc.postfix_len > 0) {
        printf("\n  Postfix section: '%.*s' (length %d)\n",
               desc.postfix_len, desc.postfix_start, desc.postfix_len);
    } else {
        printf("\n  Postfix section: none\n");
    }
    
    printf("  Total parsed: %ld bytes\n", end - input);
    test_passed++;
}

// Test basic strings
void test_basic_strings() {
    printf("\n=== BASIC STRING TESTS ===\n");
    run_test("\"hello world\"", "Basic double-quoted string");
    run_test("'a'", "Basic char literal");
    run_test("\"\"", "Empty string");
    run_test("''", "Empty char literal");
}

// Test strings with escape sequences
void test_escape_sequences() {
    printf("\n=== ESCAPE SEQUENCE TESTS ===\n");
    run_test("\"hello\\nworld\"", "Newline escape");
    run_test("\"hello\\tworld\"", "Tab escape");
    run_test("\"\\\"quoted\\\"\"", "Quoted escape");
    run_test("\"\\\\backslash\\\\\"", "Backslash escape");
    run_test("\"\\xff\\xaa\"", "Hex escapes");
    run_test("\'\\xff\\xaa\'", "Hex escapes");
    run_test("\"\\u1234\\u5678\"", "Unicode escapes");
    run_test("\"\\U0001F600\"", "Unicode 32-bit escape");
    run_test("\"Mixed \\xff and \\n escapes\"", "Mixed escapes");
}

// Test raw strings
void test_raw_strings() {
    printf("\n=== RAW STRING TESTS ===\n");
    run_test("r\"raw\\nstring\"", "Raw string with literal backslash-n");
    run_test("r\"C:\\Program Files\\MyApp\"", "Raw string with Windows paths");
    run_test("r\"Raw with \\\"quotes\\\"\"", "Raw string with escaped quotes");
    run_test("r\"Raw with \\\"", "Raw string with \\ quote at the end");
    run_test("r\"\\\"", "Raw string with escaped quote at the end");
    run_test("r\"\"", "Empty raw string");
    run_test("r\"raw\nstring\"", "Raw string with actual newline");
}

// Test strings with prefixes
void test_prefixes() {
    printf("\n=== PREFIX TESTS ===\n");
    run_test("u\"unicode string\"", "u prefix");
    run_test("u8\"UTF-8 string\"", "u8 prefix");
    run_test("u16\"wide string\"", "u16 prefix");
    run_test("u32\"wide string\"", "u32 prefix");
    run_test("b\"binary string\"", "b prefix");
    run_test("f\"formatted string\"", "f prefix");
    run_test("c'char'", "c prefix");
    run_test("uf\"unicode formatted\"", "Combined uf prefix");
    run_test("u8fr\"UTF-8 formatted raw\"", "Combined u8fr prefix");
    run_test("fu\"formatted unicode\"", "Combined fu prefix");
}

// Test multiline strings
void test_multiline_strings() {
    printf("\n=== MULTILINE STRING TESTS ===\n");
    run_test("\"\"\"multiline\nstring\"\"\"", "Basic multiline");
    run_test("\"\"\"line1\nline2\nline3\"\"\"", "Three line multiline");
    run_test("\"\"\"\nleading newline\n\"\"\"", "Multiline with leading newline");
    run_test("\"\"\"trailing newline\n\"\"\"", "Multiline with trailing newline");
    run_test("r\"\"\"raw multiline\nstring\"\"\"", "Raw multiline string");
    run_test("u8\"\"\"UTF-8 multiline\nstring\"\"\"", "UTF-8 multiline string");
}

// Test byte encodings
void test_byte_encodings() {
    printf("\n=== BYTE ENCODING TESTS ===\n");
    run_test("u8\"\\xff\\xaa\"", "UTF-8 hex bytes");
    run_test("\"\\x48\\x65\\x6c\\x6c\\x6f\"", "Hex encoding 'Hello'");
    run_test("u\"\\u1234\"", "Unicode encoding");
    run_test("\"\\x00\\x01\\x02\"", "Null and control bytes");
}

// Test postfix operators
void test_postfix() {
    printf("\n=== POSTFIX OPERATOR TESTS ===\n");
    run_test("\"123\"atoi32", "Single postfix identifier");
    run_test("\"123\"std::bigi", "Namespace postfix");
    run_test("\"hello\"world::len", "Multiple namespace levels");
    run_test("\"test\"to_string::parse", "Mixed postfix");
    run_test("u8\"\\xff\\xaa\"hex_to_int", "Postfix with prefixes");
    run_test("\"multiline\\nstring\"process::line::by::line", "Postfix with escapes");
    run_test("r\"raw\\nstring\"no_escape", "Postfix with raw string");
    run_test("\"data\"encode::base64::to_string", "Multiple postfix identifiers");
    run_test("\"value\"_suffix", "Underscore postfix");
}

// Test error cases
void test_errors() {
    printf("\n=== ERROR CASE TESTS ===\n");
    run_test("\"unterminated", "Unterminated double quote");
    run_test("'unterminated", "Unterminated single quote");
    run_test("\"\"\"unterminated multiline", "Unterminated multiline");
    run_test("invalid\"string\"", "Invalid prefix");
    run_test("uu8\"invalid\"", "Invalid u-prefix combination");
    run_test("u8u\"invalid\"", "Invalid u-prefix combination 2");
}

// Test edge cases
void test_edge_cases() {
    printf("\n=== EDGE CASE TESTS ===\n");
    run_test("\"\"\"\"\"\"", "Triple quotes with empty content");
    run_test("\"\\\\\"", "Single backslash escape");
    run_test("\"\\\"", "Escaped quote at end");
    run_test("r\"\\\"\"", "Raw string with escaped quote");
    run_test("\"\\x\"", "Incomplete hex escape");
    run_test("\"\\u\"", "Incomplete unicode escape");
}

// Comprehensive test with complex nested scenario
void test_comprehensive() {
    printf("\n=== COMPREHENSIVE TEST ===\n");
    run_test("u8r\"Complex \\xff \\n raw\\nstring\"postfix::test \"another one\"", 
             "Complex combination: prefixes + raw + escapes + postfix");
    run_test("\"\"\"Complex\nmultiline\nstring\"\"\"process::line::by::line", 
             "Complex multiline with postfix");
    run_test("fu8\"Formatted\\nUTF-8\\nstring\"convert::to::upper", 
             "Format + UTF-8 with escapes and postfix");
}

// Run all tests
void run_all_tests() {
    // test_basic_strings();
    // test_escape_sequences();
    test_raw_strings();
    // test_prefixes();
    test_multiline_strings();
    // test_byte_encodings();
    // test_postfix();
    // test_errors();
    // test_edge_cases();
    test_comprehensive();
}

int main() {
    printf("========================================\n");
    printf("   STRING LITERAL LEXER TEST SUITE\n");
    printf("========================================\n");
    printf("Features:\n");
    printf("  - Multiple prefixes (u, u8, u16, u32, r, c, f, b)\n");
    printf("  - Raw strings (r prefix)\n");
    printf("  - Format strings (f prefix)\n");
    printf("  - Multiline strings (\"\"\" \"\"\")\n");
    printf("  - Escape sequences (\\n, \\t, \\x, \\u, \\U, octal)\n");
    printf("  - Postfix operators (::identifier)\n");
    printf("  - Line splitting for content\n");
    printf("========================================\n");
    
    run_all_tests();
    
    printf("\n========================================\n");
    printf("TEST SUMMARY\n");
    printf("========================================\n");
    printf("Tests passed: %d\n", test_passed);
    printf("Tests failed: %d\n", test_failed);
    printf("Total tests: %d\n", test_passed + test_failed);
    printf("========================================\n");
    
    return test_failed > 0 ? 1 : 0;
}