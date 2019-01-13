#ifndef GETLINE_H
#define GETLINE_H

extern int getdelim(char **lineptr, unsigned int *n, int delim, FILE *stream);
extern int getline(char **lineptr, unsigned int *n, FILE *stream);

#endif /* GETLINE_H */

