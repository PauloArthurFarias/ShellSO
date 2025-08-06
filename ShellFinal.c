#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define MAX_LINE 80 // O tamanho máximo de um comando
#define MAX_ARGS 20 // O número máximo de argumentos

// Função para "quebrar" a linha de comando em argumentos
void parse_command(char *command, char **args) {
    int i = 0;
    // strtok vai "fatiar" a string 'command' usando espaços e nova linha como delimitadores
    char *token = strtok(command, " \n");
    while (token != NULL && i < MAX_ARGS) {
        args[i] = token;
        token = strtok(NULL, " \n");
        i++;
    }
    args[i] = NULL; // O último elemento do array de argumentos deve ser NULL para o execvp
}

int ler_comando(char *command, char **args)
{
    char caminho_atual_diretorio[PATH_MAX];

    if (getcwd(caminho_atual_diretorio, sizeof(caminho_atual_diretorio)) != NULL) {
        printf("%s> ", caminho_atual_diretorio); // Mostra o diretório atual
    } else {
        perror("getcwd erro");
        printf("meu-shell> "); // Fallback
    }
    fflush(stdout); // Garante que o prompt seja exibido imediatamente

    // Lê a linha de comando do usuário
    if (fgets(command, MAX_LINE, stdin) == NULL) {
        return -1; // Sai do loop se encontrar fim de arquivo (Ctrl+D)
    }
    
    // Se o usuário apenas pressionar Enter, continua para o próximo prompt
    if (strcmp(command, "\n") == 0) {
        return 0;
    }

    // Divide o comando em argumentos
    parse_command(command, args);

    return 1;
}

int executar_comando(char **args)
{

    if (args[0] != NULL && strcmp(args[0], "exit") == 0)
    {
        return 0; // Sai do loop principal do shell
    }

    int pipe_pos = -1;
    for (int i = 0; args[i] != NULL; ++i)
    {
        if(strcmp(args[i], "|") == 0)
        {
            pipe_pos = i;
            break;
        }
    }

    if (pipe_pos != -1)
    {
        args[pipe_pos] = NULL;

        char **args1 = args;
        char **args2 = &args[pipe_pos+1];

        int fd[2];
        if (pipe(fd) == -1)
        {
            perror("pipe");
            return 1;
        }

        pid_t pid1 = fork();
        if (pid1 < 0)
        {
            perror("fork");
            return 1;
        }

        if (pid1 == 0)
        {
            close(fd[0]);
            dup2(fd[1], STDOUT_FILENO);
            close(fd[1]);
            
            if (execvp(args1[0], args1) == -1)
            {
                perror("exec 1");
                exit(EXIT_FAILURE);
            }
        }
        
        pid_t pid2 = fork();
        if (pid2 < 0)
        {
            perror("fork");
            return 1;
        }
        
        if (pid2 == 0)
        {
            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);
            close(fd[0]);
            
            if(execvp(args2[0], args2) == -1)
            {
                perror("exec 2");
                exit(EXIT_FAILURE);
            }
        }

        close(fd[0]);
        close(fd[1]);
        waitpid(pid1, NULL, 0);
        waitpid(pid2, NULL, 0);

        return 1;
    }

    if (strcmp(args[0], "cd") == 0)
    {
        if(args[1] == NULL)
        {
            fprintf(stderr, "meu-shell: esperado argumento para \"cd\"\n");
        }
        else
        {
            if(chdir(args[1]) != 0)
            {
                perror("cd");
            }
        }

        return 1;
    }

    if (strcmp(args[0], "mkdir") == 0)
    {
        if(args[1] == NULL)
        {
            fprintf(stderr, "meu-shell: esperado argumento para \"mkdir\"\n");
        }
        else
        {
            if(mkdir(args[1], 0755) != 0)
            {
                perror("mkdir");
            }
        }

        return 1;
    }

    if (strcmp(args[0], "rmdir") == 0)
    {
        if(args[1] == NULL)
        {
            fprintf(stderr, "meu-shell: esperando argumento para \"rmdir\"\n");
        }
        else
        {
            if (rmdir(args[1]) != 0) 
            {
                perror("rmdir");
            }
        }
        return 1;
    }

    if (strcmp(args[0], "touch") == 0)
    {
        if(args[1] == NULL)
        {
            fprintf(stderr, "meu-shell: esperando argumento para \"touch\"\n");
        }
        else
        {
            int arquivo = open(args[1], O_CREAT | O_WRONLY, 0644);
            if (arquivo == -1)
            {
                perror("touch");
            } 
            
            else
            {
                close(arquivo);
            }
        }
        return 1;
    }

    if (strcmp(args[0], "rm") == 0)
    {
        if (args[1] == NULL)
        {
            fprintf(stderr, "meu-shell: esperando argumento para \"rm\"\n");
        }
        else
        {
            if (unlink(args[1]) != 0)
            {
                perror("rm");
            }
        }

        return 1;
    }

    pid_t pid = fork();

        if (pid < 0) {
            // Erro ao criar o processo filho
            perror("fork falhou");
        } else if (pid == 0) {
            // Este é o código que o PROCESSO FILHO executa
            if (execvp(args[0], args) == -1) {
                perror("Erro ao executar comando");
                exit(EXIT_FAILURE); // Importante sair do filho se o exec falhar
            }
        } else {
            // Este é o código que o PROCESSO PAI (o shell) executa
            // O pai espera o processo filho terminar
            wait(NULL);
        }
        
        return 1;
}


int main() {
    char command[MAX_LINE];
    char *args[MAX_ARGS];

    while (1) {
        
        int status = ler_comando(command, args);

        if(status == -1)
        {
            break;
        }

        else if(status == 0)
        {
            continue;
        }
        
        if(executar_comando(args) == 0)
        {
            break;
        }
    }

    printf("Shell encerrado.\n");
    return 0;
}