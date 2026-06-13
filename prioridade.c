#include "kernel.h"

void execute_processes_priority(void){
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
    
    reset_gantt();
    int idle_start = -1;

    terminal_writestring("Iniciando escalonamento PRIORIDADE (I/O Assincrono e Banqueiro)......\n");

    initialize_memory();
    printf("\n--- ESTADO INICIAL DA MEMORIA (VAZIA) ---\n");
    print_memory_status();
    printf("\n");

    while (process_list != NULL) {
        int all_blocked = 1;
        Process* temp = process_list;
        Process* selected = NULL;
        int min_prio = 9999;

        while(temp) {
            if (temp->state != BLOQUEADO && temp->state != BLOQUEADO_RECURSO) {
                all_blocked = 0;
            }
            if (temp->state == PRONTO && temp->priority < min_prio) {
                min_prio = temp->priority;
                selected = temp;
            }
            temp = temp->next;
        }

        if (all_blocked || !selected) {
            if (idle_start == -1) idle_start = global_time;

            printf("tempo %d: CPU Ociosa (Todos os processos bloqueados ou nenhum PRONTO)\n", global_time);
            sleep(1);
            global_time++;
            last_id = -1;
            
            temp = process_list;
            while(temp) {
                if(temp->state == BLOQUEADO) {
                    temp->blocked_time_remaining--;
                    if(temp->blocked_time_remaining <= 0) {
                        temp->state = PRONTO;
                        printf("tempo %d: processo %d concluiu E/S -> PRONTO\n", global_time, temp->id);
                    }
                }
                temp = temp->next;
            }

            int only_resource_blocks = 1;
            temp = process_list;
            while(temp) {
                if (temp->state != BLOQUEADO_RECURSO) {
                    only_resource_blocks = 0;
                    break;
                }
                temp = temp->next;
            }
            if (only_resource_blocks && process_list != NULL) {
                printf("\n>>> tempo %d: DEADLOCK DETECTADO! Sistema travado por falta de recursos. Encerrando execucao.\n", global_time);
                break;
            }

            continue;
        } else {
            if (idle_start != -1) {
                add_gantt_event(idle_start, global_time, -1, "");
                idle_start = -1;
            }
        }

        selected->state = EM_EXECUCAO;
        printf("\n--- tempo %d: processo %d entrou em execucao (Prio: %d, Quantum: %d) ---\n", global_time, selected->id, selected->priority, quantum);
        
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
        int event_start = global_time;

        while(time_run < quantum && selected->time_remaining > 0) {
            
            // Calcula quantos clocks da CPU o processo já consumiu na sua vida útil
            int tempo_decorrido = selected->total_execution_time - selected->time_remaining;

            // Busca no array pré-gerado qual é a página exata para este tick
            int requested_page = selected->page_requests[tempo_decorrido];

            // --- SIMULAÇÃO DE ACESSO À MEMÓRIA (PAGE FAULT) ---
            if (generate_memory_request(selected, requested_page) == 1) {
                selected->state = BLOQUEADO;
                selected->blocked_time_remaining = 3; 
                blocked_now = 2; // '2' identifica Page Fault para o Gantt
                printf("tempo %d: processo %d sofreu PAGE FAULT -> BLOQUEADO\n", global_time, selected->id);
                last_id = -1;
                break;
            }

            // --- GLOW DO BANQUEIRO ---
            if ((rand() % 100) < 10) { 
                int req[NUM_RESOURCES] = {0};
                int has_request = 0;
                const char* resource_names[NUM_RESOURCES] = {"impressora(s)", "disco(s)", "porta(s) de rede"};
                
                for(int i = 0; i < NUM_RESOURCES; i++) {
                    if (selected->need[i] > 0) {
                        req[i] = (rand() % selected->need[i]) + 1; 
                        has_request = 1;
                    }
                }
                
                if (has_request) {
                    for(int i = 0; i < NUM_RESOURCES; i++) {
                        if(req[i] > 0){
                            printf("tempo %d: processo %d solicitou %d %s\n", global_time, selected->id, req[i], resource_names[i]);
                        }
                    }
                    
                    if (request_resources(selected, req)) {
                        printf("tempo %d: Algoritmo do Banqueiro permitiu alocacao (Estado Seguro).\n", global_time);
                    } else {
                        printf("tempo %d: Algoritmo do Banqueiro detectou estado inseguro. Processo %d bloqueado.\n", global_time, selected->id);
                        printf("   ↳ Vetor Disponivel: [Imp: %d, Disc: %d, Red: %d]\n", available_resources[0], available_resources[1], available_resources[2]);
                        printf("   ↳ Necessidade P%d : [Imp: %d, Disc: %d, Red: %d] -> (Insuficiente para garantir seguranca)\n", selected->id, selected->need[0], selected->need[1], selected->need[2]);
                        
                        for(int i=0; i<NUM_RESOURCES; i++) selected->pending_request[i] = req[i];
                        
                        selected->state = BLOQUEADO_RECURSO;
                        blocked_now = 1;
                        last_id = -1;
                        break; 
                    }
                }
            }

            if (!blocked_now && (rand() % 100) < 15) {
                selected->state = BLOQUEADO;
                selected->blocked_time_remaining = 3; 
                blocked_now = 1;
                printf("tempo %d: processo %d solicitou E/S -> BLOQUEADO\n", global_time, selected->id);
                last_id = -1;
                break; 
            }

            sleep(1);
            global_time++;
            selected->time_remaining--;
            time_run++;
            cpu_busy++;
            printf("tempo %d: processo %d executando... (Restante: %d)\n", global_time, selected->id, selected->time_remaining);
            
            Process* update_io = process_list;
            while(update_io){
                if(update_io != selected && update_io->state == BLOQUEADO){
                    update_io->blocked_time_remaining--;
                    if(update_io->blocked_time_remaining <= 0){
                        update_io->state = PRONTO;
                        printf("tempo %d: processo %d concluiu E/S no background -> PRONTO\n", global_time, update_io->id);
                    }
                }
                update_io = update_io->next;
            }
        }

        int event_end = global_time;
        const char* note = "";
        
        if (selected->time_remaining <= 0) {
            note = " (Concluido)";
        } else if (blocked_now) {
            if (selected->state == BLOQUEADO_RECURSO) note = " (Bloqueou Recurso)";
            else if (blocked_now == 2) note = " (Page Fault)";
            else note = " (Bloqueou E/S)";
        } else {
            note = " (Preempcao)";
        }
        
        add_gantt_event(event_start, event_end, selected->id, note);

        if (selected->time_remaining <= 0) {
            printf("tempo %d: processo %d CONCLUIDO.\n", global_time, selected->id);
            
            // --- Marca as páginas como removíveis na memória ---
            make_process_pages_removable(selected->id);

            release_resources(selected);
            wake_up_resource_blocked_processes();
            
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
                printf("tempo %d: processo %d sofreu preempcao -> PRONTO.\n", global_time, selected->id);
            }
        }

        // Imprime o estado da memória
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

    print_gantt_chart();

    printf("\nTodos os processos foram executados. Tempo Global atual: %d. \n", global_time);
}
