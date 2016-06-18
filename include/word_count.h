#ifndef _WORD_COUNT_H
#define _WORD_COUNT_H

unsigned long hash(char *str);
void word_count(int m, int r, char **text, char *directory);
void *map(void *args);
void *reduce(void *args);

#endif
