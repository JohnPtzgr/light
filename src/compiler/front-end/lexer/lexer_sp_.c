/*
 * lexer_second_pass.c – Phase 2 lexer: indentation, strings, comments, line continuations.
 *
 * Uses the first‑pass token array (lptoken_t) and the original source code to
 * produce a cleaned token stream ready for parsing.
 */

#include "lexer_.h"
#include "lexer_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ------------------------------------------------------------------ */
/*  Dynamic string table – holds accumulated string parts             */
/* ------------------------------------------------------------------ */
typedef struct {
    char   *data;
    size_t  size;
    size_t  capacity;
} StringTable;

static void strtab_init(StringTable *t) {
    t->capacity = 256;
    t->data = malloc(t->capacity);
    t->size = 0;
}

static void strtab_append(StringTable *t, const char *src, size_t len) {
    while (t->size + len > t->capacity) {
        t->capacity = t->capacity * 2;
        t->data = realloc(t->data, t->capacity);
    }
    memcpy(t->data + t->size, src, len);
    t->size += len;
}

static void strtab_free(StringTable *t) {
    free(t->data);
    t->data = NULL;
    t->size = t->capacity = 0;
}

/* ------------------------------------------------------------------ */
/*  Indentation stack & helpers                                        */
/* ------------------------------------------------------------------ */
#define MAX_INDENT_STACK 128

static int calculate_indent_weight(const lptoken_t *tok, const char *source) {
    int weight = 0;
    for (const char *p = source + tok->begins_at, *end = source + tok->ends_at; p < end; p++) {
        if (*p == ' ')      weight += 1;
        else if (*p == '\t') weight += DEF_TAB_LENGTH;
    }
    return weight;
}

/* ------------------------------------------------------------------ */
/*  Token emission helper                                              */
/* ------------------------------------------------------------------ */
static void emit_token(lptoken_t *dst, size_t *dst_idx,
                       uint16_t kind, uint32_t begins, uint32_t ends,
                       uint32_t line, uint16_t column) {
    dst[*dst_idx] = (lptoken_t){
        .kind      = kind,
        .begins_at = begins,
        .ends_at   = ends,
        .line      = line,
        .column    = column
    };
    (*dst_idx)++;
}

/* ------------------------------------------------------------------ */
/*  Forward declarations of the main recursive processing functions    */
/* ------------------------------------------------------------------ */
static void process_code(
    const lptoken_t *src, size_t *src_idx,
    const char *source, size_t src_count,
    lptoken_t *dst, size_t *dst_idx,
    int *indent_stack, int *stack_top,
    int *brace_balance, bool *is_at_line_start,
    StringTable *strtab,
    int boundary_kind);   /* -1 = no boundary, TOKEN_RBRACE = until matching '}' */

static void process_string(
    const lptoken_t *src, size_t *src_idx,
    const char *source, size_t src_count,
    lptoken_t *dst, size_t *dst_idx,
    int local_indent, bool is_fstring, bool is_raw,
    int *indent_stack, int *stack_top,
    int *brace_balance, bool *is_at_line_start,
    StringTable *strtab);

static void process_char_literal(
    const lptoken_t *src, size_t *src_idx,
    const char *source,
    lptoken_t *dst, size_t *dst_idx);

/* ------------------------------------------------------------------ */
/*  process_code – main loop                                           */
/* ------------------------------------------------------------------ */
static void process_code(
    const lptoken_t *src, size_t *src_idx,
    const char *source, size_t src_count,
    lptoken_t *dst, size_t *dst_idx,
    int *indent_stack, int *stack_top,
    int *brace_balance, bool *is_at_line_start,
    StringTable *strtab,
    int boundary_kind)
{
    while (*src_idx < src_count) {
        lptoken_t cur = src[*src_idx];
        if (cur.kind == TOKEN_EOF) break;

        /* ---- 2.1 Indentation analysis (only at start of logical line) ---- */
        if (*is_at_line_start) {
            /* Skip block comments ( / .  ...  . / ) with proper nesting */
            if (cur.kind == TOKEN_COMMENTBEG) {
                int depth = 1;
                (*src_idx)++;
                while (*src_idx < src_count && depth > 0) {
                    lptoken_t nc = src[*src_idx];
                    if (nc.kind == TOKEN_COMMENTBEG) depth++;
                    else if (nc.kind == TOKEN_COMMENTEND) depth--;
                    (*src_idx)++;
                }
                if (depth != 0) {
                    fprintf(stderr, "Error: unterminated block comment\n");
                    return;
                }
                continue;   /* stay at line start */
            }
            if (cur.kind == TOKEN_COMMENTEND) {
                /* Stray end – ignore */
                (*src_idx)++;
                continue;
            }

            /* Look for indentation (SPACE token that may be spaces/tabs) */
            if (cur.kind == TOKEN_SPACE &&
                source[cur.begins_at] != '\n')   /* not a newline */
            {
                /* Check if line is empty (only spaces followed by newline) */
                size_t peek = *src_idx + 1;
                bool next_is_newline = false;
                if (peek < src_count) {
                    lptoken_t nxt = src[peek];
                    if (nxt.kind == TOKEN_SPACE &&
                        source[nxt.begins_at] == '\n')
                        next_is_newline = true;
                }
                if (next_is_newline) {
                    *src_idx += 2;   /* skip indentation and newline */
                    continue;        /* empty line – no indent/dedent */
                }

                int current_indent = calculate_indent_weight(&cur, source);
                (*src_idx)++;

                if (current_indent > (*indent_stack)[*stack_top]) {
                    (*stack_top)++;
                    assert(*stack_top < MAX_INDENT_STACK);
                    (*indent_stack)[*stack_top] = current_indent;
                    emit_token(dst, dst_idx, TOKEN_INDENT,
                               cur.begins_at, cur.ends_at,
                               cur.line, cur.column);
                } else if (current_indent < (*indent_stack)[*stack_top]) {
                    while (current_indent < (*indent_stack)[*stack_top]) {
                        emit_token(dst, dst_idx, TOKEN_DEDENT,
                                   0, 0, cur.line, cur.column);
                        (*stack_top)--;
                    }
                    if (current_indent != (*indent_stack)[*stack_top]) {
                        fprintf(stderr, "Indentation error at line %d\n", cur.line);
                        return;
                    }
                }
                *is_at_line_start = false;
                continue;
            } else {
                /* No leading spaces – current indent is 0 */
                int current_indent = 0;
                if (current_indent < (*indent_stack)[*stack_top]) {
                    while (current_indent < (*indent_stack)[*stack_top]) {
                        emit_token(dst, dst_idx, TOKEN_DEDENT, 0, 0, cur.line, cur.column);
                        (*stack_top)--;
                    }
                }
                *is_at_line_start = false;
                /* Fall through to process this token */
            }
        }

        /* ---- 2.2 Scope evaluation (for interpolation termination) ---- */
        if (cur.kind == TOKEN_LBRACE) {
            (*brace_balance)++;
        } else if (cur.kind == TOKEN_RBRACE) {
            (*brace_balance)--;
            if (boundary_kind == TOKEN_RBRACE && *brace_balance == 0) {
                /* End of interpolation expression – return to process_string */
                return;
            }
        }

        /* ---- Skip block comments inside code ---- */
        if (cur.kind == TOKEN_COMMENTBEG) {
            int depth = 1;
            (*src_idx)++;
            while (*src_idx < src_count && depth > 0) {
                lptoken_t nc = src[*src_idx];
                if (nc.kind == TOKEN_COMMENTBEG) depth++;
                else if (nc.kind == TOKEN_COMMENTEND) depth--;
                (*src_idx)++;
            }
            continue;
        }
        if (cur.kind == TOKEN_COMMENTEND) {
            (*src_idx)++;   /* stray end, ignore */
            continue;
        }

        /* ---- Line continuation: backslash + newline ---- */
        if (cur.kind == TOKEN_BACKSLASH &&
            *src_idx + 1 < src_count)
        {
            lptoken_t next = src[*src_idx + 1];
            if (next.kind == TOKEN_SPACE &&
                source[next.begins_at] == '\n')
            {
                *src_idx += 2;
                continue;   /* join the lines */
            }
        }

        /* ---- String entry (both regular, raw and f‑strings) ---- */
        if (cur.kind == TOKEN_DOUBLEQUOTE) {
            bool is_fstring = false, is_raw = false;
            if (*src_idx > 0) {
                lptoken_t prev = src[*src_idx - 1];
                if (prev.kind == TOKEN_IDENTIFIER &&
                    prev.ends_at - prev.begins_at == 1)
                {
                    char ch = source[prev.begins_at];
                    if (ch == 'f') {
                        is_fstring = true;
                        /* Remove the 'f' token from the output */
                        if (*dst_idx > 0 && dst[*dst_idx - 1].kind == TOKEN_IDENTIFIER)
                            (*dst_idx)--;
                    } else if (ch == 'r') {
                        is_raw = true;
                        if (*dst_idx > 0 && dst[*dst_idx - 1].kind == TOKEN_IDENTIFIER)
                            (*dst_idx)--;
                    }
                }
            }
            int local_indent = (*indent_stack)[*stack_top];
            (*src_idx)++;   /* step over opening quote */
            process_string(src, src_idx, source, src_count,
                           dst, dst_idx, local_indent,
                           is_fstring, is_raw,
                           indent_stack, stack_top,
                           brace_balance, is_at_line_start,
                           strtab);
            continue;
        }

        /* ---- Character literals – pass through unchanged ---- */
        if (cur.kind == TOKEN_QUOTE) {
            process_char_literal(src, src_idx, source, dst, dst_idx);
            continue;
        }

        /* ---- Newline handling (inside brackets suppress indent) ---- */
        if (cur.kind == TOKEN_SPACE && source[cur.begins_at] == '\n') {
            /* A newline token */
            if (*brace_balance == 0) {
                *is_at_line_start = true;
            } else {
                *is_at_line_start = false;   /* open‑parenthesis rule */
            }
            emit_token(dst, dst_idx, cur.kind, cur.begins_at, cur.ends_at,
                       cur.line, cur.column);
            (*src_idx)++;
            continue;
        }

        /* ---- Default: pass through any other token ---- */
        emit_token(dst, dst_idx, cur.kind, cur.begins_at, cur.ends_at,
                   cur.line, cur.column);
        (*src_idx)++;
    }
}

/* ------------------------------------------------------------------ */
/*  process_string – handles everything between ""                    */
/* ------------------------------------------------------------------ */
static void process_string(
    const lptoken_t *src, size_t *src_idx,
    const char *source, size_t src_count,
    lptoken_t *dst, size_t *dst_idx,
    int local_indent, bool is_fstring, bool is_raw,
    int *indent_stack, int *stack_top,
    int *brace_balance, bool *is_at_line_start,
    StringTable *strtab)
{
    emit_token(dst, dst_idx, TOKEN_STRING_START, 0, 0, 0, 0);

    /* We accumulate string content into a temporary local buffer before
       transferring it to the string table when emitting a STRING_PART. */
    char   tmp[4096];
    size_t tmp_len = 0;
    uint32_t part_start_offset = (uint32_t)strtab->size;   /* offset in string table */

    bool strip_mode = false;

    while (*src_idx < src_count) {
        lptoken_t cur = src[*src_idx];
        if (cur.kind == TOKEN_EOF) {
            fprintf(stderr, "Error: unterminated string literal\n");
            return;
        }

        /* ---- 3.1 Multiline stripping ---- */
        if (strip_mode) {
            if (cur.kind == TOKEN_SPACE &&
                source[cur.begins_at] != '\n')
            {
                if (is_raw) {
                    int weight = calculate_indent_weight(&cur, source);
                    if (weight > local_indent) {
                        int keep = weight - local_indent;
                        for (int i = 0; i < keep; i++) {
                            if (tmp_len < sizeof(tmp) - 1) tmp[tmp_len++] = ' ';
                        }
                    }
                }
                /* Non‑raw and f‑strings: drop all leading spaces */
                (*src_idx)++;
                continue;
            } else {
                strip_mode = false;
            }
        }

        /* ---- 3.2 Escape sequence (non‑raw strings) ---- */
        if (!is_raw && cur.kind == TOKEN_BACKSLASH) {
            if (*src_idx + 1 < src_count) {
                lptoken_t next = src[*src_idx + 1];
                if (next.kind == TOKEN_DOUBLEQUOTE) {
                    if (tmp_len < sizeof(tmp) - 1) tmp[tmp_len++] = '\"';
                    *src_idx += 2;
                    continue;
                }
                /* Other escape sequences can be added here (\\, \n, \t, etc.) */
            }
        }

        /* ---- 3.3 Interpolation ${} (f‑strings only) ---- */
        if (is_fstring && !is_raw &&
            cur.kind == TOKEN_DOLLAR &&
            *src_idx + 1 < src_count &&
            src[*src_idx + 1].kind == TOKEN_LBRACE)
        {
            /* Flush current string part to the string table */
            if (tmp_len > 0) {
                strtab_append(strtab, tmp, tmp_len);
                emit_token(dst, dst_idx, TOKEN_STRING_PART,
                           part_start_offset, (uint32_t)strtab->size,
                           0, 0);
                tmp_len = 0;
                part_start_offset = (uint32_t)strtab->size;
            }

            emit_token(dst, dst_idx, TOKEN_INTERPOLATION_START, 0, 0, 0, 0);
            *src_idx += 2;   /* skip '$' and '{' */

            /* Recursively process inner expression until matching '}' */
            int saved_balance = *brace_balance;
            *brace_balance = 1;   /* the opening brace counts */
            process_code(src, src_idx, source, src_count,
                         dst, dst_idx,
                         indent_stack, stack_top,
                         brace_balance, is_at_line_start,
                         strtab,
                         TOKEN_RBRACE);
            /* After return, brace_balance is 0 */
            emit_token(dst, dst_idx, TOKEN_INTERPOLATION_END, 0, 0, 0, 0);
            (*src_idx)++;   /* step over the closing '}' */
            *brace_balance = saved_balance;
            part_start_offset = (uint32_t)strtab->size;
            continue;
        }

        /* ---- 3.4 String termination ---- */
        if (cur.kind == TOKEN_DOUBLEQUOTE) {
            if (tmp_len > 0 || part_start_offset == strtab->size) {
                if (tmp_len > 0)
                    strtab_append(strtab, tmp, tmp_len);
                emit_token(dst, dst_idx, TOKEN_STRING_PART,
                           part_start_offset, (uint32_t)strtab->size,
                           0, 0);
            }
            emit_token(dst, dst_idx, TOKEN_STRING_END, 0, 0, 0, 0);
            (*src_idx)++;   /* skip closing quote */
            return;
        }

        /* ---- 3.5 Newline capture (multiline strings) ---- */
        if (cur.kind == TOKEN_SPACE && source[cur.begins_at] == '\n') {
            if (tmp_len < sizeof(tmp) - 1) tmp[tmp_len++] = '\n';
            strip_mode = true;
            (*src_idx)++;
            continue;
        }

        /* ---- 3.6 Standard character – copy token text verbatim ---- */
        {
            const char *start = source + cur.begins_at;
            const char *end   = source + cur.ends_at;
            while (start < end) {
                if (tmp_len >= sizeof(tmp) - 1) {
                    /* Flush the buffer to the string table */
                    strtab_append(strtab, tmp, tmp_len);
                    emit_token(dst, dst_idx, TOKEN_STRING_PART,
                               part_start_offset, (uint32_t)strtab->size,
                               0, 0);
                    part_start_offset = (uint32_t)strtab->size;
                    tmp_len = 0;
                }
                tmp[tmp_len++] = *start++;
            }
            (*src_idx)++;
        }
    }
}

/* ------------------------------------------------------------------ */
/*  process_char_literal – simple pass‑through for 'x'                */
/* ------------------------------------------------------------------ */
static void process_char_literal(
    const lptoken_t *src, size_t *src_idx,
    const char *source,
    lptoken_t *dst, size_t *dst_idx)
{
    lptoken_t cur = src[*src_idx];
    /* The first pass should have already grouped the entire character
       literal into one token.  Emit it as is. */
    emit_token(dst, dst_idx, cur.kind, cur.begins_at, cur.ends_at,
               cur.line, cur.column);
    (*src_idx)++;
}

/* ------------------------------------------------------------------ */
/*  Main entry point for the second pass                               */
/* ------------------------------------------------------------------ */
LEXER_ERR_ENUM splexer_run(
    const lptoken_t *first_pass_tokens, size_t token_count,
    const char *source_code,
    lptoken_t **out_tokens, size_t *out_count,
    char **string_table, size_t *string_table_size)
{
    /* Allocate a generous output buffer – worst case many INDENT/DEDENT */
    size_t max_out = token_count * 4;
    lptoken_t *dst = malloc(max_out * sizeof(lptoken_t));
    if (!dst) return LEXERR_INIT_FAIL;

    int indent_stack[MAX_INDENT_STACK];
    indent_stack[0] = 0;
    int stack_top = 0;
    int brace_balance = 0;
    bool is_at_line_start = true;
    size_t src_idx = 0, dst_idx = 0;

    StringTable strtab;
    strtab_init(&strtab);

    process_code(first_pass_tokens, &src_idx, source_code, token_count,
                 dst, &dst_idx,
                 indent_stack, &stack_top,
                 &brace_balance, &is_at_line_start,
                 &strtab,
                 -1);   /* no boundary */

    /* Flush any remaining DEDENTs at EOF */
    while (stack_top > 0) {
        emit_token(dst, &dst_idx, TOKEN_DEDENT, 0, 0, 0, 0);
        stack_top--;
    }

    /* Append EOF marker */
    dst[dst_idx].kind = TOKEN_EOF;
    dst_idx++;

    /* Trim output array to exact size */
    lptoken_t *final_tokens = realloc(dst, dst_idx * sizeof(lptoken_t));
    if (!final_tokens) final_tokens = dst;   /* keep the larger block */

    *out_tokens        = final_tokens;
    *out_count         = dst_idx;
    *string_table      = strtab.data;
    *string_table_size = strtab.size;

    return LEXERR_OK;
}
