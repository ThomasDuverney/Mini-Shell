/*
 * Copyright (C) 2002, Florian Barrois, Thomas Duverney
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "readcmd.h"
#include <signal.h>

void runCmd(struct cmdline * strCmd);
void traiteSignal(int signal_recu) ;
pid_t globalPID;

int main()
{
	signal(SIGINT, traiteSignal);
	signal(SIGTSTP, traiteSignal);
	while (1) {
		struct cmdline *l;
		int i, j;

		printf("shell> ");
		l = readcmd();

		/* If input stream closed, normal termination */
		if (!l) {
			printf("exit\n");
			exit(0);
		}

		if (l->err) {
			/* Syntax error, read another command */
			printf("error: %s\n", l->err);
			continue;
		}

		if (l->in) printf("in: %s\n", l->in);
		if (l->out) printf("out: %s\n", l->out);
		if (l->nbCmd) printf("nb: %i\n", l->nbCmd);

		/* Display each command of the pipe */
		for (i=0; l->seq[i]!=0; i++) {
			char **cmd = l->seq[i];
			printf("seq[%d]: ", i);
			for (j=0; cmd[j]!=0; j++) {
				printf("%s ", cmd[j]);
			}
			printf("\n");
		}

        runCmd(l);
	}
}

void runCmd(struct cmdline * strCmd){
    pid_t child_pid;
    //int child_status;
	if(strCmd->seq[1] == NULL){ // Une seule commande

	    if ((child_pid = fork()) == 0){
				signal(SIGINT, SIG_DFL);
				signal(SIGTSTP, SIG_DFL);
		        if(strCmd->out != NULL)
				{
					int fd = open(strCmd->out,O_WRONLY | O_CREAT | O_TRUNC ,S_IRUSR | S_IWUSR);
					if(dup2(fd, STDOUT_FILENO)<0){
						printf("Erreur sur le dup2 ");
						exit(1);
					}
					close(fd);

		        }

				if(strCmd->in != NULL)
				{

					int fd = open(strCmd->in, O_RDONLY,S_IRUSR | S_IWUSR);
					if(dup2(fd, STDIN_FILENO)<0){
						printf("Erreur sur le dup2 ");
						exit(1);
					}
					close(fd);
				}

				if(strCmd->bg == 1){
					setpgid(getpid(),getpid());
				}


		        if (execvp(strCmd->seq[0][0], strCmd->seq[0]) < 0)
				{     /* execute la commande  */
		            printf("*** ERROR: exec failed\n");
		            exit(1);
		        }
		}
		else{
			waitpid(-1,NULL,strCmd->bg);
		}
	}
	else{ // Plusieurs commandes avec pipe
			 //int p[2];
			 //int ** desctable = malloc(sizeof(int*)*10);
			 int desctable[strCmd->nbCmd-1][2];
			 int i =0;
			 pid_t pid = getpid();

			 while(strCmd->seq[i]!=0){
				 pipe(desctable[i]);
				//pipe(p);
				//desctable[i] = p;

				if ((pid = fork()) == 0)
				{
						// DANS LE CAS D'UNE REDIRECTION EN ENTRÉE
						if( i==0 && strCmd->in != NULL ){
							int fd = open(strCmd->in, O_RDONLY,S_IRUSR | S_IWUSR);
							if(dup2(fd, STDIN_FILENO)<0){
								printf("Erreur sur le dup2");
								exit(1);
							}
							close(fd);
						}else{
							if(i!=0){
								dup2(desctable[i-1][0],STDIN_FILENO);
								close(desctable[i-1][0]);
								close(desctable[i-1][1]);
							}
						}


						if(strCmd->seq[i+1] != NULL){
							dup2(desctable[i][1], STDOUT_FILENO);
						}else{
							// DANS LE CAS D'UNE REDIRECTION EN SORTIE
							if( strCmd->out != NULL){
								int fd = open(strCmd->out,O_WRONLY | O_CREAT | O_TRUNC ,S_IRUSR | S_IWUSR);
								if(dup2(fd, STDOUT_FILENO)<0){
									printf("Erreur sur le dup2 ");
									exit(1);
								}
								close(fd);
							}
						}

						if (execvp(strCmd->seq[i][0],strCmd->seq[i]) < 0)
						{     /* execute la commande  */
							printf("*** ERROR: exec failed\n");
							exit(1);
						}
			 }else{
				if(i!=0){
					close(desctable[i-1][1]);
					close(desctable[i-1][0]);
				}
				i++;
			 }
		  }
		  close(desctable[i-1][1]);
		  close(desctable[i-1][0]);
		  if(pid != 0){
				while(waitpid(-1,NULL,0)>0);

		  }
	}

}

void traiteSignal(int signal_recu) {

    switch (signal_recu) {
        case SIGINT :
					printf("Cédric est un dieu");
			//	if(globalPID>0)
				//	kill(globalPID,SIGINT);
        break;
				case SIGTSTP :
					printf("Cédric est un dieu");
			//	if(globalPID>0)
			//		kill(globalPID,SIGTSTP);
				break;
    		default:
          printf("Signal inattendu\n");
    }
}
