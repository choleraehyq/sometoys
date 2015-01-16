#include<stdio.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/wait.h>

int main(int argc, char **argv) {
		pid_t pid;
		char line[255];
		fprintf(stdout, "aaa\n");
		fgets(line, 255, stdin);
		pid = fork();
		if (pid == 0) {
				printf("shit\n");
		}
		else wait(NULL);
		return 0;
}
