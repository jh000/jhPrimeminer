/**
 * @file digit.c
 *
 * @brief Library of functions that operate on arrays of digits.
 *
 * Arrays of digits are assumed to be in little endian order.
 *
 * @author David Matlack (dmatlack)
 */
#ifndef __418_DIGIT_H__
#define __418_DIGIT_H__

#include "compile.h"

#define LOG2_DIGIT_BASE     32
#define DIGIT_BASE          ((unsigned long long) 1 << (LOG2_DIGIT_BASE))
#define DIGITS_CAPACITY     32

//typedef unsigned digit_t;

#define digit_t unsigned int

__device__ __host__ inline void digits_print(digit_t *digits,
                                            unsigned num_digits) {
  unsigned i;

  printf("{ ");
  for (i = 0; i < num_digits; i++) {
    printf("%x", digits[i]);
    if (i < num_digits - 1) printf(", ");
  }
  printf(" }");
}

/**
 * @brief Return true (non-zero) if all of the digits in the digits array
 * are zero (and thus the corresponding number is zero.
 */
__device__ __host__ inline int digits_is_zero(digit_t *digits,
                                       unsigned num_digits) {
  unsigned i;

  for (i = 0; i < num_digits; i++) {
    if (digits[i] != 0) return false;
  }
  return true;
}

__device__ __host__ inline int bit_at(digit_t d, unsigned index) {
  return (0x1 & (d >> index));
}

__device__ __host__ inline int digits_bit_at(digit_t *digits, unsigned bit_offset) {
  unsigned digit_index = bit_offset / LOG2_DIGIT_BASE;
  unsigned bit_index = bit_offset % LOG2_DIGIT_BASE;

  return bit_at(digits[digit_index], bit_index);
}

__device__ __host__ inline void digits_set_bit(digit_t *digits,
                                              unsigned bit_offset,
                                              unsigned bit) {
  unsigned digit_index = bit_offset / LOG2_DIGIT_BASE;
  unsigned bit_index = bit_offset % LOG2_DIGIT_BASE;
  unsigned bit_mask = 1 << bit_index;
  digit_t d = digits[digit_index];

  digits[digit_index] = (d & ~bit_mask) | (bit << bit_index);
}

__device__ __host__ inline int bits_is_zero(digit_t *digits,
                                              unsigned capacity,
                                              unsigned bit_offset) {
  unsigned i, j;
  unsigned bit_index = bit_offset % LOG2_DIGIT_BASE;

  for (i = bit_offset / LOG2_DIGIT_BASE; i < capacity; i++) {
    digit_t d = digits[i];

    /* If the digit index (i) is equal to the bit_offset (in other words,
     * this is the first digit we need to check), and the bit_index is
     * not 0 (the first bit we need to check is in the middle of the digit),
     * then we have to manually check each bit. */
    if (i == bit_offset && 0 != bit_index) {
      for (j = 0; j < LOG2_DIGIT_BASE; j++) {
        if (bit_at(d, j) != 0) return false;
      }
    }
    /* Otherwise we just have to check that the whole digit is zero (no need
     * to check each bit since we only care if ALL bits are 0 */
    else {
      if (d != 0) return false;
    }
  }

  return true;
}

__device__ __host__ inline void digits_set_zero(digit_t digits[DIGITS_CAPACITY]) {
  unsigned i;
  for (i = 0; i < DIGITS_CAPACITY; i++) digits[i] = 0;
}

__device__ __host__ inline void digits_set_lui(digit_t digits[DIGITS_CAPACITY],
                                        unsigned long z) {
  unsigned i;
  unsigned long long Z = (unsigned long long) z;

  i = 0;
  for (i = 0; i < DIGITS_CAPACITY; i++) {
    digits[i] = (digit_t) (Z % DIGIT_BASE);
    Z /= DIGIT_BASE; //FIXME
  }

}

__device__ __host__ inline void digits_set_ui(digit_t digits[DIGITS_CAPACITY],
                                        unsigned z) {
  digits[0] = z;
  unsigned i;
  for (i = 1; i < DIGITS_CAPACITY; i++) {
    digits[i] = (digit_t) 0;
  }

}

/**
 * @brief Count the number of digits in use in the digits array.
 *
 * E.g.
 *
 * digits = { 2, 0, 0, ..., 0 } represents 2 which is 1 digit
 * digits = { 0, 1, 5, 0, ..., 0 } represents 510 which is 3 digits
 *
 */
__device__ __host__ inline unsigned digits_count(digit_t digits[DIGITS_CAPACITY]) {
  int is_leading_zero = true;
  unsigned count = 0;
  int i;

  for (i = DIGITS_CAPACITY - 1; i >= 0; i--) {
    digit_t d = digits[i];

    if (0 == d && is_leading_zero) continue;

    is_leading_zero = false;
    count++;
  }

  /* special case where all digits are 0 */
  if (count == 0) return 1;

  return count;
}

/**
 * @brief Comare the two arrays of digits.
 *
 * @return  < 0 if d1 < d2
 *          = 0 if d1 == d2
 *          > 0 if d1 > d2
 *
 * @warning The return value does NOT give any indication about the relative
 * distance between the two numbers. It ONLY indicates <, >, or =.
 */
__device__ __host__ inline int digits_compare(digit_t *digits1, unsigned num_d1,
                                       digit_t *digits2, unsigned num_d2) {
  unsigned max_digits = (num_d1 > num_d2) ? num_d1 : num_d2;
  int i;

  /* Iterate backwards so that we look at the most significant digits first */
  for (i = max_digits - 1; i >= 0; i--) {
    digit_t d1 = ((unsigned) i < num_d1) ? digits1[i] : 0;
    digit_t d2 = ((unsigned) i < num_d2) ? digits2[i] : 0;

    if (d1 < d2) return -1;
    if (d1 > d2) return  1;
  }

  return 0;
}

/**
 * @brief Compare a to one.
 */
__device__ __host__ inline int digits_equal_one(digit_t *digits, unsigned capacity) {
  if (digits[0] != 1) {
    return false;
  }

  unsigned i;
  for (i = 1; i < capacity; i ++) {
    if (digits[i] != 0) {
      return false;
    }
  }
  return true;
}

/**
 * @brief Compare a to one.
 */
__device__ __host__ inline int digits_gt_one(digit_t *digits, unsigned capacity) {
  if (digits[0] > 1) {
    return true;
  }
  unsigned i;
  for (i = 1; i < capacity; i ++) {
    if (digits[i] != 0) {
      return true;
    }
  }
  return false;
}

/**
 * @breif Clip the value into two digits, the result and the carry.
 * This is used by addition and multiplication code to compute
 * carries.
 *
 * @param result Passed by reference back to the caller
 * @param carry  Passed by reference back to the caller
 */
__device__ __host__ inline void clip(unsigned long long value,
                                     digit_t* result, digit_t *carry) {
  *carry  = (digit_t) (value / DIGIT_BASE); //FIXME
  *result = (digit_t) (value % DIGIT_BASE); //FIXME
  //printf("clip(%llu): result = %u, carry = %u\n", value, *result, *carry);
}

/**
 * @brief Find the result of a + b + carry. Store the resulting carry of this
 * operation back in the carry pointer.
 */
__device__ __host__ inline digit_t add(digit_t a, digit_t b,
                                digit_t *carry) {
  unsigned long long tmp = ((unsigned long long) a) +
                           ((unsigned long long) b) +
                           ((unsigned long long) *carry);
  digit_t result;

  clip(tmp, &result, carry);

  return result;
}

/**
 * @brief Compute the multiplication of two digits (plus the carry).
 *
 * e.g If DIGIT_BASE is 10, and the input carry is 0,
 *
 *  3 x 5 = 15 = (product: 5, carry: 1)
 *
 * @return The product (as well as the carry out).
 */
__device__ __host__ inline digit_t mult(digit_t a, digit_t b, digit_t *carry) {
  unsigned long long tmp = ((unsigned long long) a) * ((unsigned long long) b) +
                           ((unsigned long long) *carry);
  digit_t result;

  clip(tmp, &result, carry);

  return result;
}


/**
 * @brief Add the digit d to the list of digits (with carry)!
 *
 * @return The carry out.
 */
__device__ __host__ inline digit_t digits_add_across(digit_t *digits,
                                              unsigned num_digits, digit_t d) {
  digit_t carry = d;
  unsigned i = 0;

  while (carry != 0 && i < num_digits) {
    digits[i] = add(digits[i], 0, &carry);
    i++;
  }

  return carry;
}

/** @breif Copy from into to. */
__device__ __host__ inline void digits_copy(digit_t to[DIGITS_CAPACITY],
                                     digit_t from[DIGITS_CAPACITY]) {
  unsigned i;

  for (i = 0; i < DIGITS_CAPACITY; i++) {
    to[i] = from[i];
  }
}

/**
 * @brief Perform DIGIT_BASE complement on the array of digits.
 *
 * For example, if DIGIT_BASE is 10, and the digits are 239487, the 10's
 * complement is:
 *                          +--------+
 * 239487 -> 760518 + 1 ->  | 760519 |
 *                          +--------+
 */
__device__ __host__ inline void digits_complement(digit_t *digits, unsigned num_digits) {
  unsigned i;

  // Complement each digit by subtracting it from BASE-1
  for (i = 0; i < num_digits; i++) {
    digits[i] = (digit_t) ((DIGIT_BASE - 1) - digits[i]);
  }

  // Add 1
  digits_add_across(digits, num_digits, 1);
}

/**
 * @brief Compute sum = op1 + op2.
 *
 * @return The carry-out of the addition (0 if there is none).
 */
__device__ __host__ inline digit_t digits_add(digit_t *sum, unsigned sum_num_digits,
                                       digit_t *op1, unsigned op1_num_digits,
                                       digit_t *op2, unsigned op2_num_digits) {
  digit_t carry = 0;
  unsigned i;

  for (i = 0; i < sum_num_digits; i++) {
    digit_t a = (i < op1_num_digits) ? op1[i] : 0;
    digit_t b = (i < op2_num_digits) ? op2[i] : 0;

    sum[i] = add(a, b, &carry);
    //printf("\tAdding (%x) + %x + %x = %x (with carry = %x)\n", prev_carry, a, b, sum[i], carry);
  }

  return carry;
}

__device__ __host__ inline digit_t digits_addeq(
                                digit_t *op1, unsigned op1_num_digits,
                                digit_t *op2, unsigned op2_num_digits) {
  digit_t carry = 0;
  unsigned i;

  for (i = 0; i < op1_num_digits; i++) {
    digit_t a = (i < op1_num_digits) ? op1[i] : 0;
    digit_t b = (i < op2_num_digits) ? op2[i] : 0;

    op1[i] = add(a, b, &carry);
  }

  return carry;
}

__device__ __host__ inline void digits_mult_u(digit_t product[DIGITS_CAPACITY],
                                              digit_t op[DIGITS_CAPACITY],
                                              digit_t d) {
  unsigned i, j;
  unsigned num_digits = DIGITS_CAPACITY/2;

  /* zero out product */
  for (i = 0; i < 2*num_digits; i++) {
    product[i] = 0;
  }

  i = 0;
  for (j = 0; j < num_digits; j++) {
    unsigned k = i + j;
    digit_t carry = 0;
    digit_t prod;

    prod = mult(d, op[j], &carry);

    digits_add_across(product + k,     2*num_digits - k,     prod);
    digits_add_across(product + k + 1, 2*num_digits - k - 1, carry);
  }

}

/**
 * @brief Compute product = op1 * op2 using the Long Multiplication
 * (Grade School Multiplication) Aglorithm.
 *
 * @warning It is assumed that op1 and op2 contain num_digits each
 * and product has room for at least 2*num_digits.
 *
 * @return The carry out.
 */
__device__ __host__ inline void long_multiplication(digit_t *product,
                                                    digit_t *op1,
                                                    digit_t *op2,
                                                    unsigned num_digits,
                                                    unsigned prod_capacity) {
  int is_op1_zero = true;
  int is_op2_zero = true;
  unsigned i, j;

  /* zero out product */
  for (i = 0; i < 2*num_digits; i++) {
    if (i < num_digits) {
      is_op1_zero = (is_op1_zero) && (op1[i] == 0);
      is_op2_zero = (is_op1_zero) && (op1[i] == 0);
    }
    product[i] = 0;
  }
  for (; i < prod_capacity; i ++) {
    product[i] = 0;
  }

  /* if either of the operands are zero, then their product is zero */
  if (is_op1_zero || is_op2_zero) return;

  for (i = 0; i < num_digits; i++) {
    for (j = 0; j < num_digits; j++) {
      unsigned k = i + j;
      digit_t carry = 0;
      digit_t prod;

      prod = mult(op2[i], op1[j], &carry);

      digits_add_across(product + k,     2*num_digits - k,     prod);
      digits_add_across(product + k + 1, 2*num_digits - k - 1, carry);
    }
  }

}

__device__ __host__ inline void karatsuba(void) {
  //TODO someway somehow someday
}

/**
 * @brief Compute op1 * op2 and store the result in product.
 *
 * @warning It is assumed that op1 and op2 contain num_digits each
 * and product has room for at least 2*num_digits.
 */
__device__ __host__ inline void digits_mult(digit_t *product,
                                            digit_t *op1,
                                            digit_t *op2,
                                            unsigned num_digits,
                                            unsigned prod_capacity) {
  long_multiplication(product, op1, op2, num_digits, prod_capacity);
}

__device__ __host__ inline void digits_rshift(digit_t *digits, unsigned capacity,
                                              unsigned shift_amount) {
  int i;

  for (i = capacity - shift_amount - 1; i >= 0; i--) {
    digits[i + shift_amount] = digits[i];
  }
  for (i = 0; i < (int) shift_amount; i++) {
    digits[i] = 0;
  }
}

__device__ __host__ inline void bits_lshift(digit_t *digits, unsigned capacity) {
  unsigned d_index = 0;
  unsigned shift_out = 0;

  for (d_index = 0; d_index < capacity; d_index++) {
    digit_t d = digits[d_index];

    digits[d_index] = shift_out | (d << 1);
    shift_out = 1 & bit_at(d, LOG2_DIGIT_BASE - 1);
  }
}

#endif /* __418_DIGIT_H__ */
