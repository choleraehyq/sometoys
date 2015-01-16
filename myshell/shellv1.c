#include<stdio.h>
#include<sys/wait.h>
#include<string.h>
#include<unistd.h>
#include<signal.h>
#include<stdlib.h>
#include<sys/types.h>
#include<dirent.h>

#define MAXSIZE 255
#define ARGSIZE 20
#define INTER_CMD_NUM 3 // number of supported internal commands 

char inter_cmd[INTER_CMD_NUM][ARGSIZE] = {"exit", "echo", "cd"};

char *arglist[ARGSIZE];
int argcount;

void parserline(char *cmd) {
		int len = strlen(cmd), ptr = 0, argptr = 0;
		argcount = 0;
		while (cmd[ptr++] == ' ');
		if (ptr) ptr--;
		if (cmd[ptr] == '\n') {
				argcount = -1;
				return;
		}
		arglist[argcount] = (char *)calloc(ARGSIZE, sizeof(char));
		for (;ptr < len;ptr++) {
				if (cmd[ptr] != ' ' && cmd[ptr] != '\n') {
					arglist[argcount][argptr++] = cmd[ptr];	
				} else if (cmd[ptr] == '\n') {
						arglist[argcount][argptr] = '\0';
						return;
				}
				else { // cmd[ptr] == ' '
						arglist[argcount++][argptr] = '\0';
				arglist[argcount] = malloc(ARGSIZE);
						argptr = 0;
				}
		}
}

int  internal_cmd(void) {
		int flag = -1;
		for (int i = 0; i < INTER_CMD_NUM; i++) {
				flag = -1;
				for (int j = 0; j < strlen(inter_cmd[i]); j++)
						if (arglist[0][j] != inter_cmd[i][j]) {
								flag = -2;
								break;
						}
				if (flag != -2) {
						flag = i;
						break;
				}
		}				// end of for-loop i
		switch (flag) {
				case 0:
						exit(0);
				case 1:
						fprintf(stdout, "%s", arglist[1]);
						return 1;
				case 2:
						if (chdir(arglist[1]) == -1) 
								fprintf(stderr, "change dir error\n");
						return 1;
				default:
						return 0;
		}
}

void my_free(void) {
		for (int i = 0;i <= argcount;i++) {
				free(arglist[i]);
				arglist[i] = NULL;
		}
		argcount = 0;
}
int main(int argc, char *argv[]) {
		char cmd[MAXSIZE];
		pid_t pid;
		while (1) {
				fprintf(stdout, ">");
				for (int i = 0;i < MAXSIZE;i++)
						cmd[i] = 0;
				fgets(cmd, MAXSIZE, stdin);
				parserline(cmd);
				if (argcount == -1) continue;
				if (internal_cmd()) {
						my_free();
						continue;
				}
				pid = fork();
				switch(pid) {
						case -1 :
								fprintf(stderr, "fork error\n");
								exit(1);
						case 0 :
								execvp(arglist[0], arglist);
								fprintf(stderr, "execute error\n");
								exit(1);
						default :
								waitpid(pid, NULL, 0);
				} //end of switch
				my_free();
		}
}
