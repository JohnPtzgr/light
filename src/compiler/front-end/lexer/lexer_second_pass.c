#include "lexer_.h"
#include "lexer_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

lpresult_t splexer_count_leading_spaces(second_pass_lexer_t* lexer,
                                        const lptoken_t* token) {
  if (token->kind == TOKEN_SPACE && lexer->source_code[token->begins_at] == '\n') {
    uint16_t result = 0;
    while (lexer->source_code[token->begins_at + result + 1] &&
           (lexer->source_code[token->begins_at + result + 1] == ' ' ||
            lexer->source_code[token->begins_at + result + 1] == '\t')) {
      if (lexer->source_code[token->begins_at + result + 1] == ' ') {
        result++;
      } else {
        result += DEF_TAB_LENGTH;
      }
    }
    if (!lexer->source_code[token->begins_at + result + 1] ||
        lexer->source_code[token->begins_at + result + 1] == '\n') {
      return (lpresult_t){.val.as_error=LEXERR_INDENT_FAIL};
    }
    return (lpresult_t){.tag=LPR_U32, .val.as_u32=result};
  }
  return (lpresult_t){.val.as_error=LEXERR_INDENT_FAIL};
}


typedef enum STRING_KIND_ENUM {
  SKND_NORMAL,
  SKND_HAS_PREFIX,
  SKND_RAW,
  SKND_FORMAT,
  SKND_RAW_FORMAT,
} STRING_KIND_ENUM;

lpresult_t splexer_advance(second_pass_lexer_t* lexer);


lpresult_t splexer_find_token(uint32_t tokens_cnt,
                              lptoken_t* tokens,
                              const lptoken_t* token) {
  uint32_t min_idx = 0;
  uint32_t max_idx = tokens_cnt - 1;
  uint32_t mid_idx = max_idx / 2;
  while (max_idx - min_idx > 1) {
    if (tokens[mid_idx].begins_at <= token->begins_at) {
      min_idx = mid_idx;
    } else {
      max_idx = mid_idx;
    }
    mid_idx = (max_idx + min_idx) / 2;
  }
  for (size_t i = min_idx; i < max_idx + 1; ++i) {
    if (!memcmp(&tokens[i], token, sizeof(lptoken_t))) {
      return (lpresult_t){.tag=LPR_U32, .val.as_u32=i};
    }
  }
  return (lpresult_t){.val.as_error=LEXERR_SPLFT_FAIL};
}


lpresult_t splexer_get_string_kind(second_pass_lexer_t* lexer,
                                   const lptoken_t* token) {
  int has_format = 0;
  int has_raw = 0;
  
  if (token->kind != TOKEN_DOUBLEQUOTE) {
    return (lpresult_t){.val.as_error=LEXERR_STRING_FAIL};
  }

  if (!token->begins_at) {
    return (lpresult_t){.tag=LPR_U32, .val.as_u32=SKND_NORMAL};
  }

  lptoken_t* prev_token = token - 1;
  uint16_t raw_escape_tok = prev_token->kind;
  uint16_t raw_escape_len = 0;
  int has_prefix = 0;

  while (raw_escape_len < DEF_RAW_STR_ESCAPE_MAX_LEN && !has_prefix) {
    switch ((token - raw_escape_len - 1)->kind) {
      case TOKEN_IDENTIFIER:
        has_prefix = 1;
        break;
      case TOKEN_SPACE:
        raw_escape_len = DEF_RAW_STR_ESCAPE_MAX_LEN;
        break;
      case TOKEN_DIGITS:
        raw_escape_len = DEF_RAW_STR_ESCAPE_MAX_LEN;
        break;
      case TOKEN_COMMENTBEG:
        raw_escape_len = DEF_RAW_STR_ESCAPE_MAX_LEN;
        break;
      case TOKEN_COMMENTEND:
        raw_escape_len = DEF_RAW_STR_ESCAPE_MAX_LEN;
        break;
      default:
        if ((token - raw_escape_len - 1)->kind != raw_escape_tok) {
          raw_escape_len = DEF_RAW_STR_ESCAPE_MAX_LEN;
        }
        break;
    }
    if (token - raw_escape_len - 1 <= lexer->lptoken_source) {
      break;
    }
    raw_escape_len++;
  }

  if (has_prefix) {
    lptoken_t* prefix_token = token - raw_escape_len - 1;
    for (size_t i = prev_token->begins_at; i < prev_token->ends_at; ++i) {
      if (lexer->source_code[i] == 'f') {
        has_format = 1;
      }
      if (lexer->source_code[i] == 'r') {
        has_raw = 1;
      }
    }

    if (has_format & has_raw) {
      return (lpresult_t){.tag=LPR_U16V,
                          .val.as_u16V[0]=SKND_RAW_FORMAT,
                          .val.as_u16V[1]=raw_escape_len};
    }
    if (has_format) {
      return (lpresult_t){.tag=LPR_U32, .val.as_u32=SKND_FORMAT};
    }
    if (has_raw) {
      return (lpresult_t){.tag=LPR_U16V,
                          .val.as_u16V[0]=SKND_RAW,
                          .val.as_u16V[1]=raw_escape_len};
    }
    return (lpresult_t){.tag=LPR_U32, .val.as_u32=SKND_HAS_PREFIX};
  }

  return (lpresult_t){.tag=LPR_U32, .val.as_u32=SKND_NORMAL};
}


static inline lpresult_t splexer_peekn(second_pass_lexer_t* lexer, int32_t offset) {
  if (lexer->current_pos + offset >= lexer->source_size) {
    return (lpresult_t){.val.as_error = LEXERR_OOB_FAIL};
  }
  if (lexer->current_pos + offset < 0) {
    return (lpresult_t){.val.as_error = LEXERR_OOB_FAIL};
  }
  uint32_t result = lexer->lptoken_source[lexer->current_pos + offset].kind;
  if (result == TOKEN_EOF) {
    return (lpresult_t){.val.as_error = LEXERR_EOF_FAIL};
  }
  return (lpresult_t){.tag=LPR_TOKEN, .val.as_token = result};
}

static inline lpresult_t splexer_peek(second_pass_lexer_t* lexer) {
  return splexer_peekn(lexer, 1);
}

lpresult_t splexer_push_token(second_pass_lexer_t* lexer, lptoken_t token) {
  if (lexer->current_lptoken < lexer->stream_size && lexer->flag_can_push) {
    lexer->lptoken_stream[lexer->current_lptoken] = token;
    lexer->current_lptoken++;
    return (lpresult_t){.tag=LPR_OK};
  }
  return (lpresult_t){.val.as_error=LEXERR_SPLPUSHT_FAIL};
}

lpresult_t splexer_pop_token(second_pass_lexer_t* lexer) {
  if (lexer->current_lptoken && lexer->flag_can_edit) {
    lexer->current_lptoken--;
    lexer->lptoken_stream[lexer->current_lptoken] = (lptoken_t){};
    return (lpresult_t){.tag=LPR_OK};
  }
  return (lpresult_t){.val.as_error=LEXERR_SPLPOPT_FAIL};
}


lpresult_t splexer_handle_comment(second_pass_lexer_t* lexer) {
  lpresult_t current_token = splexer_peekn(lexer, 0);
  if (!current_token.tag) {
    return current_token;
  }
  if (current_token.val.as_token != TOKEN_COMMENTBEG) {
    return (lpresult_t){.val.as_error=LEXERR_COMMENT_FAIL};
  }
  // char* a = "abcde{f.2}";
  uint32_t counter = 1;
  while (counter > 0) {
    lexer->current_pos++;
    lpresult_t current_token = splexer_peekn(lexer, 0);
    if (!current_token.tag) {
      return (lpresult_t){.val.as_error=LEXERR_COMMENT_FAIL};
    }
    if (current_token.val.as_token == TOKEN_COMMENTBEG) {
      counter++;
    }
    if (current_token.val.as_token == TOKEN_COMMENTEND) {
      counter--;
    }
  }
  return (lpresult_t){.tag=LPR_OK};
}


lpresult_t splexer_handle_indent(second_pass_lexer_t* lexer) {
  lpresult_t current_token_res = splexer_peekn(lexer, 0);
  if (!current_token_res.tag) {
    return current_token_res;
  }
  lptoken_t* current_lptoken = &lexer->lptoken_source[lexer->current_pos];
  uint32_t sc_pos = current_lptoken->begins_at;
  if (current_token_res.val.as_token != TOKEN_SPACE ||
      lexer->source_code[sc_pos] == '\n') {
    return (lpresult_t){.val.as_error=LEXERR_INDENT_FAIL};
  }
  lpresult_t current_indent = splexer_count_leading_spaces(lexer, current_lptoken);
  if (!current_indent.tag) {
    return current_indent;
  }
  int insert_indent = 1;
  if (DEF_INDENT_CONT_BY_BRACE &&
      lexer->brace_cnt > 0) {
    insert_indent = 0;
  }
  if (DEF_INDENT_CONT_BY_BRACKET &&
      lexer->bracket_cnt > 0) {
    insert_indent = 0;
  }
  if (DEF_INDENT_CONT_BY_PAREN &&
      lexer->parenthesis_cnt > 0) {
    insert_indent = 0;
  }
  if (sizeof(DEF_INDENT_CONT_TOKENS) > 0) {
    lpresult_t prev_token = splexer_peekn(lexer, -1);
    if (prev_token.tag) {
      for (size_t i = 0; i < sizeof(DEF_INDENT_CONT_TOKENS) / sizeof(uint16_t); ++i) {
        if ((DEF_INDENT_CONT_TOKENS)[i] == prev_token.val.as_token) {
          insert_indent = 0;
        }
      }
    }
  }
  if (insert_indent) {
    int32_t delta = current_indent.val.as_u32 - lexer->prev_indent;
    if (delta % DEF_TAB_LENGTH) {
      return (lpresult_t){.val.as_error=LEXERR_INDENT_FAIL};
    }
    while (delta > 0) {
      lptoken_t token = (lptoken_t){.begins_at=current_lptoken->begins_at,
                                    .column=current_lptoken->column,
                                    .line=current_lptoken->line,
                                    .ends_at=current_lptoken->begins_at,
                                    .kind=TOKEN_INDENT};
      splexer_push_token(lexer, token);
      delta -= DEF_TAB_LENGTH;
    }
    while (delta < 0) {
      lptoken_t token = (lptoken_t){.begins_at=current_lptoken->begins_at,
                                    .column=current_lptoken->column,
                                    .line=current_lptoken->line,
                                    .ends_at=current_lptoken->begins_at,
                                    .kind=TOKEN_DEDENT};
      splexer_push_token(lexer, token);
      delta += DEF_TAB_LENGTH;
    }
    // lexer->lptoken_stream[lexer->current_lptoken] = TOKEN_INDENT;
  }
  lexer->prev_indent = current_indent.val.as_u32;
  return (lpresult_t){.tag=LPR_OK};
}


lpresult_t splexer_handle_string_prefix(second_pass_lexer_t* lexer, uint16_t len) {
  lpresult_t result = {};
  for (size_t i = 0; i <= len; ++i) {
    result = splexer_pop_token(lexer);
    __HANDLE_RESULT;
  }
  if (len) {
    lptoken_t tok_escape = lexer->lptoken_source[lexer->current_pos - 1];
    result.tag = LPR_TOKEN;
    result.val.as_token = tok_escape.kind;
  }
  return result;
}

lpresult_t splexer_handle_simple_string(second_pass_lexer_t* lexer) {
  int continue_ = 1;
  uint32_t index = lexer->current_pos;
  while (continue_) {
    index++;
    switch (lexer->lptoken_source[index].kind) {
      case TOKEN_DOUBLEQUOTE:
        continue_ = 0;
        break;
      case TOKEN_BACKSLASH:
        index++;
        if (index + 1 < lexer->source_size) {
          break;
        }
      case TOKEN_EOF:
        return (lpresult_t){.val.as_error=LEXERR_STRING_FAIL};
      default:
        break;
    }
  }
  lexer->current_pos = index + 1;
  return (lpresult_t){.tag=LPR_OK};
}

int splexer_cmp_token_prefix(second_pass_lexer_t* lexer,
                             uint16_t* prefix,
                             uint32_t len,
                             uint32_t index) {
  
  uint32_t i = 0;
  uint32_t iplen = len;
  while (index + i + 1 < lexer->source_size && i < iplen) {
    if (lexer->lptoken_source[index + i].kind != \
        prefix[i]) {
      break;
    }
    i++;
  }
  return i == iplen;
}

lpresult_t splexer_handle_format_string_intp(second_pass_lexer_t* lexer) {
  lpresult_t result = (lpresult_t){.tag=LPR_OK};
  int continue_ = 1;
  uint32_t fsibeg = __GET_ARR_LEN(DEF_FSTR_BEG_INTERPOL_AS_TOK);
  uint32_t fsiend = __GET_ARR_LEN(DEF_FSTR_END_INTERPOL_AS_TOK);
  int32_t counter = 0;
  while (continue_) {
    uint32_t ctk = lexer->lptoken_source[lexer->current_pos].kind;
    if (ctk == (DEF_FSTR_BEG_INTERPOL_AS_TOK)[fsibeg - 1]) {
      counter++;
      lexer->flag_subcall = 1;
      result = splexer_advance(lexer);
      __HANDLE_RESULT;
      lexer->flag_subcall = 0;
    }
    if (ctk == (DEF_FSTR_END_INTERPOL_AS_TOK)[0]) {
      counter--;
      if (counter == -1 &&
          splexer_cmp_token_prefix(lexer,
                                   DEF_FSTR_END_INTERPOL_AS_TOK,
                                   __GET_ARR_LEN(DEF_FSTR_END_INTERPOL_AS_TOK),
                                   lexer->current_pos)) {
        lptoken_t tok = lexer->lptoken_source[lexer->current_pos];
        tok.kind = TOKEN_FSTRING_INTERPOL_END;
        splexer_push_token(lexer, tok);
        lexer->current_pos += __GET_ARR_LEN(DEF_FSTR_END_INTERPOL_AS_TOK);
        return (lpresult_t){.tag=LPR_OK};
      }
      
      lexer->flag_subcall = 1;
      result = splexer_advance(lexer);
      __HANDLE_RESULT;
      lexer->flag_subcall = 0;
    }
  }
  return result;
}

int splexer_fstring_check_raw_esc(second_pass_lexer_t* lexer,
                                  uint32_t index,
                                  uint16_t raw_escape_token,
                                  uint16_t raw_escape_len) {
  int result = 1;
  int counter = 1;
  if (raw_escape_len) {
    while (counter) {
      index++;
      if (index >= lexer->source_size) {
        return -1;
      }
      uint32_t token_kind = lexer->lptoken_source[index].kind;
      if (token_kind != raw_escape_token) {
        return token_kind != TOKEN_EOF;
      }
      if (counter == raw_escape_len) {
        return 1;
      }
      counter++;
    }
  }
  return result;
}

lpresult_t splexer_handle_format_string(second_pass_lexer_t* lexer,
                                        uint16_t raw_escape_token,
                                        uint16_t raw_escape_len) {
  int continue_ = 1;
  uint32_t index = lexer->current_pos;
  while (continue_) {
    index++;
    switch (lexer->lptoken_source[index].kind) {
      case TOKEN_DOUBLEQUOTE:
        int chk = splexer_fstring_check_raw_esc(lexer,
                                                index,
                                                raw_escape_token,
                                                raw_escape_len);
        if (chk) {
          if (chk == -1) {
            return (lpresult_t){.val.as_error=LEXERR_STRING_FAIL};
          }
          continue_ = 0;
        }
        break;
      case TOKEN_BACKSLASH:
        if (raw_escape_token) {
          break;
        }
        index++;
        if (index + 1 < lexer->source_size) {
          break;
        }
      case TOKEN_EOF:
        return (lpresult_t){.val.as_error=LEXERR_STRING_FAIL};
      default:
        
        if (!splexer_cmp_token_prefix(lexer,
                                      DEF_FSTR_BEG_INTERPOL_AS_TOK,
                                      __GET_ARR_LEN(DEF_FSTR_BEG_INTERPOL_AS_TOK),
                                      index)) {
          break;
        }

        lptoken_t tok = lexer->lptoken_source[index];
        tok.kind = TOKEN_FSTRING_INTERPOL_END;
        splexer_push_token(lexer, tok);

        lexer->current_pos = index + __GET_ARR_LEN(DEF_FSTR_BEG_INTERPOL_AS_TOK);
        lpresult_t result = splexer_handle_format_string_intp(lexer);
        __HANDLE_RESULT;
        break;
    }
  }
  lexer->current_pos = index + 1;
  return (lpresult_t){.tag=LPR_OK};
}

lpresult_t splexer_handle_string(second_pass_lexer_t* lexer) {
  lpresult_t current_token = splexer_peekn(lexer, 0);
  if (!current_token.tag) {
    return current_token;
  }
  if (current_token.val.as_token != TOKEN_DOUBLEQUOTE) {
    return (lpresult_t){.val.as_error=LEXERR_STRING_FAIL};
  }

  lptoken_t* current_lptoken = &lexer->lptoken_source[lexer->current_pos];
  lpresult_t str_kind = splexer_get_string_kind(lexer, current_lptoken);

  lptoken_t str_tok = (lptoken_t){.kind=TOKEN_STRING,
                                  .begins_at=current_lptoken->begins_at,
                                  .column=current_lptoken->column,
                                  .line=current_lptoken->line};
  lpresult_t result = (lpresult_t){};

  switch (str_kind.tag) {
    case LPR_ERROR:
      return str_kind;
    case LPR_U32:
      if (str_kind.val.as_u32 == SKND_NORMAL) {
        lpresult_t result = splexer_handle_simple_string(lexer);
        __HANDLE_RESULT;
        break;
      }
      str_tok.begins_at = (current_lptoken - 1)->begins_at;
      str_tok.column = (current_lptoken - 1)->column;
      str_tok.line = (current_lptoken - 1)->line;
      result = splexer_handle_string_prefix(lexer, 0);
      __HANDLE_RESULT;
      if (str_kind.val.as_u32 == SKND_HAS_PREFIX) {
        result = splexer_handle_simple_string(lexer);
        __HANDLE_RESULT;
      }
      if (str_kind.val.as_u32 == SKND_FORMAT) {
        result = splexer_handle_format_string(lexer, 0, 0);
        __HANDLE_RESULT;
      }
    case LPR_U16V:
      break;
  
    default:
      break;
  }

  splexer_push_token(lexer, str_tok);
  return (lpresult_t){.tag=LPR_OK};
}


lpresult_t splexer_handle_char(second_pass_lexer_t* lexer) {
  lptoken_t* initial_lptoken = &lexer->lptoken_source[lexer->current_pos];
  lpresult_t current_token = splexer_peekn(lexer, 0);
  lpresult_t next_token = splexer_peek(lexer);
  if (current_token.val.as_token != TOKEN_QUOTE) {
    return (lpresult_t){.val.as_error=LEXERR_CHAR_FAIL};
  }
  while (current_token.tag && next_token.tag &&
         (current_token.val.as_token != TOKEN_QUOTE ||
          next_token.val.as_token == TOKEN_BACKSLASH)) {
    lexer->current_pos++;
    next_token = current_token;
    next_token = splexer_peek(lexer);
  }
  if (!current_token.tag) {
    return current_token;
  }
  if (!next_token.tag) {
    return next_token;
  }
  lptoken_t* current_lptoken = &lexer->lptoken_source[lexer->current_pos];
  lptoken_t char_token = (lptoken_t){.begins_at=initial_lptoken->begins_at,
                                     .column=initial_lptoken->column,
                                     .line=initial_lptoken->line,
                                     .kind=TOKEN_CHAR,
                                     .ends_at=current_lptoken->begins_at - 1};
  splexer_push_token(lexer, char_token);
  return (lpresult_t){.tag=LPR_OK};
}


lpresult_t splexer_handle_brackets(second_pass_lexer_t* lexer) {
  lpresult_t current_token = splexer_peekn(lexer, 0);
  if (!current_token.tag) {
    return current_token;
  }
  switch (current_token.val.as_token) {
    case TOKEN_LPAREN:
      lexer->parenthesis_cnt++;
      break;
    case TOKEN_RPAREN:
      lexer->parenthesis_cnt--;
      break;
    case TOKEN_LBRACKET:
      lexer->bracket_cnt++;
      break;
    case TOKEN_RBRACKET:
      lexer->bracket_cnt--;
      break;
    case TOKEN_LBRACE:
      lexer->brace_cnt++;
      break;
    case TOKEN_RBRACE:
      lexer->brace_cnt--;
      break;
    default:
      return (lpresult_t){.val.as_error=LEXERR_BRACKET_FAIL};
  }
  return (lpresult_t){.tag=LPR_OK};
}


lpresult_t splexer_advance(second_pass_lexer_t* lexer) {
  lpresult_t current_token = splexer_peekn(lexer, 0);
  lpresult_t next_token = splexer_peek(lexer);
  if (!current_token.tag) {
    return current_token;
  }
  lptoken_t* current_lptoken = &lexer->lptoken_source[lexer->current_pos];
  uint32_t sc_pos = current_lptoken->begins_at;
  lpresult_t result = {.tag=LPR_OK};
  switch (current_token.val.as_token) {
    case TOKEN_COMMENTBEG:
      result = splexer_handle_comment(lexer);
      break;
    case TOKEN_COMMENTEND:
      result = (lpresult_t){.val=LEXERR_COMMENT_FAIL};
      break;
    case TOKEN_SPACE:
      if (lexer->source_code[sc_pos] == '\n') {
        result = splexer_handle_indent(lexer);
      }
      splexer_push_token(lexer, *current_lptoken);
      break;
    case TOKEN_DOUBLEQUOTE:
      result = splexer_handle_string(lexer);
      break;
    case TOKEN_QUOTE:
      result = splexer_handle_char(lexer);
      break;
    case TOKEN_LBRACE:
    case TOKEN_RBRACE:
    case TOKEN_LBRACKET:
    case TOKEN_RBRACKET:
    case TOKEN_LPAREN:
    case TOKEN_RPAREN:
      result = splexer_handle_brackets(lexer);
    default:
      splexer_push_token(lexer, *current_lptoken);
      break;
  }
  return result;
}

