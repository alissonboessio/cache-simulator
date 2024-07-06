Arquivo de configuração da cache deve ser no formato:
"%d %d %d %d %d %d %d %d"

Onde cada %d é um valor inteiro representado respectivamente: <br><br>
Política de escrita (0 - WriteThrough, 1 - WriteBack), <br>
Tamanho da linha (em potências de 2), <br>
Número de linhas (tamanho da cache) (em potências de 2), <br>
Associatividade da cache (em potências de 2),  <br>
Tempo de hit da cache,  <br>
PoliticaSubstituicao (0 - LFU, 1 - LRU, 2 - Aleatoria),  <br>
Tempo de leitura na memória principal,<br>
Tempo de escrita na memória principal.
