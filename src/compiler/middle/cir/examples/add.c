#include "../cir.h"
#include "stdio.h"
#include "sys/time.h"

TYPE_DECL(

)

DECL(
  global int y;
  fn i16 add_i16(i16 arg1, i16 arg2);
  fn num add_num(num arg1, num arg2);
  fn num24 add_num24(num24 arg1, num24 arg2);
  fn void inc_y();
  fn i16 n_fib16(i8 arg);
)

DATA(
  int y = 0;
)

noExcept noThrow fn i16 add_i16(i16 arg1, i16 arg2) {
  return arg1 + arg2;
}

noExcept noThrow fn num add_num(num arg1, num arg2) {
  return arg1 + arg2;
}

noExcept noThrow fn num24 add_num24(num24 arg1, num24 arg2) {
  return arg1 + arg2;
}

noExcept noThrow fn void inc_y() {
  global y;
  y++;
}

noExcept noThrow fn i16 n_fib16(i8 arg) {
  i16 n_e = 1;
  i16 n_o = 1;
  for (size_t i = 0; i < arg; ++i) {
    n_e = n_e + n_o;
    n_o = n_e - n_o;
  }
  return n_o;
}

static i64 anySidEffect = 0;

noExcept noThrow fn i32 iter_fact32(i8 depth) {
  // for (size_t i = 0; i < depth; ++i) {
  //   anySidEffect += 1;
  // }
  if (depth <= 0) {
    if (depth == 0) {
      return 1;
    }
    return -1;
  }
  i32 result = 1;
  for (size_t i = 2; i <= depth; ++i) {
    result *= i;
  }
  return result;
}

noExcept noThrow fn i32 lin_fact32(i8 depth) {
  // for (size_t i = 0; i < 1; ++i) {
  //   anySidEffect += 1;
  // }
  if (depth <= 0) {
    if (depth == 0) {
      return 1;
    }
    return -1;
  }
  return depth * lin_fact32(depth - 1);
}

noThrow fn i32 non_linear_fact32(i8 depth) {
  i32 non_linear_fact32_result_i32 = (i32)((size_t)INVALID);
  // for (size_t i = 0; i < 1; ++i) {
  //   anySidEffect += 1;
  // }
  try {
    if (depth < 1) {
      raise((exception_t){.exception_number=EDOM});
      // errBreakSet(non_linear_call_result_i32, 0);
      break;
    }
    non_linear_fact32_result_i32 = depth * non_linear_fact32(depth - 1);
    // errBreakSet(non_linear_call_result_i32, depth * non_linear_fact32(depth - 1));
  } except(NULL) {
    // printf("Except: %d! = %d. MWE: %d\n",
    //        depth,
    //        non_linear_fact32_result_i32,
    //        errexc.exception_number);
    non_linear_fact32_result_i32 = 1;
  } finally {
    // printf("Finally: %d! = %d. MWE: %d\n",
    //        depth,
    //        non_linear_fact32_result_i32,
    //        errexc.exception_number);
    // goto non_linear_fact_return;
  }
  non_linear_fact32_return:
  fnDeferSection  {
    // printf("Defer: %d! = %d. MWE: %d\n",
    //         depth,
    //         non_linear_fact32_result_i32,
    //         errexc.exception_number);
  }
  return non_linear_fact32_result_i32;
}

fn i32 throw_fact32(i8 depth) {
  i32 throw_fact32_result_i32 = (i32)((size_t)INVALID);
  // for (size_t i = 0; i < 1; ++i) {
  //   anySidEffect += 1;
  // }
  if (depth < 0) {
    throw(throw_fact32, (exception_t){.exception_number=EDOM});
  }
  try {
    throw_fact32_result_i32 = depth * throw_fact32(depth - 1);
    // errBreakSet(throw_fact32_result_i32, depth * throw_fact32(depth - 1));
  } except(NULL) {
    // printf("Except: %d! = %d. MWE: %d\n",
    //        depth,
    //        throw_fact32_result_i32,
    //        errexc.exception_number);
    throw_fact32_result_i32 = 1;
  } finally {
    // printf("Finally: %d! = %d. MWE: %d\n",
    //        depth,
    //        throw_fact32_result_i32,
    //        errexc.exception_number);
  }
  throw_fact32_return:
  fnDeferSection  {
    // printf("Defer: %d! = %d. MWE: %d\n",
    //         depth,
    //         throw_fact32_result_i32,
    //         errexc.exception_number);
  }
  return throw_fact32_result_i32;
}

int kernel_main_init() {
  exception_t k;
  sizeof(exception_stack_t);
  k.context->data;
  return 0;
}

int kernel_main_destroy() {
  return 0;
}

int main() {
  dec32 a = 0;
  kernel_main_init();
  i16 add_i16_r = add_i16(12, 423);
  i16 n_fib16_r = n_fib16(6);
  u32 n_nlfact32_r = non_linear_fact32(32);
  u32 n_thfact32_r = throw_fact32(32);
  printf("%d - %d\n", n_nlfact32_r, n_thfact32_r);
  i32 cnt_ = 0;
  size_t iters = 100000;
  struct timeval time_s;
  struct timeval time_f;
  
  printf("ITER START\n");
  gettimeofday(&time_s, NULL);
  for (size_t i = 0; i < iters; ++i) {
    cnt_ += iter_fact32(120);
  }
  gettimeofday(&time_f, NULL);
  printf("ITER FINISH: %llu\n", (1000000llu * (time_f.tv_sec - time_s.tv_sec) + time_f.tv_usec - time_s.tv_usec));
  printf("LIN START\n");
  gettimeofday(&time_s, NULL);
  for (size_t i = 0; i < iters; ++i) {
    cnt_ += lin_fact32(120);
  }
  gettimeofday(&time_f, NULL);
  printf("LIN FINISH: %llu\n", (1000000llu * (time_f.tv_sec - time_s.tv_sec) + time_f.tv_usec - time_s.tv_usec));
  printf("NON-LIN START\n");
  gettimeofday(&time_s, NULL);
  for (size_t i = 0; i < iters; ++i) {
    cnt_ += non_linear_fact32(120);
  }
  gettimeofday(&time_f, NULL);
  printf("NON-LIN FINISH: %llu\n", (1000000llu * (time_f.tv_sec - time_s.tv_sec) + time_f.tv_usec - time_s.tv_usec));
  printf("THROW START\n");
  gettimeofday(&time_s, NULL);
  for (size_t i = 0; i < iters; ++i) {
    cnt_ += throw_fact32(120);
  }
  gettimeofday(&time_f, NULL);
  printf("THROW FINISH: %llu\n", (1000000llu * (time_f.tv_sec - time_s.tv_sec) + time_f.tv_usec - time_s.tv_usec));
  printf("A + B is %d\n", add_i16_r);
  printf("Y is %d\n", y);
  inc_y();
  printf("Y is %d\n", y);
  printf("N-fib is %d\n", n_fib16_r);
  printf("NL: N! is %u\n", n_nlfact32_r);
  printf("TH: N! is %u\n", n_thfact32_r);
  printf("%d\n", anySidEffect);
  kernel_main_destroy();
  return 0;
}
