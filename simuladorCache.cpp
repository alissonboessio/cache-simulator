#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum {WriteThrough, WriteBack} PoliticaEscrita;
typedef enum {LFU, LRU, Aleatoria} PoliticaSubstituicao;

typedef struct {
    int tag;
    int frequencia;
    int hits;
    int dirtyBit;
} LinhaCache;

typedef struct {
    LinhaCache *linhas;
} BlocoCache;

typedef struct {
    int tamanhoBloco;
    int tamanhoLinha;
    int nmrLinhas;
    int nmrBlocos;
    int tamanhoCache;
    int associatividade;
    int tHitCache;
    int tReadMP;
    int tWriteMP;
    BlocoCache *blocos;
    PoliticaSubstituicao polSubstituicao;
    PoliticaEscrita polEscrita;
} Cache;

void inicializaCache(Cache *cache, PoliticaEscrita polEscrita, int tamanhoLinha, int nmrLinhas, int associatividade, int tHitCache, PoliticaSubstituicao polSubstituicao, int tReadMP, int tWriteMP, int tamanhoCache) {
    int nmrBlocos = nmrLinhas / associatividade;

    cache->nmrBlocos = nmrBlocos;
    cache->tamanhoBloco = tamanhoLinha * associatividade;
    cache->tamanhoLinha = tamanhoLinha;
    cache->nmrLinhas = nmrLinhas;
    cache->tamanhoCache = tamanhoLinha * nmrLinhas;
    cache->associatividade = associatividade;
    cache->tHitCache = tHitCache;
    cache->tReadMP = tReadMP;
    cache->tWriteMP = tWriteMP;
    cache->polSubstituicao = polSubstituicao;
    cache->polEscrita = polEscrita;
    cache->blocos = (BlocoCache *)malloc(nmrBlocos * sizeof(BlocoCache));
    for (int i = 0; i < nmrBlocos; i++) {
        cache->blocos[i].linhas = (LinhaCache *)malloc(associatividade * sizeof(LinhaCache));
        for (int j = 0; j < associatividade; j++) {
            cache->blocos[i].linhas[j].dirtyBit = 0;
            cache->blocos[i].linhas[j].hits = 0;
            cache->blocos[i].linhas[j].frequencia = 0;
            cache->blocos[i].linhas[j].tag = -1; // Tag inicializada como inválida
        }
    }
}

int hexToInt(char hex[32]) {
    return (int)strtol(hex, NULL, 16);
}

int calcularTagBits(int tamanhoBloco) {
    int bits = 0;
    while (tamanhoBloco >>= 1) ++bits;
    return bits;
}

int calcularBlocoBits(int nmrBlocos) {
    int bits = 0;
    while (nmrBlocos >>= 1) ++bits;
    return bits;
}

int calcularTag(int endereco, int tagBits, int blocoBits) {
    return endereco >> (tagBits + blocoBits);
}

int calcularBloco(int endereco, int tagBits, int blocoBits) {
    return (endereco >> tagBits) & ((1 << blocoBits) - 1);
}

int acessarCache(Cache *cache, int endereco, char operacao, int *hits, int *misses, int *rMP, int *wMP) {
    int tagBits = calcularTagBits(cache->tamanhoLinha);
    int blocoBits = calcularBlocoBits(cache->nmrBlocos);
    int tag = calcularTag(endereco, tagBits, blocoBits);
    int idBloco = calcularBloco(endereco, tagBits, blocoBits);
    BlocoCache *bloco = &cache->blocos[idBloco];

    for (int i = 0; i < cache->associatividade; i++) {
        if (bloco->linhas[i].tag == tag) {
            (*hits)++;
            bloco->linhas[i].hits = 0;
            bloco->linhas[i].frequencia++;
            if (operacao == 'W') {
                if (cache->polEscrita == WriteBack) {
                    bloco->linhas[i].dirtyBit = 1;
                } else {
                    (*wMP)++;
                }
            }
            return cache->tHitCache;
        }
    }

    // se chegou até aqui, quer dizer que não encontrou na cache
    (*misses)++;
    (*rMP)++;
    int idSubstituir = 0;

    switch (cache->polSubstituicao) {
        case LFU:
            for (int i = 1; i < cache->associatividade; i++) {
                if (bloco->linhas[i].frequencia < bloco->linhas[idSubstituir].frequencia) {
                    idSubstituir = i;
                }
            }
            break;
        case LRU:
            for (int i = 1; i < cache->associatividade; i++) {
                if (bloco->linhas[i].hits > bloco->linhas[idSubstituir].hits) {
                    idSubstituir = i;
                }
            }
            break;
        case Aleatoria:
            idSubstituir = rand() % cache->associatividade;
            break;
    }

    if (bloco->linhas[idSubstituir].dirtyBit) {
        (*wMP)++;
    }

    bloco->linhas[idSubstituir].tag = tag;
    bloco->linhas[idSubstituir].dirtyBit = (operacao == 'W') && (cache->polEscrita == WriteBack);
    bloco->linhas[idSubstituir].hits = 0;
    bloco->linhas[idSubstituir].frequencia = 1;

    return cache->tHitCache + cache->tReadMP + (bloco->linhas[idSubstituir].dirtyBit ? cache->tWriteMP : 0);
}

int main() {
    Cache cache;

    char nomeArqConfig[50];
    char nomeArqEntrada[50] = "oficial.cache";
    char nomeArqSaida[50];

    printf("Digite o nome do arquivo de config: ");
    gets(nomeArqConfig);
    //printf("Digite o nome do arquivo de entrada: ");
    //gets(nomeArqEntrada);

    printf("Digite o nome do arquivo de saida: ");
    gets(nomeArqSaida);

    FILE *config_file = fopen(nomeArqConfig, "r");
    FILE *input_file = fopen(nomeArqEntrada, "r");

    int tamanhoLinha, tamanhoCache, associatividade, tHitCache, tReadMP, tWriteMP, nmrLinhas;
    PoliticaSubstituicao polSubstituicao;
    PoliticaEscrita polEscrita;

    fscanf(config_file, "%d %d %d %d %d %d %d %d", &polEscrita, &tamanhoLinha, &nmrLinhas, &associatividade,  &tHitCache,  &polSubstituicao, &tReadMP, &tWriteMP);
    fclose(config_file);

    inicializaCache(&cache, polEscrita, tamanhoLinha, nmrLinhas, associatividade,  tHitCache,  polSubstituicao, tReadMP, tWriteMP, tamanhoCache);

    int nmrLeituras = 0, nmrEscritas = 0;
    int hits = 0, misses = 0, rMP = 0, wMP = 0;
    int somatorioTAcessoCache = 0;

    char endereco[32];
    char operacao;

    while (fscanf(input_file, "%s %c", endereco, &operacao) != EOF) {
        int endAux = hexToInt(endereco);
        somatorioTAcessoCache += acessarCache(&cache, endAux, operacao, &hits, &misses, &rMP, &wMP);

        if (operacao == 'R') nmrLeituras++;
        if (operacao == 'W') nmrEscritas++;
    }

    FILE *output_file = fopen(nomeArqSaida, "w");

    int totalEnderecos = nmrLeituras + nmrEscritas;

    float percentHit = (float)hits / totalEnderecos;
    float tAcessoMedioReal = somatorioTAcessoCache / totalEnderecos;

    fprintf(output_file, "Total de Enderecos: %d\n", totalEnderecos);
    fprintf(output_file, "Total de Leituras: %d\n", nmrLeituras);
    fprintf(output_file, "Total de Escritas: %d\n", nmrEscritas);
    fprintf(output_file, "Leituras MP: %d\n", rMP);
    fprintf(output_file, "Escritas MP: %d\n", wMP);
    fprintf(output_file, "%% de hits: %.4f\n", percentHit*100);
    fprintf(output_file, "Tempo de acesso medio real: %.4f ns\n", tAcessoMedioReal);

    fclose(output_file);

    printf("O resultado foi salvo no arquivo %s.\n", nomeArqSaida);
}

