#include "trie.h"

trie* makeTrie(){
    int i;
    trie* t = (trie*)malloc(sizeof(trie));
    t->num = -1;
    for(i=0;i<128;i++)
        t->child[i] = 0;
    return t;
}

void trie_insert(trie* t, int num, const char *s)
{
    if (*s == '\0')
    {
        t->num = num;
        return;
    }

    if (t->child[*s] == NULL)
    {
        t->child[*s] = makeTrie();
    }
    trie_insert(t->child[*s], num, s+1);
}

int trie_find(trie* t, const char *s)
{
    if (*s == '\0')
    {
        return t->num;
    }

    if (t->child[*s] == NULL)
    {
        return -1;
    }
    trie_find(t->child[*s], s+1);
}

void trie_free(trie* t){
    int i;
    for(i=0;i<128;i++){
        if(t->child[i]!=NULL){
            trie_free(t->child[i]);
        }
    }
    free(t);
}