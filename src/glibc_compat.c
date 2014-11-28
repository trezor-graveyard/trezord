#include <stddef.h>

#ifdef __amd64__
   #define GLIBC_COMPAT_SYMBOL(X) __asm__(".symver __" #X "_compat," #X "@GLIBC_2.2.5");
#else
   #define GLIBC_COMPAT_SYMBOL(X) __asm__(".symver __" #X "_compat," #X "@GLIBC_2.0");
#endif

// memcpy

GLIBC_COMPAT_SYMBOL(memcpy)

void *__memcpy_compat(void *, const void *, size_t);

void *__wrap_memcpy(void *dest, const void *src, size_t n)
{
    return __memcpy_compat(dest, src, n);
}

// secure_getenv

GLIBC_COMPAT_SYMBOL(__secure_getenv)

char *____secure_getenv_compat(const char *);

char *__wrap_secure_getenv(const char *name)
{
    return ____secure_getenv_compat(name);
}
