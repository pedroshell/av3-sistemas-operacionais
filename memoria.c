#include "kernel.h"

//  --- Variáveis Globais da Memória Virtual ---
//  Aqui elas são de fato alocadas na memória do compilador
Frame RAM[NUM_FRAMES];
int clock_pointer = 0;
int total_page_faults = 0;
int total_page_replacements = 0;

//  Inicializa a RAM colocando com todos os frames livres
void initialize_memory(void){
    total_page_faults = 0;
    total_page_replacements = 0;
    for(int i = 0; i < NUM_FRAMES; i++){
        RAM[i].process_id = -1;
        RAM[i].logical_page = -1;
        RAM[i].reference_bit = 0;
    }
    clock_pointer = 0;
    terminal_writestring("Memoria RAM inicializada com sucesso!\n");
}

//  Algoritmo de Segunda Chance (Substituição Global)
void handle_page_fault(Process* p, int logical_page){
    total_page_faults++;
    while(1){
        // Se o frame estiver livre ou o bit de referência é 0 (Página vítima)
        if(RAM[clock_pointer].process_id == -1 || RAM[clock_pointer].reference_bit == 0){

            // Se o frame NÃO estava livre, precisamos expulsar a página do antigo processo dono
            if(RAM[clock_pointer].process_id != -1){
                total_page_replacements++;
                int victim_pid = RAM[clock_pointer].process_id;
                int victim_page = RAM[clock_pointer].logical_page;

                printf("  -> [SUBSTITUICAO] Processo %02d perdeu a pagina %d (Frame %d liberado).\n",
                                                 victim_pid,      victim_page,   clock_pointer);

                // Procura o processo vítima na lista para atualizar sua tabela de páginas
                Process* temp = process_list;
                while(temp != NULL){
                    if(temp->id == victim_pid){
                        temp->page_table[victim_page].valid = 0;
                        temp->page_table[victim_page].frame_allocated = -1;
                        break;
                    }
                    temp = temp->next;
                }
            }

            // Aloca a nova página no frame da página vítima (frame livre)
            RAM[clock_pointer].process_id = p->id;
            RAM[clock_pointer].logical_page = logical_page;
            RAM[clock_pointer].reference_bit = 1;   // Recebe 1 porque acabou de ser carregada (página recente)

            // Atualiza a tabela de páginas no processo atual que causou o Page Fault
            p->page_table[logical_page].valid = 1;
            p->page_table[logical_page].frame_allocated = clock_pointer;
            
            printf("  -> [PAGE FAULT RESOLVIDO] Processo %02d carregou a pagina %d no Frame %d.\n",
                                                        p->id,              logical_page,   clock_pointer);
            
            // Avança o ponteiro do relógio para a próxima substituição futura e encerra a busca
            clock_pointer = (clock_pointer + 1) % NUM_FRAMES;
            break;
        }
        else{
            // O frame tem bit 1 - então damos a Segunda Chance zerando o bit e avançamos o relógio
            RAM[clock_pointer].reference_bit = 0;
            clock_pointer = (clock_pointer + 1) % NUM_FRAMES;
        }
    }
}


//  Retorna 0 para Page Hit (sucesso) e 1 para Page Fault (falha)
int generate_memory_request(Process* p){
    if (p->num_pages <= 0) return 0; // Safeguard

    // Sorteia uma página para acessar neste instante
    int target_page = rand() % p->num_pages;
    printf("[Memoria] Processo %02d solicitando acesso a pagina %d...\n",
                              p->id,                        target_page);
    
    // Consulta a Tabela de páginas do processo
    if (p->page_table[target_page].valid == 1){
        //  --- PAGE HIT ---
        int frame_idx = p->page_table[target_page].frame_allocated;
        RAM[frame_idx].reference_bit = 1;   // Atualiza o frame de uso (recente)
        printf("  -> [PAGE HIT] Pagina %d ja esta na RAM (Frame %d).\n",
                                    target_page,            frame_idx);
        return 0;   // O processo pode continuar na CPU
    } else {
        //  --- PAGE FAULT ---
        printf("  -> [PAGE FAULT] Pagina %d nao esta na RAM! Adicionando disco...\n",
                                    target_page);
        handle_page_fault(p, target_page);
        return 1;   // Avisa o escalonador que houve um Page Fault e o processo deve ser bloqueado
    }

}

void print_memory_status(void){
    printf("\n--- STATUS DA MEMORIA RAM FISICA ---\n");
    printf("Ponteiro do Relogio (Algoritmo da Segunda Chance): Apontando para o Frame [%d]\n", clock_pointer);
    printf("--------------------------------------------------\n");
    printf("| Frame | Processo | Pag. Logica | Bit Referencia |\n");
    printf("--------------------------------------------------\n");
    
    for (int i = 0; i < NUM_FRAMES; i++){

        // Destaca o Frame para onde o relógio está apontando com um "->"
        char pointer_mark = (i == clock_pointer) ? '>' : ' ';

        if (RAM[i].process_id == -1){
            // Frame livre
            printf("|  %c%02d  |  ---  |     --      |       -        |\n",
                    pointer_mark, i);
        } else{
            // Frame ocupado
            printf("|  %c%02d   |   P%02d    |     %02d      |       %d        |\n",
                    pointer_mark, i, RAM[i].process_id, RAM[i].logical_page, RAM[i].reference_bit);
        }
    }
    printf("--------------------------------------------------\n");

}





