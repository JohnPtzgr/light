#include "inttypes.h"
#include "float.h"
#include "math.h"
#include "errno.h"
#include "../../../../../extraslib/src/oop/exception.h"
#include "string.h"
// #include "stdint-gcc.h"

#ifdef __DEC32_MIN__
  typedef _Decimal32 dec32;
#endif

#ifdef __DEC64_MIN__
  typedef _Decimal64 dec64;
#endif

#ifdef __DEC128_MIN__
  typedef _Decimal128 dec128;
#endif

#define fn
#define global
#define var
#define fnDeferSection
#define noExcept
#define noThrow
#define unsafe
#define try if (!(errno = 0)) { \
  exception_t caught_exception; \
  do
#define except(ARG...) while(0); \
                       if (errno && (!((size_t){(size_t)(ARG)}) \
                           || (((exception_t*)ARG)->exception_number == \
                               errno) || \
                              (((exception_t*)ARG)->exception_number == \
                               errexc.exception_number))) { \
    caught_exception = errexc; \
    errexc.exception_number = 0; \
    errno = 0; \

#define finally caught_exception = (exception_t){}; }}
#define errBreak if (errno) { break; }
#define errBreakSet(REXPR, LEXPR...) REXPR = LEXPR; \
    if (errno) { \
      memcpy(&(REXPR), \
             &(size_t){(size_t)(INVALID)}, \
             MIN(sizeof(REXPR), \
                 sizeof(size_t))); \
      break; \
    }
#define raise(ARG...) (errno = (errexc = ARG).exception_number)
#define throw(FN_NAME, ARG...) raise(ARG); goto FN_NAME##_return;

#define DATA(ARGS...) ARGS;
#define DECL(ARGS...) ARGS;
#define TYPE_DECL(ARGS...) ARGS;


typedef unsigned char nil_t;
enum {
  nil
} nil_enum;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#ifdef __SIZEOF_INT128__
typedef __int128_t i128;
typedef __uint128_t u128;
#endif


#if __FLT16_MAX_EXP__
typedef _Float16 f16;
#else
typedef __fp16 f16;
#endif
#if __HAVE_FLOAT32
  #if __SIZEOF_FLOAT__ * __CHAR_BIT__ == 32
  typedef float f32;
  typedef float num24;
  #elif __SIZEOF_DOUBLE__ * __CHAR_BIT__ == 32
  typedef double f32;
  typedef double num24;
  #endif
#endif
#if __HAVE_FLOAT64
  #if __SIZEOF_FLOAT__ * __CHAR_BIT__ == 64
  typedef float f64;
  typedef float num54;
  #elif __SIZEOF_DOUBLE__ * __CHAR_BIT__ == 64
  typedef double f64;
  typedef double num53;
  #elif __SIZEOF_LONG_DOUBLE__ * __CHAR_BIT__ == 64
  typedef long double f64;
  typedef long double num53;
  #endif
#endif
#if __LDBL_MANT_DIG__ == 64
typedef long double f80;
typedef long double num64;
#endif
#if __HAVE_FLOAT128
typedef __float128 f128;
typedef __float128 num113;
#endif

#if __BFLT8_MAX_EXP__
typedef __bf8 bf8;
#else
typedef struct { union {uint8_t bitfield[1]; } data} bf8;
#endif

#if __BFLT16_MAX_EXP__
typedef __bf16 bf16;
#endif

typedef long double num;
typedef float _Complex cmplxf32;
typedef double _Complex cmplxf64;
typedef long double _Complex cmplxf80;
