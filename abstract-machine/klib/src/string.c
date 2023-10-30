#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  if (s == NULL) return 0;
  const char *char_ptr;
  const unsigned long int *longword_ptr;
  register unsigned long int longword, magic_bits;

  // memory alignment
  for (char_ptr = s; ((unsigned long int)char_ptr & (sizeof(unsigned long int) - 1)) != 0; ++char_ptr)
  {
    if (*char_ptr == '\0')
      return char_ptr - s;
  }

  longword_ptr = (const unsigned long int *)char_ptr;
  magic_bits = 0x7efefeffL;
  #if defined(__ISA_NATIVE__)
  magic_bits = 0x7efefefefefefeffL;
  #endif
  
  while (1)
  {
    longword = *longword_ptr++;
    // determine whether there is 0 bytes('\0) in longword (=0, no 0 bytes)
    if ((((longword + magic_bits) ^ ~longword) & ~magic_bits) != 0)
    {
      const char *cp = (const char*)(longword_ptr - 1);
      if (cp[0] == '\0') return cp - s;
      if (cp[1] == '\0') return cp - s + 1;
      if (cp[2] == '\0') return cp - s + 2;
      if (cp[3] == '\0') return cp - s + 3;
      #if defined(__ISA_NATIVE__)
      if (cp[4] == '\0') return cp - s + 4;
      if (cp[5] == '\0') return cp - s + 5;
      if (cp[6] == '\0') return cp - s + 6;
      if (cp[7] == '\0') return cp - s + 7;
      #endif
    }
  }
  // panic("Not implemented");
}

char *strcpy(char *dst, const char *src) {
  if (dst == NULL || src == NULL) return 0;
  assert(dst != NULL && src != NULL);
  unsigned char *dest = (unsigned char*)dst;
  const unsigned char *source = (const unsigned char*)src;
  
  do {
    *dest = *source;
    ++dest;
    ++source;
  } while (*source != '\0');
  *dest = '\0';
  return dst;
  // panic("Not implemented");
}

char *strncpy(char *dst, const char *src, size_t n) {
  if (dst == NULL || src == NULL) return 0; 
  assert(dst != NULL && src != NULL);
  unsigned char *dest = (unsigned char*)dst;
  const unsigned char *source = (const unsigned char*)src;

  while ((*source != '\0') && (n > 0)) {
    *dest = *source;
    --n;
    ++dest;
    ++source;
  }
  while (n > 0)
  {
    *dest++ = '\0';
    --n;
  }
  return dst;
  // panic("Not implemented");
}

char *strcat(char *dst, const char *src) {
  if (dst == NULL || src == NULL) return 0;
  strcpy(dst + strlen(dst), src);
  return dst;
  // panic("Not implemented");
}

int strcmp(const char *s1, const char *s2) {
  if (s1 == NULL || s2 == NULL) return 0;
  assert(s1 != NULL && s2 != NULL);
  const unsigned char *str1 = (const unsigned char*) s1;
  const unsigned char *str2 = (const unsigned char*) s2;
  unsigned char c1, c2;
  do {
    c1 = (unsigned char) *str1++;
    c2 = (unsigned char) *str2++;
    if (c1 == '\0') {
      return c1 - c2;
    }
  }
  while (c1 == c2);
  return c1 - c2;
  // panic("Not implemented");
}

int strncmp(const char *s1, const char *s2, size_t n) {
  if (s1 == NULL || s2 == NULL) return 0;
  assert(s1 != NULL && s2 != NULL);
  if (n == 0) return 0;
  const unsigned char *str1 = (const unsigned char*)s1;
  const unsigned char *str2 = (const unsigned char*)s2;
  unsigned char c1, c2;
  
  do {
    c1 = (unsigned char) *str1++;
    c2 = (unsigned char) *str2++;
    if (c1 == '\0')
      return c1 - c2;
  }
  while (c1 == c2 && --n > 0);
  return c1 - c2;
  // panic("Not implemented");
}

// void -> unsigned char ??
void *memset(void *s, int c, size_t n) {
  if (s == NULL) return 0;
  assert(s != NULL);
  unsigned char *src = s;
  const unsigned char uc = c;
  while (n--){
    *src++ = uc;
  }
  return s;
  // panic("Not implemented");
}

void *memmove(void *dst, const void *src, size_t n) {
  if (dst == NULL || src == NULL) return 0;
  assert(dst != NULL && src != NULL);
  unsigned char *dest = dst;
  const unsigned char *source = src;
  if (dst < src)
  {
    while (n-- > 0)
      *dest++ = *source++;
  }
  else
  {
    dest += n - 1;
    source += n - 1;
    while (n-- > 0)
      *dest-- = *source--;
  }
  return dst;
  // panic("Not implemented");
}

void *memcpy(void *out, const void *in, size_t n) {
  if (out == NULL || in == NULL) return 0;
  assert(out != NULL && in != NULL);
  // if (out > in) assert((in + n) >= out);
  // else assert((out + n) >= in);
  unsigned char *dst = out;
  const unsigned char *src = in;
  while (n--) {
    *dst++ = *src++;
  }
  return out;
  // panic("Not implemented");
}

int memcmp(const void *s1, const void *s2, size_t n) {
  // assert(s1 != NULL && s2 != NULL);
  if (n == 0 || s1 == NULL || s2 == NULL) return 0;
  const unsigned char *src1 = (const unsigned char*)s1;
  const unsigned char *src2 = (const unsigned char*)s2;
  unsigned char c1, c2;

  do {
    c1 = (unsigned char) *src1++;
    c2 = (unsigned char) *src2++;
    // if (c1 == '\0') {
    //   return c1 - c2;
    // }
  }
  while (c1 == c2 && --n > 0);
  return c1 - c2;
  // panic("Not implemented");
}

#endif
