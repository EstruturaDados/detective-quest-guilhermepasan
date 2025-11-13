#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

// Estrutura que representa uma sala da mansão (nó da árvore binária)
typedef struct Sala {
    char nome[50];
    struct Sala *esquerda;
    struct Sala *direita;
} Sala;

/*
 * Função: criarSala
 * ------------------
 * Cria uma nova sala dinamicamente com o nome informado.
 * Retorna o ponteiro para a nova sala.
 */
Sala* criarSala(const char* nome) {
    Sala* nova = (Sala*) malloc(sizeof(Sala));
    if (nova == NULL) {
        printf("Erro ao alocar memória para a sala.\n");
        exit(1);
    }
    strcpy(nova->nome, nome);
    nova->esquerda = NULL;
    nova->direita = NULL;
    return nova;
}

/*
 * Função: explorarSalas
 * ----------------------
 * Permite ao jogador explorar a mansão a partir da sala inicial.
 * O jogador escolhe 'e' (esquerda), 'd' (direita) ou 's' (sair).
 * A exploração termina quando chega a uma sala sem saídas (folha) ou o jogador decide sair.
 */
void explorarSalas(Sala* atual) {
    char escolha;

    while (atual != NULL) {
        printf("\nVocê está em: %s\n", atual->nome);

        // Caso a sala não tenha caminhos à esquerda nem à direita
        if (atual->esquerda == NULL && atual->direita == NULL) {
            printf("Não há mais caminhos a seguir. Fim da exploração!\n");
            return;
        }

        printf("Escolha um caminho:\n");
        if (atual->esquerda != NULL)
            printf(" - (e) Ir para %s\n", atual->esquerda->nome);
        if (atual->direita != NULL)
            printf(" - (d) Ir para %s\n", atual->direita->nome);
        printf(" - (s) Sair da exploração\n");

        printf("Opção: ");
        scanf(" %c", &escolha);

        if (escolha == 'e' || escolha == 'E') {
            if (atual->esquerda != NULL)
                atual = atual->esquerda;
            else
                printf("Caminho à esquerda inexistente!\n");
        } else if (escolha == 'd' || escolha == 'D') {
            if (atual->direita != NULL)
                atual = atual->direita;
            else
                printf("Caminho à direita inexistente!\n");
        } else if (escolha == 's' || escolha == 'S') {
            printf("Exploração encerrada pelo jogador.\n");
            return;
        } else {
            printf("Opção inválida. Tente novamente.\n");
        }
    }
}

/*
 * Função: liberarArvore
 * ----------------------
 * Libera toda a memória alocada para a árvore de salas.
 */
void liberarArvore(Sala* raiz) {
    if (raiz == NULL) return;
    liberarArvore(raiz->esquerda);
    liberarArvore(raiz->direita);
    free(raiz);
}

/*
 * Função principal
 * -----------------
 * Monta a estrutura fixa da mansão e inicia a exploração.
 */
int main() {
    setlocale(LC_ALL, "Portuguese");

    // Montagem manual da árvore de salas (mapa da mansão)
    Sala* hall = criarSala("Hall de Entrada");
    Sala* salaEstar = criarSala("Sala de Estar");
    Sala* cozinha = criarSala("Cozinha");
    Sala* biblioteca = criarSala("Biblioteca");
    Sala* jardim = criarSala("Jardim");
    Sala* escritorio = criarSala("Escritório");
    Sala* porao = criarSala("Porão");

    // Conexões da árvore (estrutura fixa)
    hall->esquerda = salaEstar;
    hall->direita = cozinha;
    salaEstar->esquerda = biblioteca;
    salaEstar->direita = jardim;
    cozinha->direita = escritorio;
    escritorio->direita = porao;

    printf("=== Detective Quest: A Mansão Misteriosa ===\n");
    printf("Explore os cômodos e descubra os segredos escondidos...\n");

    explorarSalas(hall);

    liberarArvore(hall);

    printf("\nObrigado por jogar Detective Quest!\n");

    return 0;
}
