#include "kernel.h"

void execute_processes_lottery(void){
    if(!process_list){
        terminal_writestring("Nenhum processo para executar.\n");
        return;
    }
    
    int quantum;
    printf("Digite o valor do Quantum: ");
    if(scanf("%d", &quantum) != 1 || quantum <= 0) quantum = 3;

    int start_t = global_time;
    int completed = 0;
    int tot_turnaround = 0;
    int tot_response = 0;
    int ctx_switches = 0;
    int cpu_busy = 0;
    int last_id = -1;

    terminal_writestring("Iniciando escalonamento LOTERIA......\n");

    initialize_memory();
    printf("\n--- ESTADO INICIAL DA MEMORIA (VAZIA) ---\n");
    print_memory_status();
    printf("\n");

    while (process_list != NULL) {
        int all_blocked = 1;
        int total_tickets = 0;
        Process* temp = process_list;
        
        while(temp) {
            if (temp->state != BLOQUEADO) {
                all_blocked = 0;
            }
            if (temp->state == PRONTO) {
                total_tickets += temp->tickets;
            }
            temp = temp->next;
        }

        if (all_blocked || total_tickets == 0) {
            printf("Tempo: %d | CPU Ociosa\n", global_time);
            sleep(1);
            global_time++;
            last_id = -1;
            
            temp = process_list;
            while(temp) {
                if(temp->state == BLOQUEADO) {
                    temp->blocked_time_remaining--;
                    if(temp->blocked_time_remaining <= 0) {
                        temp->state = PRONTO;
                        printf("Tempo: %d | Processo: %d concluiu E/S -> PRONTO\n", global_time, temp->id);
                    }
                }
                temp = temp->next;
            }
            continue;
        }

        int winning_ticket = (rand() % total_tickets) + 1;
        int current_sum = 0;
        Process* selected = NULL;
        temp = process_list;

        while(temp) {
            if (temp->state == PRONTO) {
                current_sum += temp->tickets;
                if (current_sum >= winning_ticket) {
                    selected = temp;
                    break;
                }
            }
            temp = temp->next;
        }

        if(!selected) continue;

        selected->state = EM_EXECUCAO;
        printf("\n--- Tempo: %d | Processo: %d CPU (Bilhetes: %d, Q: %d) ---\n", global_time, selected->id, selected->tickets, quantum);
        
        if (selected->first_execution_time == -1) {
            selected->first_execution_time = global_time;
            tot_response += (selected->first_execution_time - selected->arrival_time);
        }
        if (last_id != selected->id) {
            ctx_switches++;
            last_id = selected->id;
        }

        int time_run = 0;
        int blocked_now = 0;

        while(time_run < quantum && selected->time_remaining > 0) {

            // Calcula quantos clocks da CPU o processo já consumiu na sua vida útil
            int tempo_decorrido = selected->total_execution_time - selected->time_remaining;

            // Busca no array pré-gerado qual é a página exata para este tick
            int requested_page = selected->page_requests[tempo_decorrido];

            // --- SIMULAÇÃO DE ACESSO À MEMÓRIA (PAGE FAULT) ---
            if (generate_memory_request(selected, requested_page) == 1) {
                selected->state = BLOQUEADO;
                selected->blocked_time_remaining = 3; // Tempo para buscar a página no disco
                blocked_now = 1;
                printf("Tempo: %d | Processo: %d sofreu PAGE FAULT -> Aguardando disco...\n", global_time, selected->id);
                last_id = -1;
                break; // Quebra o quantum e libera a CPU
            }

            if ((rand() % 100) < 15) {
                selected->state = BLOQUEADO;
                selected->blocked_time_remaining = 3; 
                blocked_now = 1;
                printf("Tempo: %d | Processo: %d solicitou E/S -> BLOQUEADO\n", global_time, selected->id);
                last_id = -1;
                break; 
            }

            sleep(1);
            global_time++;
            selected->time_remaining--;
            time_run++;
            cpu_busy++;
            printf("Tempo: %d | Processo: %d | Restante: %d \n", global_time, selected->id, selected->time_remaining);
            
            Process* update_io = process_list;
            while(update_io){
                if(update_io != selected && update_io->state == BLOQUEADO){
                    update_io->blocked_time_remaining--;
                    if(update_io->blocked_time_remaining <= 0){
                        update_io->state = PRONTO;
                        printf("Tempo: %d | Processo: %d concluiu E/S -> PRONTO\n", global_time, update_io->id);
                    }
                }
                update_io = update_io->next;
            }
        }

        if (selected->time_remaining <= 0) {
            printf("Tempo: %d | Processo: %d CONCLUIDO.\n", global_time, selected->id);
            tot_turnaround += (global_time - selected->arrival_time);
            completed++;
            if(selected == process_list) {
                process_list = selected->next;
            } else {
                Process* prev = process_list;
                while(prev->next != selected) prev = prev->next;
                prev->next = selected->next;
            }
            free(selected);
            last_id = -1;
        } else {
            if (!blocked_now) {
                selected->state = PRONTO;
                printf("Tempo: %d | Processo: %d sofreu preempcao -> PRONTO.\n", global_time, selected->id);
            }
        }

        // Imprime o estado da memória após o processo perder a CPU
        print_memory_status();
        printf("\n");
    }
    
    float avg_turnaround = completed > 0 ? (float)tot_turnaround / completed : 0;
    float avg_response = completed > 0 ? (float)tot_response / completed : 0;
    int total_time = global_time - start_t;
    float cpu_util = total_time > 0 ? ((float)cpu_busy / total_time) * 100.0 : 0;

    printf("\n--- METRICAS DE EXECUCAO ---\n");
    printf("Processos concluidos: %d\n", completed);
    printf("Tempo total da simulacao: %d\n", total_time);
    printf("Utilizacao da CPU: %.2f%%\n", cpu_util);
    printf("Trocas de contexto: %d\n", ctx_switches);
    printf("Tempo de resposta medio: %.2f\n", avg_response);
    printf("O tempo de resposta global do sistema é: %.2f\n", avg_turnaround);

    // --- MÉTRICAS DE MEMÓRIA ---
    int occupied_frames = 0;
    for(int i = 0; i < NUM_FRAMES; i++) {
        if(RAM[i].process_id != -1) occupied_frames++;
    }
    float mem_util = ((float)occupied_frames / NUM_FRAMES) * 100.0;
    printf("Total de Page Faults (Faltas de Pagina): %d\n", total_page_faults);
    printf("Total de Substituicoes de Pagina: %d\n", total_page_replacements);
    printf("Utilizacao final da RAM: %.2f%% (%d/%d frames ocupados)\n", mem_util, occupied_frames, NUM_FRAMES);

    printf("\nTodos os processos foram executados. Tempo Global atual: %d. \n", global_time);
}
