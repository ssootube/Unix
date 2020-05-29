#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#define MAX_CMD_ARG 30

const char *prompt = "myshell> ";
char* cmdvector[MAX_CMD_ARG];
char cmdline[BUFSIZ];

void fatal(char *str){
	perror(str);
	exit(1);
}

int makelist(char *s, const char* delimiters, char** list, int MAX_LIST){
	int i = 0;
	int numtokens =0;
	char *snew =NULL;

	if( (s==NULL)|| (delimiters ==NULL)) return -1;

	snew = s+ strspn(s, delimiters);
	if( (list[numtokens]=strtok(snew, delimiters)) ==NULL)
		return numtokens;

	numtokens =1;

	while(1){
		if( (list[numtokens] = strtok(NULL, delimiters)) == NULL)
			break;
		if(numtokens == (MAX_LIST-1)) return -1;
		numtokens++;
	}
	return numtokens;
}
char oldpath[BUFSIZ];
void my_cd(char* path){
char temp[BUFSIZ];
	getcwd(temp,BUFSIZ);
	if(path==NULL || !strcmp("~",path)) chdir(getenv("HOME"));
	else if(!strcmp("-",path)) chdir(oldpath);
	else chdir(path);
	strcpy(oldpath,temp);
}

int background = 0;
int check_background(char *cmdline){
int i;
for(i=0;i<strlen(cmdline);++i)
	if(cmdline[i] == '&'){
	cmdline[i] = ' ';
	return 1;
	}
	return 0;
}

pid_t pid;
void background_signal(int sig){
if(pid==waitpid(-1,(int*)NULL,WNOHANG)) pid = -1;
}

int cmdvector_check(char** cmdvector,char* check){
	for(int i = 0; cmdvector[i] != NULL; ++i){
		if(!strcmp(cmdvector[i],check)){
			return i;
		}
}
return -1;
}
int check_redirection(char** cmdvector,char* In_Out){
 		int fd;
		int i =	cmdvector_check(cmdvector,In_Out);
		if(i == -1) return 0;
		if(cmdvector[i+1] == NULL) return -1;
		else if((fd = open(cmdvector[i+1],O_RDWR | O_CREAT, 0644)) == -1){
			fatal(cmdvector[i+1]);
		}
		if(!strcmp(In_Out,">"))	dup2(fd,STDOUT_FILENO);
		else 	dup2(fd,STDIN_FILENO);
		close(fd);
		cmdvector[i] = NULL; cmdvector[i+1] = NULL;
		for(i= i+2 ; cmdvector[i] != NULL; ++i){
			cmdvector[i-2] = cmdvector[i];
			cmdvector[i] = NULL;
		}
		return 0;
}

int check_pipe(char** _cmdvector){
	pid_t _pid;
	int fd[2];
	int i = 0;
	while((i = cmdvector_check(_cmdvector,"|")) != -1){
		char* pipe_child[i+1];
		for(int k = 0 ;_cmdvector[k] != NULL ; ++k){
			if(k<i)	pipe_child[k] = _cmdvector[k];
			else if(k == i) 	pipe_child[k] = NULL;
			else continue;
		}
		int t = 0;
		for(i = i+1;_cmdvector[i] != NULL ; ++i){
			 _cmdvector[t] = _cmdvector[i];
			 _cmdvector[i] = NULL;
			 t++;
		}
		_cmdvector[t] = NULL;
		pipe(fd);
		_pid = fork();
		switch(_pid){
			case -1 : fatal("fork()");
			case 0:
			check_redirection(pipe_child,"<");

			dup2(fd[1],STDOUT_FILENO);
			close(fd[0]);
			close(fd[1]);
			execvp(pipe_child[0],pipe_child);
			exit(0);

			default:
			dup2(fd[0],STDIN_FILENO);
			close(fd[1]);
			close(fd[0]);
		}
	}

	  	check_redirection(_cmdvector,">");
			check_redirection(_cmdvector,"<");
			execvp(_cmdvector[0],_cmdvector);
			exit(0);
	}
int main(int argc, char **argv){
	signal(SIGINT,SIG_IGN);
	signal(SIGQUIT,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGTTOU,SIG_IGN);
	int i = 0;
	struct sigaction act;
	act.sa_handler = background_signal;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGCHLD,&act,0);
	while(1){
		fputs(prompt,stdout);
		start:
		fgets(cmdline,BUFSIZ,stdin);
		if(!strchr(cmdline,'\n')) goto start;
		cmdline[ strlen(cmdline)-1] = '\0';
		background = check_background(cmdline);
		makelist(cmdline," \t",cmdvector,MAX_CMD_ARG);

		if(cmdvector[0] == NULL) continue;
		if(!strcmp(cmdvector[0],"exit")) return 0;
		else if(!strcmp(cmdvector[0],"cd")){
		       	my_cd(cmdvector[1]);
			continue;
		}

		switch(pid=fork()){
		case 0:
		setpgid(0,0);
		signal(SIGQUIT,SIG_DFL);
		signal(SIGINT,SIG_DFL);
		signal(SIGTSTP,SIG_DFL);
		if(!background)
			tcsetpgrp(STDIN_FILENO,getpgid(0));
		check_pipe(cmdvector);

		fatal("main()");
		case -1:
			fatal("main()");
		default:
			if(!background){
				while(1){
				if(pid != -1){
		if(pid ==  waitpid(pid,(int*)NULL,WUNTRACED)) break;
				}
				else break;
				}
			tcsetpgrp(STDIN_FILENO,getpgid(0));
			}
		}
		fflush(NULL);
	}
	return 0;
}
