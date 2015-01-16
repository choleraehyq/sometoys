#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
//for gcc. clang can compile it successfully without the above line

#define MAXLINE 255
#define ARGSIZE 20
#define INNER_CMD_NUM 3

char inner_cmd[INNER_CMD_NUM][MAXLINE] = {"exit", "echo", "cd"};
//supported inner command

char cmdline[MAXLINE], *arglist[MAXLINE];
int argcount, cmdptr, pipeflag = 0;
//cmdptr: point to next char
//pipeflag: if a pipe has already opened, it is 1, else is 0.
int fd[2];
//fd[0] for read, fd[1] for write;

void execstring(void);
void myfree(void);
char getnextchar(void);
void ungetnextchar(void);
void getnextcmd(void);
void ig_space(void);
// ignore space and tab before next cmd
void execute(char *arglist[]);
void pipereadopen(void);
void pipewriteopen(void);
int iscmdend(int);
int innercmd(void);
void printarg(void);

int main(int argc, char **argv) {
		char curdir[MAXLINE];
		while (1) {
				if (getcwd(curdir, MAXLINE))
				fprintf(stdout, "%s>", curdir);
				bzero(cmdline, sizeof(cmdline));
				fgets(cmdline, MAXLINE, stdin);
				cmdptr = 0;
				execstring();
				myfree();
		}
		return 0;
}

void execstring(void) {
		char c = getnextchar();
		if (isalpha(c)) {
				ungetnextchar();
				getnextcmd();
//				printarg(); //tiaoshi
				if ((c = getnextchar()) != '<' && c != '>' && c != '|') {
						ungetnextchar();
						execute(arglist);
				}
		}
		if (c == '\n')
				return;
}

void myfree(void) {
		for (int i = 0;i <= argcount; i++) {
				free(arglist[i]);
				arglist[i] = NULL;
		}
}

char getnextchar(void) {
		return cmdline[cmdptr++];
}

void ungetnextchar(void) {
		cmdptr--;
}

void getnextcmd(void) {
		ig_space();
		int argptr = 0;
		argcount = -1;
		while (!iscmdend(cmdptr)) {
				argcount++;
				arglist[argcount] = (char *)calloc(ARGSIZE, sizeof(char));
				argptr = 0;
				while (cmdline[cmdptr] != ' ' && cmdline[cmdptr] != '\t') {
						if (cmdline[cmdptr] == '\n') {
								arglist[argcount][argptr] = '\0';
								cmdptr--;
								return;
						}
						arglist[argcount][argptr++] = cmdline[cmdptr++];
				}
				arglist[argcount][argptr] = '\0';
				ig_space();
		}
}

void ig_space(void) {
		while (cmdline[cmdptr] == ' ' || cmdline[cmdptr] == '\t')
				cmdptr++;
}

void execute(char *arglist[]) {
		if (innercmd()) 
				return;
		pid_t pid;
		pid = fork();
		switch(pid) {
				case -1:
						perror("fork():");
						exit(1);
				case 0:
						if (pipeflag)
								pipereadopen();
						execvp(arglist[0], arglist);
						perror("execute:");
						exit(1);
				default:
						waitpid(pid, NULL, 0);
		} // end of switch
}

void pipereadopen(void) {
		close(fd[1]);
		if ((dup2(fd[0], STDIN_FILENO)) != STDIN_FILENO) {
				perror("dup2 error to stdin:");
				exit(1);
		}
		close(fd[0]);
		pipeflag = 0;
}

void pipewriteopen(void) {
		close(fd[0]);
		if ((dup2(fd[0], STDOUT_FILENO)) != STDOUT_FILENO) {
				perror("dup2 error to stdout:");
				exit(1);
		}
		close(fd[1]);
		pipeflag = 1;
}

int iscmdend(int ptr) {
		char t = cmdline[ptr];
		if (t == ';' || t == '|' || t == '>' || t == '<')
				return 1;
		return 0;
}

int innercmd(void) {
		int flag = -1;
		for (int i = 0; i < INNER_CMD_NUM; i++) {
				if (strcmp(inner_cmd[i], arglist[0]) == 0) {
						flag = i;
						break;
				}
		}
		switch (flag) {
				case 0:
						exit(0);
				case 1:
						fprintf(stdout, "%s", arglist[1]);
						return 1;
				case 2:
						if (chdir(arglist[1]) == -1)
								perror("cd:");
						return 1;
				default:
						return 0;
		}
}

void printarg(void) {
		printf("%d ", argcount);
		for (int i = 0;i <= argcount; i++) 
				printf("%s ", arglist[i]);
		printf("\n");
}
