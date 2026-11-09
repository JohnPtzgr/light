#ifndef LEXER_H_
#define LEXER_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define __AUX_GET_ARG_1(ARG1, ARGS...) ARG1
#define __AUX_GET_ARG_2(ARG1, ARG2, ARGS...) ARG2
#define __AUX_GET_ARG_3(ARG1, ARG2, ARG3, ARGS...) ARG3

#define __GET_ARR_LEN(ARR) (sizeof(ARR) / sizeof((ARR)[0]))

#define __HANDLE_RESULT {if (!result.tag) {return result;}}

/*
  .token - symbol's associated token
  .symbol_str - symbol's string representation
  .symkind - symbol's kind:
                        0 - the whole word is considered a symbol -> (STRING)
                        1 - any character is a symbol             -> [STRING]
  .symlen - symbol's string length
*/
#define DEF_SYM(NAME, SYMBOL...) (symbol_t){.token = TOKEN_##NAME, \
                                            .symbol_str=__AUX_GET_ARG_1(SYMBOL, ""), \
                                            .symkind=__AUX_GET_ARG_2(SYMBOL, 0, 0), \
                                            .symlen=sizeof(__AUX_GET_ARG_1(SYMBOL, "")), \
                                            .sympass=__AUX_GET_ARG_3(SYMBOL, 0, 0, 0)}, 

#define LEXER_WINDOW_LENGTH 8
#define LEXER_INIT_TOKENS 1024
#define LEXER_PICK_INIT_TOKENS 64


/* Helper macro for keyword checking */
#define IS_KEYWORD(ident, len) is_keyword_check(ident, len)



typedef struct symbol_t {
  uint8_t     symlen     ;
  uint8_t     sympass:3  ;
  uint8_t     symmask:5  ;
  uint16_t    token      ;
  uint32_t    symkind    ;
  const char* symbol_str ;
} symbol_t;


typedef struct lptoken_t {
  uint16_t kind      ;
  uint16_t column    ;
  uint32_t line      ;
  uint32_t begins_at ;
  uint32_t ends_at   ;
} lptoken_t;


// typedef struct token_t {
//   uint32_t kind;
//   uint32_t line;
//   uint16_t column;
//   uint16_t multichar:1;
//   uint16_t no_space_follow:1;
//   uint16_t padding:14;
//   uint32_t length;
//   const char* begins_at;
// } token_t;


typedef struct first_pass_lexer_t {
  char*      source_code     ;
  lptoken_t* lptoken_stream  ;
  uint32_t   current_lptoken ;
  uint32_t   current_pos     ;
  uint32_t   current_line    ;
  uint16_t   current_column  ;
  uint16_t   flags           ;
} first_pass_lexer_t;


typedef struct second_pass_lexer_t {
  char*      source_code     ;
  lptoken_t* lptoken_source  ;
  lptoken_t* lptoken_stream  ;
  uint64_t   flag_subcall:1  ;
  uint64_t   flag_can_push:1 ;
  uint64_t   flag_owns_src:1 ;
  uint64_t   flag_can_edit:1 ;
  uint64_t   flag:60         ;
  uint32_t   source_size     ;
  uint32_t   stream_size     ;
  uint32_t   current_lptoken ;
  uint32_t   current_pos     ;
  uint16_t   parenthesis_cnt ;
  uint16_t   bracket_cnt     ;
  uint16_t   brace_cnt       ;
  uint16_t   prev_indent     ;
} second_pass_lexer_t;


typedef enum LEXER_ERR_ENUM {
  LEXERR_OK = 0,
  LEXERR_INIT_FAIL,
  LEXERR_DESTROY_FAIL,
  LEXERR_PATTERN_FAIL,
  LEXERR_TOKENIZATION_FAIL,
  LEXERR_FIRST_PASS_FAIL,
  LEXERR_SECOND_PASS_FAIL,
  LEXERR_THIRD_PASS_FAIL,
  LEXERR_INVALID_STATE,
  LEXERR_COMMENT_FAIL,
  LEXERR_EOF_FAIL,
  LEXERR_OOB_FAIL,
  LEXERR_INDENT_FAIL,
  LEXERR_STRING_FAIL,
  LEXERR_CHAR_FAIL,
  LEXERR_BRACKET_FAIL,
  LEXERR_SPLFT_FAIL,
  LEXERR_SPLPOPT_FAIL,
  LEXERR_SPLPUSHT_FAIL,
  LEXERR_IO_FAIL,
} LEXER_ERR_ENUM;


typedef struct lpresult_t {
  uint32_t tag:8         ;
  uint32_t meta:24       ;
  union {
    uint32_t  as_error   ; // 0   ; LEXER_ERR_ENUM
    char      as_OK  [0] ; // 1   ; void
    uint16_t  as_token   ; // 2   ; TOKEN_KIND_ENUM
    char      as_char    ; // 3   ; char

    uint8_t   as_u8      ; // 4   ; uint8_t
    uint16_t  as_u16     ; // 5   ; uint16_t
    uint32_t  as_u32     ; // 6   ; uint32_t
    int8_t    as_i8      ; // 7   ; int8_t
    int16_t   as_i16     ; // 8   ; int16_t
    int32_t   as_i32     ; // 9   ; int32_t

    uint16_t  as_tokV[2] ; // 10  ; TOKEN_KIND_ENUM

    uint8_t   as_u8V [4] ; // 11  ; uint8_t[4]
    uint16_t  as_u16V[2] ; // 12  ; uint16_t[2]
    uint32_t  as_u32V[1] ; // 13  ; uint32_t[1]
    int8_t    as_i8V [4] ; // 14  ; int8_t[4]
    int16_t   as_i16V[2] ; // 15  ; int16_t[2]
    int32_t   as_i32V[1] ; // 16  ; int32_t[1]
  } val;
} lpresult_t;

typedef enum LPRESULT_ENUM {
  LPR_ERROR,
  LPR_OK,
  LPR_TOKEN,
  LPR_CHAR,

  LPR_U8,
  LPR_U16,
  LPR_U32,
  LPR_I8,
  LPR_I16,
  LPR_I32,

  LPR_TOKENV,

  LPR_U8V,
  LPR_U16V,
  LPR_U32V,
  LPR_I8V,
  LPR_I16V,
  LPR_I32V,

} LPRESULT_ENUM;


lpresult_t fplexer_init(first_pass_lexer_t* lexer, const char* filename);
lpresult_t fplexer_destroy(first_pass_lexer_t* lexer);
lpresult_t fplexer_move_source_code(first_pass_lexer_t* lexer, const char** trg);
lpresult_t fplexer_move_tokens(first_pass_lexer_t* lexer, const lptoken_t** trg);
lpresult_t fplexer_print_tokens(first_pass_lexer_t* lexer);



// typedef struct lexer_t {
//   uint16_t lexer_indent_depth;
//   uint16_t lexer_flags:14;
//   uint16_t lexer_phase2pass:3;
//   uint16_t lexer_phase:1;
//   uint16_t lexer_substitute:1;
//   uint32_t current_token_length;
//   token_t current_token;
//   const char* current_token_begins;
//   const char* current_pos;
//   uint32_t current_line;
//   uint32_t current_column;
//   size_t sym_window_begins;
//   symbol_t sym_window[LEXER_WINDOW_LENGTH];
//   token_t* token_stream;
//   token_t* past_stream;
//   const char* source;
//   size_t source_size;
//   size_t token_capacity;
//   size_t token_count;
// } lexer_t;

// LEXER_ERR_ENUM lexer_init(lexer_t* lexer, const char* filename, uint16_t indent_depth);
// LEXER_ERR_ENUM lexer_pick_from(lexer_t* lexer, lexer_t* src, uint16_t indent_depth);
// LEXER_ERR_ENUM lexer_join_with(lexer_t* lexer, lexer_t* trg, uint16_t indent_depth);
// LEXER_ERR_ENUM lexer_destroy(lexer_t* lexer);
// LEXER_ERR_ENUM lexer_first_pass_next(lexer_t* lexer);
// LEXER_ERR_ENUM lexer_second_pass_next(lexer_t* lexer);
// LEXER_ERR_ENUM lexer_first_pass(lexer_t* lexer);
// LEXER_ERR_ENUM lexer_second_pass(lexer_t* lexer);
// LEXER_ERR_ENUM lexer_full_pass(lexer_t* lexer);
bool is_keyword_check(const char* start, size_t length);



extern const symbol_t DELIMITER_SYMBOLS[];
extern const symbol_t KEYWORD_SYMBOLS[];


typedef struct CharClassifier {
  bool (*is_digit)(char c);
  bool (*is_alpha)(char c);
  bool (*is_alnum)(char c);
  bool (*is_whitespace)(char c);
  bool (*is_operator_start)(char c);
  bool (*is_identifier_start)(char c);
  bool (*is_identifier_continue)(char c);
} CharClassifier;

extern const CharClassifier ASCII_CLASSIFIER;


#endif
