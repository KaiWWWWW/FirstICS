#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

static char* my_itoa(int value, char* str, int base)
{
  char *ret = str;
  if (value == 0)
  {
    *str++ = '0';
    *str = '\0';
    return ret;
  }

  if (base == 10 && value < 0)
  {
    value = -value;
    *str++ = '-';
  }

  unsigned int num = value;
  while (num != 0)
  {
    if (num % base < 10)
      *str++ = '0' + (char)(num % base);
    else
      *str++ = 'a' + (char)(num % base);
    num /= base;
  }
  *str = '\0';

  // reverse
  char *left = (*ret == '-' ? ret + 1 : ret);
  char *right = str - 1;
  char tmp;
  for (; left < right; left++, right--)
  {
    tmp = *left;
    *left = *right;
    *right = tmp;
  }

  return ret;
}

int printf(const char *fmt, ...) {
  char out[128];
  va_list args;
  int ret;
  va_start(args, fmt);
  ret = vsprintf(out, fmt, args);
  va_end(args);
  putstr(out);
  return ret;
  // panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  char *buf = out;
  const char *str = fmt;
  int width = 0;

  while (*str != '\0')
  {
    if (ap == NULL) break;
    else if (*str != '%') {
      *buf++ = *str++;
    }
    else {
      ++str;
      /* width */
      if (*str == '0') {
        str++;
        if (*str <= '9' && *str > '0') {
          width = *str - '0';
          ++str;
        }
      }
      /* type */
      switch (*str++)
      {
      case 'd':
        int digital_val;
        char str_digital[32];

        digital_val = va_arg(ap, int);
        my_itoa(digital_val, str_digital, 10);

        int digital_val_len = strlen(str_digital);
        if (width > digital_val_len) {
          memset(buf, '0', width - digital_val_len);
          buf += (width - digital_val_len);
        }
        buf = strncpy(buf, str_digital, digital_val_len);
        buf += digital_val_len;
        break;
      
      case 's':
        char *str_val;
        str_val = va_arg(ap, char *);
        buf = strncpy(buf, str_val, strlen(str_val));
        buf += strlen(str_val);
        break;
      
      default:
        break;
      }
      width = 0;
    }
  }
  *buf = '\0';
  return buf - out;
  // panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
  va_list args;
  int ret;
  va_start(args, fmt);
  ret = vsprintf(out, fmt, args);
  va_end(args);
  return ret;
  // panic("Not implemented");
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
