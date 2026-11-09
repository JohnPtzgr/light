#include "./hashtable.h"
#include "time.h"
#include "stdlib.h"
#include "stdio.h"
#include "stdint.h"
#include "string.h"


static inline hash128_t default_t_hasher(const char* key, size_t len) {
  // return (TableHash){.hashX=xxh32(key, len, 0), .hashY=xxh32(key, len, 1)};
  return (hash128_t){.as_int.dble[0]=fast_hash(key, 0), .as_int.dble[1]=fast_hash(key, 1)};
  // return test_hash(key, len);
}

#define ScalarFX 50

int main() {

  // const size_t ax = 1;
  // const size_t bx = 5000000;
  const size_t sc = 200000;
  const size_t default_size = (1llu << 6);

  HashTable t;
  clock_t time_s;
  clock_t time_f;
  size_t count_overwrites = 0;
  const size_t ops = sc * ScalarFX;
  char* str = (char*)malloc(32 * sc * ScalarFX);
  size_t rnd = 1;
  for (size_t i = 0; i < 32 * sc * ScalarFX; ++i) {
    rnd <<= 2;
    rnd ^= (rnd >> 33);
    rnd = (rnd * 147307587625942813) ^ 428565993458070227;
    rnd ^= (rnd >> 33);
    str[i] = 1 + (rnd & 254);
  }
  rnd = 1;
  for (size_t i = 0; i < ops; ++i) {
    // rnd = (rnd * 428565993458070227 * (str[i * 16] + 1)) % 147307587625942813;
    // str[i * 16 + (rnd & 15)] = 1 + (rnd & 127);
    str[i * 16 + 15] = '\0';
  }
  htable_init(&t, default_size, (hasher_t)default_t_hasher);
  printf("init\n");
  time_s = clock();
  for (size_t i = 0; i < ops; ++i) {
    // printf("%d = \"%s\" = %d(X) = %d(Y)\n", i, str + 16 * i, defaultVTableHasher(str + 16 * i, 0), defaultVTableHasher(str + 16 * i, 1));
    HashData* insert_res = htable_insert(&t, str + 16 * i, 16, (void*)i);
    if (!insert_res) {
      printf("%d - Critical Error!\n", i);
      free(str);
      htable_destroy(&t);
      exit(0);
    }
    // if (i == 3358787) {
    //   printf("Insert result(3358787) is %u\n", insert_res);
    // }
    // if (!__vTable_check__(&t, str + 16 * i)) {
    //   printf("%d - Critical Error!\n", i);
    //   free(str);
    //   __vTable_destroy__(&t);
    //   exit(0);
    // }
  }
  time_f = clock();
  printf("Alpfa: %f\n", htable_loadf(&t));
  printf("Time diff - %d ms\n", (time_f - time_s) * 1000 / CLOCKS_PER_SEC);
  volatile char *ptr = (char*)t.data;
  time_s = clock();
  for (int i = 0; i < (ops / 4096); i++) {
      (void)*ptr;  // Just touch memory
      ptr += 4096; // Skip to next page
  }
  time_f = clock();
  printf("Time diff - %d ms\n", (time_f - time_s) * 1000 / CLOCKS_PER_SEC);
  // Stay within the same page (no TLB misses)
  volatile HashGroupData* ptr3 = (HashGroupData*)t.data;
  char sum3 = 0;
  time_s = clock();
  for (int i = 0; i < ops; i++) {
    sum3 += (char)(size_t)ptr3[(i >> 2) & 0xFFF].data[i & 2].value;  // Within same 4KB page
  }
  time_f = clock();
  printf("Time diff - %d ms\n", (time_f - time_s) * 1000 / CLOCKS_PER_SEC);
  printf("Sum: %d (prevent optimization)\n", sum3);
  time_s = clock();
  rnd = 1;
  volatile int a = 0;
  volatile int b = 1;
  for (size_t i = 0; i < ops; ++i) {
    for (size_t j = 0; j < 1; j++) {
      if (!htable_get(&t, str + 16 * i, 16)) {
        printf("%d - Critical Error!\n", i);
        free(str);
        htable_destroy(&t);
        exit(0);
      } else {
        a += i;
      }
      if (b) {
        b += a + 1;
      }
    }
  }
  time_f = clock();
  printf("Alpha: %f\n", htable_loadf(&t));
  printf("Time diff - %d ms\n", (time_f - time_s) * 1000 / CLOCKS_PER_SEC);
  time_s = clock();
  rnd = 1;
  a = 0;
  b = 1;
  for (size_t i = 0; i < ops; ++i) {
    for (size_t j = 0; j < 100; j++) {
      if (!htable_get(&t, str + 16 * i, 16)) {
        printf("%d - Critical Error!\n", i);
        free(str);
        htable_destroy(&t);
        exit(0);
      } else {
        a += i;
      }
      if (b) {
        b += a + 1;
      }
    }
  }
  time_f = clock();
  printf("Time diff - %d ms\n", (time_f - time_s) * 1000 / CLOCKS_PER_SEC);
  time_s = clock();
  rnd = 1;
  a = 0;
  b = 1;
  for (size_t i = 0; i < ops; ++i) {
    for (size_t j = 0; j < 1000; j++) {
      if (!htable_get(&t, str + 16 * i, 16)) {
        printf("%d - Critical Error!\n", i);
        free(str);
        htable_destroy(&t);
        exit(0);
      } else {
        a += i;
      }
      if (b) {
        b += a + 1;
      }
    }
  }
  time_f = clock();
  printf("Time diff - %d ms\n", (time_f - time_s) * 1000 / CLOCKS_PER_SEC);
  // for (size_t i = 0; i < t.size; ++i) {
  //   printf("%d - %d - %d - %d\n", t.data[i].data, t.data[i].mask, t.data[i].hash.hashX, t.data[i].hash.hashY);
  // }
  time_s = clock();
  rnd = 1;
  a = 0;
  b = 1;
  for (size_t i = 0; i < ops; ++i) {
    for (size_t j = 0; j < 1; j++) {
      if (htable_get(&t, str + 16 * (i) + 1, 16)) {
        a++;
      }
    }
  }
  time_f = clock();
  printf("Time diff - %d ms with %d False+\n", (time_f - time_s) * 1000 / CLOCKS_PER_SEC, a);
  free(str);
  printf("destroy\n");
  htable_destroy(&t);
  return 0;
}