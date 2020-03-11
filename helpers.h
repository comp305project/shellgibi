#include <dirent.h>
#include <stdio.h>
#include <regex.h>

int match_count(char *start_pattern) {

}

void list_commands(char *start_pattern) {
  regex_t regex;
  int reti;
  int match_count = 0;
  char pattern[100] = "";

  sprintf(pattern, "^%s", start_pattern);
  regcomp(&regex, pattern, 0);

  DIR *dir;
  struct dirent *ent;
  dir = opendir("/usr/bin");
  while ((ent = readdir (dir)) != NULL) {
    if (!regexec(&regex, ent->d_name, 0, NULL, 0)) {
      match_count++;
      printf ("%s\n", ent->d_name);
    }
  }
  closedir (dir);
  regfree(&regex);
}

