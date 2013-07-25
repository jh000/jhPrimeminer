/**
 * @file mpz.c
 *
 * @brief Multiple Precision arithmetic code.
 *
 * @author David Matlack (dmatlack)
 */
#ifndef __418_MPZ_H__
#define __418_MPZ_H__

#include "compile.h"
#include "cuda_string.h"
#include "digit.h"

#define MPZ_NEGATIVE      1
#define MPZ_NONNEGATIVE  0

typedef unsigned int uint32_t;

/** @brief struct used to represent multiple precision integers (Z). */
typedef struct __align__(8) {  
  uint32_t capacity;
  digit_t  digits[DIGITS_CAPACITY];
  uint32_t sign;
} mpz_t;

/*struct mpz_t __align__(8) {
  unsigned int capacity;
  digit_t  digits[DIGITS_CAPACITY];
  char     sign;
};*/

__device__ __host__ inline char* mpz_get_str(mpz_t *mpz, char *str, int bufsize);

/**
 * @brief Check that the mpz_t struct has enough memory to store __capacity
 * digits.
 */
#ifndef __CUDACC__
#define CHECK_MEM(__mpz, __capacity) \
  do {                                                                  \
    if ((__mpz)->capacity < (__capacity)) {                             \
      printf("MPZ memory error at %s:%d.\n", __func__, __LINE__);       \
      printf("\tmpz capacity: %u, requested capacity %u\n",             \
             (__mpz)->capacity, (__capacity));                          \
      assert(0);                                                        \
    }                                                                   \
  } while (0)
#else
#define CHECK_MEM(__mpz, __capacity)
#endif

/**
 * @brief Sanity check the sign.
 */
#ifndef __CUDACC__
#define CHECK_SIGN(__mpz) \
  do {                                                                  \
    if (digits_is_zero((__mpz)->digits, (__mpz)->capacity) &&           \
        (__mpz)->sign != MPZ_NONNEGATIVE) {                             \
      printf("MPZ Sign Error at %s:%d: Value is 0 but sign is %d.\n",   \
             __func__, __LINE__, (__mpz)->sign);                        \
      assert(0);\
    }                                                                   \
    if ((__mpz)->sign != MPZ_NEGATIVE &&                                \
        (__mpz)->sign != MPZ_NONNEGATIVE) {                             \
      printf("MPZ Sign Error at %s:%d: Invalid sign %d.\n",             \
             __func__, __LINE__, (__mpz)->sign);                        \
      assert(0);\
    }                                                                   \
  } while (0)
#else
#define CHECK_SIGN(__mpz)
#endif

/**
 * @brief Do some sanity checking on the mpz_t sign field.
 */
#ifndef __CUDACC__
#define CHECK_STRS(s1, s2)                                            \
  do {                                                                \
    if (strcmp(s1, s2)) {                                             \
      printf("Input string %s became %s!\n", s1, s2);                 \
    }                                                                 \
  } while (0)
#else
#define CHECK_STRS(s1, s2)
#endif


__device__ __host__ inline int mpz_is_negative(mpz_t *mpz) {
  return (mpz->sign == MPZ_NEGATIVE);
}

__device__ __host__ inline void mpz_negate(mpz_t *mpz) {
  mpz->sign = (mpz->sign == MPZ_NEGATIVE) ? MPZ_NONNEGATIVE : MPZ_NEGATIVE;
  if (digits_is_zero(mpz->digits, mpz->capacity)) {
    mpz->sign = MPZ_NONNEGATIVE;
  }
}

/**
 * @brief Initialize an mpz struct to 0.
 */
__device__ __host__ void mpz_init(mpz_t *mpz) {
  mpz->capacity = DIGITS_CAPACITY;
  digits_set_zero(mpz->digits); 
  mpz->sign = MPZ_NONNEGATIVE; 
}

/**
 * @brief Assign an mpz_t struct to the value of another mpz_t struct.
 */
__device__ __host__ inline void mpz_set(mpz_t *to, mpz_t *from) {
  unsigned int i;

  for (i = 0; i < to->capacity; i++) {
    digit_t d = (i < from->capacity) ? from->digits[i] : 0;
    to->digits[i] = d;
  }

  to->sign = from->sign;

  CHECK_SIGN(to);
  CHECK_SIGN(from);
}

/**
 * @brief Set the mpz integer to the provided integer.
 */
__device__ __host__ inline void mpz_set_i(mpz_t *mpz, int z) {
  mpz->sign = (z < 0) ? MPZ_NEGATIVE : MPZ_NONNEGATIVE;
  digits_set_lui(mpz->digits, abs(z));
}

/**
 * @brief Set the mpz integer to the provided integer.
 */
__device__ __host__ inline void mpz_set_lui(mpz_t *mpz, unsigned long z) {
  mpz->sign = MPZ_NONNEGATIVE;
  digits_set_lui(mpz->digits, z);
}

/**
 * @brief Set the mpz integer to the provided integer.
 */
__device__ __host__ inline void mpz_set_ui(mpz_t *mpz, unsigned int z) {
  mpz->sign = MPZ_NONNEGATIVE;
  digits_set_lui(mpz->digits, z);
}

/**
 * @brief Set the mpz integer based on the provided (hex) string.
 */
__device__ __host__ void mpz_set_str(mpz_t *mpz, char *user_str, unsigned int index) {
  int num_digits;
  int i;
  int is_zero;

  for (i = 0; i < mpz->capacity; i++) mpz->digits[i] = 0;

  char *str = user_str;

  /* Check if the provided number is negative */
  if (str[0] == '-') {
    mpz->sign = MPZ_NEGATIVE;
    str ++; // the number starts at the next character
  }
  else {
    mpz->sign = MPZ_NONNEGATIVE;
  }

  int len = cuda_strlen(str);
  int char_per_digit = LOG2_DIGIT_BASE / 4;
  num_digits = ((len - 1) / char_per_digit) +1;

  //if(index==0)
  //	printf("set_str: numdigits ==%i\n", num_digits );
  CHECK_MEM(mpz, num_digits);

  digits_set_zero(mpz->digits);

  is_zero = true;

  for (i = 0; i < num_digits; i ++) {
    str[len - i * char_per_digit] = (char) 0;

    int offset = (int) max(len - (i + 1) * char_per_digit, 0);
    char *start = str + offset;

    digit_t d = cuda_strtol(start, NULL, 16);

    //if(index==0)
//	printf("[0] digit %x from string %s offset %i\n", d, start,offset);

    /* keep track of whether or not every digit is zero */
    is_zero = is_zero && (d == 0);

    /* parse the string backwards (little endian order) */
    mpz->digits[i] = d;
  }

  /* Just in case the user gives us -0 as input */
  if (is_zero) mpz->sign = MPZ_NONNEGATIVE;

#if 0
  mpz_get_str(mpz, buf, bufsize);
  CHECK_STRS(user_str, buf);
#endif
}

__device__ __host__ inline void mpz_get_binary_str(mpz_t *mpz, char *str, unsigned int s) {
  (void) mpz;
  (void) str;
  (void) s;
}

/**
 * @brief Destroy the mpz_t struct.
 *
 * @deprecated
 */
__device__ __host__ inline void mpz_destroy(mpz_t *mpz) {
  (void) mpz;
}

/**
 * @brief Add two multiple precision integers.
 *
 *      dst := op1 + op2
 *
 * @warning It is assumed that all mpz_t parameters have been initialized.
 * @warning Assumes dst != op1 != op2
 */
__device__ __host__ inline void mpz_add(mpz_t *dst, mpz_t *op1, mpz_t *op2) {
#ifdef __CUDACC__
  unsigned int op1_digit_count = digits_count(op1->digits);
  unsigned int op2_digit_count = digits_count(op2->digits);

  /* In addition, if the operand with the most digits has D digits, then
   * the result of the addition will have at most D + 1 digits. */
  unsigned int capacity = max(op1_digit_count, op2_digit_count) + 1;

  /* Make sure all of the mpz structs have enough memory to hold all of
   * the digits. We will be doing 10's complement so everyone needs to
   * have enough digits. */
  CHECK_MEM(dst, capacity);
  CHECK_MEM(op1, capacity);
  CHECK_MEM(op2, capacity);
#endif

  digits_set_zero(dst->digits);

  /* If both are negative, treate them as positive and negate the result */
  if (mpz_is_negative(op1) && mpz_is_negative(op2)) {
    digits_add(dst->digits, dst->capacity,
               op1->digits, op1->capacity,
               op2->digits, op2->capacity);
    dst->sign = MPZ_NEGATIVE;
  }
  /* one or neither are negative */
  else {
    digit_t carry_out;

    /* Perform 10's complement on negative numbers before adding */
    if (mpz_is_negative(op1)) digits_complement(op1->digits, op1->capacity);
    if (mpz_is_negative(op2)) digits_complement(op2->digits, op2->capacity);

    carry_out = digits_add(dst->digits, dst->capacity,
                           op1->digits, op1->capacity,
                           op2->digits, op2->capacity);

    /* If there is no carryout, the result is negative */
    if (carry_out == 0 && (mpz_is_negative(op1) || mpz_is_negative(op2))) {
      digits_complement(dst->digits, dst->capacity);
      dst->sign = MPZ_NEGATIVE;
    }
    /* Otherwise, the result is non-negative */
    else {
      dst->sign = MPZ_NONNEGATIVE;
    }

    /* Undo the 10s complement after adding */
    if (mpz_is_negative(op1)) digits_complement(op1->digits, op1->capacity);
    if (mpz_is_negative(op2)) digits_complement(op2->digits, op2->capacity);
  }

  CHECK_SIGN(op1);
  CHECK_SIGN(op2);
  CHECK_SIGN(dst);
}

__device__ __host__ inline void mpz_addeq(mpz_t *op1, mpz_t *op2) {

  /* If both are negative, treate them as positive and negate the result */
  if (mpz_is_negative(op1) && mpz_is_negative(op2)) {
    digits_addeq(op1->digits, op1->capacity,
               op2->digits, op2->capacity);
    op1->sign = MPZ_NEGATIVE;
  }
  /* one or neither are negative */
  else {
    digit_t carry_out;

    /* Perform 10's complement on negative numbers before adding */
    if (mpz_is_negative(op1)) digits_complement(op1->digits, op1->capacity);
    if (mpz_is_negative(op2)) digits_complement(op2->digits, op2->capacity);

    carry_out = digits_addeq(op1->digits, op1->capacity,
                             op2->digits, op2->capacity);

    /* If there is no carryout, the result is negative */
    if (carry_out == 0 && (mpz_is_negative(op1) || mpz_is_negative(op2))) {
      digits_complement(op1->digits, op1->capacity);
      op1->sign = MPZ_NEGATIVE;
    }
    /* Otherwise, the result is non-negative */
    else {
      op1->sign = MPZ_NONNEGATIVE;
    }

    /* Undo the 10s complement after adding */
    if (mpz_is_negative(op2)) digits_complement(op2->digits, op2->capacity);
  }

  CHECK_SIGN(op1);
  CHECK_SIGN(op2);
}

/**
 * @brief Perform dst := op1 - op2.
 *
 * @warning Assumes that all mpz_t parameters have been initialized.
 * @warning Assumes dst != op1 != op2
 */
__device__ __host__ inline void mpz_sub(mpz_t *dst, mpz_t *op1, mpz_t *op2) {
  mpz_negate(op2);
  mpz_add(dst, op1, op2);
  mpz_negate(op2);
}

/**
 * @brief Perform op1 -= op2.
 *
 * @warning Assumes that all mpz_t parameters have been initialized.
 * @warning Assumes op1 != op2
 */
__device__ __host__ inline void mpz_subeq(mpz_t *op1, mpz_t *op2) {
  mpz_negate(op2);
  mpz_addeq(op1, op2);
  mpz_negate(op2);
}

/**
 * @brief Perform dst := op1 * op2.
 *
 * @warning Assumes that all mpz_t parameters have been initialized.
 * @warning Assumes dst != op1 != op2
 */
__device__ __host__ inline void mpz_mult(mpz_t *dst, mpz_t *op1, mpz_t *op2) {
  unsigned int op1_digit_count = digits_count(op1->digits);
  unsigned int op2_digit_count = digits_count(op2->digits);
  unsigned int capacity = max(op1_digit_count, op2_digit_count);

  /* In multiplication, if the operand with the most digits has D digits,
   * then the result of the addition will have at most 2D digits. */
  CHECK_MEM(dst, 2*capacity);
  CHECK_MEM(op1,   capacity);
  CHECK_MEM(op2,   capacity);

  /* Done by long_multiplication */
  /* digits_set_zero(dst->digits); */

  /* We pass in capacity as the number of digits rather that the actual
   * number of digits in each mpz_t struct. This is done because the
   * multiplication code has some assumptions and optimizations (e.g.
   * op1 and op2 to have the same number of digits) */
  digits_mult(dst->digits, op1->digits, op2->digits, capacity, dst->capacity);

  /* Compute the sign of the product */
  dst->sign = (op1->sign == op2->sign) ? MPZ_NONNEGATIVE : MPZ_NEGATIVE;
  if (MPZ_NEGATIVE == dst->sign && digits_is_zero(dst->digits, dst->capacity)) {
    dst->sign = MPZ_NONNEGATIVE;
  }

  CHECK_SIGN(op1);
  CHECK_SIGN(op2);
  CHECK_SIGN(dst);
}

/**
 * @return
 *      < 0  if a < b
 *      = 0  if a = b
 *      > 0  if a > b
 *
 * @warning This function does not give any indication about the distance
 * between a and b, just the relative distance (<, >, =).
 */
#define MPZ_LESS    -1
#define MPZ_GREATER  1
#define MPZ_EQUAL    0
__device__ __host__ inline int mpz_compare(mpz_t *a, mpz_t *b) {
  int cmp;
  int negative;

  if (MPZ_NEGATIVE == a->sign && MPZ_NONNEGATIVE == b->sign) return MPZ_LESS;
  if (MPZ_NEGATIVE == b->sign && MPZ_NONNEGATIVE == a->sign) return MPZ_GREATER;

  /* At this point we know they have the same sign */
  cmp = digits_compare(a->digits, a->capacity, b->digits, b->capacity);
  negative = mpz_is_negative(a);

  if (cmp == 0) return MPZ_EQUAL;

  if (negative) {
    return (cmp > 0) ? MPZ_LESS : MPZ_GREATER;
  }
  else {
    return (cmp < 0) ? MPZ_LESS : MPZ_GREATER;
  }
}

/** @brief Return true if a == b */
__device__ __host__ inline int mpz_equal(mpz_t *a, mpz_t *b) {
  return (mpz_compare(a, b) == 0);
}
/** @brief Return true if a == 1 */
__device__ __host__ inline int mpz_equal_one(mpz_t *a) {
  if (MPZ_NEGATIVE == a->sign) {
    return false;
  }
  return digits_equal_one(a->digits, a->capacity);
}
/** @brief Return true if a < b */
__device__ __host__ inline int mpz_lt(mpz_t *a, mpz_t *b) {
  return (mpz_compare(a, b) < 0);
}
/** @brief Return true if a <= b */
__device__ __host__ inline int mpz_lte(mpz_t *a, mpz_t *b) {
  return (mpz_compare(a, b) <= 0);
}
/** @brief Return true if a > b */
__device__ __host__ inline int mpz_gt(mpz_t *a, mpz_t *b) {
  return (mpz_compare(a, b) > 0);
}
/** @brief Return true if a == 1 */
__device__ __host__ inline int mpz_gt_one(mpz_t *a) {
  if (MPZ_NEGATIVE == a->sign) {
    return false;
  }
  return digits_gt_one(a->digits, a->capacity);
}
/** @brief Return true if a >= b */
__device__ __host__ inline int mpz_gte(mpz_t *a, mpz_t *b) {
  return (mpz_compare(a, b) >= 0);
}

/**
 * @brief Return the string representation of the integer represented by the
 * mpz_t struct.
 *
 * @warning If buf is NULL, the string is dynamically allocated and must
 * therefore be freed by the user.
 */
__device__ __host__ inline char* mpz_get_str(mpz_t *mpz, char *str, int bufsize) {
  int print_zeroes = 0; // don't print leading 0s
  int i;
  int str_index = 0;

  if (mpz_is_negative(mpz)) {
    str[0] = '-';
    str_index = 1;
  }

  for (i = mpz->capacity - 1; i >= 0; i--) {
    digit_t digit = mpz->digits[i];

    if (digit != 0 || print_zeroes) {
      if (bufsize < str_index + 8) {
        return NULL;
      }
      if (!print_zeroes) {
        str_index += sprintf(str + str_index, "%x", digit);
      }
      else {
        str_index += sprintf(str + str_index, "%08x", digit);
      }
      print_zeroes = 1;
    }
  }

  str[str_index] = (char) 0;

  /* the number is zero */
  if (print_zeroes == 0) {
    str[0] = '0';
    str[1] = (char) 0;
  }

  return str;
}

__device__ __host__ inline void mpz_print(mpz_t *mpz) {

  bool print_zeroes = false; // don't print leading 0s

  printf("mpz_capacity: %i ", mpz->capacity);
  for (int i = mpz->capacity - 1; i >= 0; i--) {

    if ( mpz->digits[i] != 0 || print_zeroes)
	{
	    print_zeroes = true;
	    printf("%08x", mpz->digits[i]);
	}
   }
   printf("\n");
//#ifndef __CUDACC__
  //char str[1024];
  //mpz_get_str(mpz, str, 1024);
  
//#endif
}

__device__ __host__ inline void mpz_set_bit(mpz_t *mpz, unsigned bit_offset,
                                            unsigned int bit) {
  digits_set_bit(mpz->digits, bit_offset, bit);

  if (MPZ_NEGATIVE == mpz->sign && bit == 0 &&
      digits_is_zero(mpz->digits, mpz->capacity)) {
    mpz->sign = MPZ_NONNEGATIVE;
  }
}

__device__ __host__ inline void mpz_bit_lshift(mpz_t *mpz) {
  bits_lshift(mpz->digits, mpz->capacity);

  if (MPZ_NEGATIVE == mpz->sign && digits_is_zero(mpz->digits, mpz->capacity)) {
    mpz->sign = MPZ_NONNEGATIVE;
  }
}

__device__ __host__ inline void mpz_div(mpz_t *q, mpz_t *r, mpz_t *n,
                                            mpz_t *d) {
  unsigned int n_digit_count = digits_count(n->digits);
  unsigned int num_bits;
  int i;
  int nsign = n->sign;
  int dsign = d->sign;

  num_bits = n_digit_count * LOG2_DIGIT_BASE;

  mpz_set_ui(q, 0);
  mpz_set_ui(r, 0);

  n->sign = MPZ_NONNEGATIVE;
  d->sign = MPZ_NONNEGATIVE;

  if (mpz_gt(n, d)) {

    for (i = num_bits - 1; i >= 0; i--) {
      unsigned int n_i;

      // r = r << 1
      mpz_bit_lshift(r);

      // r(0) = n(i)
      n_i = digits_bit_at(n->digits, i);
      mpz_set_bit(r, 0, n_i);

      // if (r >= d)
      if (mpz_gte(r, d)) {
        // r = r - d
        mpz_subeq(r, d);

        // q(i) = 1
        //printf("Setting bit %d of q to 1\n", i);
        //printf("\tBefore: "); mpz_print(q); printf("\n");
        mpz_set_bit(q, i, 1);
        //printf("\tAfter: "); mpz_print(q); printf("\n");
      }
    }

    /* Compute the sign of the division */
    q->sign = (nsign == dsign) ? MPZ_NONNEGATIVE : MPZ_NEGATIVE;
    if (MPZ_NEGATIVE == q->sign && digits_is_zero(q->digits, q->capacity)) {
      q->sign = MPZ_NONNEGATIVE;
    }
  }
  else {
    // quotient = 0
    mpz_set_ui(q, 0);
    // remainder = numerator
    mpz_set(r, n);
  }

  n->sign = nsign;
  d->sign = dsign;

  CHECK_SIGN(q);
  CHECK_SIGN(r);
  CHECK_SIGN(n);
  CHECK_SIGN(d);
}

/**
 * @brief Compute the GCD of op1 and op2.
 *
 * Euclidean Algorithm:
 *
 *    while (b != 0) {
 *      t := b
 *      b := a % b
 *      a := t
 *    }
 *    gcd = a
 */
__device__ __inline__ void mpz_gcd_tmp(mpz_t *gcd, mpz_t *op1, mpz_t *op2,
                                       // tmps
                                       mpz_t *tmp1, mpz_t *tmp2,
                                       mpz_t *tmp3) {
  mpz_t *a = gcd;
  mpz_t *b = tmp1;
  mpz_t *mod = tmp2;
  mpz_t *quo = tmp3;

  int compare = mpz_compare(op1, op2);

  mpz_set(a, (compare > 0) ? op1 : op2);
  mpz_set(b, (compare > 0) ? op2 : op1);

  while (!digits_is_zero(b->digits, b->capacity)) {
    mpz_div(quo, mod, a, b);
    mpz_set(a, b);
    mpz_set(b, mod);
  }
}

__device__ __inline__ void mpz_gcd(mpz_t *gcd, mpz_t *op1, mpz_t *op2) {
  mpz_t tmp1;
  mpz_t tmp2;
  mpz_t tmp3;

  mpz_init(&tmp1);
  mpz_init(&tmp2);
  mpz_init(&tmp3);

  mpz_gcd_tmp(gcd, op1, op2, &tmp1, &tmp2, &tmp3);
}

/**
 * Using exponentiation by squaring algorithm:
 *
 *  function modular_pow(base, exponent, modulus)
 *    result := 1
 *    while exponent > 0
 *      if (exponent mod 2 == 1):
 *         result := (result * base) mod modulus
 *      exponent := exponent >> 1
 *      base = (base * base) mod modulus
 *    return result
 */
__device__ __inline__ void mpz_powmod_tmp(mpz_t *result, mpz_t *base,
                                          mpz_t *exp, mpz_t *mod,
                                          // temps
                                          mpz_t *tmp1, mpz_t *tmp2,
                                          mpz_t *tmp3) {
  unsigned int iteration;

  mpz_t *b = tmp3;

  // result = 1
  mpz_set_ui(result, 1);

  // _base = base % mod
  mpz_set(tmp1, base);
  mpz_div(tmp2, b, tmp1, mod);

  iteration = 0;
  while (!bits_is_zero(exp->digits, exp->capacity, iteration)) {
    // if (binary_exp is odd)
    if (digits_bit_at(exp->digits, iteration) == 1) {
      // result = (result * base) % mod
      mpz_mult(tmp1, result, b);
      mpz_div(tmp2, result, tmp1, mod);
    }

    // binary_exp = binary_exp >> 1
    iteration++;

    // base = (base * base) % mod
    mpz_set(tmp1, b);
    mpz_mult(tmp2, b, tmp1);
    mpz_div(tmp1, b, tmp2, mod);
  }
}

__device__ __inline__ void mpz_powmod(mpz_t *result, mpz_t *base,
                                      mpz_t *exp, mpz_t *mod) {
  mpz_t tmp1;
  mpz_t tmp2;
  mpz_t tmp3;

  mpz_init(&tmp1);
  mpz_init(&tmp2);
  mpz_init(&tmp3);

  mpz_powmod_tmp(result, base, exp, mod, &tmp1, &tmp2, &tmp3);
}



__device__ __inline__ void mpz_pow(mpz_t *result, mpz_t *base, unsigned int exponent) {
  mpz_t tmp;
  unsigned int i;

  mpz_init(&tmp);

  // result = 1
  mpz_set_ui(result, 1);
  for (i = 0; i < exponent; i++) {
    // result *= base
    mpz_mult(&tmp, result, base);
    mpz_set(result, &tmp);
  }
}

/**
 * @brief Compute a += i
 */
__device__ __inline__ void mpz_addeq_i(mpz_t *a, int i) {
  if (0 == i) return;

  if (i < 0) {
    digits_complement(a->digits, a->capacity);
    digits_add_across(a->digits, a->capacity, -i);
    digits_complement(a->digits, a->capacity);
  }
  else {
    digits_add_across(a->digits, a->capacity, i);
  }
}

/**
 * @brief Compute result = a * i
 */
__device__ __inline__ void mpz_mult_u(mpz_t *result, mpz_t *a, unsigned int i) {
  digits_mult_u(result->digits, a->digits, i);

  result->sign = a->sign;
  if (0 == i) result->sign = MPZ_NONNEGATIVE;

  CHECK_SIGN(result);
}

#endif /* __418_MPZ_H__ */
