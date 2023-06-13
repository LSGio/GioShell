
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_COMMAND_LENGTH 100
#define MAX_NUMBER_OF_COMMANDS 100

int HISTORY_INDEX = 0;
char HISTORY_BUFFER[MAX_NUMBER_OF_COMMANDS][MAX_COMMAND_LENGTH + 1] = { '\0' };
int HISTORY_PID_BUFFER[MAX_NUMBER_OF_COMMANDS] = { 0 };
char SPACE_DELIMITER_STRING[2] = {' ', '\0'};
char COMMAND_DELIMITERS[4] = {' ', '\n', '\t', '\0'};
char EXIT_COMMAND[5] = {'e', 'x', 'i', 't', '\0'};
char HISTORY_COMMAND[8] = {'h', 'i', 's', 't', 'o', 'r', 'y', '\0'};
char CD_COMMAND[3] = {'c', 'd', '\0'};
char PATH_VAR_NAME[5] = {'P', 'A', 'T', 'H', '\0'};
char PATH_DELIMITER[2] = {':', '\0'};

void append_argv_to_path(int argc, char** argv);
void append_to_history(char command[], pid_t pid);
void print_prompt(void);
void print_history();
void trim_whitespaces(char command[]);
int is_empty_command(char command[]);
int get_number_of_arguments(char command[]);
void execute_command(char command[], int argc, char** argv);

int main(int argc, char* argv[]) {

	append_argv_to_path(argc, argv);

    while (HISTORY_INDEX < MAX_NUMBER_OF_COMMANDS) {
		char current_command_buffer[MAX_COMMAND_LENGTH + 1] = { '\0' };
        print_prompt();
        fgets(current_command_buffer, MAX_COMMAND_LENGTH, stdin);
		trim_whitespaces(current_command_buffer);
		if(is_empty_command(current_command_buffer))
			continue;
        execute_command(current_command_buffer, argc, argv);
		HISTORY_INDEX++;
    }
	exit(0);
    
}

void append_argv_to_path(int argc, char** argv) {
	
	if (argc < 2) {

		// no arguments supplied
		return;

	}

	char* current_path = getenv(PATH_VAR_NAME);
	int new_path_len = strlen(current_path);
	for (int i = 1; argv[i] != NULL; i++) {

		new_path_len += 1;
		new_path_len += strlen(argv[i]);

	}

	new_path_len++;
	char* new_path = (char*) malloc(new_path_len * sizeof(char));
	strcpy(new_path, current_path);

	for (int i = 1; argv[i] != NULL; i++) {

		strcat(new_path, PATH_DELIMITER);
		strcat(new_path, argv[i]);

	}

	new_path[new_path_len - 1] = '\0';
	setenv(PATH_VAR_NAME, new_path, 1);
	free(new_path);
	return;

}

void append_to_history(char current_command_buffer[MAX_COMMAND_LENGTH + 1], pid_t pid) {

	strncpy(HISTORY_BUFFER[HISTORY_INDEX], current_command_buffer, MAX_COMMAND_LENGTH);
	HISTORY_PID_BUFFER[HISTORY_INDEX] = pid;

}

void print_prompt(void) {

    printf("$ ");
    fflush(stdout);

}

void print_history() {

	for(int i = 0; i <= HISTORY_INDEX; i++) {
		printf("%d %s\n", HISTORY_PID_BUFFER[i], HISTORY_BUFFER[i]);
	}

}

void trim_whitespaces(char current_command_buffer[MAX_COMMAND_LENGTH + 1]) {

	char current_command_copy[MAX_COMMAND_LENGTH + 1] = { '\0' };
	char trimmed_command_buffer[MAX_COMMAND_LENGTH + 1] = { '\0' };
	strncpy(current_command_copy, current_command_buffer, MAX_COMMAND_LENGTH);

	char* token = strtok(current_command_copy, COMMAND_DELIMITERS);
	while(token != NULL) {
		strcat(trimmed_command_buffer, token);
		strcat(trimmed_command_buffer, SPACE_DELIMITER_STRING);
		token = strtok(NULL, COMMAND_DELIMITERS);
	}
	strncpy(current_command_buffer, trimmed_command_buffer, MAX_COMMAND_LENGTH);

}

int is_empty_command(char current_command_buffer[MAX_COMMAND_LENGTH + 1]) {

	char current_command_copy[MAX_COMMAND_LENGTH + 1] = { '\0' };
	strncpy(current_command_copy, current_command_buffer, MAX_COMMAND_LENGTH);
	return strtok(current_command_copy, COMMAND_DELIMITERS) == NULL;

}

int get_number_of_arguments(char current_command_buffer[MAX_COMMAND_LENGTH + 1]) {

	char current_command_copy[MAX_COMMAND_LENGTH + 1] = { '\0' };
	strncpy(current_command_copy, current_command_buffer, MAX_COMMAND_LENGTH);

	int number_of_arguments = 0;
	char* token = strtok(current_command_copy, COMMAND_DELIMITERS);
	while (token != NULL) {
		token = strtok(NULL, COMMAND_DELIMITERS);
		number_of_arguments++;
	}
	return number_of_arguments;
	
}

void execute_command(char current_command[MAX_COMMAND_LENGTH + 1], int argc, char** argv) {

	// if we get here, command is not empty and we need to execute it

	// check if it's a built=in command
	char current_command_copy[MAX_COMMAND_LENGTH + 1] = { '\0' };
	strncpy(current_command_copy, current_command, MAX_COMMAND_LENGTH);
	char* cmd = strtok(current_command_copy, COMMAND_DELIMITERS);
	
	if (!strcmp(cmd, EXIT_COMMAND)) {

		// No need to add exit to the history
		exit(0);
		return;

	}
	
	if (!strcmp(cmd, HISTORY_COMMAND)) {

		append_to_history(current_command, getpid());
		print_history(HISTORY_INDEX);
		return;

	}

	if(!strcmp(cmd, CD_COMMAND)) {

		append_to_history(current_command, getpid());
		char* path = strtok(NULL, COMMAND_DELIMITERS);
		if (chdir(path)) {
			perror("cd failed");
		}
		return;

	}

	// if we get here, it's an external command and the real fun begins...

	pid_t pid = fork();
	int status;

	if (pid < 0) {

		perror("fork failed");
		return;

	}

	if (pid == 0) {

		// Child process here
		current_command_copy[MAX_COMMAND_LENGTH] = '\0';
		strncpy(current_command_copy, current_command, MAX_COMMAND_LENGTH);

		// allocate memory for args and prepare array of args
		int number_of_arguments = get_number_of_arguments(current_command);

		// The list of arguments must be NULL terminated, so we add 1 to the allocation.
		char** args = (char**) malloc((number_of_arguments + 1) * sizeof(char*));

		// Adding the NULL terminator
		args[number_of_arguments] = NULL;

		// Allocating memory for each string in the args array and copying the argument
		char* token = strtok(current_command_copy, COMMAND_DELIMITERS);
		for (int i = 0; token != NULL; i++) {
			args[i] = (char*) malloc(sizeof(char) * (strlen(token) + 1));
			strcpy(args[i], token);
			token = strtok(NULL, COMMAND_DELIMITERS);
		}

		execvp(args[0], args);
		// exec returns only if error occured, so no need to check for it...
		perror("exec failed");
		exit(-1);

	} else {

		// Parent process here
		append_to_history(current_command, pid);
		wait(&status);
		return;

	}
	
}