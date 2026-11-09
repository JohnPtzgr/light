// parser.h
#ifndef PARSER_H
#define PARSER_H

#include "ast_DEP.h"

typedef struct Parser {
  Lexer* lexer;           // The lexer
  Token current;          // Current token being examined
  Token previous;         // Previous token (for error reporting)
  const char* filename;   // Current file being parsed
  
  // Error handling
  bool had_error;
  bool panic_mode;
  char error_msg[256];
  
  // For tracking scope (will expand later)
  int scope_depth;
  
  // For AST building
  AstArena* arena;        // AST allocation arena (to be defined)
} Parser;

// Parser API
Parser* parser_create(Lexer* lexer, const char* filename);
void parser_destroy(Parser* parser);

// Core parsing functions
AstExpr* parse_expr(Parser* parser);
AstStmt* parse_stmt(Parser* parser);
AstDecl* parse_decl(Parser* parser);
AstType* parse_type(Parser* parser);

// Helper functions
void parser_advance(Parser* parser);
bool parser_check(Parser* parser, TokenType type);
bool parser_match(Parser* parser, TokenType type);
bool parser_expect(Parser* parser, TokenType type, const char* message);
void parser_error(Parser* parser, const char* format, ...);
void parser_error_prev(Parser* parser, const char* format, ...);
void parser_sync(Parser* parser);  // Error recovery

#endif // PARSER_H