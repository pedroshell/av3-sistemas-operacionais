# Kernel Simples - escalonador de processos

Simulador didático em C dos algoritmos: FCFS, Round Robin, Fila de Prioridade e Loteria.

## Como compilar e executar

Abra o terminal na pasta do projeto e rode o comando abaixo para compilar:

```bash
gcc main.c processos.c fcfs.c rr.c prioridade.c loteria.c comparador.c banqueiro.c -o simulador
```
para executar utilize

```bash
./simulador
```


## Teste em lote

Para testar todos os algoritmos com os mesmos processos, crie um arquivo de texto (ex: processos.txt) na mesma pasta.

Cada linha representa um processo no formato Tempo_Execução Prioridade Bilhetes.

Exemplo:
```bash
5 1 10
8 3 5
3 2 15
