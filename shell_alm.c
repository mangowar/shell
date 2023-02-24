#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <signal.h>
#define MAXS 150
int back = 0;
typedef struct list1
{
    char* com;
    int mode;
    struct list1 *next;
} list;
void handler(int a)
{

}
 void destroy(list *lst)                                                        //освобождение памяти, выделенной под список команд-конвейров
{
    while (lst != NULL) {
        list* q = lst;
        lst = lst->next;
        free(q->com);
        free(q);
    }
}
void brekets_skip(char* str, int start, int* position)
{
    (*position)++;
    int balance = 1;
    while(balance && str[*position] != '\0')
    {
        if (str[*position] == '(')
        {
            balance++;
            (*position)++;
        }
        else if (str[*position] == ')')
        {
            balance--;
            (*position)++;
        }
        (*position)++;
    }
}

void copy (char* dest, char* source, int start, int amount)
{
    //printf("%s\n", source);
    int a = 0;                                                                  //кол-во если amount больше длины
    for(int i = 0; i < amount && source[i] != '\0'; i++)
    {
        dest[i] = source[i + start];
        //printf("%c\n", source[i]);
        a++;
    }
    dest[a] = '\0';
    //printf("%s\n", dest);
}

char** str_pars(char* str)                                                                //делим строку на простую команду и её аргументы
{
    //printf("%s\n", str);
    if (str[0] == ' ')
        str++;
    int t = 9;                                                                            //изначальное число элементов
    int j = 0;
    int s_len = strlen(str) + 1;
    int start = 0;
    char* buf;
    char** head = calloc(t, sizeof(char *));
    int i = -1;
    do
    {
        i++;
        if (str[i] == ' ' || str[i] == '\0')
        {
            buf = calloc(MAXS, s_len);
            int amount = i - start;
            copy(buf, str, start, amount);
            start = i + 1;
            if (j == (t-1))
            {
                t += 8;
                head = realloc(head, t*sizeof(char *));
            }
            head[j] = buf;
            //printf("%s\n", buf);
            j++;
            start = i+1;
        }
    }
    while(str[i] != '\0');
    head[j] = NULL;
    return head;
}

char** pipeline_link(char* str, int* argc)                                                           //выделяем отдельные команды, составляющие конвейер
{
    if (str[0] == ' ')
        str++;
    int vec_len = 9;
    int s_len = strlen(str) + 1;
    char** head = calloc(vec_len, sizeof(char *));
    char *buf;
    int position = -1;
    int start = 0;
    int amount;
    int j = 0;                                                                            //номер строки
    do
    {
        position++;
        if (str[position] == '(')
            brekets_skip(str, position, &position);
        if (str[position] == '|' || str[position] == '\0')
        {
            amount = position - start;
            if(amount)
            {
            	buf = calloc(s_len, sizeof(char));
                copy(buf, str, start, amount);
                if(buf[strlen(buf) - 1] == ' ')
                    buf[strlen(buf) - 1] = '\0';
                if (j == (vec_len - 1))
                {
                    vec_len += 8;
                    head = realloc(head, sizeof(char *)*vec_len);
                }
                head[j] = buf;
                j++;
                if (str[position + 1] == ' ')
                    position++;
                start = position + 1;
            }
        }

    } while (str[position] != '\0');
    head[j] = NULL;
    *argc = j;
    return head;

}

int in_out (char* str, int* in_f, int* out_f, int* position)                                                         //открытие перенаправляющих файлов
{
    int flag = 0;
    char file_name[4097];
    int file_in = 0;
    int file_out = 1;
    if (str[*position] == '\0')
    {
        // *in_f = file_in;
        // *out_f = file_out;
        return 0;
    }
    for (int i = 0; i < 2; i++)
    {
        if (str[*position] == '>' || str[*position] == '<')
        {
            // flag = 1;
            if (str[*position] == '<')
            {
                flag = 1;
                (*position)++;
            }
            else if (str[*position + 1] != '>')
            {
                flag = 2;
                (*position)++;
            }
            else
            {
                flag = 3;
                (*position) += 2;
            }
            int j = 0;
            while (str[*position] != ' ' && str[*position] != '\0')
            {
                file_name[j] = str[*position];
                j++;
                (*position)++;
            }
            file_name[j] = '\0';
            //printf("%s\n", file_name);                                                        //проверка
            if (flag == 1)
            {
                file_in = open(file_name, O_RDONLY);
            }
            else if (flag == 2)
                file_out = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, 0777);
            else
                file_out = open(file_name, O_WRONLY | O_CREAT | O_APPEND, 0777);
            if (str[*position] != '\0')
                (*position)++;
        }
    }
    if (flag)
    {
        *in_f = file_in;
        *out_f = file_out;
    }
    return flag;
}

int pipeline(char* str)
{
    //printf("%s\n", str);
    int n;
    int in_file = 0; 
    int out_file = 1;
    int position = 0;
    int start = 0;
    pid_t pid;
    char buf[MAXS];
    if (in_out(str, &in_file, &out_file, &position) != 0)
    {
        start = position;
    }
    int in_buf = in_file;
    int out_buf = out_file;
    while ((str[position] != '<') && (str[position] != '>') && (str[position] != '\0'))
    {
        position++;
    }
    copy(buf, str, start, position - start);
    if (buf[strlen(buf) - 1] == ' ')
        buf[strlen(buf) - 1] = '\0';
    //printf("%s\n", buf);
    in_out(str, &in_file, &out_file, &position);
    if (in_file != 0)
        in_buf = in_file;
    if (out_file != 1)
        out_buf = out_file;
    //printf("%d %d\n", in_buf, out_buf);
    char** chain = pipeline_link(buf, &n);
    //printf("%d\n", n);
    int fd[n-1][2];
    for (int i = 0; i < n; i++)
    {
        pipe(fd[i]);
        if (chain[i][0] == '(')
        {
        	if ((pid = fork()) == 0)
        	{
		        if (i == 0)
		            close(fd[i][0]);
		        if (i == n-1)
		            close(fd[i][1]);
		        if ((i == 0) && (in_buf != 0))
		        {
		            //printf("%d\n", in_file);
		            dup2(in_buf, 0);
		            close(in_buf); 
		        }
		        if (i)
		        {
		            dup2(fd[i-1][0], 0);
		            close(fd[i-1][0]);
		        }
		        if (i != n-1)
		        {
		            dup2(fd[i][1], 1);
		            close(fd[i][1]);
		        }
		        if ((i == n-1) && (out_buf != 1))
		        {
		            dup2(out_buf, 1);
		            close(out_buf);
            	}
            	char buf2[MAXS];
            	strncpy(buf2, chain[i] + 1, strlen(chain[i]) - 2);
            	buf2[strlen(buf2)] = '\0';
            	//printf("%s\n", buf2);
            	shell_command(buf2);
            	exit(1);
        	}
        }
        else
        {
            //printf("%s\n", chain[i]);
        	char** arglist = str_pars(chain[i]);
		    if ((pid = fork()) == 0)
		    {
		        if (i == 0)
		            close(fd[i][0]);
		        if (i == n-1)
		            close(fd[i][1]);
		        if ((i == 0) && (in_buf != 0))
		        {
		            //printf("%d\n", in_file);
		            dup2(in_buf, 0);
		            close(in_buf); 
		        }
		        if (i)
		        {
		            dup2(fd[i-1][0], 0);
		            close(fd[i-1][0]);
		        }
		        if (i != n-1)
		        {
		            dup2(fd[i][1], 1);
		            close(fd[i][1]);
		        }
		        if ((i == n-1) && (out_file != 1))
		        {
		            dup2(out_buf, 1);
		            close(out_buf);
		        }
		            execvp(arglist[0], arglist);
		            exit(-1);
		    }
		    else
		    {
		        close(fd[i-1][0]);
		        close(fd[i][1]);
		        for (int j = 0; arglist[j] != NULL; j++)
		        {
		            free(arglist[j]);
		        }
		        free(arglist);
		    }
		}
    }
    for (int j = 0; j < n; j++)
    {
        for (int k = 0; k < strlen(chain[j]); k++)
            chain[j][k] = '\0';
        free(chain[j]);
    }
    free(chain);
    int flag;
    int status;
    while(wait(&status) != -1);
    flag = WIFEXITED(status) & !WEXITSTATUS(status);
    //printf("%d\n", flag);
    return flag;
}
    

list* pipeline_commands(char* str)                                              //разбор команды с условным выполнением на список команд-конвейеров
{
    //printf("%s\n", str);
    if (str[strlen(str) - 1] == ' ')
        str[strlen(str) - 1] = '\0';
    int position = -1;
    int start = 0;
    int amount;
    list* head = NULL;
    list* pointer;
    char* command;
    do
    {
        position++;
        if (str[position] == '(')
            brekets_skip(str, position, &position);
        if ((str[position] == '|' && str[position + 1] == '|') || (str[position] == '&' && str[position + 1] == '&') || (str[position] == '\0'))
        {
            int condit;
            command = (char *)malloc(strlen(str));
            amount = position - start;
            copy(command, str, start, amount);
            //strncpy(command, str+start, amount);
            if (str[position + 2] == ' ')
                position++;
            if (command[strlen(command) - 1] == ' ')
                command[strlen(command) - 1] = '\0';
            //printf("%s %ld\n", command, strlen(command));
            condit = ((str[position] == '|') ? 1 : 0);
            if (head == NULL)
            {
                head = (list *)malloc(sizeof(list));
                head -> mode = condit;
                head -> com = command;
                head -> next = NULL;
                pointer = head;
            }
            else
            {
                pointer -> next = (list *)malloc(sizeof(list));
                pointer = pointer -> next;
                pointer -> mode = condit;
                pointer -> com = command;
                pointer -> next = NULL;
            }
            if (str[position] != '\0')
                position++;
            start = position + 1;
        }
    } while (str[position] != '\0');
    // pointer = head;
    // while(pointer)                                                                          //проверка
    // {
    //     printf("%s %ld\n", pointer -> com, strlen(pointer->com));
    //     pointer = pointer -> next;
    // }
    return head;
}

int shell_command(char* str)                                                       //вызов команды с условным выполнением
{
    //printf("%s %ld\n", str, strlen(str));
    list* arglist = pipeline_commands(str);
    list* pointer = arglist;
    int flag = 0;
    do
    {
        //printf("%s\n", pointer -> com);
        flag = pipeline(pointer -> com) ^ pointer->mode;
        pointer = pointer->next;
        //printf("%d\n", flag);
        if (flag == 0 && pointer != NULL)
        {
            pointer = pointer->next;
        }
    }
    while(pointer);
    destroy(arglist);
    return flag;
}



void back_mode(char* str)
{
    pid_t pid1, pid2;
    int fd = open("/dev/null",O_RDWR,0777);
    if ((pid1 = fork()) == 0)
    {
        if ((pid2 = fork()) == 0)
        {
            dup2(fd,0);
            signal(SIGINT,SIG_IGN);
            shell_command(str);
            exit(0);
        }
        else
        {
            exit(0);
        }
    }
}

void conditional_execution(char* str)                           //разделяю на команды с условным выполнением
{
    //printf("%s\n", str);
    char command[MAXS];
    int position = -1;
    int start = 0;
    int amount;
    do
    {
        position++;
        if (str[position] == '(')
            brekets_skip(str, position, &position);
        if (str[position] == ';' || str[position] == '\0' || (str[position] == '&' && str[position + 1] != '&' && str[position - 1] != '&'))
        {
            amount = position - start;
            if (amount)
            {
                copy(command, str, start, amount);
                if (command[strlen(command) - 1] == ' ')
                    command[strlen(command) - 1] = '\0';
                //printf("%s %ld\n", command, strlen(command));
            }

            //start = position + 1;
            if (str[position] == ';' || str[position] == '\0')
            {
                //printf("%s\n", command);
                //printf("+\n");
                shell_command(command);
            }
            else
            {
                //shell_command(command);
                back_mode(command);
            }
            for(int i = 0; i < MAXS; i++)
            {
                command[i] = '\0';
            }
            if (str[position + 1] == ' ' || str[position + 1] == '\0')
                position++;
            start = position + 1;
        }

    }
    while(str[position] != '\0');
}

int main(int argc, char* argv[])
{
    while(1)
    {
        printf("$ ");
        char command_line[MAXS];
        fgets(command_line, MAXS, stdin);
        command_line[strlen(command_line) - 1] = '\0';
        //printf("%s\n", command_line);
        if (!strlen(command_line))
            continue;
        conditional_execution(command_line);
        for(int i = 0; i < MAXS; i++)
        {
            command_line[i] = '\0';
        }
        fflush(stdin);
    }
}
