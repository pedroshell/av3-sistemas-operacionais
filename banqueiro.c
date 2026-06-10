#include "kernel.h"

// Verifica se o estado atual do sistema é seguro
int is_safe_state(void) {
    int work[NUM_RESOURCES];
    Process* temp;

    // 1. Inicializa Work = Available
    for (int i = 0; i < NUM_RESOURCES; i++) {
        work[i] = available_resources[i];
    }

    // 2. Inicializa Finish (usando temp_finish na struct)
    int num_processes = 0;
    temp = process_list;
    while (temp) {
        temp->temp_finish = 0;
        num_processes++;
        temp = temp->next;
    }

    int count = 0;
    while (count < num_processes) {
        int found_process = 0;
        temp = process_list;

        while (temp) {
            if (temp->temp_finish == 0) {
                int can_allocate = 1;
                // Verifica se Need <= Work para todos os recursos
                for (int j = 0; j < NUM_RESOURCES; j++) {
                    if (temp->need[j] > work[j]) {
                        can_allocate = 0;
                        break;
                    }
                }

                // Se pode alocar, simula a execução e devolução dos recursos
                if (can_allocate) {
                    for (int j = 0; j < NUM_RESOURCES; j++) {
                        work[j] += temp->allocation[j];
                    }
                    temp->temp_finish = 1;
                    found_process = 1;
                    count++;
                }
            }
            temp = temp->next;
        }

        // Se iterou toda a lista e não encontrou nenhum processo capaz de terminar, é INSEGURO
        if (found_process == 0) {
            return 0; // Estado Inseguro
        }
    }
    return 1; // Estado Seguro
}

// Tenta alocar os recursos solicitados
int request_resources(Process* p, int request[]) {
    for (int i = 0; i < NUM_RESOURCES; i++) {
        // Validações básicas
        if (request[i] > p->need[i]) {
            printf("Erro: Processo %d pediu mais recursos do que o maximo declarado.\n", p->id);
            return 0; 
        }
        if (request[i] > available_resources[i]) {
            return 0; // Recursos insuficientes no momento, deve bloquear
        }
    }

    // Fingir que alocou (Cópia Temporária dos Estados)
    for (int i = 0; i < NUM_RESOURCES; i++) {
        available_resources[i] -= request[i];
        p->allocation[i] += request[i];
        p->need[i] -= request[i];
    }

    // Roda o Algoritmo do Banqueiro
    if (is_safe_state()) {
        return 1; // Sucesso! Estado é seguro, alocação mantida.
    } else {
        // ROLLBACK: Estado inseguro, desfaz a alocação
        for (int i = 0; i < NUM_RESOURCES; i++) {
            available_resources[i] += request[i];
            p->allocation[i] -= request[i];
            p->need[i] += request[i];
        }
        return 0; // Negado!
    }
}

// Devolve os recursos quando o processo termina
void release_resources(Process* p) {
    for (int i = 0; i < NUM_RESOURCES; i++) {
        available_resources[i] += p->allocation[i];
        p->allocation[i] = 0;
        p->need[i] = p->max_need[i]; // Opcional, apenas por coerência
    }
}

// Varre a lista tentando acordar processos bloqueados por recursos
void wake_up_resource_blocked_processes(void) {
    Process* temp = process_list;
    while (temp) {
        if (temp->state == BLOQUEADO_RECURSO) {
            // Tenta submeter a requisição pendente novamente
            if (request_resources(temp, temp->pending_request)) {
                printf("Tempo: %d | Processo: %d acordou! Recursos pendentes CONCEDIDOS.\n", global_time, temp->id);
                temp->state = PRONTO;
                // Zera o pedido pendente
                for(int i=0; i<NUM_RESOURCES; i++) temp->pending_request[i] = 0;
            }
        }
        temp = temp->next;
    }
}
