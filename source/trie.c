#include "trie.h"

trie *makeTrie()
{
    int i;
    trie *t = (trie *)malloc(sizeof(trie));
    t->num = -1; // 기본 등록번호는 -1 : 문자열이 등록되지 않았음
    for (i = 0; i < 128; i++)
        t->child[i] = 0;
    return t;
}

void trie_insert(trie *t, int num, const char *s)
{
    if (*s == '\0') // 문자열의 끝에 도달했으면
    {
        t->num = num; // 등록
        return;
    }

    if (t->child[*s] == NULL) // 메모리가 할당되어있지 않으면
    {
        t->child[*s] = makeTrie(); // Trie 메모리 생성
    }
    trie_insert(t->child[*s], num, s + 1); // 다음 문자열 진행
}

int trie_find(trie *t, const char *s)
{
    if (*s == '\0') // 문자열의 끝에 도달했으면
    {
        return t->num; // 등록번호 리턴
    }

    if (t->child[*s] == NULL) // 정보가 없으면
    {
        return -1; // -1 리턴
    }
    trie_find(t->child[*s], s + 1); // 다음 문자를 탐색
}

void trie_free(trie *t)
{
    int i;
    for (i = 0; i < 128; i++)
    {
        if (t->child[i] != NULL)
        {                           // 동적할당 되어있으면
            trie_free(t->child[i]); // free
        }
    }
    free(t);
}