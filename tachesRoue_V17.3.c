/****************************************************************
	SUPAERO							
																
	Application roue a reaction				
	Os = linux+xenomai											
 Jean-Christophe BILLARD - Jerome LEBORGNE - Hugo TOURAINE	
																
	tachesRoue.c												

rev 17.2 + tps wcet + tps attente requete
***************************************************************/

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
unsigned char arret, clientConnecte, termine;
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
int loi;	/* N° loi (0 boucle ouverte)*/
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
	// SP pour envoi sur demande tt les 100ms hors experimentation en cours  
	TypeEchantillon echantillon;
	float tabCapteurs[4];
	//mutex pas nécessaire ici car pas de concurrence d'acces
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

	int i;
	RT_QUEUE_INFO queueInfo;
	TypeEchantillon echantillon;
	float blocCapteurs[200];//max = (100ms/2ms) * 4 valeurs  OU queueInfo.nmessages * 4 apres le inquire
	//on interroge la file pour connaitre le nb d'echantillons
	if (rt_queue_inquire(&idFileEchantillonsCourbes, &queueInfo)!=0)
		rt_printf("erreur interrogation file");
		
				//il faut maintenant remplir le tableau
				//si queueInfo.nmessages = 0 possible si PA > 100ms
				for (i = 0; i < queueInfo.nmessages; i++){
 
					// on recupere un echantillon (ou le suivant) dans la file
					if(rt_queue_read(&idFileEchantillonsCourbes, &echantillon, sizeof(echantillon), TM_NONBLOCK)<0)//retour = nbBytes lus ou -err
						rt_printf("rt_queue_read(&idFileEchantillonsCourbes, &echantillon, sizeof(echantillon), TM_NONBLOCK)<0) error");

					//on remplit le tableau par bloc de 4 en incrementant de  l'indice * 4
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
		rt_printf("MAJ, init. des parametres experimentation.coeffLoi[i]: %d valeur: %f \n ", i, parametresExperimentation.coeffLoi[i]);
	}

	rt_printf("parametres experimentation, duree: %d , PL: %d, PA: %d \n", parametresExperimentation.duree, parametresExperimentation.periodeActivationLoi,
																	parametresExperimentation.periodeLectureCapteurs);
}

void DisplayInfoTask(void){
	RT_TASK tid;
	RT_TASK_INFO InfosTache; 
	int cr;
	cr = rt_task_inquire(rt_task_self(), &InfosTache);
	if (cr == 0){//si inquire OK on affiche les stats de la tache. exectmime = tps exec en mode primaire
		rt_printf("Nom Tache : %s, Priorite : %d, texec MP  %ld.%06ld ms\n", InfosTache.name, InfosTache.bprio, (InfosTache.exectime/1000000));
		rt_printf("Commutations : %d , Mode : %x\n", InfosTache.modeswitches, InfosTache.status);
	}
	else{
		rt_printf("Erreur de lecture de 1 etat de la tache (numero %d )\n", cr);
	}
}
// info tasks avec un s pour les 3 taches
void DisplayInfoTasks(void){
	RT_TASK tid;
	RT_TASK_INFO InfosTache;
	int cr;
	
	//pour info idTDialogue
	cr = rt_task_inquire(&idTDialogue, &InfosTache);
	if (cr == 0){//affichage en ms a verifier (10^-9 - 10^-3 = 10^-6)
		rt_printf("Nom Tache : %s, Priorite : %d, texec MP  %ld.%06ld ms\n", InfosTache.name, InfosTache.bprio, (long)(InfosTache.exectime / 1000000));
		rt_printf("Commutations : %d , Mode : %x\n", InfosTache.modeswitches, InfosTache.status);
	}
	else{
		rt_printf("Erreur de lecture de 1 etat de la tache (numero %d )\n", cr);
	}

	//pour info idtDureePeriodesEtInfosCourbes
	cr = rt_task_inquire(&idtDureePeriodesEtInfosCourbes, &InfosTache);
	if (cr == 0){
		rt_printf("Nom Tache : %s, Priorite : %d, texec MP  %ld.%06ld ms\n", InfosTache.name, InfosTache.bprio, (long)(InfosTache.exectime / 1000000));
		rt_printf("Commutations : %d , Mode : %x\n", InfosTache.modeswitches, InfosTache.status);
	}
	else{
		rt_printf("Erreur de lecture de 1 etat de la tache (numero %d )\n", cr);
	}
	// pour idtGererLoiConsigne
	cr = rt_task_inquire(&idtGererLoiConsigne, &InfosTache);
	if (cr == 0){
		rt_printf("Nom Tache : %s, Priorite : %d, texec MP  %ld.%06ld ms\n", InfosTache.name, InfosTache.bprio,(long)(InfosTache.exectime / 1000000));
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

void tDureePeriodesEtInfosCourbes(){//alias T1
	
	TypeEchantillon echantillon;
	//init du timer donne la cadence toutes les 1ms
	rt_task_set_periodic(NULL, TM_NOW, 1000000);
	for (;;){
		// on endort la tache pour qu'elle attende son activation par sem_v (debut) depuis la tache tDialogue
		if(rt_sem_p(&idSemDebut, TM_INFINITE) != 0)
			rt_printf("erreur rt_sem_p(&idSemDebut, TM_INFINITE) de tDureePeriodesEtInfosCourbes");
		// remise à 0 des flags sur debut nouvelle experimentation apres un termine = 1 ou arret =1
		termine = 0;
		arret =0;

		// init des variables locals
		char PL = parametresExperimentation.periodeActivationLoi;
		char PA = parametresExperimentation.periodeLectureCapteurs;
		int dureeExperimentation = parametresExperimentation.duree;
		int dureeEnCours = 0;
		//
		RTIME debutExec = 0;
		RTIME finExec = 0;
		RTIME delta = 0;
		RTIME wcet = 0;

		while (termine == 0){
			
			finExec = rt_timer_read();
			if (debutExec != 0){// On l'affiche si pas premier passage
				delta = finExec - debutExec;
				rt_printf("Tps d'execution tDureePeriodesEtInfosCourbes : %ld.%06ld ms\n", (long)delta / 1000000, (long)delta % 1000000);
				if (delta > wcet){
					wcet = delta;
					rt_printf("--> nouveau wcet tDureePeriodesEtInfosCourbes : %ld.%06ld ms\n", (long)wcet / 1000000, (long)wcet % 1000000);
				}
			}
			// attendre la prochaine periode
			rt_task_wait_period(NULL);
			debutExec = rt_timer_read();
			// on a fait un tour en plus d'une ms
			dureeEnCours++;

			//la duree en cours est elle un multiple de PL
			if (dureeEnCours % PL == 0){
				//si oui on active la tache loi et consignes
				rt_sem_v(&idSemPL);
			}// fin if dureeEnCours % PL

			if (dureeEnCours % PA == 0){
				// On execute PA (info courbes)
				rt_mutex_acquire(&idMutexCAN, TM_INFINITE);//infinite car normalement on maitrise le mutex
				acquisitionEchantillon(&echantillon);
				rt_mutex_release(&idMutexCAN);
				//On ecrit l'echantillon dans la file
				if (rt_queue_write(&idFileEchantillonsCourbes, &echantillon,sizeof(echantillon),Q_NORMAL) != 0)
					rt_printf("erreur rt_queue_write");

			}// fin if dureeEnCours % PA

			//prise en compte du changement d'etat des flags
			if (dureeEnCours >= dureeExperimentation || arret == 1 || clientConnecte == 0){
				// le flag terminee est scrute par tdialogue et tGererLoiConsigne
				termine = 1;
				rt_printf("termine vient de passe a 1 car dureeEnCours. :%d arret : %d client conn. :%d \n", dureeEnCours, arret, clientConnecte);
				//Au cas ou tGererLoiConsigne est endormi elle ne peut prendre en compte la MAJ du flag dc on la reveil
				rt_sem_v(&idSemPL);
			}//fin if (dureeEnCours >= dureeExperimentation || arret == 1 || clientConnecte == 0) 
		
		
		}//fin while (termine == 0)
		//pour infos stats en sortant.
		DisplayInfoTasks();
	}//fin for(;;)
	
}// fin tDureePeriodesEtInfosCourbes()

/****************************************/
void tGererLoiConsigne(){// allias T2
/****************************************/
	for (;;){// boucle infini pour que la tache existe

		RTIME debutExec = 0;//pour mesure tps exec
		RTIME finExec = 0;//pour mesure tps exec
		float cde = 0.0;
		TypeEchantillon echantillon;

		// sem_p destine a l'initialisation lors du premier passage ou du lancement d'une nouvelle experimentation
		if (rt_sem_p(&idSemPL, TM_INFINITE) != 0)
			rt_printf("erreur rt_sem_p(&idSemPL, TM_INFINITE) exterieur de tGererLoiConsigne ");
			
		while (termine == 0){
			
			rt_mutex_acquire(&idMutexCAN, TM_INFINITE);//infinite car normalement on maitrise le mutex
			acquisitionEchantillon(&echantillon);
			rt_mutex_release(&idMutexCAN);

			switch(parametresExperimentation.loi){
			rt_printf("valeur switch loi : %d \n", parametresExperimentation.loi);
				
				case 11 :{ //Loi 1 Boucle position voir cahier des charges page 8
					
						cde = (((parametresExperimentation.valFinConsigne - echantillon.positionPlateau)
								*parametresExperimentation.coeffLoi[1]) - echantillon.vitessePlateau)
								*parametresExperimentation.coeffLoi[2];

						rt_printf("valeur type loi : %d  coef kp : %f coef kv : %f valeur cde: %f \n",
								parametresExperimentation.loi, parametresExperimentation.coeffLoi[1], parametresExperimentation.coeffLoi[1], cde);

				}break;
				
				default:{ rt_printf("type loi non reconnue ou non code \n");
				}break;
							
			
			}// fin switch (parametresExperimentation.loi)*/
			
			applicationIConsigne(cde); 

			finExec = rt_timer_read();// fin mesure tps exec tache
			if (debutExec != 0)//si on est pas sur la premiere boucle affichage du tps d'execution de la tache
				rt_printf("Tps d'execution tGererLoiConsigne: %ld.%06ld ms\n", (long)(finExec - debutExec) / 1000000, (long)(finExec - debutExec) % 1000000);
			
			//sem_p intérieur(dc on est dans une experimentation en cours) pour mise en veille lors de la fin de chaque passage
			//active par sem_v (% PL) de tDureePeriodesEtInfosCourbes
			if (rt_sem_p(&idSemPL, TM_INFINITE) != 0)
				rt_printf("erreur rt_sem_p(&idSemPL, TM_INFINITE) interieur de tGererLoiConsigne");
			
			debutExec = rt_timer_read();//on commence la mesure du temps d'exec

		}//fin while (termine == 0)
	// on est sortie donc termine == 1 il faut arreter la roue
	applicationIConsigne(0.0);
	}//fin for (;;) 
}//fin void tGererLoiConsigne()


/********************************************************/
void tDialogue(){ // alias T3
/***************************************************'*****/
#define MESS_ACK 'A' /* message d'acquittement */

	float tableauReels[TAILLE_MAX_TABLEAU_REELS];
	short int nbReelsLus;
	char typeMessage, caractereMessage;
	int socketServeur;
	int numeroPort = 7800;
	int erreur; /* indicateur d erreur */

	RTIME debutExec = 0;
	RTIME finExec = 0;
	RTIME delta = 0;
	RTIME wcet = 0;
	/* boucle de lecture des messages */
	for (;;) {
		erreur = 0;
		/* ouverture socket serveur */
		socketServeur = ouvrirSocketServer(numeroPort);
		rt_printf(" port %d ouvert\n", numeroPort);
		if (socketServeur == ERROR) erreur = 1;
		/* Initialisations a effectuer en debut de chaque connexion */
		arret = 0; //flag scruter par le while de tDureePeriodesEtInfosCourbes 
		clientConnecte = 1;//flag scruter par le while de tDureePeriodesEtInfosCourbes
		/* boucle de lecture des requetes */
		rt_printf(" connexion Active \n");
		while (erreur == 0) {
		
			finExec = rt_timer_read();// Mesure fin exec
			if (debutExec != 0){// On l'affiche si pas premier passage
				delta = finExec - debutExec;
					rt_printf("Tps d'execution tDialogue : %ld.%06ld ms\n", (long) delta / 1000000, (long) delta % 1000000);
				if (delta > wcet){
					wcet = delta;
					rt_printf("--> nouveau wcet tDialogue : %ld.%06ld ms\n", (long) wcet / 1000000, (long) wcet % 1000000);
				}
			}

			typeMessage = lectureMessage();//lecture bloquante en attente message cmd station sol + utilisation recv non tps reel dans socketTCP.c au lieu de rt_dev_recv = commutation de contexte			
			debutExec = rt_timer_read();//Message recu on commence la mesure du temps d'exec
			rt_printf("Tps d'attente reception requetes : %ld.%06ld ms\n", (long)(debutExec - finExec) / 1000000, (long)(finExec - debutExec) % 1000000);

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
								//arret est scrute par T1 tt le 1 ms
								arret = 1;
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
	arret = 0;
	clientConnecte = 0;
	termine = 0;
	/* creation des files de messages */
	rt_queue_create(&idFileEchantillonsCourbes, "fileEchantillons", 50 * sizeof(TypeEchantillon), 50, Q_FIFO);
	/* creation des semaphores binaires */
	rt_sem_create(&idSemDebut,"semDebut", 0, S_FIFO);// sfifo == rate monotonic
	rt_sem_create(&idSemPL, "semPL", 0, S_FIFO);// sfifo == rate monotonic
	/* creation des semaphores d'exclusion mutuelle */
	rt_mutex_create(&idMutexCAN,"mutexCAN");
	/* creation des tâches de l'application */
	printf(" Creation des taches de l'application	\n");
	
	//creation dans le sens inverse des activations

	rt_task_spawn(&idtGererLoiConsigne, "tGererLoiConsigne_T2", 0, 6, T_FPU, &tGererLoiConsigne, NULL);//n'appelle personne
	rt_task_spawn(&idtDureePeriodesEtInfosCourbes, "tDureePeriodesEtInfosCourbes_T1", 0, 7, T_FPU, &tDureePeriodesEtInfosCourbes, NULL);//appelle T2
	rt_task_spawn(&idTDialogue, "tDialogue_T3", 0, 5, T_FPU, &tDialogue, NULL);//appelle T1
	
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
