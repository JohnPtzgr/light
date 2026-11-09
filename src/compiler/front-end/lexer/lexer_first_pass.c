#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "./lexer_config.h"
#include "./lexer_.h"


const symbol_t DELIMITER_SYMBOLS[] = {
  DEF_LIST_DELIM_SYM
  {}
};

const int DELIM_SYM_CNT = sizeof(DELIMITER_SYMBOLS) / sizeof(symbol_t) - 1;

inline static ptrdiff_t get_current_token_index(lptoken_t* stream, lptoken_t* current_token) {
  return current_token - stream;
}

lpresult_t fplexer_init(first_pass_lexer_t* lexer, const char* filename) {
  if (lexer == NULL || filename == NULL) {
    return (lpresult_t){.val.as_error=LEXERR_INIT_FAIL};
  }

  memset(lexer, 0, sizeof(first_pass_lexer_t));
  
  lexer->current_line = 1;
  lexer->current_column = 1;
  
  FILE* file = fopen(filename, "rb");
  if (file == NULL) {
    return (lpresult_t){.val.as_error=LEXERR_INIT_FAIL};
  }
  
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  if (file_size <= 0) {
    fclose(file);
    return (lpresult_t){.val.as_error=LEXERR_INIT_FAIL};
  }
  
  char* source = (char*)malloc(file_size + 1);
  if (source == NULL) {
    fclose(file);
    return (lpresult_t){.val.as_error=LEXERR_INIT_FAIL};
  }
  
  size_t bytes_read = fread(source, 1, file_size, file);
  if (bytes_read != (size_t)file_size) {
    free(source);
    fclose(file);
    return (lpresult_t){.val.as_error=LEXERR_INIT_FAIL};
  }
  source[file_size] = '\0';
  fclose(file);
  
  lexer->current_pos = 0;
  lexer->source_code = source;
  
  printf("LEXING FROM FILE (%s):\n\'\'\'\n%s\n\'\'\'\n", filename, lexer->source_code);

  lexer->lptoken_stream = calloc(file_size + 1, sizeof(lptoken_t));
  lexer->lptoken_stream[file_size] = (lptoken_t){.kind=TOKEN_EOF};
  
  return (lpresult_t){.tag=LPR_OK};
}

lpresult_t fplexer_destroy(first_pass_lexer_t* lexer) {
  errno = 0;

  if (lexer->source_code) {
    free(lexer->source_code);
  }

  if (lexer->lptoken_stream) {
    free(lexer->lptoken_stream);
  }

  memset(lexer, 0, sizeof(first_pass_lexer_t));
  return (lpresult_t){.tag=LPR_OK};
}

lpresult_t fplexer_peek(first_pass_lexer_t* lexer) {
  if (!lexer->source_code[lexer->current_pos] ||
      !lexer->source_code[lexer->current_pos + 1]) {
    return (lpresult_t){.val.as_error=LEXERR_EOF_FAIL};
  }
  char next = lexer->source_code[lexer->current_pos + 1];
  return (lpresult_t){.tag=LPR_CHAR, .val.as_char=next};
}

const char* lptoken_name(lptoken_t* token) {
  const char* tok_name = NULL;
  switch (token->kind) {
    case 1 ... (TOKEN_SINGLE_CHAR_BOUNDARY - 1):
      tok_name = DELIMITER_SYMBOLS[token->kind - 1].symbol_str;
      break;
    case TOKEN_DIGITS:
      tok_name = "DIG";
      break;
    case TOKEN_IDENTIFIER:
      tok_name = "IDN";
      break;
    case TOKEN_COMMENTBEG:
      tok_name = "/.";
      break;
    case TOKEN_COMMENTEND:
      tok_name = "./";
      break;
  }
  return tok_name;
}

const char* lptoken_str(lptoken_t* token, char* buffer, size_t buffer_size) {
  const char* tok_name = lptoken_name(token);
  snprintf(buffer,
           buffer_size,
           "FPT{KND(%d), LN(%d), CL(%d), S(\'%s\')}",
           token->kind,
           token->line,
           token->column,
           tok_name);
  return buffer;
}

void fplexer_print_state(first_pass_lexer_t* lexer) {
  char buffer[1024] = {};
  printf("LEXER{LN(%d), CL(%d), TOKID(%d), CHRID(%d), CTOK(%s)}\n",
         lexer->current_line,
         lexer->current_column,
         lexer->current_lptoken,
         lexer->current_pos,
         lptoken_str(&lexer->lptoken_stream[lexer->current_lptoken], buffer, 1024));
}

void fplexer_handle_comments(first_pass_lexer_t* lexer) {
  if (!strncmp(DEF_COMMENT_SINGLE,
               lexer->source_code + lexer->current_pos,
               sizeof(DEF_COMMENT_SINGLE) - 1)) {
    lexer->lptoken_stream[lexer->current_lptoken].ends_at = \
          lexer->current_pos;
    lexer->current_lptoken++;
    lexer->lptoken_stream[lexer->current_lptoken].kind = TOKEN_EMPTY;
    while (lexer->source_code[lexer->current_pos] != '\n' &&
           lexer->source_code[lexer->current_pos]) {
      lexer->current_pos++;
    }
    lexer->current_column = 1;
    lexer->current_line++;
  }
}

lpresult_t fplexer_advance(first_pass_lexer_t* lexer) {
  lpresult_t next = (lpresult_t){.val.as_error=LEXERR_TOKENIZATION_FAIL};
  fplexer_handle_comments(lexer);
  if (lexer->source_code[lexer->current_pos]) {
    next.tag = LPR_CHAR;
    next.val.as_char = lexer->source_code[lexer->current_pos];
  }
  lexer->lptoken_stream[lexer->current_lptoken].ends_at = \
        lexer->current_pos;
  char next_char = next.val.as_char;
  // fplexer_print_state(lexer);
  switch (lexer->lptoken_stream[lexer->current_lptoken].kind) {
    case TOKEN_EOF:
      if (lexer->current_lptoken) {
        lexer->lptoken_stream[lexer->current_lptoken - 1].ends_at = lexer->current_pos;
      }
      return (lpresult_t){.val.as_error=LEXERR_EOF_FAIL};
    case TOKEN_EMPTY:
      lexer->lptoken_stream[lexer->current_lptoken].begins_at = lexer->current_pos;
      lexer->lptoken_stream[lexer->current_lptoken].column = lexer->current_column;
      lexer->lptoken_stream[lexer->current_lptoken].line = lexer->current_line;
      switch (lexer->source_code[lexer->current_pos]) {
        case '\0':
          if (lexer->current_lptoken) {
            lexer->lptoken_stream[lexer->current_lptoken - 1].ends_at = lexer->current_pos;
          }
          return (lpresult_t){.val.as_error=LEXERR_EOF_FAIL};
        case DEF_COMMENT_BLOCK_1:
          if (lexer->source_code[lexer->current_pos + 1] &&
              lexer->source_code[lexer->current_pos + 1] == DEF_COMMENT_BLOCK_2) {
            lexer->lptoken_stream[lexer->current_lptoken].kind = TOKEN_COMMENTBEG;
            lexer->current_column++;
            lexer->current_pos++;
          } else {
            lexer->lptoken_stream[lexer->current_lptoken].kind = \
                (DEF_COMMENT_BEGIN_AS_TOK[0]);
          }
          break;
        case DEF_COMMENT_BLOCK_2:
          if (lexer->source_code[lexer->current_pos + 1] &&
              lexer->source_code[lexer->current_pos + 1] == DEF_COMMENT_BLOCK_1) {
            lexer->lptoken_stream[lexer->current_lptoken].kind = TOKEN_COMMENTEND;
            lexer->current_column++;
            lexer->current_pos++;
          } else {
            lexer->lptoken_stream[lexer->current_lptoken].kind = \
                (DEF_COMMENT_END_AS_TOK[0]);
          }
          break;
        case '0'...'9':
          lexer->lptoken_stream[lexer->current_lptoken].kind = TOKEN_DIGITS;
          break;
        case 'A'...'Z':
        case 'a'...'z':
          lexer->lptoken_stream[lexer->current_lptoken].kind = TOKEN_IDENTIFIER;
          break;
        case ' ':
          lexer->lptoken_stream[lexer->current_lptoken].kind = TOKEN_SPACE;
          break;
        case '\t':
          lexer->current_column += DEF_TAB_LENGTH - 1;
          lexer->lptoken_stream[lexer->current_lptoken].kind = TOKEN_SPACE;
          break;
        case '\n':
          lexer->current_line++;
          lexer->current_column = 0;
          lexer->lptoken_stream[lexer->current_lptoken].line = lexer->current_line;
          lexer->lptoken_stream[lexer->current_lptoken].column = 1;
          lexer->lptoken_stream[lexer->current_lptoken].kind = TOKEN_SPACE;
          break;
        default:
          for (size_t i = 0; i < DELIM_SYM_CNT; ++i) {
            if (DELIMITER_SYMBOLS[i].symbol_str[0] == \
                lexer->source_code[lexer->current_pos]) {
              lexer->lptoken_stream[lexer->current_lptoken].kind = \
                DELIMITER_SYMBOLS[i].token;
              break;
            }
            if (1 + i == DELIM_SYM_CNT) {
              // char c = lexer->source_code[lexer->current_pos];
              // printf("LETF!: (%c) - (%d)\n", c, (int)c);
              return (lpresult_t){.val.as_error=LEXERR_TOKENIZATION_FAIL};
            }
          }
          break;
      }
      lexer->current_column++;
      lexer->current_pos++;
      break;
    case TOKEN_IDENTIFIER:
      if (next.tag == 0) {
        return next;
      }
      switch (next_char) {
        case 'a'...'z':
        case 'A'...'Z':
          lexer->current_column++;
          lexer->current_pos++;
          break;
        
        default:
          lexer->current_lptoken++;
          return fplexer_advance(lexer);
      }
      break;
    
    case TOKEN_DIGITS:
      if (next.tag == 0) {
        return next;
      }
      switch (next_char) {
        case '0'...'9':
          lexer->current_column++;
          lexer->current_pos++;
          break;
        
        default:
          lexer->current_lptoken++;
          return fplexer_advance(lexer);
      }
      break;
    
    case TOKEN_SPACE:
      if (next.tag == 0) {
        return next;
      }
      switch (next_char) {
        case ' ':
          lexer->current_column++;
          lexer->current_pos++;
          break;
        case '\t':
          lexer->current_column += DEF_TAB_LENGTH;
          lexer->current_pos++;
          break;
        
        default:
          lexer->current_lptoken++;
          return fplexer_advance(lexer);
      }
      break;
  
    default:
      lexer->current_lptoken++;
      return fplexer_advance(lexer);
  }
  return (lpresult_t){.tag=LPR_U32, .val.as_u32=lexer->current_lptoken};
}

lpresult_t fplexer_next(first_pass_lexer_t* lexer) {
  size_t current_token_idx = lexer->current_lptoken;
  while (current_token_idx == lexer->current_lptoken) {
    lpresult_t next = fplexer_advance(lexer);
    if (next.tag == 0) {
      return next;
    }
  }
  uint32_t result = lexer->lptoken_stream[current_token_idx].kind;
  return (lpresult_t){.tag=LPR_TOKEN, .val.as_token=result};
}

lpresult_t fplexer_lex(first_pass_lexer_t* lexer) {
  lpresult_t result = {.tag=LPR_TOKEN};
  while ((result = fplexer_next(lexer)).tag) {;};
  size_t index = lexer->current_pos;
  while (lexer->source_code[index]) { index++; };
  lexer->current_lptoken++;
  lexer->lptoken_stream[lexer->current_lptoken] = (lptoken_t){
    .begins_at = index,
    .ends_at = index,
    .kind = TOKEN_EOF,
  };
  return result;
}

lpresult_t fplexer_move_source_code(first_pass_lexer_t* lexer, const char** trg) {
  if (lexer->source_code) {
    *trg = lexer->source_code;
    lexer->source_code = NULL;
    return (lpresult_t){.tag=LPR_OK};
  }
  return (lpresult_t){.val.as_error=LEXERR_INIT_FAIL};
}

lpresult_t fplexer_move_tokens(first_pass_lexer_t* lexer, const lptoken_t** trg) {
  lpresult_t result = fplexer_lex(lexer);
  if (result.val.as_error == LEXERR_EOF_FAIL) {
    *trg = lexer->lptoken_stream;
    lexer->lptoken_stream = NULL;
    return (lpresult_t){.tag=LPR_OK};
  }
  return result;
}

lpresult_t fplexer_print_tokens(first_pass_lexer_t* lexer) {
  lpresult_t result = fplexer_lex(lexer);
  if (result.val.as_error == LEXERR_EOF_FAIL) {
    for (size_t i = 0; i < lexer->current_lptoken - 1; ++i) {
      lptoken_t tok = lexer->lptoken_stream[i];
      const char* tok_name = lptoken_name(&tok);
      char buffer[1024] = {};
      size_t length = lexer->lptoken_stream[i].ends_at - \
          lexer->lptoken_stream[i].begins_at;
      strncpy(buffer, lexer->source_code + tok.begins_at, length);
      buffer[length] = '\0';
      printf("TOKEN (%d): [\'%s\' / \'%s\'] on LN: (%d), CL: (%d), BA: (%d), EA: (%d)\n",
             tok.kind,
             tok_name,
             buffer,
             tok.line,
             tok.column,
             tok.begins_at,
             tok.ends_at);
    }
    return (lpresult_t){.tag=LPR_OK};
  }
  printf("TAG: %d, ERROR: %d, LN: %d, CL: %d\n", result.tag, result.val.as_error, lexer->current_line, lexer->current_column);
  return result;
}

int main() {
  // const char input[] = "a: i32 = 0;\n\n  b := a + 1;";
  first_pass_lexer_t lexer;
  fplexer_init(&lexer, "/home/just_engineer/dev/light/examples/1.lt");
  fplexer_print_tokens(&lexer);
  fplexer_destroy(&lexer);
}

