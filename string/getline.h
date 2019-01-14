#ifndef GETLINE_H
#define GETLINE_H

ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream);
ssize_t getline(char **lineptr, size_t *n, FILE *stream);

#endif /* GETLINE_H */

