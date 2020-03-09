#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "helpers.h"


int main(int argc, char const *argv[]) {
  char *time = argv[1];
  char *filename = argv[2];
  char *hour = strtok(time, ".");
  char *minute = strtok(NULL, ".");

  char command[1000] = "";

  strcat(command, "(crontab -l ; echo \"");
  strcat(command, minute);
  strcat(command, " ");
  strcat(command, hour);
  strcat(command, " * * * /usr/bin/aplay ");
  strcat(command, filename);
  strcat(command, "\") | sort - | uniq - | crontab -");

  system(command);

  return 0;
}
