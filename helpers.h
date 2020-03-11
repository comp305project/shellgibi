#include <dirent.h>
#include <stdio.h>
#include <regex.h>

struct array_struct {
  int count;
  char *array[500];
};

int match_count(char *start_pattern) {
  regex_t regex;
  int reti;
  int match_count = 0;

  regcomp(&regex, start_pattern, 0);

  DIR *dir;
  struct dirent *ent;
  dir = opendir("/usr/bin");
  while ((ent = readdir (dir)) != NULL) {
    if (!regexec(&regex, ent->d_name, 0, NULL, 0)) {
      match_count++;
    }
  }
  closedir (dir);
  regfree(&regex);

  return match_count;
}

struct array_struct list_commands(char *start_pattern) {
  int count;
  regex_t regex;
  int reti;
  char pattern[100] = "";

  struct array_struct result;
  int i = 0;

  sprintf(pattern, "^%s", start_pattern);
  regcomp(&regex, pattern, 0);
  count = match_count(pattern);

  DIR *dir;
  struct dirent *ent;
  dir = opendir("/usr/bin");
  while ((ent = readdir (dir)) != NULL) {
    if (!regexec(&regex, ent->d_name, 0, NULL, 0)) {
      result.array[i] = ent->d_name;
      i++;
    }
  }
  closedir (dir);
  regfree(&regex);

  result.count = match_count;
  return result;
}

