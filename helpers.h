
// Removes file names and symbols related to I/O redirection from command arguments
//  otherwise arguments cause errors since we pass them directly
char** remove_file_args(char **args) {
    char **new_args = malloc(sizeof(args));
    int new_args_index = 0;

    int len = sizeof(args) / sizeof(args[0]);
    for (int i = 0; i < len; i++) {
        if (args[i] != ">" || args[i] != ">>" || args[i] != "<") {
            new_args[new_args_index++] = args[i];
        }
    }
    return new_args;
}
