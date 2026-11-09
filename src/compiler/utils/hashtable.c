#include "./hashtable.h"
#include "stdio.h"

// #define MAX(A, B) ((A > B) ? A : B)
#define MAX(A, B) (B)


typedef struct { uint8_t result; uint8_t checker; uint64_t idx; uint64_t id0; } htable_probe_t;


static inline bloom_filter_t htable_bloom_get(hash64_t hash) {
  return (bloom_filter_t){.sngl={(hash.as_int.quad[0] ^ hash.as_int.quad[3]) << (hash.as_int.quad[1] >> 4)}};
}

// static inline bloom_filter_t htable_bloom_get(hash64_t hash) {
//   return (bloom_filter_t){.sngl={0}};
// }


static inline htable_probe_t htable_probe_int(HashGroup* groups, key128_t key, size_t group_num) {
  size_t id0 = (key.hashkey.primary_hash.as_int.sngl[0]) & group_num;
  // size_t id0 = (hash.hashX^(-hash.hashY)) & (table->size - 1);
  size_t cnt = -1;
  do { cnt++; } while ((cnt < 62) && ((groups[(id0 + cnt) & group_num].mask.sngl.upper & 3) == 3));
  if (__glibc_unlikely(cnt >= 62)) {
    return (htable_probe_t){};
  }
  return (htable_probe_t){.result = cnt, .checker = cnt + 1, .idx = (id0 + cnt) & group_num, .id0 = id0};
}


static inline size_t htable_insert_at_int(HashTable* table,
                                          HashGroup* groups,
                                          HashGroupData* group_data,
                                          key128_t key,
                                          HashData data,
                                          htable_probe_t probe) {
  size_t id = groups[probe.idx].mask.sngl.upper++ & 3;
  groups[probe.idx].keys[id] = key;
  // groups[probe.idx].mask.sngl.filter.sngl[0] |= htable_bloom_get(key.hashkey.primary_hash).sngl[0];
  groups[probe.id0].mask.sngl.upper |= (4llu << probe.result);
  // groups[probe.id0].mask.sngl.lower |= (1u << MAX(0, probe.result - 62u));
  group_data[probe.idx].data[id] = data;
  return 0;
}


static inline size_t htable_reserve_int(HashTable* table, HashGroup* groups, HashGroupData* data, size_t new_capacity) {
  table->groups = (HashGroup*)calloc(new_capacity, sizeof(HashGroup));
  table->data = (HashGroupData*)calloc(new_capacity, sizeof(HashGroupData));
  if (!groups || !data) {
    if (groups) {
      free(groups);
    }
    if (data) {
      free(data);
    }
    return new_capacity;
  }
  printf("WRNG: Reserve! Alpha: %f, Expected new capacity: %d\n", htable_loadf(table), new_capacity);
  htable_probe_t probe = (htable_probe_t){};
  size_t group_num = table->capacity;
  for (size_t i = 0; i <= group_num; ++i) {
    for (size_t j = 0; j < (groups[i].mask.sngl.upper & 3); ++j) {
      probe = htable_probe_int(table->groups, groups[i].keys[j], new_capacity - 1);
      if (__glibc_unlikely(!probe.checker)) {
        // printf("Reserve: %d, %d, GSIZE: %d, KEY: %d/%d\n", i, j, groups[i].mask.sngl.upper & 3, groups[i].keys[j].fullhash.as_int.dble[0], groups[i].keys[j].fullhash.as_int.dble[1]);
        free(table->groups);
        free(table->data);
        return htable_reserve_int(table, groups, data, new_capacity * 2);
      }
      htable_insert_at_int(table,
                           table->groups,
                           table->data,
                           groups[i].keys[j],
                           data[i].data[j],
                           probe);
    }
  }
  free(groups);
  free(data);
  return new_capacity;
}


HashTable* htable_init(HashTable* table, size_t init_capacity, hasher_t hasher) {
  table->capacity = htable_reserve_int(table, NULL, NULL, init_capacity) - 1;
  table->hasher = hasher;
  table->size = 0;
  return table;
}


void htable_destroy(HashTable* table) {
  if (table->groups) {
    free(table->groups);
  }
  if (table->data) {
    free(table->data);
  }
}


HashData* htable_insert(HashTable* table, void const* key, size_t key_len, void* data) {
  key128_t hkey = (key128_t){.fullhash=table->hasher(key, key_len)};
  htable_probe_t probe = htable_probe_int(table->groups, hkey, table->capacity);
  // printf("INS PROBE: %d, %d, %d, %d\n", probe.checker, probe.id0, probe.result, probe.idx);
  if (__glibc_unlikely(!probe.checker)) {
    // printf("Insert FAil! HASH %d \n", hkey.hashkey.primary_hash.as_int.sngl[0]);
    table->capacity = htable_reserve_int(table, table->groups, table->data, 2 * table->capacity + 2) - 1;
    probe = htable_probe_int(table->groups, hkey, table->capacity);
  }
  HashGroup* groups = table->groups;
  HashGroupData* group_data = table->data;
  size_t id = groups[probe.idx].mask.sngl.upper++ & 3;
  // if (id > 2) {
  //   printf("Insert FAil! ID: %d\n", id);
  // }
  groups[probe.idx].keys[id] = hkey;
  // groups[probe.idx].mask.sngl.filter.sngl[0] |= htable_bloom_get(hkey.hashkey.primary_hash).sngl[0];
  groups[probe.id0].mask.sngl.upper |= (4llu << probe.result);
  // groups[probe.id0].mask.sngl.lower |= (1u << MAX(0, probe.result - 62u));
  group_data[probe.idx].data[id] = (HashData){.key = key, .value = data};
  table->size++;
  return &group_data[probe.idx].data[id];
}


static inline HashData htable_delete_int(HashTable* table, key128_t key) {
  htable_probe_t probe = htable_probe_int(table->groups, key, table->capacity);
  size_t idx = 0;
  table->groups[probe.idx].mask.sngl.filter.sngl[0] = 0;
  for (size_t i = 0; i < (table->groups[probe.idx].mask.sngl.upper & 3); ++i) {
    if (!memcmp(&table->groups[probe.idx].keys[i], &key, sizeof(key128_t))) {
      idx = i;
    } else {
      table->groups[probe.idx].mask.sngl.filter.sngl[0] |= htable_bloom_get(table->groups[probe.idx].keys[i].hashkey.primary_hash).sngl[0];
    }
  }
  HashData data = table->data[probe.idx].data[idx];
  table->groups[probe.idx].mask.sngl.upper--;
  table->groups[probe.idx].keys[idx] = table->groups[probe.idx].keys[table->groups[probe.idx].mask.sngl.upper & 3];
  table->data[probe.idx].data[idx] = table->data[probe.idx].data[table->groups[probe.idx].mask.sngl.upper & 3];
  return data;
}


HashData* htable_insert_at(HashTable* table, void const* key, size_t key_len, void* data, uint8_t group_chain_index) {
  key128_t hkey = (key128_t){.fullhash=table->hasher(key, key_len)};
  size_t id0 = (hkey.hashkey.primary_hash.as_int.sngl[0]) & table->capacity;
  size_t idx = (id0 + group_chain_index) & table->capacity;
  size_t id = table->groups[idx].mask.sngl.upper & 3;
  if (id < 3) {
    table->groups[idx].mask.sngl.upper++;
    table->size++;
  } else {
    id = 2;
    key128_t old_key = table->groups[idx].keys[id];
    HashData old_data = htable_delete_int(table, old_key);
    table->groups[idx].mask.sngl.upper++;
    htable_probe_t old_probe = htable_probe_int(table->groups, old_key, table->capacity);
    htable_insert_at_int(table, table->groups, table->data, old_key, old_data, old_probe);
  }
  HashGroup* groups = table->groups;
  HashGroupData* group_data = table->data;
  groups[idx].keys[id] = hkey;
  groups[idx].mask.sngl.filter.sngl[0] |= htable_bloom_get(hkey.hashkey.primary_hash).sngl[0];
  group_data[idx].data[id].value = data;
  groups[id0].mask.sngl.upper |= (4llu << group_chain_index);
  groups[id0].mask.sngl.lower |= (1u << MAX(0, group_chain_index - 62u));
  return &group_data[idx].data[id];
}


HashData* htable_get(HashTable* table, void const* key, size_t key_len) {
  
  HashGroup* t_groups = table->groups;
  key128_t hkey = (key128_t){.fullhash=table->hasher(key, key_len)};
  size_t id0 = (hkey.hashkey.primary_hash.as_int.sngl[0]) & table->capacity;
  size_t idx = id0;
  uint32_t filter = htable_bloom_get(hkey.hashkey.primary_hash).sngl[0];

  size_t mask = table->groups[id0].mask.sngl.upper >> 2;

  while (mask) {
    size_t primary_bit = mask & (-mask);
    mask ^= primary_bit;
    idx = (id0 + __builtin_ctzll(primary_bit)) & table->capacity;
    HashGroup* group_i = t_groups + idx;
    // if ((group_i->mask.sngl.filter.sngl[0] & filter) == filter) {
    //   for (size_t i = 0; i < (group_i->mask.sngl.upper & 3); ++i) {
    //     if (hkey.fullhash.as_int.sngl[0] == group_i->keys[i].fullhash.as_int.sngl[0]) {
    //       return &table->data[idx].data[i];
    //     }
    //   }
    // }
    for (size_t i = 0; i < (group_i->mask.sngl.upper & 3); ++i) {
      if (hkey.fullhash.as_int.sngl[0] == group_i->keys[i].fullhash.as_int.sngl[0]) {
        return &table->data[idx].data[i];
      }
    }
  }

  return NULL;
}

int htable_check(HashTable* table, void const* key, size_t key_len) {

  key128_t hkey = (key128_t){.fullhash=table->hasher(key, key_len)};
  size_t id0 = (hkey.hashkey.primary_hash.as_int.sngl[0]) & table->capacity;
  size_t idx = id0;
  uint32_t filter = htable_bloom_get(hkey.hashkey.primary_hash).sngl[0];

  size_t mask = table->groups[id0].mask.sngl.upper >> 2;

  while (mask) {
    size_t primary_bit = mask & (-mask);
    mask ^= primary_bit;
    idx = (id0 + __builtin_ctzll(primary_bit)) & table->capacity;
    HashGroup* group_i = &table->groups[idx];
    if (group_i->mask.sngl.filter.sngl[0] & filter == filter) {
      for (size_t i = 0; i < group_i->mask.sngl.upper & 3; ++i) {
        if (!memcmp(&hkey, &group_i->keys[i], sizeof(key128_t))) {
          return 1;
        }
      }
    }
  }

  return 0;
}

double htable_loadf(HashTable* table) {
  return ((double)table->size) / (3 * table->capacity + 3);
}

// int htable_join(HashTable* this, HashTable* other);
// int htable_extend(HashTable* table, size_t new_capacity);
// int htable_probe(HashTable* table, void const* key, size_t key_len);
// int htable_pushforward(HashTable* table, void const* key, size_t key_len);
// int htable_pushfront(HashTable* table, void const* key, size_t key_len);
// int htable_pushbackward(HashTable* table, void const* key, size_t key_len);
// int htable_pushback(HashTable* table, void const* key, size_t key_len);

