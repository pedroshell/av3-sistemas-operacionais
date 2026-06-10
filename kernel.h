#ifndef KERNEL_H
#define KERNEL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

// --- Configuração de Recursos do Banqueiro ---
#define NUM_RESOURCES 3 // Ex: 0 = Impressora, 1 = Disco, 2 = Rede

// --- Configuração de Recursos da Memória Virtual ---
#define NUM_FRAMES 8            // Tamanho da RAM física | pode ser alterada para até 12 para exemplos diferentes
#define MAX_PAGES_PER_PROCESS 6 // Limite máximo de páginas que um processo pode ter

typedef struct {
    int valid;              // 1 se a página está na RAM, 0 se ocorreu Page Fault
    int frame_allocated;    // Índice do frame da página na memória RAM (se valid == 1)
} PageTableEntry;

typedef struct {
    int process_id;         // ID do processo dono da página
    int logical_page;        // QUAL página do processo está neste Frame da memória RAM
    int reference_bit;      // Bit 'R' - para o Algoritmo de Segunda Chance (0 ou 1)
} Frame;

typedef enum{
    PRONTO,
    EM_EXECUCAO,
    BLOQUEADO,
    BLOQUEADO_RECURSO // Novo estado para prevenção de deadlock
}ProcessState;

typedef struct Process{
    int id;
    int arrival_time;
    int total_execution_time;
    int time_remaining;
    int blocked_time_remaining;
    int priority;
    int tickets;
    int first_execution_time;
    ProcessState state;
    
    // --- NOVOS CAMPOS: Algoritmo do Banqueiro ---
    int max_need[NUM_RESOURCES];        // Matriz Max 
    int allocation[NUM_RESOURCES];      // Matriz Allocation 
    int need[NUM_RESOURCES];            // Matriz Need 
    int pending_request[NUM_RESOURCES]; // Salva requisição pendente
    int temp_finish;                    // Flag para verificação de segurança
    
    // --- NOVOS CAMPOS: Memória Virtual ---
    int num_pages;                  // Quantidade de páginas deste processo
    PageTableEntry* page_table;     // Vetor que será a Tabela de Páginas

    struct Process* next;
}Process;

// Variáveis globais do sistema original
extern Process* process_list;
extern int next_process_id;
extern int global_time;

// Variável global do Banqueiro
extern int available_resources[NUM_RESOURCES];

// Funções originais
void create_process(void);
void list_processes(void);
void terminate_process(int id);
void execute_processes_fcfs(void);
void execute_processes_rr(void);
void execute_processes_priority(void);
void execute_processes_lottery(void);
void wait_user_input(void);
void terminal_initialize(void);
void terminal_writestring(const char* str);
void load_processes_from_file(const char* filename);
void compare_algorithms(void);

// --- Novas funções do Banqueiro ---
int is_safe_state(void);
int request_resources(Process* p, int request[]);
void release_resources(Process* p);
void wake_up_resource_blocked_processes(void);


// --- Estruturas do Gráfico de Gantt ---
#define MAX_GANTT_EVENTS 1000
typedef struct {
    int start_time;
    int end_time;
    int process_id;
    char note[32];
} GanttEvent;

extern GanttEvent gantt_log[MAX_GANTT_EVENTS];
extern int gantt_count;

void add_gantt_event(int start, int end, int pid, const char* note);
void print_gantt_chart(void);
void reset_gantt(void);

// --- Variáveis Globais de Memória Virtual ---
extern Frame RAM[NUM_FRAMES];   // Número de Frames da memória
extern int clock_pointer;       // Ponteiro do 'relógio' da memória virtual (utilizado no Algoritmo de Segunda Chance)

// --- Funções de Memória Virtual ---
void initialize_memory(void);
int generate_memory_request(Process* p);
void handle_page_fault(Process* p, int logical_page);



#endif
