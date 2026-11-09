#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "./lexer.h"

// Function to read entire file into a string
char* read_file(const char* filename, size_t* out_len) {
  FILE* file = fopen(filename, "rb");
  if (!file) {
    fprintf(stderr, "Error: Could not open file '%s'\n", filename);
    return NULL;
  }
  
  // Get file size
  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  // Allocate buffer (+1 for null terminator)
  char* buffer = (char*)malloc(size + 1);
  if (!buffer) {
    fprintf(stderr, "Error: Could not allocate memory for file\n");
    fclose(file);
    return NULL;
  }
  
  // Read file
  size_t bytes_read = fread(buffer, 1, size, file);
  buffer[bytes_read] = '\0';
  
  fclose(file);
  
  if (out_len) *out_len = bytes_read;
  return buffer;
}

// Function to lex a file and print all tokens
void lex_file(const char* filename) {
  size_t source_len;
  char* source = read_file(filename, &source_len);
  
  if (!source) {
    return;
  }
  
  printf("Lexing file: %s\n", filename);
  printf("========================================\n");
  
  Lexer lexer;
  lexer_init(&lexer, source, source_len, filename);
  
  int token_count = 0;
  while (1) {
    Token token = lexer_next(&lexer);
    
    printf("Token %3d: %-12s '%.*s' at %d:%d\n",
         ++token_count,
         token_type_name(token.type),
         (int)token.length, token.start,
         token.line, token.column);
    
    if (token.type == TOKEN_CTRLEOF) {
      break;
    }
    
    if (token.type == TOKEN_CTRLERROR) {
      lexer_print_error(&lexer, "lexing file");
      break;
    }
  }
  
  printf("========================================\n");
  printf("Total tokens: %d\n", token_count);
  
  free(source);
}

// Simple main function for testing
int main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <filename.light>\n", argv[0]);
    return 1;
  }
  
  lex_file(argv[1]);
  return 0;
}