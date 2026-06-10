#include "kernel.h"

Process* process_list = NULL;
int next_process_id = 1;
int global_time = 0;

// Variável global do Banqueiro: Total de Recursos (Impressoras, Discos, Rede)
int available_resources[NUM_RESOURCES] = {5, 3, 4};

void create_process(void){
    Process* new_process = (Process*)malloc(sizeof(Process));
    
    if(!new_process){
        terminal_writestring("Erro: Falha ao criar processo!\n");
        return;
    }
    
    int total = rand() % 15+1;

    new_process->id = next_process_id++;
    new_process->total_execution_time = total;
    new_process->time_remaining = total;
    new_process->state = PRONTO;
    new_process->arrival_time = global_time;
    new_process->blocked_time_remaining = 0;
    new_process->first_execution_time = -1;
    
    new_process->priority = (rand() % 5) + 1;
    new_process->tickets = (rand() % 20) + 1;
    
    // --- Setup do Banqueiro ---
    for (int i = 0; i < NUM_RESOURCES; i++) {
        new_process->max_need[i] = rand() % (available_resources[i] + 1);
        new_process->allocation[i] = 0;
        new_process->need[i] = new_process->max_need[i];
        new_process->pending_request[i] = 0;
        new_process->temp_finish = 0;
    }

    //  --- Setup da Memória Virtual ---
    //  Sorteia um número de páginas para o process entre 3 e MAX_PAGES_PER_PROCESS
    new_process->num_pages = (rand() % (MAX_PAGES_PER_PROCESS - 2)) + 3;

    //  Alocar o vetor na tabela de páginas dinamicamente
    new_process->page_table = (PageTableEntry*)malloc(new_process->num_pages * sizeof(PageTableEntry));

    //  Inicializar a tabela de páginas (Nenhuma página está na RAM no início)
    for (int i = 0; i < new_process->num_pages; i++){
        new_process->page_table[i].valid = 0;
        new_process->page_table[i].frame_allocated = -1;
    }

    new_process->next = NULL;

    if(!process_list){
        process_list = new_process;
    }else{
        Process* temp = process_list;
        while(temp->next){
            temp=temp->next;
        }
        temp->next = new_process;
    }
    
    terminal_writestring("Processo criado com sucesso!\n");
    // Declaração visual no momento da criação
    printf(" -> Necessidade Maxima declarada: [Imp: %d, Disc: %d, Red: %d]\n", 
           new_process->max_need[0], new_process->max_need[1], new_process->max_need[2]);
}

void list_processes(void){
    Process* current = process_list;
    if(!current){
        terminal_writestring("Nenhum processo ativo!\n");
        return;
    }
    
    printf("\n--- LISTA DE PROCESSOS ATIVOS ---\n");
    printf("Recursos Globais Disponiveis: [Imp: %d, Disc: %d, Red: %d]\n", available_resources[0], available_resources[1], available_resources[2]);
    printf("------------------------------------------------------------------------------------------------------\n");
    
    while(current){
        printf("ID: %02d | Estado: %-17s | Exec: %02d/%02d | Max: [%d,%d,%d] | Alocado: [%d,%d,%d] | Need: [%d,%d,%d]\n",
                current->id,
                (current->state == PRONTO) ? "PRONTO":
                (current->state == EM_EXECUCAO) ? "EM_EXECUCAO":
                (current->state == BLOQUEADO_RECURSO) ? "BLOQUEADO_RECURSO":
                (current->state == BLOQUEADO) ? "BLOQUEADO":
                "CONCLUIDO",
                current->time_remaining,
                current->total_execution_time,
                current->max_need[0], current->max_need[1], current->max_need[2],
                current->allocation[0], current->allocation[1], current->allocation[2],
                current->need[0], current->need[1], current->need[2]);
    
        current = current->next;
    }
    printf("------------------------------------------------------------------------------------------------------\n");
}

void terminate_process(int id){
    Process* current = process_list;
    Process* prev = NULL;
    if (current == NULL) {
        terminal_writestring("Nenhum processo na lista para remover.\n");
        return;
    }
    
    if(current->id==id){
        process_list = current->next;
        // Se o processo morrer antes do fim, devolve os recursos ao sistema
        release_resources(current);
        wake_up_resource_blocked_processes();
        
        terminal_writestring("Processo ERRADICADO com sucesso.\n");
        free(current->page_table);
        free(current);
        return;
    }
    
    while(current != NULL && current->id!=id){
        prev = current;
        current = current->next;
    }
    
    if(current == NULL){
        terminal_writestring("Processo inexistente\n");
        return;
    }
    
    prev->next = current->next;
    
    // Devolve os recursos ao sistema
    release_resources(current);
    wake_up_resource_blocked_processes();
    
    free(current);
    terminal_writestring("Processo ERRADICADO com sucesso.\n");
}

void terminal_initialize(void){
    terminal_writestring("Inicializando terminal...... \n");
}

void terminal_writestring(const char* str){
    printf("%s", str);
}

void wait_user_input(void){
    terminal_writestring("\nPressione ENTER para voltar...\n");
    fflush(stdout); 
    
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
    
    getchar();
}

// --- Implementação do Gráfico de Gantt ---
GanttEvent gantt_log[MAX_GANTT_EVENTS];
int gantt_count = 0;

void add_gantt_event(int start, int end, int pid, const char* note) {
    if (gantt_count < MAX_GANTT_EVENTS) {
        gantt_log[gantt_count].start_time = start;
        gantt_log[gantt_count].end_time = end;
        gantt_log[gantt_count].process_id = pid;
        snprintf(gantt_log[gantt_count].note, sizeof(gantt_log[gantt_count].note), "%s", note);
        gantt_count++;
    }
}

void print_gantt_chart(void) {
    printf("\n--- LINHA DO TEMPO DA CPU (GANTT) ---\n");
    for (int i = 0; i < gantt_count; i++) {
        if (gantt_log[i].process_id == -1) {
            printf("[%02d-%02d]: [OCIOSO] ", gantt_log[i].start_time, gantt_log[i].end_time);
            int dur = gantt_log[i].end_time - gantt_log[i].start_time;
            for(int j=0; j<dur; j++) printf("--");
            printf("\n");
        } else {
            int duration = gantt_log[i].end_time - gantt_log[i].start_time;
            printf("[%02d-%02d]: P%02d ", gantt_log[i].start_time, gantt_log[i].end_time, gantt_log[i].process_id);
            if (duration == 0) {
                printf("▓▓"); // Apenas tentou e bloqueou instantaneamente
            } else {
                for (int j = 0; j < duration; j++) printf("██"); // Progresso real na CPU
            }
            printf("%s\n", gantt_log[i].note);
        }
    }
    printf("-------------------------------------\n");
}

void reset_gantt(void) {
    gantt_count = 0;
}
