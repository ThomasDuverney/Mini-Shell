/*
 * Copyright (C) 2017, Florian Barrois, Thomas Duverney
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "readcmd.h"
#include <signal.h>

//void runCmd(struct cmdline * strCmd);
void traite_signal(int signal_recu) ;
void affiche_invite();
void executer_commandes(struct cmdline * strCmd);
void execute_commande_dans_un_fils(int numCmd,struct cmdline * strCmd,int ** descTable);
int executer_commandes_arriere_plan(struct cmdline * strCmd);

static pid_t globalPID;

int main()
{
	signal(SIGINT,traite_signal);
	signal(SIGTSTP,traite_signal);
	signal(SIGCHLD,traite_signal);
	while (1) {
		struct cmdline *l;
		affiche_invite();
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
    executer_commandes(l);

	}
}

void traite_signal(int signal_recu) {
			printf("valeur de global : %i",globalPID);
    switch (signal_recu) {
        case SIGINT :

					if(globalPID>0)
						kill(globalPID,SIGINT);
        	break;
				case SIGTSTP :
					printf("le pid de l'appelant %i",getpid());
					if(globalPID>0){
						printf("Je tue");
						fflush(stdout);
						kill(globalPID,SIGTSTP);
					}
					break;
				case SIGCHLD :
					printf("SIGCHLD détecté !\n");
					while(waitpid(-1,NULL, WNOHANG) > 0);
					break;
    		default:
          printf("Signal inattendu\n");
    }
}

void affiche_invite(void) {
    // insére le nom du répertoire courant
			char * chemin = malloc(50);
		 	chemin = getcwd(chemin,100);
			char * user = getenv("USER");
			printf("%s>",user);
			fflush(stdout);
			free(chemin);
}

void executer_commandes(struct cmdline * strCmd){
	if(!executer_commandes_arriere_plan(strCmd)){

		int **descTable = malloc(sizeof(int *)*strCmd->nbCmd-1); // Tableau de pipes
		for (int i = 0; i < strCmd->nbCmd; i++) {
				// on lance l'exécution de la commande dans un fils
				execute_commande_dans_un_fils(i,strCmd,descTable);
		}
		// Le père attends à la mort de tout ses fils
		while(waitpid(-1,NULL,0)>0);

		//On libère le talbeau de pipes
		for (int j=0; j<strCmd->nbCmd-1; j++)
				free(descTable[j]);
		free(descTable);

 }
}

void execute_commande_dans_un_fils(int numCmd,struct cmdline * strCmd,int ** descTable){
	// Si on a plusieurs commande
 if (numCmd != strCmd->nbCmd - 1) {
	 			// création d'un pipe dans le tableau de pipe
	 			descTable[numCmd] = malloc(sizeof(int)*2);
        if (pipe(descTable[numCmd]) == -1) {
            perror("Echec création tube");
            exit(1);
        }
 }
 // Création d'un fils pour executer la commande
 if ((globalPID = fork()) == 0){

	 // Si on a une redirection en entrée sur la première commande
	 if( numCmd==0 && strCmd->in != NULL ){
		 int fd = open(strCmd->in, O_RDONLY,S_IRUSR | S_IWUSR);
		 if(dup2(fd, STDIN_FILENO)<0){
			 printf("Erreur sur le dup2");
			 exit(1);
		 }
		 close(fd);
	 }
	 /* Si c'est la première commande, on redirige la sortie
	 		à l'entré du pipe*/
	 if(numCmd == 0){
		 if(numCmd != strCmd->nbCmd - 1){
			 dup2(descTable[numCmd][1], STDOUT_FILENO);
			 close(descTable[numCmd][1]);
			 close(descTable[numCmd][0]);
		 }
	/* Si c'est le dernier pipe alors on redirige uniquement
	 	 la sortie du pipe précédent sur son entrée */
	 }else if(numCmd ==  strCmd->nbCmd - 1){
		 dup2(descTable[numCmd-1][0],STDIN_FILENO);
		 close(descTable[numCmd-1][1]);
		 close(descTable[numCmd-1][0]);
	/* Sinon on redirige l'entrée et sortie des commandes
	 	avec le pipe précedent et le pipe actuel */
	 }else{
		 dup2(descTable[numCmd][1], STDOUT_FILENO);
		 dup2(descTable[numCmd-1][0], STDIN_FILENO);
		 close(descTable[numCmd-1][1]);
		 close(descTable[numCmd-1][0]);
		 close(descTable[numCmd][1]);
		 close(descTable[numCmd][0]);
	}
/*Si on est la dernière commande et qu'il y a une redirection
	en sortie */
	if(numCmd ==  strCmd->nbCmd-1 && strCmd->out != NULL){
		int fd = open(strCmd->out,O_WRONLY | O_CREAT | O_TRUNC ,S_IRUSR | S_IWUSR);
		if(dup2(fd, STDOUT_FILENO)<0){
			printf("Erreur sur le dup2 ");
			exit(1);
		}
		close(fd);
	}
	signal(SIGINT,SIG_DFL);
	signal(SIGTSTP,SIG_DFL);
	signal(SIGCHLD,SIG_DFL);
/* On execute la commande*/
	if (execvp(strCmd->seq[numCmd][0],strCmd->seq[numCmd]) < 0){
		printf("*** ERROR: exec failed\n");
		exit(1);
	}

/* Dans le père on ferme les pipes précédent au fur et à mesure
	 si on est pas la première commande*/
 }else{
	 if(numCmd!=0){
		 close(descTable[numCmd-1][1]);
		 close(descTable[numCmd-1][0]);
	 }
 }
}

int executer_commandes_arriere_plan(struct cmdline * strCmd){
/* Si il n'y a pas de commande suivante et que l'esperluette est présent
	dans la commande*/
	if( strCmd->seq[1] == NULL && strCmd->bg == 1){
			setpgid(getpid(),getpid()); // Change le groupe du processus pour pas être tué par son père
		  if (fork() == 0){
				if (execvp(strCmd->seq[0][0], strCmd->seq[0]) < 0)	{  // execute la commande
						printf("*** ERROR: exec failed\n");
						exit(1);
				}
			}else{
				waitpid(-1,NULL,strCmd->bg); // Le père n'attends pas son fils, WHNOANG
			}
			return 1;
 }else{return 0;}
}
