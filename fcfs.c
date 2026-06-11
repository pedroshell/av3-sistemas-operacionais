#include "kernel.h"

void execute_processes_fcfs(void){
    if(!process_list){
        terminal_writestring("Nenhum processo para executar.\n");
        return;
    }
    
    int start_t = global_time;
    int completed = 0;
    int tot_turnaround = 0;
    int tot_response = 0;
    int ctx_switches = 0;
    int cpu_busy = 0;
    int last_id = -1;

    reset_gantt();
    int idle_start = -1;

    terminal_writestring("Iniciando escalonamento FCFS (com Banqueiro e Gantt)......\n");
    
    printf("\n--- ESTADO INICIAL DA MEMORIA (VAZIA) ---\n");
    print_memory_status();
    printf("\n");

    while (process_list != NULL) {
        int all_blocked = 1;
        Process* temp = process_list;
        Process* selected = NULL;

        // Busca o primeiro processo PRONTO da fila (comportamento FCFS)
        while(temp) {
            if (temp->state != BLOQUEADO && temp->state != BLOQUEADO_RECURSO) {
                all_blocked = 0;
                if (selected == NULL) selected = temp; 
            }
            temp = temp->next;
        }

        if (all_blocked) {
            if (idle_start == -1) idle_start = global_time;
            printf("tempo %d: CPU Ociosa (Todos os processos bloqueados)\n", global_time);
            sleep(1);
            global_time++;
            last_id = -1;
            
            // FCFS Original trata E/S de forma síncrona. 
            // O loop abaixo previne loop infinito se todos os processos sofrerem Deadlock.
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
        printf("\n--- tempo %d: processo %d entrou em execucao ---\n", global_time, selected->id);

        if (selected->first_execution_time == -1) {
            selected->first_execution_time = global_time;
            tot_response += (selected->first_execution_time - selected->arrival_time);
        }
        if (last_id != selected->id) {
            ctx_switches++;
            last_id = selected->id;
        }

        int blocked_now = 0;
        int event_start = global_time;

        while(selected->time_remaining > 0){

            // --- SIMULAÇÃO DE ACESSO À MEMÓRIA (PAGE FAULT) ---
            if (generate_memory_request(selected) == 1) {
                // Em FCFS a E/S é síncrona. A CPU inteira pausa aguardando o disco.
                selected->state = BLOQUEADO;
                printf("tempo %d: processo %d sofreu PAGE FAULT -> Aguardando disco...\n", global_time, selected->id);
                
                // Fecha o bloco atual no Gantt com o motivo do bloqueio
                add_gantt_event(event_start, global_time, selected->id, " (Page Fault)");
                
                sleep(3); // Simula o tempo de busca no disco
                global_time += 3;
                last_id = -1;
                
                selected->state = EM_EXECUCAO;
                printf("tempo %d: processo %d carregou a pagina -> Retornando a execucao\n", global_time, selected->id);
                
                ctx_switches++;
                last_id = selected->id;
                event_start = global_time; // Inicia um novo bloco no Gantt
                continue; // Pula para a próxima iteração para tentar acessar a RAM de novo
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

            // O FCFS original lida com E/S de forma síncrona, pausando a CPU inteira.
            if (!blocked_now && (rand() % 100) < 15) {
                selected->state = BLOQUEADO;
                printf("tempo %d: processo %d solicitou E/S -> BLOQUEADO\n", global_time, selected->id);
                
                // Finaliza o evento atual no Gantt e cria o buraco da E/S
                add_gantt_event(event_start, global_time, selected->id, " (Bloqueou E/S)");
                
                sleep(2); 
                global_time += 2;
                last_id = -1;
                
                selected->state = EM_EXECUCAO;
                printf("tempo %d: processo %d concluiu E/S -> PRONTO -> EM_EXECUCAO\n", global_time, selected->id);
                
                ctx_switches++;
                last_id = selected->id;
                event_start = global_time; // Reinicia a fatia no Gantt
            }

            sleep(1);
            global_time++;
            selected->time_remaining--;
            cpu_busy++;
            printf("tempo %d: processo %d executando... (Restante: %d)\n", global_time, selected->id, selected->time_remaining);
        }
        
        int event_end = global_time;
        const char* note = "";
        
        if (selected->time_remaining <= 0) {
            note = " (Concluido)";
        } else if (blocked_now) {
            note = " (Bloqueou Recurso)";
        }
        
        add_gantt_event(event_start, event_end, selected->id, note);

        if (selected->time_remaining <= 0) {
            printf("tempo %d: processo %d CONCLUIDO.\n", global_time, selected->id);
            
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
    
    print_gantt_chart();
    
    printf("\nTodos os processos foram executados. Tempo Global atual: %d. \n", global_time);
}
