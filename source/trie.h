#ifndef TRIE
#define TRIE

#include <stdlib.h>

/**
* Trie 구조체
*
* num : 문자열 등록번호
* child : 다음 문자에 대한 Trie
*/
typedef struct mytrie
{
    int num;
    struct mytrie *child[128];
} trie;

/**
* Trie 를 생성한다
*/
trie *makeTrie();

/**
* 재귀를 이용해 문자열을 등록한다
*
* @param t : 현재 문자에 대한 trie
* @param num : 문자열 등록 번호
* @param s : 탐색할 문자
*/
void trie_insert(trie *t, int num, const char *s);

/**
* 재귀를 이용해 문자열의 등록번호를 탐색한다
*
* @param t : 현재 문자에 대한 trie
* @param s : 탐색할 문자
*/
int trie_find(trie *t, const char *s);

/**
* 재귀를 이용해 트라이의 메로리를 정리한다
*
* @param t : 현재 문자에 대한 trie
*/
void trie_free(trie *t);

#endif