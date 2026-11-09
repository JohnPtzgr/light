#include <string.h>
#include "hash.h"


typedef union {
  struct {
    hash64_t primary_hash;
    void* primary_key;
  } hashkey;
  hash128_t fullhash;
} key128_t;


typedef union {
  uint32_t sngl[1];
  uint16_t dble[2];
  uint8_t quad[4];
} bloom_filter_t;


typedef union {
  uint8_t dodec[16];
  struct {
    uint64_t upper;
    uint32_t lower;
    bloom_filter_t filter;
  } sngl;
} probe_mask_t;


typedef struct {
  key128_t keys[3];
  probe_mask_t mask;
} HashGroup;


typedef struct {
  void* value;
  void const* key;
} HashData;


typedef struct {
  uint8_t meta[16];
} HashGroupMetaData;


typedef struct {
  HashData data[3];
  HashGroupMetaData meta;
} HashGroupData;


typedef struct {
  HashGroup* groups;
  HashGroupData* data;
  size_t capacity;
  size_t size;
  hasher_t hasher;
} HashTable;



extern HashTable* htable_init(HashTable* table, size_t init_capacity, hasher_t hasher);
void htable_destroy(HashTable* table);
HashData* htable_insert(HashTable* table, void const* key, size_t key_len, void* data);
HashData* htable_insert_at(HashTable* table, void const* key, size_t key_len, void* data, uint8_t group_chain_index);
HashData htable_delete(HashTable* table, void const* key, size_t key_len);
HashData* htable_get(HashTable* table, void const* key, size_t key_len);
int htable_check(HashTable* table, void const* key, size_t key_len);
double htable_loadf(HashTable* table);
// int htable_join(HashTable* this, HashTable* other);
// int htable_extend(HashTable* table, size_t new_capacity);
// int htable_probe(HashTable* table, void const* key, size_t key_len);
// int htable_pushforward(HashTable* table, void const* key, size_t key_len);
// int htable_pushfront(HashTable* table, void const* key, size_t key_len);
// int htable_pushbackward(HashTable* table, void const* key, size_t key_len);
// int htable_pushback(HashTable* table, void const* key, size_t key_len);
