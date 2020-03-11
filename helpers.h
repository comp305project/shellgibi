#include <dirent.h>
#include <stdio.h>
#include <regex.h>
#include <string.h>

void list_working_dir() {
  DIR *d;
  struct dirent *dir;
  d = opendir(".");
  if (d) {
    while ((dir = readdir(d)) != NULL) {
      printf("%s\n", dir->d_name);
    }
    closedir(d);
  }
}

int match_count(char *start_pattern) {
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
      if (strcmp(start_pattern, ent->d_name) == 0) {
        return 1;
      }
      match_count++;
    }
  }
  closedir (dir);
  regfree(&regex);

  return match_count;
}

char* list_commands(char *start_pattern) {
  int count;
  regex_t regex;
  int reti;
  char pattern[100] = "";

  char *result;

  sprintf(pattern, "^%s", start_pattern);
  regcomp(&regex, pattern, 0);
  count = match_count(start_pattern);

  DIR *dir;
  struct dirent *ent;
  dir = opendir("/usr/bin");
  while ((ent = readdir (dir)) != NULL) {
    if (!regexec(&regex, ent->d_name, 0, NULL, 0)) {
      if (count > 1) {
        if (strcmp(start_pattern, ent->d_name) == 0) {
          // return the command if there is an exact match
          return ent->d_name;
        }
        printf("%s\n", ent->d_name);
      } else if (count == 1) {
        result = ent->d_name;
        if (strcmp(result, start_pattern) == 0) {
          return result;
        }
      }
    }
  }
  closedir (dir);
  regfree(&regex);

  if (count != 1)
    return "";

  return result;
}

