#ifndef __STRNDUP

#ifndef weak_alias
# define __strndup strndup
#endif

char *__strndup(const char *s, size_t n);

#endif // !__STRNDUP
