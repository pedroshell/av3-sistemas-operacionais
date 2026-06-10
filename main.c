#include "kernel.h"

int main() {
    srand(time(NULL));
    
    int opcao;
    terminal_initialize();
    
    while(1){
        
        terminal_writestring("\n Kernel Simples \n");
        terminal_writestring("\n 1. Executar Processos (FCFS) \n");
        terminal_writestring("\n 2. Executar Processos (Round Robin) \n");
        terminal_writestring("\n 3. Executar Processos (Prioridade) \n");
        terminal_writestring("\n 4. Executar Processos (Loteria) \n");
        terminal_writestring("\n 5. Criar Processo \n");
        terminal_writestring("\n 6. Listar os Processos \n");
        terminal_writestring("\n 7. Terminar Processo \n");
        terminal_writestring("\n 8. Sair \n");
        terminal_writestring("\n 9. Comparar Algoritmos (Ler Arquivo) \n");
        terminal_writestring("\n Escolha uma opcao \n");
        
        if (scanf("%d", &opcao) != 1) {
            while(getchar() != '\n');
            continue;
        }
        
        switch(opcao){
            case 1:
                execute_processes_fcfs();
                break;
            case 2:
                execute_processes_rr();
                break;
            case 3:
                execute_processes_priority();
                break;
            case 4:
                execute_processes_lottery();
                break;
            case 5:
                create_process();
                break;
            case 6:
                list_processes();
                wait_user_input();
                break;
            case 7:
                terminal_writestring("Digite o ID do processo a ser terminado: \n");
                int id;
                if(scanf("%d", &id) == 1){
                    terminate_process(id);
                }
                break;
            case 8:
                terminal_writestring("Encerrando Kernel...\n");
                exit(0);
            case 9:
                compare_algorithms();
                wait_user_input();
                break;
            default:
                terminal_writestring("Opcao Invalida!\n");
        }
        sleep(1);
        global_time++;
    }
    return 0;
}