#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>            //termios, TCSANOW, ECHO, ICANON
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>

// header including our custom helper functions
#include "helpers.h"
#include "commands.h"

const char *sysname = "shellgibi";

// used to store the next command for autocomplete
// only the parent process modifies and uses this
char existing_command[100] = "";
char bookmark[300];

enum return_codes {
    SUCCESS = 0,
    EXIT = 1,
    UNKNOWN = 2,
    RESUME = 3,
};

#define WRITE_END 1
#define READ_END  0

/**
 * Prints a command struct
 * @param struct command_t *
 */
void print_command(struct command_t *command) {
    int i = 0;
    printf("Command: <%s>\n", command->name);
    printf("\tIs Background: %s\n", command->background ? "yes" : "no");
    printf("\tNeeds Auto-complete: %s\n", command->auto_complete ? "yes" : "no");
    printf("\tRedirects:\n");
    for (i = 0; i < 3; i++)
       printf("\t\t%d: %s\n", i, command->redirects[i] ? command->redirects[i] : "N/A");
    printf("\tArguments (%d):\n", command->arg_count);
    for (i = 0; i < command->arg_count; ++i)
        printf("\t\tArg %d: %s\n", i, command->args[i]);
    if (command->next) {
        printf("\tPiped to:\n");
        print_command(command->next);
    }

}

/**
 * Release allocated memory of a command
 * @param  command [description]
 * @return         [description]
 */
int free_command(struct command_t *command) {
    if (command->arg_count) {
        for (int i = 0; i < command->arg_count; ++i)
            free(command->args[i]);
        free(command->args);
    }
    for (int i = 0; i < 3; ++i)
        if (command->redirects[i])
            free(command->redirects[i]);
    if (command->next) {
        free_command(command->next);
        command->next = NULL;
    }
    free(command->name);
    free(command);
    return 0;
}

/**
 * Show the command prompt
 * @return [description]
 */
int show_prompt(char *initial_cmd) {
    char cwd[1024], hostname[1024];
    gethostname(hostname, sizeof(hostname));
    getcwd(cwd, sizeof(cwd));
    //printf("%s@%s:%s %s$ ", getenv("USER"), hostname, cwd, sysname);
    printf("~%s ", cwd);
    return 0;
}

/**
 * Parse a command string into a command struct
 * @param  buf     [description]
 * @param  command [description]
 * @return         0
 */
int parse_command(char *buf, struct command_t *command) {
    const char *splitters = " \t"; // split at whitespace
    int index, len;
    len = strlen(buf);
    while (len > 0 && strchr(splitters, buf[0]) != NULL) // trim left whitespace
    {
        buf++;
        len--;
    }
    while (len > 0 && strchr(splitters, buf[len - 1]) != NULL)
        buf[--len] = 0; // trim right whitespace

    if (len > 0 && buf[len - 1] == '?') // auto-complete
        command->auto_complete = true;
    if (len > 0 && buf[len - 1] == '&') // background
        command->background = true;

    char *pch = strtok(buf, splitters);
    command->name = (char *) malloc(strlen(pch) + 1);
    if (pch == NULL)
        command->name[0] = 0;
    else
        strcpy(command->name, pch);

    command->args = (char **) malloc(sizeof(char *));

    int redirect_index;
    int arg_index = 0;
    char temp_buf[1024], *arg;
    while (1) {
        // tokenize input on splitters
        pch = strtok(NULL, splitters);
        if (!pch) break;
        arg = temp_buf;
        strcpy(arg, pch);
        len = strlen(arg);

        if (len == 0) continue; // empty arg, go for next
        while (len > 0 && strchr(splitters, arg[0]) != NULL) // trim left whitespace
        {
            arg++;
            len--;
        }
        while (len >0 && strchr(splitters, arg[len - 1]) != NULL) arg[--len] = 0; // trim right whitespace
        if (len == 0) continue; // empty arg, go for next

        // piping to another command
        if (strcmp(arg, "|") == 0) {
            struct command_t *c = malloc(sizeof(struct command_t));
            int l = strlen(pch);
            pch[l] = splitters[0]; // restore strtok termination
            index = 1;
           while (pch[index] == ' ' || pch[index] == '\t') index++; // skip whitespaces

            parse_command(pch + index, c);
            pch[l] = 0; // put back strtok termination
            c->read_from_pipe = true;
            command->next = c;

            continue;
        }

        // background process
        if (strcmp(arg, "&") == 0)
            continue; // handled before

        // handle input redirection
        redirect_index = -1;
        if (arg[0] == '<')
            //read from file
            redirect_index = 0;
        if (arg[0] == '>') {
            if (len > 1 && arg[1] == '>') {
                //append write
                redirect_index = 2;
                arg++;
                len--;
            }
                //rewrite
            else redirect_index = 1;
        }
        if (redirect_index != -1) {
            command->redirects[redirect_index] = malloc(len);
            strcpy(command->redirects[redirect_index], arg + 1);
            continue;
        }

        // normal arguments
        if (len > 2 && ((arg[0] == '"' && arg[len - 1] == '"')
                       || (arg[0] == '\'' && arg[len - 1] == '\''))) // quote wrapped arg
        {
            arg[--len] = 0;
            arg++;
        }
       command->args = (char **) realloc(command->args, sizeof(char *) * (arg_index + 1));
        command->args[arg_index] = (char *) malloc(len + 1);
        strcpy(command->args[arg_index++], arg);
    }
    command->arg_count = arg_index;
    return 0;
}

void prompt_backspace() {
    putchar(8); // go back 1
    putchar(' '); // write empty over
    putchar(8); // go back 1 again
}

/**
 * Prompt a command from the user
 * @param  buf      [description]
 * @param  buf_size [description]
 * @return          [description]
 */
int prompt(struct command_t *command, char* initial_cmd) {
    int index = 0;
    char c;
    char buf[4096];
    static char oldbuf[4096];

    // tcgetattr gets the parameters of the current terminal
    // STDIN_FILENO will tell tcgetattr that it should write the settings
    // of stdin to oldt
    static struct termios backup_termios, new_termios;
    tcgetattr(STDIN_FILENO, &backup_termios);
    new_termios = backup_termios;
    // ICANON normally takes care that one line at a time will be processed
    // that means it will return if it sees a "\n" or an EOF or an EOL
    new_termios.c_lflag &= ~(ICANON | ECHO); // Also disable automatic echo. We manually echo each char.
    // Those new settings will be set to STDIN
    // TCSANOW tells tcsetattr to change attributes immediately.
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);


    //FIXME: backspace is applied before printing chars
    show_prompt(initial_cmd);
    printf(" %s", existing_command);
    int multicode_state = 0;
    buf[0] = 0;
    for (int i = 0; i < (int) strlen(existing_command); i++) {
      buf[index++] = existing_command[i];
    }
    while (1) {
        c = getchar();
        // printf("Keycode: %u\n", c); // DEBUG: uncomment for debugging

        if (c == 9) // handle tab
        {
            buf[index++] = '?'; // autocomplete
            break;
        }

        if (c == 127) // handle backspace
        {
            if (index > 0) {
                prompt_backspace();
                index--;
            }
            continue;
        }
        if (c == 27 && multicode_state == 0) // handle multi-code keys
        {
            multicode_state = 1;
            continue;
        }
        if (c == 91 && multicode_state == 1) {
            multicode_state = 2;
            continue;
        }
        if (c == 65 && multicode_state == 2) // up arrow
        {
            int i;
            while (index > 0) {
                prompt_backspace();
                index--;
            }
            for (i = 0; oldbuf[i]; ++i) {
                putchar(oldbuf[i]);
                buf[i] = oldbuf[i];
            }
            index = i;
            continue;
        } else
            multicode_state = 0;

        putchar(c); // echo the character
        buf[index++] = c;
        if (index >= sizeof(buf) - 1) break;
        if (c == '\n') // enter key
            break;
        if (c == 4) // Ctrl+D
            return EXIT;
    }
    if (index > 0 && buf[index - 1] == '\n') // trim newline from the end
        index--;
    buf[index++] = 0; // null terminate string

    strcpy(oldbuf, buf);

    parse_command(buf, command);

    // print_command(command); // DEBUG: uncomment for debugging

    // restore the old settings
    tcsetattr(STDIN_FILENO, TCSANOW, &backup_termios);
    return SUCCESS;
}

int process_command(struct command_t *command);

int main() {
  int code;
    while (1) {
        struct command_t *command = malloc(sizeof(struct command_t));
        memset(command, 0, sizeof(struct command_t)); // set all bytes to 0

        code = prompt(command, "");
        if (code == EXIT) break;

        code = process_command(command);
        if (code == EXIT) break;

        free_command(command);
    }

    printf("\n");
    return 0;
}

int process_command(struct command_t *command) {
    int r;
    if (strcmp(command->name, "") == 0) return SUCCESS;

    if (strcmp(command->name, "exit") == 0)
        return EXIT;

    if (strcmp(command->name, "cd") == 0) {
        if (command->arg_count > 0) {
            r = chdir(command->args[0]);
            if (r == -1)
                printf("-%s: %s: %s\n", sysname, command->name, strerror(errno));
            return SUCCESS;
        }
    }
    if (strcmp(command->name, "setcb") == 0) {
        if (command->arg_count > 0) {
            strcpy(bookmark,command->args[0]);
            return SUCCESS;
        }
    }

    if (strcmp(command->name, "cb") == 0) {
        if(bookmark == NULL){
            printf("Please set a bookmark first, typing 'setcdb **DIRECTORY**'\n");
        }else{
            r = chdir(bookmark);
            if (r == -1)
                printf("-%s: %s: %s\n", sysname, command->name, strerror(errno));
        }
        
        return SUCCESS;
    }

    strcpy(existing_command, "");
     if (command->auto_complete) {
       printf("\n");
       command->name[strlen(command->name) - 1] = 0; // remove last ?
       char* match_result = list_commands(command->name);

       // match result == "" => there are more than one results
       if (strcmp(match_result, "") != 0) {
         // show new prompt with existing command in both cases
         strcpy(existing_command, match_result);
         if (strcmp(match_result, command->name) == 0) {
           printf("Working directory: \n");
           list_working_dir();
         } else {
           printf("Autocompleted command.\n");
           // autocomplete
         }
       }
       return SUCCESS;
     }

    pid_t pid = fork();
    if (pid == 0) // child
    {
        /// This shows how to do exec with environ (but is not available on MacOs)
        // extern char** environ; // environment variables
        // execvpe(command->name, command->args, environ); // exec+args+path+environ

        /// This shows how to do exec with auto-path resolve
        // add a NULL argument to the end of args, and the name to the beginning
        // as required by exec

        // increase args size by 2
        command->args = (char **) realloc(
                command->args, sizeof(char *) * (command->arg_count += 2));

        // shift everything forward by 1
        for (int i = command->arg_count - 2; i > 0; --i)
            command->args[i] = command->args[i - 1];

        // set args[0] as a copy of name
        command->args[0] = strdup(command->name);
        // set args[arg_count-1] (last) to NULL
        command->args[command->arg_count - 1] = NULL;

        //execvp(command->name, command->args); // exec+args+path
        char path[2048]; //if not long enough.....
        strcpy(path, "/bin/");
        strcat(path, command->name); //also might be /usr/bin)

        int out_file = dup(STDOUT_FILENO);
        int in_file = dup(STDIN_FILENO);
        int stdout_no = dup(STDOUT_FILENO);
        int stdin_no = dup(STDIN_FILENO);

        if (command->redirects[0] != NULL) {
            printf("Reading input from file %s.\n", command->redirects[0]);
            in_file = open(command->redirects[0], O_RDONLY);
            dup2(in_file, STDOUT_FILENO);
        }
        if (command->redirects[1] != NULL) {
           printf("Redirecting output (truncate) to file %s.\n", command->redirects[1]);
            out_file = open(command->redirects[1], O_RDWR | O_CREAT | O_TRUNC, 0666);
            dup2(out_file, STDOUT_FILENO);
        }
        if (command->redirects[2] != NULL) {
           printf("Redirecting output (append) to file %s.\n", command->redirects[2]);
            out_file = open(command->redirects[2], O_RDWR | O_CREAT | O_APPEND, 0666);
            dup2(out_file, STDOUT_FILENO);
        }


        if (command->next != NULL) {
          pipe(command->next->incoming_pipe);
          if (fork() == 0) {
            // reader
            close(STDIN_FILENO);
            dup2(command->next->incoming_pipe[READ_END], STDIN_FILENO);
            close(command->next->incoming_pipe[WRITE_END]);
            process_command(command->next);
            exit(0);
          } else {
            // sender
            close(STDOUT_FILENO);
            dup2(command->next->incoming_pipe[WRITE_END], STDOUT_FILENO);
            close(command->next->incoming_pipe[READ_END]);
          }
        }

        // ---
        // NEW COMMANDS
        // ---
        if (strcmp(command->name, "alarm") == 0){
          alarm_command(command);
        } else if (strcmp(command->name, "myjobs") == 0) {
          myjobs(command);
        } else if (strcmp(command->name, "pause") == 0) {
          pause_command(command);
        } else if (strcmp(command->name, "mybg") == 0) {
          mybg(command);
        } else if (strcmp(command->name, "myfg") == 0) {
          myfg(command);
        } else if (strcmp(command->name, "virus") == 0) {
          // custom command 1
          show_covid_status();
        }

        execv(path, command->args);

        // redirect output back to stdout after command executes

        exit(0);
        /// TODO: do your own exec with path resolving using execv()
    } else {
        if (!command->background)
            wait(0); // wait for child process to finish
        return SUCCESS;


    }

    // TODO: your implementation here

    printf("-%s: %s: command not found\n", sysname, command->name);
    return UNKNOWN;
}
