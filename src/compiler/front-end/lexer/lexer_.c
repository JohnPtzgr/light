#include "lexer_config.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>


const symbol_t KEYWORD_SYMBOLS[] = {
  DEF_LIST_KEYWORD_SYM
  {}
};

static inline bool ascii_is_digit(char c) {
  return c >= '0' && c <= '9';
}

static inline bool ascii_is_alpha(char c) { 
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static inline bool ascii_is_alnum(char c) {
  return ascii_is_digit(c) || ascii_is_alpha(c);
}

static inline bool ascii_is_whitespace(char c) { 
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static inline bool ascii_is_operator_start(char c) {
  return c == '=' || c == '!' || c == '<' || c == '>' || 
         c == '&' || c == '|' || c == '+' || c == '-' ||
         c == '*' || c == '/' || c == '%' || c == ':' ||
         c == '.' || c == '?';
}

static inline bool ascii_is_identifier_start(char c) {
    return ascii_is_alpha(c) || c == '_';
}

static inline bool ascii_is_identifier_continue(char c) {
  return ascii_is_alnum(c) || c == '_';
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

const CharClassifier ASCII_CLASSIFIER = {
  .is_digit = ascii_is_digit,
  .is_alpha = ascii_is_alpha,
  .is_alnum = ascii_is_alnum,
  .is_whitespace = ascii_is_whitespace,
  .is_operator_start = ascii_is_operator_start,
  .is_identifier_start = ascii_is_identifier_start,
  .is_identifier_continue = ascii_is_identifier_continue
};

LEXER_ERR_ENUM lexer_init(lexer_t* lexer, const char* filename, uint16_t indent_depth) {
  if (lexer == NULL || filename == NULL) {
    return LEXERR_INITFAIL;
  }
  
  /* Zero out the lexer structure */
  memset(lexer, 0, sizeof(lexer_t));
  
  /* Store configuration */
  lexer->lexer_indent_depth = indent_depth;
  lexer->current_line = 1;
  lexer->current_column = 1;
  
  /* Open and read file */
  FILE* file = fopen(filename, "rb");
  if (file == NULL) {
    return LEXERR_INITFAIL;
  }
  
  /* Get file size */
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  if (file_size <= 0) {
    fclose(file);
    return LEXERR_INIT_FAIL;
  }
  
  /* Allocate memory for source + null terminator */
  char* source = (char*)malloc(file_size + 1);
  if (source == NULL) {
    fclose(file);
    return LEXERR_INITFAIL;
  }
  
  /* Read the entire file */
  size_t bytes_read = fread(source, 1, file_size, file);
  if (bytes_read != (size_t)file_size) {
    free(source);
    fclose(file);
    return LEXERR_INITFAIL;
  }
  source[file_size] = '\0';
  fclose(file);
  
  /* Initialize lexer fields */
  lexer->source = source;
  lexer->source_size = file_size;
  lexer->current_pos = source;
  lexer->current_token_begins = source;
  lexer->sym_window_begins = source;
  lexer->current_token_length = 0;
  
  /* Initialize symbol window */
  for (int i = 0; i < LEXER_WINDOW_LENGTH && i < file_size; ++i) {
    lexer->sym_window[i] = (symbol_t){.symbol_str=&source[i], .symlen=1};
  }
  /* Pad remaining window slots with null */
  for (int i = file_size; i < LEXER_WINDOW_LENGTH; ++i) {
    lexer->sym_window[i] = (symbol_t){};
  }
  
  /* Allocate initial token stream */
  lexer->token_capacity = LEXER_INIT_TOKENS;
  lexer->token_count = 0;
  lexer->token_stream = (token_t*)calloc(lexer->token_capacity, sizeof(token_t));
  if (lexer->token_stream == NULL) {
    free((void*)lexer->source);
    memset(lexer, 0, sizeof(lexer_t));
    return LEXERR_INITFAIL;
  }
  
  return LEXERR_OK;
}

LEXER_ERR_ENUM lexer_pick_from(lexer_t* lexer, lexer_t* src, uint16_t indent_depth) {
  if (!src || !lexer) {
    return LEXERR_INITFAIL;
  }
  *lexer = *src;
  lexer->current_token_begins = NULL;
  lexer->current_token_length = 0;
  lexer->lexer_substitute = 1;
  lexer->sym_window_begins = lexer->current_pos;
  lexer->token_count = 0;
  lexer->lexer_indent_depth = indent_depth;
  
  /* Allocate initial token stream */
  lexer->token_capacity = LEXER_PICK_INIT_TOKENS;
  lexer->token_stream = (token_t*)calloc(lexer->token_capacity, sizeof(token_t));
  if (lexer->token_stream == NULL) {
    free((void*)lexer->source);
    memset(lexer, 0, sizeof(lexer_t));
    return LEXERR_INITFAIL;
  }
  return LEXERR_OK;
}

LEXER_ERR_ENUM lexer_extend(lexer_t* lexer) {
  size_t capacity = lexer->token_capacity;
  lexer->token_capacity *= 2;
  token_t* old_token_stream = lexer->token_stream;
  lexer->token_stream = (lexer_t*)malloc(sizeof(token_t) * capacity);
  if (!lexer->token_stream) {
    free(old_token_stream);
    lexer_destroy(lexer);
    return LEXERR_EXTENDFAIL;
  }
  memcpy(lexer->token_stream, old_token_stream, capacity * sizeof(token_t));
  free(old_token_stream);
  return LEXERR_OK;
}

LEXER_ERR_ENUM lexer_extend_to(lexer_t* lexer, size_t new_capacity) {
  size_t capacity = lexer->token_capacity;
  lexer->token_capacity - new_capacity;
  token_t* old_token_stream = lexer->token_stream;
  lexer->token_stream = (lexer_t*)malloc(sizeof(token_t) * new_capacity);
  if (!lexer->token_stream) {
    free(old_token_stream);
    lexer_destroy(lexer);
    return LEXERR_EXTENDFAIL;
  }
  memcpy(lexer->token_stream, old_token_stream, capacity * sizeof(token_t));
  free(old_token_stream);
  return LEXERR_OK;
}

LEXER_ERR_ENUM lexer_join_with(lexer_t* lexer, lexer_t* trg, uint16_t indent_depth) {
  while (trg->token_count + lexer->token_count > trg->token_capacity) {
    uint64_t lexextres = 0;
    if (!(lexextres = lexer_extend_to(trg, trg->token_capacity + lexer->token_stream))) {
      return lexextres | LEXERR_JOINFAIL;
    }
  }
  memcpy(trg->token_stream + trg->token_count, lexer->token_stream, lexer->token_count);
  trg->token_count += lexer->token_count;
  return LEXERR_OK;
}

LEXER_ERR_ENUM lexer_add_token(lexer_t* lexer) {
  while (lexer->token_count + 1 > lexer->token_capacity) {
    uint64_t lexextres = 0;
    if (!(lexextres = lexer_extend(lexer))) {
      return lexextres | LEXERR_ADDTOKENFAIL;
    }
  }
  lexer->token_stream[lexer->token_count] = lexer->current_token;
  lexer->token_count++;
  lexer->current_token_begins = lexer->current_token_begins + lexer->current_token_length;
  lexer->current_token_length = 0;
  lexer->current_token = (token_t){.begins_at=lexer->current_token_begins};
  return LEXERR_OK;
}

LEXER_ERR_ENUM lexer_destroy(lexer_t* lexer) {
  if (lexer == NULL) {
    return LEXERR_DESTROY_FAIL;
  }
  if (lexer->source != NULL && !lexer->lexer_substitute) {
    free((void*)lexer->source);
    lexer->source = NULL;
  }
  if (lexer->token_stream != NULL) {
    free(lexer->token_stream);
    lexer->token_stream = NULL;
  }
  memset(lexer, 0, sizeof(lexer_t));
  
  return LEXERR_OK;
}

// #define ADD +

LEXER_ERR_ENUM lexer_advance(lexer_t* lexer) {
  for (int i = LEXER_WINDOW_LENGTH - 1; i > 0; --i) {
    lexer->sym_window[i] = lexer->sym_window[i - 1];
  }
  if (!lexer->lexer_phase) {
    lexer->sym_window[0] = (symbol_t){.symbol_str=lexer->current_pos + 1, .symlen=1};
  } else {
    lexer->sym_window[0] = (symbol_t){.token=lexer->past_stream[(uint64_t)lexer->current_pos].kind,
                                      .symbol_str=lexer->past_stream[(uint64_t)lexer->current_pos].begins_at,
                                      .symlen=lexer->past_stream[(uint64_t)lexer->current_pos].length};
  }
  lexer->current_pos++;
}

LEXER_ERR_ENUM lexer_first_pass_next(lexer_t* lexer) {
  
}

LEXER_ERR_ENUM lexer_first_pass_advance(lexer_t* lexer) {
  if (!lexer->sym_window[0].symlen) {
    return LEXERR_PHASEONEFAIL;
  }
  switch (lexer->sym_window[0].symbol_str[0]) {
    case '\0':
      
    case 'a' ... 'z':
    case 'A' ... 'Z':
    case '_':
      if (lexer->current_token.kind == TOKEN_IDENTIFIER) {
        lexer->current_token_length++;
        lexer_advance(lexer);
      } else {
        
      }
      break;

    case '0' ... '9':

      break;

    default:
      break;
  }
}

LEXER_ERR_ENUM lexer_compile_multipass_tokens(lexer_t* lexer, symbol_t* symbols, size_t symbol_cnt) {

}

LEXER_ERR_ENUM lexer_second_pass_next(lexer_t* lexer) {

}

LEXER_ERR_ENUM lexer_first_pass(lexer_t* lexer) {

}

LEXER_ERR_ENUM lexer_second_pass(lexer_t* lexer) {

}

LEXER_ERR_ENUM lexer_full_pass(lexer_t* lexer) {

}
