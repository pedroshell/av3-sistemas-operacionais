#include "kernel.h"

void execute_processes_rr(void){
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
    
    // Inicia os rastreadores do Gantt
    reset_gantt();
    int idle_start = -1;

    terminal_writestring("Iniciando escalonamento ROUND ROBIN (I/O Assincrono e Banqueiro)......\n");

    initialize_memory();
    printf("\n--- ESTADO INICIAL DA MEMORIA (VAZIA) ---\n");
    print_memory_status();
    printf("\n");

    while (process_list != NULL) {
        int all_blocked = 1;
        Process* temp = process_list;
        
        while(temp) {
            if (temp->state != BLOQUEADO && temp->state != BLOQUEADO_RECURSO) {
                all_blocked = 0;
                break;
            }
            temp = temp->next;
        }

        if (all_blocked) {
            if (idle_start == -1) idle_start = global_time; // Começa a contar a ociosidade
            
            printf("tempo %d: CPU Ociosa (Todos os processos bloqueados)\n", global_time);
            sleep(1);
            global_time++;
            last_id = -1;
            
            temp = process_list;
            while(temp) {
                if(temp->state == BLOQUEADO) {
                    temp->blocked_time_remaining--;
                    if(temp->blocked_time_remaining <= 0) {
                        temp->state = PRONTO;
                        printf("tempo %d: Processo %d concluiu E/S -> PRONTO\n", global_time, temp->id);
                    }
                }
                temp = temp->next;
            }
            continue; 
        } else {
            // Se saiu do estado ocioso, registra o bloco no Gantt
            if (idle_start != -1) {
                add_gantt_event(idle_start, global_time, -1, "");
                idle_start = -1;
            }
        }

        Process* current = process_list;

        if (current->state == BLOQUEADO || current->state == BLOQUEADO_RECURSO) {
            if(current->next != NULL){
                process_list = current->next;
                Process* tail = process_list;
                while(tail->next != NULL) { tail = tail->next; }
                tail->next = current;
                current->next = NULL;
            }
            continue; 
        }

        if (current->state == PRONTO) {
            current->state = EM_EXECUCAO;
            printf("\n--- tempo %d: Processo %d assumiu a CPU (Quantum: %d) ---\n", global_time, current->id, quantum);
            
            if (current->first_execution_time == -1) {
                current->first_execution_time = global_time;
                tot_response += (current->first_execution_time - current->arrival_time);
            }
            if (last_id != current->id) {
                ctx_switches++;
                last_id = current->id;
            }
            
            int time_run = 0;
            int blocked_now = 0;
            int event_start = global_time; // Marca início da fatia no Gantt

            while(time_run < quantum && current->time_remaining > 0) {
            
                // Calcula quantos clocks da CPU o processo já consumiu na sua vida útil
                int tempo_decorrido = current->total_execution_time - current->time_remaining;

                // Busca no array pré-gerado qual é a página exata para este tick
                int requested_page = current->page_requests[tempo_decorrido];

                // --- SIMULAÇÃO DE ACESSO À MEMÓRIA (PAGE FAULT) ---
                // Verifica a memória antes de qualquer coisa (recursos ou I/O)
                if (generate_memory_request(current, requested_page) == 1){
                    // Retornou 1: Page Fault!
                    // O processo precisa ir ao disco buscar a página. Ele é bloqueado.
                    current->state = BLOQUEADO;
                    current->blocked_time_remaining = 3; // Tempo simulado para buscar no disco
                    blocked_now = 1;
                    last_id = 1;

                    // Como acessar o disco é muito lento, ele perde o resto do Quantum nesta iteração
                    break;
                }

                // --- GLOW DO BANQUEIRO ---
                if ((rand() % 100) < 10) { 
                    int req[NUM_RESOURCES] = {0};
                    int has_request = 0;
                    const char* resource_names[NUM_RESOURCES] = {"impressora(s)", "disco(s)", "porta(s) de rede"};
                    
                    for(int i = 0; i < NUM_RESOURCES; i++) {
                        if (current->need[i] > 0) {
                            req[i] = (rand() % current->need[i]) + 1; 
                            has_request = 1;
                        }
                    }
                    
                    if (has_request) {
                        for(int i = 0; i < NUM_RESOURCES; i++) {
                            if(req[i] > 0){
                                printf("tempo %d: Processo %d solicitou %d %s\n", global_time, current->id, req[i], resource_names[i]);
                            }
                        }
                        
                        if (request_resources(current, req)) {
                            printf("tempo %d: Algoritmo do Banqueiro permitiu alocacao (Estado Seguro).\n", global_time);
                        } else {
                            printf("tempo %d: Algoritmo do Banqueiro detectou estado inseguro. Processo %d bloqueado.\n", global_time, current->id);
                            printf("   ↳ Vetor Disponivel: [Imp: %d, Disc: %d, Red: %d]\n", available_resources[0], available_resources[1], available_resources[2]);
                            printf("   ↳ Necessidade P%d : [Imp: %d, Disc: %d, Red: %d] -> (Insuficiente para garantir seguranca)\n", current->id, current->need[0], current->need[1], current->need[2]);
                            
                            for(int i=0; i<NUM_RESOURCES; i++) current->pending_request[i] = req[i];
                            
                            current->state = BLOQUEADO_RECURSO;
                            blocked_now = 1;
                            last_id = -1;
                            break; 
                        }
                    }
                }

                if (!blocked_now && (rand() % 100) < 15) {
                    current->state = BLOQUEADO;
                    current->blocked_time_remaining = 3; 
                    blocked_now = 1;
                    printf("tempo %d: Processo %d solicitou E/S -> BLOQUEADO\n", global_time, current->id);
                    last_id = -1;
                    break; 
                }

                sleep(1);
                global_time++;
                current->time_remaining--;
                time_run++;
                cpu_busy++;
                printf("tempo %d: Processo %d | Restante: %d \n", global_time, current->id, current->time_remaining);
                
                Process* update_io = current->next;
                while(update_io){
                    if(update_io->state == BLOQUEADO){
                        update_io->blocked_time_remaining--;
                        if(update_io->blocked_time_remaining <= 0){
                            update_io->state = PRONTO;
                            printf("tempo %d: Processo %d concluiu E/S no background -> PRONTO\n", global_time, update_io->id);
                        }
                    }
                    update_io = update_io->next;
                }
            }

            int event_end = global_time;
            const char* note = "";
            
            // Avalia o motivo do fim da fatia para plotar no Gantt
            if (current->time_remaining <= 0) {
                note = " (Concluido)";
            } else if (blocked_now) {
                if (current->state == BLOQUEADO_RECURSO) note = " (Bloqueou Recurso)";
                // Como não criamos um estado exclusivo para Memória, usamos a mensagem para diferenciar
                else if (current->blocked_time_remaining == 3) note = "(Bloq E/S / Page Fault)";
                else note = " (Bloqueou E/S)";
            } else {
                note = " (Preempcao)";
            }
            
            add_gantt_event(event_start, event_end, current->id, note);

            if (current->time_remaining <= 0) {
                printf("tempo %d: Processo %d CONCLUIDO.\n", global_time, current->id);
                
                release_resources(current);
                wake_up_resource_blocked_processes();
                
                tot_turnaround += (global_time - current->arrival_time);
                completed++;
                process_list = current->next; 
                free(current);
                last_id = -1;
            } else {
                if (!blocked_now) {
                    current->state = PRONTO;
                    printf("tempo %d: Processo %d sofreu preempcao por Quantum -> PRONTO.\n", global_time, current->id);
                }
                
                if (current->next != NULL) {
                    process_list = current->next;
                    Process* tail = process_list;
                    while(tail->next != NULL) { tail = tail->next; }
                    tail->next = current;
                    current->next = NULL;
                }
            }
            // Imprime o estado da memória no final da fatia (PRINT A CADA ITERAÇÃO)
            print_memory_status();
            printf("\n"); // Uma quebra de linha extra para organizar a tela
        }
    }

    // Imprime o estado final da memória no final da fatia
    print_memory_status();
    printf("\n");
    
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

    // CHAMA A IMPRESSÃO DO GRÁFICO
    print_gantt_chart();
    
    printf("\nTodos os processos foram executados. Tempo Global atual: %d. \n", global_time);
}
