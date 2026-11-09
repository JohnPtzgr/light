// ast.h
#ifndef AST_H
#define AST_H

#include "../lexer/lexer.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


typedef struct AstNode AstNode;
typedef struct AstExpr AstExpr;
typedef struct AstStmt AstStmt;
typedef struct AstDecl AstDecl;
typedef struct AstType AstType;


typedef struct {
  const char* filename;
  int line;
  int column;
  int length;
} SourceLoc;

#define SOURCE_LOC_INIT { NULL, 0, 0, 0 }


typedef enum {
  // Expressions
  AST_EXPR_LITERAL_INT,
  AST_EXPR_LITERAL_FLOAT,
  AST_EXPR_LITERAL_STRING,
  AST_EXPR_LITERAL_CHAR,
  AST_EXPR_LITERAL_BOOL,
  AST_EXPR_LITERAL_NIL,
  AST_EXPR_IDENTIFIER,
  AST_EXPR_BINARY,
  AST_EXPR_UNARY,
  AST_EXPR_CALL,
  AST_EXPR_INDEX,
  AST_EXPR_MEMBER,
  AST_EXPR_CAST,
  AST_EXPR_IF,
  AST_EXPR_BLOCK,
  AST_EXPR_ARRAY_LIT,
  AST_EXPR_STRUCT_LIT,
  AST_EXPR_TUPLE_LIT,
  AST_EXPR_ANON_STRUCT_LIT,
  
  // Statements
  AST_STMT_LET,
  AST_STMT_ASSIGN,
  AST_STMT_IF,
  AST_STMT_WHILE,
  AST_STMT_FOR,
  AST_STMT_RETURN,
  AST_STMT_EXPR,
  AST_STMT_BLOCK,
  AST_STMT_MATCH,
  
  // Declarations
  AST_DECL_FUNCTION,
  AST_DECL_STRUCT,
  AST_DECL_ENUM,
  AST_DECL_UNION,
  AST_DECL_DATA,
  AST_DECL_STATE,
  AST_DECL_EVENT,
  AST_DECL_TRAIT,
  AST_DECL_IMPL,
  AST_DECL_MODULE,
  AST_DECL_EXTERN,
  AST_DECL_TYPE_ALIAS,
  AST_DECL_CONST,
  AST_DECL_STATIC,
  
  // Types
  AST_TYPE_PRIMITIVE,
  AST_TYPE_PRODUCT,      // T * U
  AST_TYPE_SUM,          // T ^ U
  AST_TYPE_UNION,        // T | U
  AST_TYPE_FUNCTION,     // T -> U
  AST_TYPE_ARRAY,        // [T] or [T; n]
  AST_TYPE_SET,          // {T}
  AST_TYPE_POINTER,      // T*
  AST_TYPE_REFERENCE,    // T&
  AST_TYPE_OPTIONAL,     // T?
  AST_TYPE_FALLIBLE,     // T!
  AST_TYPE_TUPLE,        // (T, U, V)
  AST_TYPE_NAMED,        // Type name
  AST_TYPE_GENERIC,      // Type<T>
  AST_TYPE_INFERRED,     // _ (placeholder)
  
  AST_NODE_COUNT
} AstNodeKind;

// ============================================================================
// Modifiers
// ============================================================================
typedef enum {
  STORAGE_NONE = 0,
  STORAGE_STATIC = 1 << 0,
  STORAGE_UNIQUE = 1 << 1,
  STORAGE_SHARED = 1 << 2,
  STORAGE_AUTOREF = 1 << 3,
  STORAGE_ARENA = 1 << 4,
  STORAGE_THREADLOCAL = 1 << 5,
} StorageModifier;

typedef enum {
  MUTABILITY_IMMUTABLE = 0,  // const (default)
  MUTABILITY_MUTABLE = 1 << 0,  // mut
} MutabilityModifier;

typedef enum {
  CONCURRENCY_NONE = 0,
  CONCURRENCY_ATOMIC = 1 << 0,
} ConcurrencyModifier;

typedef enum {
  OPTIMIZATION_NONE = 0,
  OPTIMIZATION_RESTRICT = 1 << 0,
  OPTIMIZATION_VOLATILE = 1 << 1,
  OPTIMIZATION_PURE = 1 << 2,
  OPTIMIZATION_CONST = 1 << 3,
} OptimizationModifier;

typedef struct {
  StorageModifier storage;
  MutabilityModifier mutability;
  ConcurrencyModifier concurrency;
  OptimizationModifier optimization;
  const char* custom_allocator;  // NULL for default
} TypeModifiers;

#define TYPE_MODIFIERS_INIT { STORAGE_NONE, MUTABILITY_IMMUTABLE, CONCURRENCY_NONE, OPTIMIZATION_NONE, NULL }

// ============================================================================
// Effect annotation
// ============================================================================
typedef enum {
  EFFECT_IO,
  EFFECT_SYS,
  EFFECT_ACQUIRE,
  EFFECT_RELEASE,
  EFFECT_MOVE,
  EFFECT_GLOBAL,
} EffectKind;

typedef struct AstEffect {
  EffectKind kind;
  const char* target;  // For global/move/acquire/release: variable name
  SourceLoc loc;
  struct AstEffect* next;
} AstEffect;

// ============================================================================
// AST node base
// ============================================================================
struct AstNode {
  AstNodeKind kind;
  SourceLoc loc;
};

// ============================================================================
// Expression nodes
// ============================================================================
typedef struct {
  AstNode base;
  union {
    // Literals
    struct {
      union {
        int64_t int_value;
        double float_value;
        bool bool_value;
      };
      const char* suffix;  // For numeric/string suffixes
    } literal;
    
    // Identifier
    struct {
      const char* name;
    } ident;

    // Binary operation
    struct {
      AstExpr* left;
      AstExpr* right;
      TokenType op;  // Uses your TOKEN_SCPLUS, TOKEN_SCMINUS, etc.
    } binary;
    
    // Unary operation
    struct {
      AstExpr* operand;
      TokenType op;  // Uses your TOKEN_SCMINUS, TOKEN_SCBANG, etc.
    } unary;
    
    // Function call
    struct {
      AstExpr* callee;
      AstExpr** args;
      size_t arg_count;
    } call;
    
    // Index operation (array[index])
    struct {
      AstExpr* object;
      AstExpr* index;
    } index;
    
    // Member access (object.field)
    struct {
      AstExpr* object;
      const char* field;
      bool is_positional;  // .0, .1, etc.
      int positional_index;  // For .0, .1
    } member;
    
    // Cast (x as Type)
    struct {
      AstExpr* expr;
      AstType* type;
    } cast;
    
    // If expression
    struct {
      AstExpr* condition;
      AstExpr* then_expr;
      AstExpr* else_expr;
    } if_expr;
    
    // Block expression
    struct {
      AstStmt** stmts;
      size_t stmt_count;
      AstExpr* result;  // Optional result expression
    } block;
    
    // Array literal [1, 2, 3]
    struct {
      AstExpr** elements;
      size_t element_count;
    } array_lit;
    
    // Struct literal Point{1, 2} or Point{.x = 1, .y = 2}
    struct {
      AstType* type;  // Can be NULL for anonymous
      struct {
        const char* name;  // NULL for positional
        AstExpr* value;
      } * fields;
      size_t field_count;
    } struct_lit;
    
    // Tuple literal (1, "hello", true)
    struct {
      AstExpr** elements;
      size_t element_count;
    } tuple_lit;
    
    // Anonymous struct literal .{x = 1, y = 2}
    struct {
      struct {
        const char* name;
        AstExpr* value;
      } * fields;
      size_t field_count;
    } anon_struct_lit;
  };
} AstExpr;

// Update token type checks in parser
static bool is_binary_operator(TokenType type) {
  switch (type) {
    case TOKEN_SCPLUS:
    case TOKEN_SCMINUS:
    case TOKEN_SCSTAR:
    case TOKEN_SCSLASH:
    case TOKEN_SCPERCENT:
    case TOKEN_SCASSIGN:
    case TOKEN_SCLESS:
    case TOKEN_SCGREATER:
    // Add comparison operators when you add them
    // case TOKEN_SCEQ:
    // case TOKEN_SCNE:
    // case TOKEN_SCLE:
    // case TOKEN_SCGE:
    // case TOKEN_SCAND:
    // case TOKEN_SCOR:
      return true;
    default:
      return false;
  }
}

static bool is_unary_operator(TokenType type) {
  switch (type) {
    case TOKEN_SCMINUS:
    case TOKEN_SCBANG:
    case TOKEN_SCTILDE:
    case TOKEN_SCSTAR:  // Dereference?
      return true;
    default:
      return false;
  }
}

// ============================================================================
// Statement nodes
// ============================================================================
typedef struct {
  AstNode base;
  union {
    // Let declaration: let x = 42; or let! x = 42;
    struct {
      const char* name;
      AstType* type;  // Can be NULL for inferred
      AstExpr* initializer;
      bool is_heap;   // true for let!, false for let
      TypeModifiers modifiers;
      AstEffect* effects;  // For the initialization
    } let;
    
    // Assignment: x = 42;
    struct {
      AstExpr* target;
      AstExpr* value;
    } assign;
    
    // If statement
    struct {
      AstExpr* condition;
      AstStmt* then_stmt;
      AstStmt* else_stmt;
    } if_stmt;
    
    // While loop
    struct {
      AstExpr* condition;
      AstStmt* body;
    } while_loop;
    
    // For loop
    struct {
      const char* var_name;
      AstExpr* iterable;
      AstStmt* body;
    } for_loop;
    
    // Return statement
    struct {
      AstExpr* value;  // Can be NULL for void return
    } return_stmt;
    
    // Expression statement (discards result)
    struct {
      AstExpr* expr;
    } expr_stmt;
    
    // Block statement
    struct {
      AstStmt** stmts;
      size_t stmt_count;
    } block;
    
    // Match statement
    struct {
      AstExpr* value;
      struct {
        AstExpr* pattern;  // Pattern AST (simplified for now)
        AstStmt* body;
      } * arms;
      size_t arm_count;
    } match_stmt;
  };
} AstStmt;

// ============================================================================
// Type nodes
// ============================================================================
struct AstType {
  AstNode base;
  TypeModifiers modifiers;
  union {
    // Primitive type: i32, string, bool, etc.
    struct {
      const char* name;
    } primitive;
    
    // Product type: T * U
    struct {
      AstType* left;
      AstType* right;
    } product;
    
    // Sum type: T ^ U
    struct {
      AstType* left;
      AstType* right;
    } sum;
    
    // Union type: T | U
    struct {
      AstType* left;
      AstType* right;
    } union_type;
    
    // Function type: T -> U
    struct {
      AstType* param;   // Could be tuple for multiple params
      AstType* result;
    } function;
    
    // Array type: [T] or [T; n]
    struct {
      AstType* element;
      AstExpr* size;    // NULL for dynamic arrays
    } array;
    
    // Set type: {T}
    struct {
      AstType* element;
    } set;
    
    // Pointer type: T*
    struct {
      AstType* pointee;
    } pointer;
    
    // Reference type: T&
    struct {
      AstType* referent;
      const char* lifetime;  // NULL for no explicit lifetime
    } reference;
    
    // Optional type: T?
    struct {
      AstType* base;
    } optional;
    
    // Fallible type: T!
    struct {
      AstType* base;
    } fallible;
    
    // Tuple type: (T, U, V)
    struct {
      AstType** elements;
      size_t element_count;
    } tuple;
    
    // Named type: Point (struct/enum/union name)
    struct {
      const char* name;
      AstType** type_args;  // Generic arguments
      size_t type_arg_count;
    } named;
    
    // Generic type: T in function<T>
    struct {
      const char* name;
    } type_param;
    
    // Inferred type: _ (placeholder)
    struct {
      // No extra data
    } inferred;
  };
};

// ============================================================================
// Declaration nodes
// ============================================================================
typedef struct {
  AstNode base;
  const char* name;
  SourceLoc name_loc;
  AstEffect* effects;  // For function declarations
  
  union {
    // Function declaration: fn foo(x: i32) -> i32 { ... }
    struct {
      struct {
        const char* name;
        AstType* type;
      } * params;
      size_t param_count;
      AstType* return_type;
      AstStmt* body;
      bool is_pub;
      bool is_unsafe;
      const char** type_params;  // Generic type parameters
      size_t type_param_count;
    } function;
    
    // Struct definition: struct Point { x: i32; y: i32; }
    struct {
      struct {
        const char* name;
        AstType* type;
      } * fields;
      size_t field_count;
      bool is_pub;
      bool is_packed;
      int alignment;  // 0 for default
    } struct_def;
    
    // Enum definition: enum Option<T> { Some(T); None; }
    struct {
      struct {
        const char* name;
        AstType** types;  // Types for each variant
        size_t type_count;
      } * variants;
      size_t variant_count;
      bool is_pub;
      const char** type_params;
      size_t type_param_count;
    } enum_def;
    
    // Union definition: union Number { i: i32; f: f64; }
    struct {
      struct {
        const char* name;
        AstType* type;
      } * members;
      size_t member_count;
      bool is_pub;
    } union_def;
    
    // Data definition: data User { name: str; id: i64; }
    struct {
      struct {
        const char* name;
        AstType* type;
      } * fields;
      size_t field_count;
      bool is_pub;
      const char* allocator;  // Custom allocator
    } data_def;
    
    // State machine definition
    struct {
      struct {
        const char* name;
        struct {
          const char* field_name;
          AstType* type;
        } * fields;
        size_t field_count;
      } * states;
      size_t state_count;
      
      struct {
        const char* name;
        const char* from_state;
        const char* to_state;
        struct {
          const char* field_name;
          AstExpr* value;
        } * transition_fields;
        size_t transition_field_count;
      } * events;
      size_t event_count;
      
      bool is_pub;
    } state_def;
    
    // Trait definition
    struct {
      const char** methods;
      size_t method_count;
      bool is_pub;
    } trait_def;
    
    // Impl block
    struct {
      const char* trait_name;
      const char* type_name;
      AstDecl** methods;
      size_t method_count;
    } impl_def;
    
    // Module definition
    struct {
      AstDecl** decls;
      size_t decl_count;
      AstStmt* init;   // init block
      AstStmt* exit;   // exit block
      bool is_pub;
    } module_def;
    
    // Extern block
    struct {
      const char* language;  // "C", "ASM"
      const char* standard;  // "c99", "x86_64"
      struct {
        const char* name;
        AstType* type;
      } * captures;
      size_t capture_count;
      const char* code;  // Raw code string
    } extern_def;
    
    // Type alias
    struct {
      AstType* aliased_type;
      bool is_pub;
    } type_alias;
    
    // Constant declaration
    struct {
      AstExpr* value;
      bool is_pub;
    } const_def;
    
    // Static variable
    struct {
      AstType* type;
      AstExpr* initializer;
      bool is_pub;
      bool is_mut;
    } static_def;
  };
} AstDecl;

// ============================================================================
// AST creation functions
// ============================================================================
AstExpr* ast_create_expr_literal_int(int64_t value, const char* suffix, SourceLoc loc);
AstExpr* ast_create_expr_literal_float(double value, const char* suffix, SourceLoc loc);
AstExpr* ast_create_expr_literal_string(const char* value, size_t len, const char* suffix, SourceLoc loc);
AstExpr* ast_create_expr_literal_char(char value, const char* suffix, SourceLoc loc);
AstExpr* ast_create_expr_literal_bool(bool value, SourceLoc loc);
AstExpr* ast_create_expr_literal_nil(SourceLoc loc);
AstExpr* ast_create_expr_identifier(const char* name, SourceLoc loc);
AstExpr* ast_create_expr_binary(AstExpr* left, TokenType op, AstExpr* right, SourceLoc loc);
AstExpr* ast_create_expr_unary(TokenType op, AstExpr* operand, SourceLoc loc);
AstExpr* ast_create_expr_call(AstExpr* callee, AstExpr** args, size_t arg_count, SourceLoc loc);
AstExpr* ast_create_expr_index(AstExpr* object, AstExpr* index, SourceLoc loc);
AstExpr* ast_create_expr_member(AstExpr* object, const char* field, bool is_positional, int pos_index, SourceLoc loc);
AstExpr* ast_create_expr_cast(AstExpr* expr, AstType* type, SourceLoc loc);
AstExpr* ast_create_expr_if(AstExpr* cond, AstExpr* then_expr, AstExpr* else_expr, SourceLoc loc);
AstExpr* ast_create_expr_block(AstStmt** stmts, size_t stmt_count, AstExpr* result, SourceLoc loc);
AstExpr* ast_create_expr_array_lit(AstExpr** elements, size_t count, SourceLoc loc);
AstExpr* ast_create_expr_struct_lit(AstType* type, const char** names, AstExpr** values, size_t count, SourceLoc loc);
AstExpr* ast_create_expr_tuple_lit(AstExpr** elements, size_t count, SourceLoc loc);
AstExpr* ast_create_expr_anon_struct_lit(const char** names, AstExpr** values, size_t count, SourceLoc loc);

AstStmt* ast_create_stmt_let(const char* name, AstType* type, AstExpr* init, bool is_heap, 
                             TypeModifiers mods, AstEffect* effects, SourceLoc loc);
AstStmt* ast_create_stmt_assign(AstExpr* target, AstExpr* value, SourceLoc loc);
AstStmt* ast_create_stmt_if(AstExpr* cond, AstStmt* then_stmt, AstStmt* else_stmt, SourceLoc loc);
AstStmt* ast_create_stmt_while(AstExpr* cond, AstStmt* body, SourceLoc loc);
AstStmt* ast_create_stmt_for(const char* var, AstExpr* iter, AstStmt* body, SourceLoc loc);
AstStmt* ast_create_stmt_return(AstExpr* value, SourceLoc loc);
AstStmt* ast_create_stmt_expr(AstExpr* expr, SourceLoc loc);
AstStmt* ast_create_stmt_block(AstStmt** stmts, size_t count, SourceLoc loc);
AstStmt* ast_create_stmt_match(AstExpr* value, AstExpr** patterns, AstStmt** bodies, size_t count, SourceLoc loc);

AstType* ast_create_type_primitive(const char* name, TypeModifiers mods, SourceLoc loc);
AstType* ast_create_type_product(AstType* left, AstType* right, TypeModifiers mods, SourceLoc loc);
AstType* ast_create_type_sum(AstType* left, AstType* right, TypeModifiers mods, SourceLoc loc);
AstType* ast_create_type_union(AstType* left, AstType* right, TypeModifiers mods, SourceLoc loc);
AstType* ast_create_type_function(AstType* param, AstType* result, TypeModifiers mods, SourceLoc loc);
AstType* ast_create_type_array(AstType* element, AstExpr* size, TypeModifiers mods, SourceLoc loc);
AstType* ast_create_type_set(AstType* element, TypeModifiers mods, SourceLoc loc);
AstType* ast_create_type_pointer(AstType* pointee, TypeModifiers mods, SourceLoc loc);
AstType* ast_create_type_reference(AstType* referent, const char* lifetime, TypeModifiers mods, SourceLoc loc);
AstType* ast_create_type_optional(AstType* base, TypeModifiers mods, SourceLoc loc);
AstType* ast_create_type_fallible(AstType* base, TypeModifiers mods, SourceLoc loc);
AstType* ast_create_type_tuple(AstType** elements, size_t count, TypeModifiers mods, SourceLoc loc);
AstType* ast_create_type_named(const char* name, AstType** args, size_t arg_count, TypeModifiers mods, SourceLoc loc);
AstType* ast_create_type_param(const char* name, TypeModifiers mods, SourceLoc loc);
AstType* ast_create_type_inferred(SourceLoc loc);

AstDecl* ast_create_decl_function(const char* name, const char** type_params, size_t type_param_count,
                                 const char** param_names, AstType** param_types, size_t param_count,
                                 AstType* return_type, AstStmt* body, bool is_pub, bool is_unsafe,
                                 AstEffect* effects, SourceLoc loc);
AstDecl* ast_create_decl_struct(const char* name, const char** field_names, AstType** field_types,
                               size_t field_count, bool is_pub, bool is_packed, int alignment, SourceLoc loc);
AstDecl* ast_create_decl_enum(const char* name, const char** variant_names, AstType*** variant_types,
                             size_t* variant_type_counts, size_t variant_count, bool is_pub,
                             const char** type_params, size_t type_param_count, SourceLoc loc);
// ... more creation functions

// ============================================================================
// AST destruction
// ============================================================================
void ast_free_expr(AstExpr* expr);
void ast_free_stmt(AstStmt* stmt);
void ast_free_type(AstType* type);
void ast_free_decl(AstDecl* decl);

// ============================================================================
// AST printing (for debugging)
// ============================================================================
void ast_print_expr(AstExpr* expr, int indent);
void ast_print_stmt(AstStmt* stmt, int indent);
void ast_print_type(AstType* type, int indent);
void ast_print_decl(AstDecl* decl, int indent);

#endif // AST_H