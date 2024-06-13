#ifndef __TRIM

#ifndef weak_alias
# define __trim trim
#endif

char *trim(char *str);
char *trim_chars(char *str, const char *char_set);

#endif // !__TRIM