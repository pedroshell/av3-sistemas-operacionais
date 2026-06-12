#include "kernel.h"

void load_processes_from_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Erro ao abrir o arquivo: %s\n", filename);
        return;
    }
    
    // Novas variáveis para ler a necessidade máxima (Max_Imp, Max_Disco, Max_Rede)
    int exec, prio, tick, max_r0, max_r1, max_r2;
    
    // Lê 6 valores por linha agora
    while (fscanf(file, "%d %d %d %d %d %d", &exec, &prio, &tick, &max_r0, &max_r1, &max_r2) == 6) {
        Process* new_process = (Process*)malloc(sizeof(Process));
        new_process->id = next_process_id++;
        new_process->total_execution_time = exec;
        new_process->time_remaining = exec;
        new_process->state = PRONTO;
        new_process->arrival_time = global_time;
        new_process->blocked_time_remaining = 0;
        new_process->first_execution_time = -1;
        new_process->priority = prio;
        new_process->tickets = tick;
        new_process->temp_finish = 0;
        
        // --- Setup do Banqueiro com os dados do arquivo ---
        new_process->max_need[0] = max_r0;
        new_process->max_need[1] = max_r1;
        new_process->max_need[2] = max_r2;
        
        for (int i = 0; i < NUM_RESOURCES; i++) {
            new_process->allocation[i] = 0;
            new_process->need[i] = new_process->max_need[i];
            new_process->pending_request[i] = 0;
        }

        //  --- SETUP DA MEMÓRIA VIRTUAL (PAGINAÇÃO) ---
        //  Cria entre 1 e MAX_PAGES_PER_PROCESS (definido no kernel.h como 6)
        new_process->num_pages = (rand() % MAX_PAGES_PER_PROCESS) + 1;

        //  Aloca a memória para o vetor da tabela de páginas
        new_process->page_table = (PageTableEntry*)malloc(new_process->num_pages * sizeof(PageTableEntry));

        //  PRÉ-GERAÇÃO DA SEQUÊNCIA DE ACESSOS À MEMÓRIA
        //  Cria um array do tamanho do tempo de execução do processo
        new_process->page_requests = (int*)malloc(exec * sizeof(int));
        for (int i = 0; i < exec; i++){
            // Sorteia qual página será acessada em cada clock de CPU
            new_process->page_requests[i] = rand() % new_process->num_pages;
        }

        //  Inicializa a tabela
        for (int i = 0; i < new_process->num_pages; i++){
            new_process->page_table[i].valid = 0;
            new_process->page_table[i].frame_allocated = -1;
        }
        //  -------------------------------------------------------------------------------------------------

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
    }
    fclose(file);
}

void compare_algorithms(void) {
    char filename[256];
    printf("Digite o nome do arquivo (ex: processos.txt): ");
    scanf("%255s", filename);

    printf("\n================ FCFS ================\n");
    global_time = 0;
    next_process_id = 1;
    initialize_memory();
    srand(42);
    load_processes_from_file(filename);
    if (process_list) execute_processes_fcfs();

    printf("\n============ ROUND ROBIN ============\n");
    global_time = 0;
    next_process_id = 1;
    initialize_memory();
    srand(42);
    load_processes_from_file(filename);
    if (process_list) execute_processes_rr();

    printf("\n============= PRIORIDADE =============\n");
    global_time = 0;
    next_process_id = 1;
    initialize_memory();
    srand(42);
    load_processes_from_file(filename);
    if (process_list) execute_processes_priority();

//    printf("\n============== LOTERIA ==============\n");
//    global_time = 0;
//    next_process_id = 1;
//    initialize_memory();
//    srand(42);
//    load_processes_from_file(filename);
//    if (process_list) execute_processes_lottery();
}
