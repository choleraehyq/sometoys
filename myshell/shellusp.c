#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<limits.h>
#include<dirent.h>

#define QUIT_STRING "exit"
#define FFLAG (O_WRONLY | O_CREAT | O_TRUNC)
#define FMODE (S_IRUSR | S_IWUSR)

void perror_exit(char *s);
int makeargv(const char *s, const char *delimiters, char ***argvp);
int parseandredirectin(char *s);
int parseandredirectout(char *s);
void executeredirect(char *s, int in, int out);
void executecmd(char *incmd);

int main(void) {
		pid_t childpid;
		char inbuf[255];
		int len;
		char curdir[255];

		while (1) {
				if (getcwd(curdir, 255))
						 fprintf(stdout, "%s>", curdir);
				if (fgets(inbuf, 255 , stdin) == NULL)
						continue;
				len = strlen(inbuf);
				if (inbuf[len-1] == '\n') 
						inbuf[len-1] = 0;
				if (strcmp(inbuf, QUIT_STRING) == 0)
						break;
				if ((childpid = fork()) == -1) {
						perror("Failed to fork child");
						continue;
				} 
				if (childpid == 0) {
						executecmd(inbuf);
						return 1;
				} else { 
				//		waitpid(childpid, NULL, 0);
						sleep(1);
				}
		}
		return 0;
}

void perror_exit(char *s) {
		perror(s);
		exit(1);
}

int parseandredirectin(char *cmd) {
		int error;
		int infd;
		char *infile;
		if ((infile = strchr(cmd, '<')) == NULL) 
				return 0;
		*infile = 0;
		infile = strtok(infile+1, " \t");
		if (infile == NULL)
				return 0;
		if ((infd = open(infile, O_RDONLY)) == -1)
				return -1;
		if (dup2(infd, STDIN_FILENO) == -1) {
				error = errno;
				close(infd);
				errno = error;
				return -1;
		}
		return close(infd);
}

int parseandredirectout(char *cmd) {
		int error;
		int outfd;
		char *outfile;

		if ((outfile = strchr(cmd, '>')) == NULL)
				return 0;
		*outfile = 0;
		outfile = strtok(outfile+1, " \t");
		if (outfile == NULL) 
				return 0;
	 	if ((outfd = open(outfile, FFLAG, FMODE)) == -1)
				return -1;
		if (dup2(outfd, STDOUT_FILENO) == -1) {
				error = errno;
				close(outfd);
				errno = error;
				return -1;
		}
		return close(outfd);
}

void executeredirect(char *s, int in, int out) {
		char **chargv;
		char *pin;
		char *pout;

		if (in && (pin = strchr(s, '<')) != NULL &&
			out && (pout = strchr(s, '>')) != NULL && (pin > pout)) {
				if (parseandredirectin(s) == -1) {
						perror("Failed to redirect input");
						return;
				}
				in = 0;
		}
		if (out && (parseandredirectout(s) == -1)) 
				perror_exit("Failed to redirect output");
		else if (in && (parseandredirectin(s) == -1))
				perror_exit("Failed to redirect input");
		else if (makeargv(s, " \t", &chargv) <= 0)
				fprintf(stderr, "Failed to parse command line\n");
		else {
				execvp(chargv[0], chargv);
				perror("Failed to execute command");
		}
		exit(1);
}

void executecmd(char *cmds) {
		int child;
		int count;
		int fds[2];
		int i;
		char **pipelist;

		count = makeargv(cmds, "|", &pipelist);
		if (count <= 0) {
				fprintf(stderr, "Failed to find any commands\n");
				exit(1);
		}
		for (i = 0;i < count-1; i++) {
				if (pipe(fds) == -1) 
						perror_exit("Failed to create pipes");
				else if ((child = fork()) == -1)
						perror_exit("Failed to create process");
				else if (child) { //Parent
						if (dup2(fds[1], STDOUT_FILENO) == -1)
								perror_exit("Failed to connect pipeline");
						executeredirect(pipelist[i], i==0, 0);
						exit(1);
				}
				if (dup2(fds[0], STDIN_FILENO) == -1) 
						perror_exit("Failed to connect next pipeline");
				if (close(fds[0]) || close(fds[1]))
						perror_exit("Failed to do final close");
		}
		executeredirect(pipelist[i], i==0, 1);
		exit(1);
}

int makeargv(const char *s, const char *delimiters, char ***argvp) {
		int error;
		int i;
		int numtokens;
		const char *snew;
		char *t;

		if ((s == NULL) || (delimiters == NULL) || (argvp == NULL)) {
				errno = EINVAL;
				return -1;
		}
		*argvp = NULL;
		snew = s + strspn(s, delimiters);
		if ((t = malloc(strlen(snew) + 1)) == NULL) 
				return -1;
		strcpy(t, snew);
		numtokens = 0;
		if (strtok(t, delimiters) != NULL) 
				for (numtokens = 1; strtok(NULL, delimiters) != NULL;numtokens++);
		if ((*argvp = malloc((numtokens + 1) * sizeof(char *))) == NULL) {
				error = errno;
				free(t);
				errno = error;
				return -1;
		}

		if (numtokens == 0) 
				free(t);
		else {
				strcpy(t, snew);
				**argvp = strtok(t, delimiters);
				for (i = 1;i < numtokens; i++)
						*(*(argvp) + i) = strtok(NULL, delimiters);
		}
		*((*argvp) + numtokens) = NULL;
		return numtokens;
}
