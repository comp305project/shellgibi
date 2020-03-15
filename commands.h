#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

struct command_t {
  char *name;
  bool background;
  bool auto_complete;
  int arg_count;
  char **args;
  char *redirects[3];
  bool read_from_pipe;
  int incoming_pipe[2];
  struct command_t *next;
};

int show_covid_status() {
  FILE *apifile;

  system("curl --silent https://wuhan-coronavirus-api.laeyoung.endpoint.ainize.ai/jhu-edu/brief > covid-api.txt");
  apifile = fopen("covid-api.txt", "r");

  if (apifile < 0) {
    printf("Cannot establish network connection. Try again later\n");
    return 1;
  }

  char text[100];
  fscanf(apifile, "%s", text);

  char *token;
  int cases[3];
  int token_count = 0;
  token = strtok(text, ":,}");
  while (token != NULL) {
    if (token_count % 2 == 1) {
      cases[token_count / 2] = atoi(token);
    }
    token = strtok(NULL, ":,}");
    token_count++;
  }

  printf("--- COVID-19 Live Status ---\n");
  printf("Confirmed: \t%d\n", cases[0]);
  printf("Deaths: \t%d\n", cases[1]);
  printf("Recovered: \t%d\n", cases[2]);
  printf("----------------------------\n");

  fclose(apifile);
  system("rm covid-api.txt");

  return 0;
}

void myjobs(struct command_t *command) {
  char *cmd = "ps -U $USER";
  system(cmd);
}

void pause_command(struct command_t *command) {
  if (command->args[1] != NULL) {
    char cmd[1000] = "";
    strcat(cmd, "kill -STOP");
    strcat(cmd, command->args[1]);
    system(cmd);
  }
}

void mybg(struct command_t *command) {
   if (command->args[1] != NULL) {
    char cmd[1000] = "";
    strcat(cmd, "kill -CONT");
    strcat(cmd, command->args[1]);
    system(cmd);
  }
}

void myfg(struct command_t *command) {
   if (command->args[1] != NULL) {
    char cmd[1000] = "";
    strcat(cmd, "kill -CONT");
    strcat(cmd, command->args[1]);
    system(cmd);
  }
}

void alarm_command(struct command_t *command) {
  if (command->args[1] != NULL) {
    if (command-> args[2] != NULL) {
      char *time = command->args[1];
      char *filename = command->args[2];
      char *hour = strtok(time, ".");
      char *minute = strtok(NULL, ".");
      char cmd[100] = "";

      strcat(cmd, "(crontab -l ; echo \"");
      strcat(cmd, minute);
      strcat(cmd, " ");
      strcat(cmd, hour);
      strcat(cmd, "* * * /usr/bin/aplay ");
      strcat(cmd, filename);
      strcat(cmd, "\") | sort - | uniq - | crontab -");

      system(cmd);
    }
  }
}
