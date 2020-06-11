#ifndef TRIE
#define TRIE

#include <stdlib.h>

typedef struct mytrie{
    int num;
    struct mytrie* child[128];
}trie;


trie* makeTrie();
void trie_insert(trie* t, int num, const char *s);
int trie_find(trie* t, const char *s);
void trie_free(trie* t);

#endif