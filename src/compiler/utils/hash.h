#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

typedef union {
  union {
    uint64_t sngl[1];
    uint32_t dble[2];
    uint16_t quad[4];
    uint8_t oct[8];
  } as_int; 
  union {
  } as_hash;
} hash64_t;


typedef union {
  union {
    __uint128_t sngl[1];
    uint64_t dble[2];
    uint32_t quad[4];
    uint16_t oct[8];
    uint8_t hex[16];
  } as_int;
  union {
    struct {
      hash64_t primary;
      hash64_t secondary;
    } single[1];
    hash64_t dble[2];
  } as_hash;
} hash128_t;


typedef hash128_t(*hasher_t)(void const* key, size_t key_len);


hash128_t fastihasher(uint64_t key, size_t len);

uint32_t xxh32(const char* key, size_t len, uint32_t seed);

// Your interface
size_t xxh_hasher(const char* key, char id);

size_t fast_hash(const char* key, char id);

hash128_t test_hash(const void* key, size_t len);


