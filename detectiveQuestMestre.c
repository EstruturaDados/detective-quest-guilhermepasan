/*
 Detective Quest - Sistema de pistas (versão melhorada)
 - Navegação com voltar (back), esquerda/direita, sair
 - Pistas coletadas armazenadas em BST com contador (conta duplicatas)
 - Tabela hash associa pista -> suspeito
 - Ao final, resumo completo e veredito (>=2 pistas para acusação válida)
 
 Compilar:
    gcc -std=c11 -O2 -Wall detective_pistas_v2.c -o detective_pistas_v2
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#define MAX_NAME 64
#define HASH_SIZE 53    /* número primo para buckets */
#define STACK_MAX 128   /* profundidade máxima para "voltar" */

/* ----------------------- Estruturas ----------------------- */

/* Nó da árvore de salas (mapa da mansão) */
typedef struct Sala {
    char nome[MAX_NAME];
    char pista[MAX_NAME]; /* string vazia "" -> sem pista */
    struct Sala *esquerda;
    struct Sala *direita;
} Sala;

/* Nó da BST que armazena pistas coletadas; inclui contador para duplicatas */
typedef struct BSTNode {
    char pista[MAX_NAME];
    int contador;               /* quantas vezes a pista foi coletada */
    struct BSTNode *esq;
    struct BSTNode *dir;
} BSTNode;

/* Entrada na tabela hash (encadeamento) */
typedef struct HashEntry {
    char pista[MAX_NAME];       /* chave */
    char suspeito[MAX_NAME];    /* valor */
    struct HashEntry *prox;
} HashEntry;

/* Tabela hash */
typedef struct {
    HashEntry *buckets[HASH_SIZE];
} HashTable;

/* -------------------- Protótipos das funções -------------------- */

/* criarSala() – cria dinamicamente uma sala */
Sala* criarSala(const char *nome, const char *pista);

/* explorarSalas() – navega pela árvore e ativa o sistema de pistas */
void explorarSalas(Sala *raiz, BSTNode **raizPistas, HashTable *ht);

/* inserirPista() / adicionarPista() – insere/atualiza a pista coletada na BST */
BSTNode* inserirPista(BSTNode *raiz, const char *pista);
BSTNode* buscarPistaNode(BSTNode *raiz, const char *pista); /* retorna ponteiro ou NULL */

/* inserirNaHash() – insere associação pista/suspeito na tabela hash */
void inicializarHash(HashTable *ht);
unsigned long hash_djb2(const char *str);
void inserirNaHash(HashTable *ht, const char *pista, const char *suspeito);
const char* encontrarSuspeito(HashTable *ht, const char *pista);

/* verificarSuspeitoFinal() – fase de julgamento final */
void verificarSuspeitoFinal(BSTNode *raizPistas, HashTable *ht);

/* auxiliares: imprimir pistas (in-order), liberar estruturas, listar suspeitos */
void imprimirPistasComContagem(BSTNode *raiz, HashTable *ht);
void coletarSuspeitosUnicos(HashTable *ht, char nomes[][MAX_NAME], int *qtd);
void imprimirSuspeitos(HashTable *ht);
void liberarBST(BSTNode *raiz);
void liberarArvore(Sala *raiz);
void liberarHash(HashTable *ht);

/* -------------------- Implementações -------------------- */

/* criarSala: aloca e inicializa dinamicamente uma sala com nome e pista */
Sala* criarSala(const char *nome, const char *pista) {
    Sala *s = (Sala*) malloc(sizeof(Sala));
    if (!s) {
        fprintf(stderr, "Erro: falha ao alocar memória para sala.\n");
        exit(EXIT_FAILURE);
    }
    strncpy(s->nome, nome, MAX_NAME-1);
    s->nome[MAX_NAME-1] = '\0';
    if (pista && pista[0] != '\0') {
        strncpy(s->pista, pista, MAX_NAME-1);
        s->pista[MAX_NAME-1] = '\0';
    } else {
        s->pista[0] = '\0';
    }
    s->esquerda = s->direita = NULL;
    return s;
}

/* inserirPista: insere pista na BST; se existir, incrementa contador */
BSTNode* inserirPista(BSTNode *raiz, const char *pista) {
    if (!pista || pista[0] == '\0') return raiz;
    if (raiz == NULL) {
        BSTNode *n = (BSTNode*) malloc(sizeof(BSTNode));
        if (!n) { fprintf(stderr, "Erro de memória BST\n"); exit(EXIT_FAILURE); }
        strncpy(n->pista, pista, MAX_NAME-1);
        n->pista[MAX_NAME-1] = '\0';
        n->contador = 1;
        n->esq = n->dir = NULL;
        return n;
    }
    int cmp = strcmp(pista, raiz->pista);
    if (cmp == 0) {
        raiz->contador += 1; /* incrementa duplicata */
    } else if (cmp < 0) {
        raiz->esq = inserirPista(raiz->esq, pista);
    } else {
        raiz->dir = inserirPista(raiz->dir, pista);
    }
    return raiz;
}

/* buscarPistaNode: retorna nó se existir */
BSTNode* buscarPistaNode(BSTNode *raiz, const char *pista) {
    if (!raiz || !pista) return NULL;
    int cmp = strcmp(pista, raiz->pista);
    if (cmp == 0) return raiz;
    if (cmp < 0) return buscarPistaNode(raiz->esq, pista);
    return buscarPistaNode(raiz->dir, pista);
}

/* Inicializa a tabela hash (define buckets como NULL) */
void inicializarHash(HashTable *ht) {
    for (int i = 0; i < HASH_SIZE; ++i) ht->buckets[i] = NULL;
}

/* djb2 hash */
unsigned long hash_djb2(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*str++))
        hash = ((hash << 5) + hash) + c;
    return hash % HASH_SIZE;
}

/* inserirNaHash: insere par pista->suspeito (substitui se já existir) */
void inserirNaHash(HashTable *ht, const char *pista, const char *suspeito) {
    if (!pista || !suspeito) return;
    unsigned long key = hash_djb2(pista);
    HashEntry *cur = ht->buckets[key];
    while (cur) {
        if (strcmp(cur->pista, pista) == 0) {
            strncpy(cur->suspeito, suspeito, MAX_NAME-1);
            cur->suspeito[MAX_NAME-1] = '\0';
            return;
        }
        cur = cur->prox;
    }
    /* não encontrou, cria novo entry */
    HashEntry *entry = (HashEntry*) malloc(sizeof(HashEntry));
    if (!entry) { fprintf(stderr, "Erro hash malloc\n"); exit(EXIT_FAILURE); }
    strncpy(entry->pista, pista, MAX_NAME-1);
    entry->pista[MAX_NAME-1] = '\0';
    strncpy(entry->suspeito, suspeito, MAX_NAME-1);
    entry->suspeito[MAX_NAME-1] = '\0';
    entry->prox = ht->buckets[key];
    ht->buckets[key] = entry;
}

/* encontrarSuspeito: retorna ponteiro interno para nome do suspeito ou NULL */
const char* encontrarSuspeito(HashTable *ht, const char *pista) {
    if (!pista) return NULL;
    unsigned long key = hash_djb2(pista);
    HashEntry *cur = ht->buckets[key];
    while (cur) {
        if (strcmp(cur->pista, pista) == 0) {
            return cur->suspeito;
        }
        cur = cur->prox;
    }
    return NULL;
}

/* explorarSalas: interação com o jogador; mantém pilha para voltar */
void explorarSalas(Sala *raiz, BSTNode **raizPistas, HashTable *ht) {
    if (!raiz) return;

    Sala *pilha[STACK_MAX];
    int topo = -1;       /* -1 = vazio */
    Sala *atual = raiz;
    char entrada[16];

    while (1) {
        printf("\nVocê está em: %s\n", atual->nome);

        /* coleta de pista, se existir */
        if (atual->pista[0] != '\0') {
            BSTNode *n = buscarPistaNode(*raizPistas, atual->pista);
            if (!n) {
                printf("Você encontrou uma pista: \"%s\"\n", atual->pista);
                *raizPistas = inserirPista(*raizPistas, atual->pista);
            } else {
                printf("Você já coletou a pista aqui: \"%s\" (já coletada %d vez(es)).\n",
                       n->pista, n->contador);
                /* se desejar, poderia incrementar novamente ao revisitar; aqui não incrementa */
            }

            const char *s = encontrarSuspeito(ht, atual->pista);
            if (s)
                printf("-> Esta pista aponta para: %s\n", s);
            else
                printf("-> Esta pista não está associada a nenhum suspeito conhecido.\n");
        } else {
            printf("Nenhuma pista aparente nesta sala.\n");
        }

        /* Opções de movimento (inclui 'b' para voltar quando possível) */
        printf("\nOpções de movimento:\n");
        if (atual->esquerda) printf(" - (e) Ir para %s\n", atual->esquerda->nome);
        if (atual->direita) printf(" - (d) Ir para %s\n", atual->direita->nome);
        if (topo >= 0) printf(" - (b) Voltar para %s\n", pilha[topo]->nome);
        printf(" - (s) Sair da exploração\n");
        printf("Escolha: ");

        if (scanf("%15s", entrada) != 1) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {}
            printf("Entrada inválida. Tente novamente.\n");
            continue;
        }

        if (strcmp(entrada, "e") == 0 || strcmp(entrada, "E") == 0) {
            if (atual->esquerda) {
                /* empilha atual e vai para esquerda */
                if (topo + 1 >= STACK_MAX) {
                    printf("Impossível empilhar mais (limite). Ação cancelada.\n");
                } else {
                    pilha[++topo] = atual;
                    atual = atual->esquerda;
                }
            } else {
                printf("Caminho à esquerda inexistente.\n");
            }
        } else if (strcmp(entrada, "d") == 0 || strcmp(entrada, "D") == 0) {
            if (atual->direita) {
                if (topo + 1 >= STACK_MAX) {
                    printf("Impossível empilhar mais (limite). Ação cancelada.\n");
                } else {
                    pilha[++topo] = atual;
                    atual = atual->direita;
                }
            } else {
                printf("Caminho à direita inexistente.\n");
            }
        } else if (strcmp(entrada, "b") == 0 || strcmp(entrada, "B") == 0) {
            if (topo >= 0) {
                atual = pilha[topo--]; /* desempilha */
            } else {
                printf("Não há sala anterior para voltar.\n");
            }
        } else if (strcmp(entrada, "s") == 0 || strcmp(entrada, "S") == 0) {
            printf("Exploração encerrada pelo jogador.\n");
            break;
        } else {
            printf("Opção inválida. Use e, d, b ou s.\n");
        }
    }
}

/* imprimirPistasComContagem: imprime as pistas coletadas em ordem e mostra suspeito relacionado */
void imprimirPistasComContagem(BSTNode *raiz, HashTable *ht) {
    if (!raiz) return;
    imprimirPistasComContagem(raiz->esq, ht);
    const char *sus = encontrarSuspeito(ht, raiz->pista);
    if (sus)
        printf(" - \"%s\" (coletada %d vez(es)) => aponta para: %s\n", raiz->pista, raiz->contador, sus);
    else
        printf(" - \"%s\" (coletada %d vez(es)) => aponta para: (nenhum)\n", raiz->pista, raiz->contador);
    imprimirPistasComContagem(raiz->dir, ht);
}

/* coletarSuspeitosUnicos: preenche array com nomes únicos de suspeitos encontrados na hash */
void coletarSuspeitosUnicos(HashTable *ht, char nomes[][MAX_NAME], int *qtd) {
    *qtd = 0;
    for (int i = 0; i < HASH_SIZE; ++i) {
        for (HashEntry *e = ht->buckets[i]; e; e = e->prox) {
            /* verificar se já está na lista */
            int encontrado = 0;
            for (int k = 0; k < *qtd; ++k) {
                if (strcmp(nomes[k], e->suspeito) == 0) { encontrado = 1; break; }
            }
            if (!encontrado) {
                strncpy(nomes[*qtd], e->suspeito, MAX_NAME-1);
                nomes[*qtd][MAX_NAME-1] = '\0';
                (*qtd)++;
                if (*qtd >= HASH_SIZE) return; /* segurança */
            }
        }
    }
}

/* imprime lista de suspeitos conhecidos */
void imprimirSuspeitos(HashTable *ht) {
    char nomes[HASH_SIZE][MAX_NAME];
    int qtd = 0;
    coletarSuspeitosUnicos(ht, nomes, &qtd);
    if (qtd == 0) {
        printf("Nenhum suspeito registrado no sistema.\n");
        return;
    }
    printf("\nSuspeitos conhecidos:\n");
    for (int i = 0; i < qtd; ++i) {
        printf(" %d) %s\n", i+1, nomes[i]);
    }
}

/* contadorPistasParaSuspeito: percorre BST e soma contadores de pistas que apontam para 'suspeito' */
static int contadorPistasParaSuspeito(BSTNode *raiz, HashTable *ht, const char *suspeito) {
    if (!raiz) return 0;
    int total = 0;
    total += contadorPistasParaSuspeito(raiz->esq, ht, suspeito);
    const char *s = encontrarSuspeito(ht, raiz->pista);
    if (s && strcmp(s, suspeito) == 0) total += raiz->contador;
    total += contadorPistasParaSuspeito(raiz->dir, ht, suspeito);
    return total;
}

/* verificarSuspeitoFinal: mostra resumo, lista suspeitos e pede acusação */
void verificarSuspeitoFinal(BSTNode *raizPistas, HashTable *ht) {
    printf("\n========= RESUMO DA INVESTIGAÇÃO =========\n");

    if (!raizPistas) {
        printf("Você não coletou nenhuma pista durante a exploração.\n");
    } else {
        printf("Pistas coletadas:\n");
        imprimirPistasComContagem(raizPistas, ht);
    }

    /* Mostrar suspeitos conhecidos */
    imprimirSuspeitos(ht);

    /* Perguntar pelo acusado */
    char acusado[MAX_NAME];
    printf("\nDigite o nome do suspeito que deseja acusar (ou deixe em branco para não acusar): ");
    /* limpar buffer até newline anterior */
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
    if (fgets(acusado, sizeof(acusado), stdin) == NULL) {
        printf("Entrada inválida.\n");
        return;
    }
    /* remover newline */
    size_t len = strlen(acusado);
    if (len > 0 && acusado[len-1] == '\n') acusado[len-1] = '\0';

    if (acusado[0] == '\0') {
        printf("Nenhuma acusação realizada. Investigação encerrada.\n");
        return;
    }

    int cont = contadorPistasParaSuspeito(raizPistas, ht, acusado);
    printf("\nPistas que apontam para '%s': %d\n", acusado, cont);
    if (cont >= 2) {
        printf("Acusação válida: existem evidências suficientes para prender %s.\n", acusado);
    } else {
        printf("Acusação fraca: não há pistas suficientes para culpar %s.\n", acusado);
    }
}

/* liberarBST: libera memória da BST */
void liberarBST(BSTNode *raiz) {
    if (!raiz) return;
    liberarBST(raiz->esq);
    liberarBST(raiz->dir);
    free(raiz);
}

/* liberarArvore: libera memória da árvore de salas */
void liberarArvore(Sala *raiz) {
    if (!raiz) return;
    liberarArvore(raiz->esquerda);
    liberarArvore(raiz->direita);
    free(raiz);
}

/* liberarHash: libera todas entradas da hash */
void liberarHash(HashTable *ht) {
    for (int i = 0; i < HASH_SIZE; ++i) {
        HashEntry *cur = ht->buckets[i];
        while (cur) {
            HashEntry *prox = cur->prox;
            free(cur);
            cur = prox;
        }
        ht->buckets[i] = NULL;
    }
}

/* -------------------- main: monta mapa, hash e roda exploração -------------------- */

int main(void) {
    setlocale(LC_ALL, "Portuguese");

    /* Inicializa tabela hash */
    HashTable ht;
    inicializarHash(&ht);

    /* Montagem do mapa (árvore de salas) */
    Sala *hall = criarSala("Hall de Entrada", "pegada barro fora da porta");
    Sala *salaEstar = criarSala("Sala de Estar", "xícara quebrada");
    Sala *cozinha = criarSala("Cozinha", "faca limpa no balcão");
    Sala *biblioteca = criarSala("Biblioteca", "página arrancada do diário");
    Sala *jardim = criarSala("Jardim", "fio de cabelo loiro");
    Sala *escritorio = criarSala("Escritório", "bilhete com ameaça");
    Sala *porao = criarSala("Porão", "pegada barro fora da porta"); /* mesma pista do hall */
    Sala *quarto = criarSala("Quarto Principal", "anel com inicial gravada");
    Sala *lavat = criarSala("Lavabo", "mancha de tinta azul");

    /* Conexões (exemplo): */
    hall->esquerda = salaEstar;
    hall->direita = cozinha;
    salaEstar->esquerda = biblioteca;
    salaEstar->direita = jardim;
    cozinha->direita = escritorio;
    escritorio->direita = porao;
    biblioteca->esquerda = quarto;
    biblioteca->direita = lavat;

    /* Inserir associações pista -> suspeito na hash (dados fixos) */
    inserirNaHash(&ht, "pegada barro fora da porta", "Sr. Morais");
    inserirNaHash(&ht, "xícara quebrada", "Sra. Duarte");
    inserirNaHash(&ht, "faca limpa no balcão", "Chef Marco");
    inserirNaHash(&ht, "página arrancada do diário", "Sra. Duarte");
    inserirNaHash(&ht, "fio de cabelo loiro", "Jovem Lia");
    inserirNaHash(&ht, "bilhete com ameaça", "Sr. Morais");
    inserirNaHash(&ht, "anel com inicial gravada", "Condessa");
    inserirNaHash(&ht, "mancha de tinta azul", "Pintor Raul");

    /* BST das pistas coletadas (inicialmente vazia) */
    BSTNode *raizPistas = NULL;

    printf("=== Detective Quest: Sistema de Investigações (versão melhorada) ===\n");
    printf("Explore a mansão, colete pistas e, ao final, faça sua acusação.\n");
    printf("Comandos de navegação: e (esquerda), d (direita), b (voltar), s (sair).\n");

    /* Exploração interativa a partir do Hall */
    explorarSalas(hall, &raizPistas, &ht);

    /* Fase final: acusação */
    verificarSuspeitoFinal(raizPistas, &ht);

    /* Limpeza de memória */
    liberarBST(raizPistas);
    liberarArvore(hall);
    liberarHash(&ht);

    printf("\nSessão encerrada. Obrigado por jogar.\n");
    return 0;
}
