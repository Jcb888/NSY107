/****************************************************************)
(*	SUPAERO														*)
(*																*)
(*	Application roue a reaction									*)
(*	Os = linux+xenomai											*)
(* Jean-Christophe BILLARD - Jerome LEBORGNE - Hugo TOURAINE	*)
(*																*)
(*	tachesRoue.c												*)
(****************************************************************/

/* Includes */
#include <math.h>
#include <stdio.h>
#include <termios.h> 
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h> 
#include <sys/fcntl.h> 
#include <sys/time.h> 
#include <native/task.h> 
#include <native/timer.h> 
#include <native/gueue.h> 
#include <native/sem.h> 
#include <native/event.h> 
#include <native/mutex.h> 
#include <rtdk.h>
#include "libUtilRoue.h" 
#include "socketsTCP.h"

/*********************************************************/
/* declarations de variables globales */
/*********************************************************/
/* declaration de constantes */
# define
/* declaration des identificateurs des taches */
RTTASK idTDialogue;
/* Identificateurs des semaphores pour synchronisation */
/* 	*/
/* 	 A completer 	 */
RTSEM idSemXX;
/* semaphores d'exclusion mutuelle */
/*	*/
/* 	 A completer 	 */
RTMUTEX idMutexXX; 
/* Identificateurs des files
/*
/* 	 A completer 	 */
RTQUEUE idFileXX; 


/* variables pour communication et synchronisation entre taches par memoire commune */

typedef struct
int loi;	/* N loi (0 boucle ouverte*/
int option; /* option d'experimentation */
int duree;
int periodeLectureCapteurs; /*  en ms */
int periodeActivationLoi; /* en ms */
int profilConsigne;
float valFinConsigne;
int dureeMonteeConsigne;	/* en ms */
float coeffLoi[15];
} TypeParametresExperimentation;

TypeParametresExperimentation parametresExperimentation;

/* infosEtalonnage declare dans libUtilRoue.c */
extern TypeEtalon infosEtalonnage;

/*******************************************************************************
* Les sous programmes utilises par les taches
*
*
*******************************************************************************/
/**************************************/
void envoyerValeursCourantesCapteurs()
/**************************************/
TypeEchantillons echantillon;
float tabCapteurs[4];
acquisitionEchantillon(&echantillon);
tabCapteurs[0] = echantillon.vitesseMoteur;;
tabCapteurs[1] = echantillon.vitessePlateau;
tabCapteurs[2] = echantillon.positionPlateau;
tabCapteurs[3] = echantillon.courantMoteur;
ecrireTableauReels('T', tabCapteurs, 4);
/**************************************/
void envoyerDernierBlocEchantillons()
/**************************************/
float blocCapteurs[50 * 4];
int i;
/* simulation de l'envoi de 10 echantillons constants */
/*	a modifier ... */
for (i = 0; i < 50; i++){
blocCapteurs[(i * 4)] = 100;
blocCapteurs[(i * 4) + 1] = 1;
blocCapteurs[(i * 4) + 2] = 0.5;
blocCapteurs[(i * 4) + 3] = 0.2;
}
ecrireTableauReels('S', blocCapteurs, 50 * 4);


/***************************************************************/
void miseAJourparametresExperimentation(float tableauReels[])
/***************************************************************/
Int i;
parametresExperimentation.loi = tableauReels[0];
parametresExperimentation.option = tableauReels[1];
tableauReels[2] = tableauReels[2] * 1000.0;
parametresExperimentation.duree = tableauReels[2];
tableauReels[3] = tableauReels[3] * 1000.0;
parametresExperimentation.periodeLectureCapteurs = tableauReels[3];
tableauReels[4] = tableauReels[4] * 1000.0;
parametresExperimentation.periodeActivationLoi = tableauReels[4];
parametresExperimentation.profilConsigne = tableauReels[5];
parametresExperimentation.valFinConsigne = tableauReels[6];
tableauReels[7] = tableauReels[7] * 1000.0; // MAJ format du tps
parametresExperimentation.dureeMonteeConsigne = tableauReels[7];
for (i = 0; i < 15; I++){
	parametresExperimentation.coeffLoi[i] = tableauReels[8 + i];
}

rt_printf(" d %d , pl %d, pe %d \n", parametresExperimentation.duree, parametresExperimentation.periodeActivationLoi, parametresExperimentation.periodeLectureCapteurs);
void DisplayInfoTask(void)
RT_TASK tid;
RTTASK_INFO InfosTache;
int cr;
cr = rt_task_inguire(rt_task_self(), &InfosTache);
if (cr == 0){
	rt_printf("Nom Tache : %s, Priorite : %d, texec MP %d \n", InfosTache.name, InfosTache.bprio, InfosTache.exectime);
	rt_printf("Commutations : %d , Mode : %x\n", InfosTache.modeswitches, InfosTache.status);
}
else{
	rt_printf("Erreur de lecture de 1 etat de la tache (numero %d )\n", cr);
}
/*******************************************************************************
*
Les TACHES
*
*******************************************************************************/
/* 	 A completer 	 */


/********************************************************/
void tDialogue(){
	/***************************************************'*****/
#define MESS_ACK 'A' /* message d'acquittement */

	float tableauReels[TAILLE_MAX_TABLEAU_REELS];
	short int nbReelsLus;
	char typeMessage, caractereMessage;
	int socketServeur;
	int numeroPort = 7800;
	int erreur; /* indicateur d erreur */

	/* boucle de lecture des messages */
	for (;;) {
		erreur = 0;
		/* ouverture socket serveur */
		socketServeur = ouvrirSocketServer(numeroPort);
		rt_printf(" port %d ouvert\n", numeroPort);
		if (socketServeur == ERROR) erreur = l;
		/* Initialisations a effectuer en debut de chaque connexion */
		/* boucle de lecture des requetes */
		rt_printf(" connexion Active \n");
		while (erreur == 0) {
			typeMessage = lectureMessage();
				if (typeMessage < 0) { /* erreur de lecture socket */
					printf(" erreur de lecture socket\n");
					erreur = l;
				}
				else
				{
					switch (typeMessage){

						case TYPE CAR :
						{
							caractereMessage = retourneCaractereLu();

							switch (caractereMessage){

								case 'A': /* arret experimentation */
								{
											  /* 	 A completer 	 */
											  ecrireCaractere(MESS_ACE);
								} break;

								case 'D': /* debut experimentation */
								{	/* 	 A completer 	 */
											  ecrireCaractere(MESS_ACK);
								} break;

								case 'C':
								{	/* lecture valeurs courantes capteurs */
											envoyerValeursCourantesCapteurs();
								} break;
								case 'B': /* Lecture bloc des derniers echantillons */
								{
											  envoyerDernierBlocEchantillons();
								}

							} /* Fin switch (caractereMessage) */
						} break;

						case TYPE_TABLEAU_REELS:
						{
						retourneTableauReelsLu(tableauReels, &nbReelsLus);
						caractereMessage = retourneCaractereLu();
							switch (caractereMessage)
							{
								case 'E': /* parametres d'etalonnage */
								{
												miseAJourInfosEtalonnage(tableauReels);
												ecrireCaractere(MESS_ACK);
								} break;

								case 'L': /* parametres loi de commande */
								{
												miseAJourparametresExperimentation(tableauReels);
												ecrireCaractere(MESS_ACK);
								}
							 }/* Fin switch (caractereMessage) */
						}/*fin case tableau reels*/
				} /* Fin switch (typeMessage) */
			} /* fin du else erreur*/
		} /* fin du while erreur==0 */

	fermetureSocket();
	rt_printf(" port %d ferme \n", numeroPort);
	} /*fin for (;;) */
}/*fin tDialogue*/

void catch_signal(int sig)
printf(" Signal = %d\n", sig);
/*******************************************************************************
*
* main()
*
* Description: cree puis detruit tous les elements composant l'application
*******************************************************************************/
int main(int argc, char* argv[]){
	/* initialisation du gestionnaire de signal pour arret de l'application par cntl-c */
	signal(SIGTERM, catch_signal);
	signal(SIGINT, catch_signal);
	/* verrouillage de la pagination memoire pour l'application */
	mlockall(mCLCURRENTIMCLFUTURE);
	/* initialisation du driver permettant d'effectuer des printf en mode primaire */
	rt_print_auto_init(1);
	/* initialisation de la carte d'entrees-sorties analogiques */
	printf("	 Demarrage appli temps reel	 \n");
	printf(" Init de la carte DMM32	\n");
	initCarteDMM32();
	printf(" DMM32 initialisee\n");
	/* Initialisation des variables */
	....
	/* creation des files de messages */
	rt_queue_create  	....
	/* creation des semaphores binaires */
	rt sem_create	....
	/* creation des semaphores d'exclusion mutuelle */
	rt_mutex create .....
	/* creation des tâches de l'application */
	rt_task_spawn .....
	printf(" Creation des taches de l'application	\n");
	rt_task_spawn(&idTDialogue, "tDialogue", 0, 5, T_FPU, &tDialogue, NULL);
	/* 	 A completer 	 */
	printf(" Taches creees\n");
	/* attente cntl-c pour arreter l'application */
	pause();
	/* destruction des taches */
	/*	*/
	printf("\n \n Destruction \n - Des taches\n - Des queues\n - Des semaphores et mutex\n	");
	rt_task_delete(&idTDialogue);
	/* 	 A completer 	 */
	/* destruction des fiches de l'application */
	rt_task_delete ...
	/* destruction des files de messages */
	rt_queue_delete	....
	/* destruction des semaphores binaires */
	rt_sem_delete ....
	/* destruction des semaphores d'exclusion mutuelle */
	rt_mutex_delete ...
	/* 	 A completer 	 */
	printf(" Application terminee\n");
	return (0);
}
