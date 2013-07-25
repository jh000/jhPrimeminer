/**
 * @file cuda_string.h
 *
 * @brief String functions to call from cuda code.
 */


#ifndef __418_CUDA_STRING_H__
#define __418_CUDA_STRING_H__

inline __device__ __host__ int cuda_strlen(const char *str) {
  int len = 0;

  while (str[len] != (char) 0) len++;

  return len;
}

#define FL_UNSIGNED   1
#define FL_NEG        2
#define FL_OVERFLOW   4
#define FL_READDIGIT  8

//#define ULONG_MAX     0xffffffffUL       // Maximum unsigned long value

#define islower(c) ((c) >= 'a' && (c) <= 'z') 
#define isupper(c) ((c) >= 'A' && (c) <= 'Z')
 
#define tolower(c) (isupper(c) ? ((c) - 'A' + 'a') : (c))
#define toupper(c) (islower(c) ? ((c) - 'a' + 'A') : (c))
#define isdigit(c) ((c) >= '0' && (c) <= '9')
#define isalpha(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))
#define isspace(c) ((c)==' ')


/*stroxl function Copyright (C) 2002-2011 Michael Ringgaard.

All rights reserved. 

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright 
   notice, this list of conditions and the following disclaimer.  
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.  
3. Neither the name of the project nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission. 

*/

__device__ __host__ unsigned long cuda_strtoxl(char *nptr, char **endptr, int ibase, int flags) {
  unsigned char *p;
  char c;
  unsigned long number;
  unsigned digval;
  unsigned long maxval;

  p = (unsigned char *) nptr;
  number = 0;

  c = *p++;
  while (isspace(c)) c = *p++;

  if (c == '-') {
    flags |= FL_NEG;
    c = *p++;
  } else if (c == '+') {
    c = *p++;
  }

  if (ibase < 0 || ibase == 1 || ibase > 36) {
    if (endptr) *endptr = (char *) nptr;
    return 0L;
  } else if (ibase == 0) {
    if (c != '0') {
      ibase = 10;
    } else if (*p == 'x' || *p == 'X') {
      ibase = 16;
    } else {
      ibase = 8;
    }
  }

  if (ibase == 16) {
    if (c == '0' && (*p == 'x' || *p == 'X')) {
      ++p;
      c = *p++;
    }
  }

  maxval = ULONG_MAX / ibase;

  for (;;) {
    if (isdigit(c)) {
      digval = c - '0';
    } else if (isalpha(c)) {
      digval = toupper(c) - 'A' + 10;
    } else {
      break;
    }

    if (digval >= (unsigned) ibase) break;

    flags |= FL_READDIGIT;

    if (number < maxval || (number == maxval && (unsigned long) digval <= ULONG_MAX % ibase)) {
      number = number * ibase + digval;
    } else {
      flags |= FL_OVERFLOW;
    }

    c = *p++;
  }

  --p;

  if (!(flags & FL_READDIGIT)) {
    if (endptr) p = (unsigned char*)nptr;
    number = 0;
  } else if ((flags & FL_OVERFLOW) || (!(flags & FL_UNSIGNED) && (((flags & FL_NEG) && (number < LONG_MIN)) || (!(flags & FL_NEG) && (number > LONG_MAX))))) {
//#ifndef KERNEL
//    errno = ERANGE;
//#endif

    if (flags & FL_UNSIGNED) {
      number = ULONG_MAX;
    } else if (flags & FL_NEG) {
      number = LONG_MIN;
    } else {
      number = LONG_MAX;
    }
  }

  if (endptr) *endptr = (char *) p;

  if (flags & FL_NEG) number = (unsigned long) (-(long) number);

  return number;
}

__device__ __host__ long cuda_strtol(char *nptr, char **endptr, int ibase) {
  return (long) cuda_strtoxl(nptr, endptr, ibase, 0);
}

#endif /* __418_CUDA_STRING_H__ */

