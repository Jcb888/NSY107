//﻿/****************************************************************)
//(*	SUPAERO							*)
//(*																
//(*	Application roue a reaction				*)
//(*	Os = linux+xenomai											*)
//(* Jean-Christophe BILLARD - Jerome LEBORGNE - Hugo TOURAINE	*)
///																///
//	tachesRoue.c												*)
//
//rev 10
//
//ToDo  profil consigne , option loi
//	
//(****************************************************************/

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
#include <native/queue.h> 
#include <native/sem.h> 
#include <native/event.h> 
#include <native/mutex.h> 
#include <rtdk.h>
#include "libUtilRoue.h"
#include "socketsTCP.h"

/*********************************************************/
/* declarations de variables globales */
/*********************************************************/
// déclaration des VG type flag: change sur evenement puis prise en compte par scrutation des taches
char arret, clientConnecte, termine;
/* declaration de constantes */
/* # define */
/* declaration des identificateurs des taches */
RT_TASK idTDialogue, idtDureePeriodesEtInfosCourbes, idtGererLoiConsigne;

/* Identificateurs des semaphores pour synchronisation */

RT_SEM idSemDebut, idSemPL;

/* semaphores d'exclusion mutuelle */

RT_MUTEX idMutexCAN; 

/* Identificateurs des files */

RT_QUEUE idFileEchantillonsCourbes; 

/* variables pour communication et synchronisation entre taches par memoire commune */

typedef struct {
int loi;	/* N loi (0 boucle ouverte*/
int option; /* option d'experimentation */
int duree;
int periodeLectureCapteurs; /*PA  en ms */
int periodeActivationLoi; /* PL en ms */
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
void envoyerValeursCourantesCapteurs(){
/**************************************/
	TypeEchantillon echantillon;
	float tabCapteurs[4];
	acquisitionEchantillon(&echantillon);
	tabCapteurs[0] = echantillon.vitesseMoteur;;
	tabCapteurs[1] = echantillon.vitessePlateau;
	tabCapteurs[2] = echantillon.positionPlateau;
	tabCapteurs[3] = echantillon.courantMoteur;
	ecrireTableauReels('T', tabCapteurs, 4);
}
/**************************************/
void envoyerDernierBlocEchantillons(){
	/**************************************/
	//ce sous programme gere le transfert des echantillons
	// de la file vers la station sol à chacune de ses demandes


	//float blocCapteurs[50 * 4];
	//int i;
	///* simulation de l'envoi de 10 echantillons constants */
	///*	a modifier ... */
	//for (i = 0; i < 50; i++){
	//	blocCapteurs[(i * 4)] = 100;
	//	blocCapteurs[(i * 4) + 1] = 1;
	//	blocCapteurs[(i * 4) + 2] = 0.5;
	//	blocCapteurs[(i * 4) + 3] = 0.2;
	//}
	//ecrireTableauReels('S', blocCapteurs, 50 * 4);
	//ToDo prise en compte Nb echantillons ds la file
	int i;
	RT_QUEUE_INFO queueInfo;
	TypeEchantillon echantillon;
	//on interroge la file pour connaitre le nb d'echantillons
	if (rt_queue_inquire(&idFileEchantillonsCourbes, &queueInfo) != 0)
		rt_printf("erreur interrogation file");

		//if (queueInfo.nmessages){//si file = 0 que fait'on
		// on envoit un tableau vide ou en envoit rien ?
		// et si on envoit rien risque t'on atteindre le DL
	
				//on peut initialiser le tableau à la bonne taille.
				float blocCapteurs[queueInfo.nmessages];

				//il faut maintenant remplir le tableau

				for (i = 0; i <queueInfo.nmessages; i++){

					// on recupere un echantillon (ou le suivant) dans la file
					if (rt_queue_read(&idFileEchantillonsCourbes, &echantillon, sizeof(echantillon), TM_NONBLOCK) != 0)
						rt_printf("erreur lecture file");

					acquisitionEchantillon(&echantillon);
					//on remplit le tableau par bloc de 4 en incrementant de  l'indice
					// a chaque boucle
					blocCapteurs[4 * i + 0] = echantillon.vitesseMoteur;
					blocCapteurs[4 * i + 1] = echantillon.vitessePlateau;
					blocCapteurs[4 * i + 2] = echantillon.positionPlateau;
					blocCapteurs[4 * i + 3] = echantillon.courantMoteur;
				}// fin for le tableau d'echantillons est remplit

		

				//on gere ici le cas experimentation termine
				if (termine){
					ecrireTableauReels('F', blocCapteurs, queueInfo.nmessages * 4);
				}
				else{
					ecrireTableauReels('S', blocCapteurs, queueInfo.nmessages * 4);
				}

		//}fin if (queueInfo.nmessages)

}


/***************************************************************/
void miseAJourparametresExperimentation(float tableauReels[]){
/***************************************************************/
	int i;
	parametresExperimentation.loi = tableauReels[0];
	parametresExperimentation.option = tableauReels[1];
	tableauReels[2] = tableauReels[2] * 1000.0;// adaptation format temps
	parametresExperimentation.duree = tableauReels[2];
	tableauReels[3] = tableauReels[3] * 1000.0;
	parametresExperimentation.periodeLectureCapteurs = tableauReels[3];
	tableauReels[4] = tableauReels[4] * 1000.0;
	parametresExperimentation.periodeActivationLoi = tableauReels[4];
	parametresExperimentation.profilConsigne = tableauReels[5];
	parametresExperimentation.valFinConsigne = tableauReels[6];
	tableauReels[7] = tableauReels[7] * 1000.0;
	parametresExperimentation.dureeMonteeConsigne = tableauReels[7];
	for (i = 0; i < 15; i++){
		parametresExperimentation.coeffLoi[i] = tableauReels[8 + i];
	}

	rt_printf("parametres experimentation: d %d , pl %d, pe %d \n", parametresExperimentation.duree, parametresExperimentation.periodeActivationLoi,
																	parametresExperimentation.periodeLectureCapteurs);
}

void DisplayInfoTask(void){
	RT_TASK tid;
	RT_TASK_INFO InfosTache;idFileEchantillonsCourbes;
	int cr;
	cr = rt_task_inquire(rt_task_self(), &InfosTache);
	if (cr == 0){
		rt_printf("Nom Tache : %s, Priorite : %d, texec MP %d \n", InfosTache.name, InfosTache.bprio, InfosTache.exectime);
		rt_printf("Commutations : %d , Mode : %x\n", InfosTache.modeswitches, InfosTache.status);
	}
	else{
		rt_printf("Erreur de lecture de 1 etat de la tache (numero %d )\n", cr);
	}
}
/*******************************************************************************
*
Les TACHES
*
*******************************************************************************/

void tDureePeriodesEtInfosCourbes(){//alias T2
	
	TypeEchantillon echantillon;
	//init du timer donne la cadence toutes les 1ms
	rt_task_set_periodic(NULL, TM_NOW, 1000000);
	for (;;){
		// on endort la tache pour qu'elle attende son activation par sem_v (debut) depuis la tache tDialogue
		int retour = rt_sem_p(&idSemDebut, TM_INFINITE);
		rt_printf("code retour rt_sem_p: %d \n", retour);
		// pour remise à 0 du flag sur debut nouvelle experimentation apres un termine = 1
		termine = 0;
		
		// init des variables locals
		char PL = parametresExperimentation.periodeActivationLoi;
		char PA = parametresExperimentation.periodeLectureCapteurs;
		int dureeExperimentation = parametresExperimentation.duree;
		int dureeEnCours = 0;

		while (termine == 0){
			// attendre la prochaine periode
			rt_task_wait_period(NULL);
			// on a fait un tour en plus d'une ms
			dureeEnCours++;

			//la duree en cours est elle un multiple de PL
			if (dureeEnCours % PL == 0){
				//si oui on active la tache loi et consignes
				rt_sem_v(&idSemPL);
			}// fin if dureeEnCours % PL

			if (dureeEnCours % PA == 0){
				// On execute PA (info courbes)
				// mutex pour accex CAN gere par SP ?
				acquisitionEchantillon(&echantillon);
				//On ecrit l'echantillon dans la file
				if (rt_queue_write(&idFileEchantillonsCourbes, &echantillon,sizeof(echantillon),Q_NORMAL) != 0)
					rt_printf("erreur rt_queue_write");

			}// fin if dureeEnCours % PA

			//prise en compte du changement d'etat des flags ? meilleur emplacement
			if (dureeEnCours >= dureeExperimentation || arret == 1 || clientConnecte == 0){
				// le flag terminee est scrute par T1 et T3
				termine = 1;
				rt_printf("termine vient de passe a 1 car dureeEnCours. :%d arret : %d client conn. :%d \n", dureeEnCours, arret, clientConnecte);
				//Au cas ou T3 est endormi il ne peut prendre en compte la MAJ du flag dc on le reveil
				rt_sem_v(&idSemPL);
			}//fin if (dureeEnCours >= dureeExperimentation || arret == 1 || clientConnecte == 0) 
		
		
		}//fin while (termine == 0)
		
	}//fin for(;;)
}// fin tDureePeriodesEtInfosCourbes()

void tGererLoiConsigne(){// allias T3

	for (;;){// boucle infini pour que la tache existe
		// sem_p destine a l'initialisation lors du premier passage (ou nouvelle phase)
		int retour = rt_sem_p(&idSemPL, TM_INFINITE);
		rt_printf("code retour rt_sem_p: %d \n", retour);

		
			
		while (termine == 0){
			
			float cde = 0.0;
			TypeEchantillon echantillon;
			// mutex pour accex CAN gere par SP ?
			acquisitionEchantillon(&echantillon);
			
				//ToDo generer la montee selon type de profil consigne(0:echelon 1:rampe ou 2:ralliement) sur
				
			//METTRE EN COMMENTAIRE EN PHASEDE DE MISE AU POINT UTILISER applicationIConsigne(0.3) A LA PLACE
			switch(parametresExperimentation.loi){
			
				case 0 :{
					//genererProfilConsigne()
					//ToDo prise en compte option loi
					cde = (((parametresExperimentation.valFinConsigne - echantillon.positionPlateau)*2.0) - echantillon.vitessePlateau)*2.0;
					rt_printf("valeur type loi : %d  valeur cde: %f \n",parametresExperimentation.loi,  cde);
					rt_printf("valeur finconsigne : %f  valeur positionPlateau: %f valeur vitessePlateau: %f \n", parametresExperimentation.valFinConsigne, echantillon.positionPlateau, echantillon.vitessePlateau);
					
				}break;
				
				case 11 :{ // voir cahier des charges page 8 ?? consigne en Radian --> ?? conversion en amperes ??
					
					if (parametresExperimentation.option == 0){// 0 : fermée
						// param experimentation MAJ ds miseAJourparametresExperimentation()
						cde = (((parametresExperimentation.valFinConsigne - echantillon.positionPlateau)
								*parametresExperimentation.coeffLoi[1]) - echantillon.vitessePlateau)
								*parametresExperimentation.coeffLoi[2];

						rt_printf("valeur type loi : %d  coef kp : %f coef kv : %f valeur cde: %f \n",
								parametresExperimentation.loi, parametresExperimentation.coeffLoi[1], parametresExperimentation.coeffLoi[1], cde);
					}
					if (parametresExperimentation.option == 1){// 1 : voisinage RP
						// param experimentation MAJ ds miseAJourparametresExperimentation()
						cde = (((parametresExperimentation.valFinConsigne - echantillon.positionPlateau)
							*parametresExperimentation.coeffLoi[1]) - echantillon.vitessePlateau)
							*parametresExperimentation.coeffLoi[2];

						rt_printf("valeur type loi : %d  coef kp : %f coef kv : %f valeur cde: %f \n",
							parametresExperimentation.loi, parametresExperimentation.coeffLoi[1], parametresExperimentation.coeffLoi[1], cde);
					}

					// ? gerer parametresExperimentation.option non reconnue ?


				}break;
				
				default : rt_printf("type loi non reconnue \n");
				break;			
			
			}// fin switch (parametresExperimentation.loi)*/
			
			applicationIConsigne(0.3); // INSERER cde qd mise au point ok

			//sem_p pour mise en veille lors de la fin premiers passage
			// puis des suivants (activation par sem_v de T2)
			int retour = rt_sem_p(&idSemPL, TM_INFINITE);
			rt_printf("code retour rt_sem_p: %d \n", retour);
		}//fin while (termine == 0)
	// on est sortie donc termine == 1 il faut arreter la roue
	applicationIConsigne(0.0);
	}//fin for (;;) 

}//fin void tGererLoiConsigne()


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
		if (socketServeur == ERROR) erreur = 1;
		/* Initialisations a effectuer en debut de chaque connexion */
		arret = 0;
		clientConnecte = 1;
		/* boucle de lecture des requetes */
		rt_printf(" connexion Active \n");
		while (erreur == 0) {
			typeMessage = lectureMessage();
				if (typeMessage < 0) { /* erreur de lecture socket */
					printf(" erreur de lecture socket\n");
					erreur = 1;
				}
				else
				{
					switch (typeMessage){

						case TYPE_CAR :
						{
							caractereMessage = retourneCaractereLu();

							switch (caractereMessage){

								case 'A': /* arret experimentation */
								{
								//arret est scrute par T2 tt le 1 ms
								arret = 1;
								//clientConnecte = 0; // utile souhaitable ? pour avertir autres taches
								ecrireCaractere(MESS_ACK);
								} break;

								case 'D': /* debut experimentation */
								{
								rt_sem_v(&idSemDebut);
								ecrireCaractere(MESS_ACK);
								} break;

								case 'C':
								{	/* lecture valeurs courantes capteurs */
								// cas avant debut experimentation selon 
								//page 9 et 10 du cahier des charges
								envoyerValeursCourantesCapteurs();
								} break;
								case 'B': /* Lecture bloc des derniers echantillons */
								{// la on est dans le cas d'une experimentation en cours
								 // cas fin experimentation gere dans le SP 
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
								//Fn externe
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
	clientConnecte = 0;
	rt_printf(" port %d ferme \n", numeroPort);
	} /*fin for (;;) */
}/*fin tDialogue*/

void catch_signal(int sig){
	printf(" Signal = %d\n", sig);
}
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
	mlockall(MCL_CURRENT|MCL_FUTURE);
	/* initialisation du driver permettant d'effectuer des printf en mode primaire */
	rt_print_auto_init(1);
	/* initialisation de la carte d'entrees-sorties analogiques */
	printf("Demarrage appli temps reel	 \n");
	printf(" Init de la carte DMM32	\n");
	initCarteDMM32();
	printf(" DMM32 initialisee\n");
	/* Initialisation des variables */
	/* creation des files de messages */
	rt_queue_create(&idFileEchantillonsCourbes, "fileEchantillons", 50 * sizeof(TypeEchantillon), 50, Q_FIFO);
	/* creation des semaphores binaires */
	rt_sem_create(&idSemDebut,"semDebut", 0, S_FIFO);// sfifo au cas ou une prio + grande le bloque ?
	rt_sem_create(&idSemPL, "semPL", 0, S_FIFO);// sfifo au cas ou une prio + grande le bloque ?
	/* creation des semaphores d'exclusion mutuelle */
	rt_mutex_create(&idMutexCAN,"mutexCAN");
	/* creation des tâches de l'application */
	printf(" Creation des taches de l'application	\n");
	
	//creation dans le sens inverse des activations
	rt_task_spawn(&idtGererLoiConsigne, "tGererLoiConsigne", 0, 6, T_FPU, &tGererLoiConsigne, NULL);
	rt_task_spawn(&idtDureePeriodesEtInfosCourbes, "tDureePeriodesEtInfosCourbes", 0, 7, T_FPU, &tDureePeriodesEtInfosCourbes, NULL);
	rt_task_spawn(&idTDialogue, "tDialogue", 0, 5, T_FPU, &tDialogue, NULL);
	
	/* 	 A completer 	 */
	printf(" Taches creees\n");
	/* attente cntl-c pour arreter l'application */
	pause();
	/* destruction des taches */

	printf("\n \n Destruction \n - Des taches\n - Des queues\n - Des semaphores et mutex\n	");
	//destruction dans l'ordre d'activation
	rt_task_delete(&idTDialogue);
	rt_task_delete(&idtDureePeriodesEtInfosCourbes);
	rt_task_delete(&idtGererLoiConsigne);

	/* destruction des files de messages */
	rt_queue_delete(&idFileEchantillonsCourbes);
	/* destruction des semaphores binaires */
	rt_sem_delete(&idSemDebut);
	rt_sem_delete(&idSemPL);
	/* destruction des semaphores d'exclusion mutuelle */
	rt_mutex_delete(&idMutexCAN);
	/* 	 A completer 	 */
	printf(" Application terminee\n");
	return (0);
}
