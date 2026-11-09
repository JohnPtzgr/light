#include "parser_DEP.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Parser lifecycle
// ============================================================================
Parser* parser_create(Lexer* lexer, const char* filename) {
  Parser* parser = (Parser*)malloc(sizeof(Parser));
  if (!parser) return NULL;
  
  parser->lexer = lexer;
  parser->filename = filename;
  parser->had_error = false;
  parser->panic_mode = false;
  parser->scope_depth = 0;
  parser->arena = ast_arena_create();  // We'll need to implement this
  
  // Get first token
  parser->current = lexer_next(lexer);
  parser->previous = parser->current;
  
  return parser;
}

void parser_destroy(Parser* parser) {
  if (parser) {
    ast_arena_destroy(parser->arena);
    free(parser);
  }
}

// ============================================================================
// Token handling
// ============================================================================
void parser_advance(Parser* parser) {
  parser->previous = parser->current;
  parser->current = lexer_next(parser->lexer);
}

bool parser_check(Parser* parser, TokenType type) {
  return parser->current.type == type;
}

bool parser_match(Parser* parser, TokenType type) {
  if (parser_check(parser, type)) {
    parser_advance(parser);
    return true;
  }
  return false;
}

bool parser_expect(Parser* parser, TokenType type, const char* message) {
  if (parser_check(parser, type)) {
    parser_advance(parser);
    return true;
  }
  
  parser_error(parser, "Expected %s, got %s. %s",
               token_type_name(type),
               token_type_name(parser->current.type),
               message);
  return false;
}

// ============================================================================
// Error handling
// ============================================================================
void parser_error(Parser* parser, const char* format, ...) {
  parser->had_error = true;
  
  if (parser->panic_mode) return;
  
  fprintf(stderr, "%s:%d:%d: error: ",
          parser->filename,
          parser->current.line,
          parser->current.column);
  
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fprintf(stderr, "\n");
  
  // Show the line with caret
  // ... (similar to lexer error display)
}

void parser_error_prev(Parser* parser, const char* format, ...) {
  parser->had_error = true;
  
  if (parser->panic_mode) return;
  
  fprintf(stderr, "%s:%d:%d: error: ",
          parser->filename,
          parser->previous.line,
          parser->previous.column);
  
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fprintf(stderr, "\n");
}

void parser_sync(Parser* parser) {
  parser->panic_mode = false;
  
  // Skip until we find a statement boundary
  while (!parser_check(parser, TOKEN_CTRLEOF)) {
    if (parser_check(parser, TOKEN_SCSEMICOLON)) {
      parser_advance(parser);
      return;
    }
    
    switch (parser->current.type) {
      case TOKEN_LET:
      case TOKEN_FN:
      case TOKEN_STRUCT:
      case TOKEN_ENUM:
      case TOKEN_IMPL:
      case TOKEN_MOD:
      case TOKEN_PUB:
      case TOKEN_EXTERN:
      case TOKEN_RETURN:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_FOR:
        return;
      default:
        parser_advance(parser);
        break;
    }
  }
}

// ============================================================================
// Expression parsing (pratt parser style)
// ============================================================================
typedef enum {
  PREC_NONE,
  PREC_ASSIGN,      // =
  PREC_OR,          // ||
  PREC_AND,         // &&
  PREC_EQUAL,       // == !=
  PREC_COMPARE,     // < > <= >=
  PREC_SHIFT,       // << >>
  PREC_TERM,        // + -
  PREC_FACTOR,      // * / %
  PREC_UNARY,       // ! - ~
  PREC_CALL,        // . () []
  PREC_PRIMARY
} Precedence;

static Precedence get_precedence(TokenType type) {
  switch (type) {
    case TOKEN_SCASSIGN: return PREC_ASSIGN;
    // case TOKEN_SCOR: return PREC_OR;      // When added
    // case TOKEN_SCAND: return PREC_AND;     // When added
    // case TOKEN_SCEQ: case TOKEN_SCNE: return PREC_EQUAL;  // When added
    case TOKEN_SCLESS: case TOKEN_SCGREATER: return PREC_COMPARE;
    case TOKEN_SCPLUS: case TOKEN_SCMINUS: return PREC_TERM;
    case TOKEN_SCSTAR: case TOKEN_SCSLASH: case TOKEN_SCPERCENT: return PREC_FACTOR;
    default: return PREC_NONE;
  }
}

static AstExpr* parse_primary(Parser* parser) {
  Token token = parser->current;
  
  switch (token.type) {
    case TOKEN_NUMBER:
      parser_advance(parser);
      // TODO: Parse number literal (check for float/int)
      return ast_create_expr_literal_int(0, NULL, 
                 (SourceLoc){parser->filename, token.line, token.column, token.length});
    
    case TOKEN_STRING:
      parser_advance(parser);
      // TODO: Extract string content
      return ast_create_expr_literal_string("", 0, NULL,
                 (SourceLoc){parser->filename, token.line, token.column, token.length});
    
    case TOKEN_CHAR:
      parser_advance(parser);
      // TODO: Extract char
      return ast_create_expr_literal_char(0, NULL,
                 (SourceLoc){parser->filename, token.line, token.column, token.length});
    
    case TOKEN_TRUE:
    case TOKEN_FALSE:
      parser_advance(parser);
      return ast_create_expr_literal_bool(token.type == TOKEN_TRUE,
                 (SourceLoc){parser->filename, token.line, token.column, token.length});
    
    case TOKEN_NIL:
      parser_advance(parser);
      return ast_create_expr_literal_nil(
                 (SourceLoc){parser->filename, token.line, token.column, token.length});
    
    case TOKEN_IDENTIFIER:
      parser_advance(parser);
      return ast_create_expr_identifier(
                 extract_token_string(token),  // Need helper to extract
                 (SourceLoc){parser->filename, token.line, token.column, token.length});
    
    case TOKEN_SCLPAREN:
      parser_advance(parser);
      AstExpr* expr = parse_expr(parser);
      parser_expect(parser, TOKEN_SCRPAREN, "Expected ')' after expression");
      return expr;
    
    default:
      parser_error(parser, "Unexpected token in expression");
      return NULL;
  }
}

static AstExpr* parse_unary(Parser* parser) {
  if (is_unary_operator(parser->current.type)) {
    TokenType op = parser->current.type;
    SourceLoc loc = {parser->filename, parser->current.line, 
                     parser->current.column, parser->current.length};
    parser_advance(parser);
    
    AstExpr* operand = parse_unary(parser);
    return ast_create_expr_unary(op, operand, loc);
  }
  
  return parse_primary(parser);
}

static AstExpr* parse_binary(Parser* parser, Precedence min_prec) {
  AstExpr* left = parse_unary(parser);
  
  while (true) {
    TokenType op = parser->current.type;
    Precedence prec = get_precedence(op);
    
    if (prec == PREC_NONE || prec < min_prec) {
      break;
    }
    
    parser_advance(parser);
    AstExpr* right = parse_binary(parser, (Precedence)(prec + 1));
    
    left = ast_create_expr_binary(left, op, right,
           (SourceLoc){parser->filename, parser->previous.line, 
                       parser->previous.column, parser->previous.length});
  }
  
  return left;
}

AstExpr* parse_expr(Parser* parser) {
  return parse_binary(parser, PREC_NONE);
}

// ============================================================================
// Statement parsing (to be expanded)
// ============================================================================
AstStmt* parse_stmt(Parser* parser) {
  switch (parser->current.type) {
    case TOKEN_LET:
      return parse_let_stmt(parser);
    case TOKEN_IF:
      return parse_if_stmt(parser);
    case TOKEN_WHILE:
      return parse_while_stmt(parser);
    case TOKEN_FOR:
      return parse_for_stmt(parser);
    case TOKEN_RETURN:
      return parse_return_stmt(parser);
    case TOKEN_SCLBRACE:
      return parse_block_stmt(parser);
    default:
      return parse_expr_stmt(parser);
  }
}

static AstStmt* parse_let_stmt(Parser* parser) {
  Token let_token = parser->current;
  parser_advance(parser); // consume 'let'
  
  bool is_heap = false;
  if (parser_match(parser, TOKEN_SCBANG)) {
    is_heap = true;
  }
  
  // Parse variable name
  if (!parser_expect(parser, TOKEN_IDENTIFIER, "Expected variable name")) {
    return NULL;
  }
  
  const char* name = extract_token_string(parser->previous);
  
  // Parse optional type annotation
  AstType* type = NULL;
  if (parser_match(parser, TOKEN_SCCOLON)) {
    type = parse_type(parser);
  }
  
  // Parse initializer
  if (!parser_expect(parser, TOKEN_SCASSIGN, "Expected '=' in let declaration")) {
    return NULL;
  }
  
  AstExpr* init = parse_expr(parser);
  parser_expect(parser, TOKEN_SCSEMICOLON, "Expected ';' after let declaration");
  
  return ast_create_stmt_let(name, type, init, is_heap,
                             TYPE_MODIFIERS_INIT, NULL,
                             (SourceLoc){parser->filename, let_token.line,
                                         let_token.column, let_token.length});
}

// ... more statement parsers

// ============================================================================
// Type parsing (to be expanded)
// ============================================================================
AstType* parse_type(Parser* parser) {
  // TODO: Implement full type parsing with precedence
  // For now, just handle primitives and identifiers
  
  switch (parser->current.type) {
    case TOKEN_IDENTIFIER:
    case TOKEN_BOOL:
    case TOKEN_VOID:
    case TOKEN_VAR:
    case TOKEN_OBJECT:
    case TOKEN_PTR:
    case TOKEN_TYPE: {
      Token token = parser->current;
      parser_advance(parser);
      return ast_create_type_primitive(extract_token_string(token),
                                       TYPE_MODIFIERS_INIT,
                                       (SourceLoc){parser->filename, token.line,
                                                   token.column, token.length});
    }
    
    default:
      parser_error(parser, "Expected type");
      return NULL;
  }
}
