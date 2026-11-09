#include "../../front-end/lexer/lexer.h"
#include "../../utils/hashtable.h"
#include <limits.h>


typedef struct TypeInfo;

enum {
  INVARK_ERROR = -1,
  INVARK_OK = 0,
  INVARK_DEFAULT,
  INVARK_OPTIONAL,
  INVARK_EXCEPTION,
  INVARK_PANIC,
} INVARIANT_KIND_ENUM;

typedef int invariant_kind_t;

typedef struct {
  const char* name;
  TypeInfo* type;
  size_t offset;
  invariant_kind_t (*invariant)(TypeInfo* type, void* self, ... );
  size_t virtual:1;
  void* default_var;
} TypeField;

typedef struct {
  TypeInfo* type;
  void* data;
} ResultType;

enum {
  FNEFFECT_ACQUIRE=1,  // mandatory
  FNEFFECT_RELEASE,    // mandatory
  FNEFFECT_MOVE,       // mandatory for resource
  FNEFFECT_ASSIGN,
  FNEFFECT_MUTATE,
  FNEFFECT_IO,
  FNEFFECT_SYS,
  FNEFFECT_GLOBAL,     // mandatory
  FNEFFECT_EXCEPT,     // mandatory
  FNEFFECT_KILL,       // mandatory
} FNEFFECT_ENUM;

enum {
  FNEFFECTTRG_NONE,
  FNEFFECTTRG_VARIABLE,  // mandatory
  FNEFFECTTRG_RETURN,    // mandatory
  FNEFFECTTRG_GLOBAL,    // mandatory
} FNEFFECTTRG_ENUM;

typedef struct {
  size_t effect_kind:4;
  size_t target_kind:2;
  #if UINTPTR_MAX == 0xffffffffffffffffULL
    size_t arguments:58;
  #else
    size_t arguments:26;
  #endif
  const char* identifier_name;
} FunEffect;

typedef struct FunArg {
  TypeInfo* type;
  void* default_var;
} FunArg;

typedef struct Method {
  const char* name;
  FunArg* arg_types;
  TypeInfo* return_type;
  invariant_kind_t (*arg_invariants)(TypeInfo* type, void* self, ... );
  invariant_kind_t (*return_invariants)(TypeInfo* type, void* self, ... );
  ResultType (*impl_function)(TypeInfo* type, void* self, ... );
  FunEffect* effects;
  size_t m_safe:1;
  size_t m_async:1;
  size_t m_access:2; // 4
  size_t m_vararg:1;
  size_t m_varret:1;
  size_t m_constexpr:1;
  size_t m_generic:1; // 8
  size_t m_static:1;
  size_t m_pure:1;
  #if UINTPTR_MAX == 0xffffffffffffffffULL
  size_t index:54;
  #else
  size_t index:22;
  #endif
} Method;

typedef struct Function {
  const char* name;
  FunArg* arg_types;
  TypeInfo* return_type;
  invariant_kind_t (*arg_invariants)(TypeInfo* type, void* self, ... );
  invariant_kind_t (*return_invariants)(TypeInfo* type, void* self, ... );
  ResultType (*impl_function)(TypeInfo* type, void* self, ... );
  FunEffect* effects;
  size_t m_safe:1;
  size_t m_async:1;
  size_t m_access:2; // 4
  size_t m_vararg:1;
  size_t m_varret:1;
  size_t m_constexpr:1;
  size_t m_generic:1; // 8
  size_t m_static:1;
  size_t m_pure:1;
  #if UINTPTR_MAX == 0xffffffffffffffffULL
    size_t __pad:54;
  #else
    size_t __pad:22;
  #endif
} Function;

typedef struct Trait {
  const char* name;
  Function* functions;
} Trait;

typedef struct Lifetime {
  size_t scope_id;                        // 0 for static, -1 for runtime
  #if UINTPTR_MAX == 0xffffffffffffffffULL
    size_t valid_from:32;
    size_t valid_till:32;
  #else
    size_t valid_from:16;
    size_t valid_till:16;
  #endif
} Lifetime;

typedef struct {
  const char* name;
  TypeInfo* type;
  Lifetime lifetime;
  invariant_kind_t (*invariant)(TypeInfo* type, void* self, ... );
  void* value;
} Variable;

enum {
  TIBIT_DEFAULT = 0,
  TIBIT_PTR,
  TIBIT_REF,
  TIBIT_LIFETIMEREF,
  TIBIT_ARRAY,
  TIBIT_SOA,
  TIBIT_SLICE,
  TIBIT_DICT,
  TIBIT_SET,
  TIBIT_TUPLE_ST,
  TIBIT_TUPLE_MT,
  TIBIT_INTERSECTION,
  TIBIT_UNION,
  TIBIT_ENUM,
  TIBIT_FUNCTION,
  TIBIT_LAMBDA,
} TYPEINFOBIT_ENUM;

typedef struct TypeInfo {
  const char* name;
  size_t m_storage:3;    // default, stack, static, unique, shared, autoref, arena, threadlocal
  size_t m_const:1;      // 4
  size_t m_restrict:1;
  size_t m_volatile:1;
  size_t m_constexpr:1;
  size_t m_atomic:1;       // 8
  size_t is_safe:1;
  size_t rembitsize:3;   // 12
  size_t default_type:4; // 16
  #if UINTPTR_MAX == 0xffffffffffffffffULL
    size_t alignment:48;
  #else
    size_t alignment:16;
  #endif
  struct {
    Method* static_methods;     // Static methods
    Method* virtual_methods;    // virtual methods
    HashTable* runtime_methods; // Runtime methods
  } vtable;
  size_t size;
  union {
    struct {
      TypeField* fields;
      invariant_kind_t (*invariant)(TypeInfo* type, void* self, ... );
      Trait* traits;
      TypeInfo* derived_from;
    } raw_type;                        // T
    struct {
      TypeInfo* raw_type;
    } ptr_type;                        // T*
    struct {
      TypeInfo* raw_type;
    } ref_type;                        // T&
    struct {
      TypeInfo* raw_type;
      Lifetime* lifetime;
      const char* name;
    } lifetime_ref_type;               // T&'a
    struct {
      TypeInfo* raw_type;
      size_t size;
    } array_type;                      // [T] or [T; N]
    struct {
      TypeInfo* raw_type;
      size_t size;
    } soa_type;                        // []T
    struct {
      TypeInfo* raw_type;
      size_t from;
      size_t to;
    } slice_type;                      // [T:] or [T: a..b]
    struct {
      TypeInfo* key_type;
      TypeInfo* value_type;
    } dict_type;                       // {T: Y}
    struct {
      TypeInfo* key_type;
    } set_type;                        // {T}
    struct {
      const char** field_names;
      TypeInfo* value_type;
      size_t size;
    } tuple_st_type;                   // (T, ) or (T, T) or (a: T, b: T) or (T; N) or T * T or T * N
    struct {
      const char** field_names;
      TypeInfo** value_types;
      size_t size;
    } tuple_mt_type;                   // (T, Y) or (a: T, b: Y) or T * Y
    struct {
      TypeInfo* raw_type_1;
      TypeInfo* raw_type_2;
    } intersection_type;               // T & Y
    struct {
      TypeInfo* raw_type_1;
      TypeInfo* raw_type_2;
    } union_type;                      // T | Y
    struct {
      TypeInfo* raw_type_1;
      TypeInfo* raw_type_2;
    } enum_type;                       // T ^ Y
    struct {
      Function* function;
    } function_type;                   // fn f(a: T, b: Y) -> X
    struct {
      size_t m_vararg:1;
      size_t m_varret:1;
      size_t m_constexpr:1;
      size_t m_generic:1; // 8
      #if UINTPTR_MAX == 0xffffffffffffffffULL
        size_t arg_cnt:60;
      #else
        size_t arg_cnt:28;
      #endif
      union {
        Variable* captures;
        TypeInfo* args;
      }* arg_types;
      TypeInfo* return_type;
      ResultType (*impl_function)(TypeInfo* type, void* self, ... );
    } lambda_type;                     // |x: T| -> ... or |x: T|[some_var_to_capture] -> ...
  };
  TypeInfo** generic_arguments;        // NULL if not generic, *NULL if generic but empty
  void* default_value;
} TypeInfo;


const int a = sizeof(TypeInfo);


typedef struct SourceLoc {
  const char* file;
  uint32_t line;
  uint32_t column;
} SourceLoc;


typedef struct AstNode {
  enum {
    ASTN_DECL_IDENTIFIER=1<<16,
    ASTN_DECL_VAR=1<<17,
    ASTN_ERROR=0,
    ASTN_DECL_TYPE,
    ASTN_DECL_FUNCTION,
    ASTN_DECL_STRUCT,
    ASTN_DECL_CLASS,
    ASTN_DECL_ENUM,
    ASTN_DECL_UNION,
    ASTN_DECL_GENERIC_FUNCTION,
    ASTN_DECL_GENERIC_STRUCT,
    ASTN_DECL_GENERIC_CLASS,
    ASTN_DECL_GENERIC_ENUM,
    ASTN_DECL_GENERIC_UNION,
    ASTN_DECL_TRAIT,
    ASTN_DECL_MOD,
    ASTN_DECL_IMPL,
    ASTN_DECL_MACRO,
    ASTN_DECL_EXTERN,
    ASTN_DECL_GLOBAL,
    ASTN_DECL_LOCAL_MUT,
    ASTN_DECL_LOCAL_CONST,
    ASTN_DECL_CONSTEXPR,
    ASTN_LAMBDA_LITERAL,
    ASTN_TYPE_LITERAL,
    ASTN_COMPOUND_LITERAL,
    ASTN_STRING_LITERAL,
    ASTN_INT_LITERAL,
    ASTN_FLOAT_LITERAL,
    ASTN_TUPLE_LITERAL,
    ASTN_SET_LITERAL,
    ASTN_DICT_LITERAL,
    ASTN_ARRAY_LITERAL,
    ASTN_SLICE_LITERAL,
    ASTN_SOA_LITERAL,

  } node_type;
  SourceLoc location;
  union {
    struct {
      TypeInfo* typeinfo;

    } decltype;
  } node;
} AstNode;

