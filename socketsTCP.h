
#ifndef socketsTcp
#define socketsTcp
/*     Communication par socket TCP       */

#define ERROR -1
/* declaration de constantes 
*/
#define LONGUEUR_MAX_MESSAGE 1024
/* longueur maximum en octets du data
 */
#define LONGUEUR_MAX_DATA LONGUEUR_MAX_MESSAGE-4 
/* nb max de reels dans un message
*/
#define TAILLE_MAX_TABLEAU_REELS  LONGUEUR_MAX_DATA / 4 
/* nb max d entiers dans un message
 */
#define TAILLE_MAX_TABLEAU_ENTIERS LONGUEUR_MAX_DATA / 2 

#define TYPE_MESSAGE_TAB_REELS 'L'

#define TYPE_ENTIER  'I'          // Entier 16 bits

#define TYPE_REEL  'R'            // Reel simple precision sur 4 octets
#define TYPE_TABLEAU_ENTIERS  'A' // Tableau d entiers codes sur  16 bits

#define TYPE_TABLEAU_REELS 'T'    // Tableau de reels simple precision
#define POSITIONS_TOURELLE 'P'    // Tableau de reels positions Site et Azimut

#define TYPE_CAR  'C'             // Caractere 1 octet

#define TYPE_CHAINE  'S'          // Chaine de caracteres

#define COMMANDE_DEBUT 'A'
#define COMMANDE_SITE_PLUS 'B'
#define COMMANDE_SITE_MOINS 'C'
#define COMMANDE_AZIMUT_PLUS 'D'
#define COMMANDE_AZIMUT_MOINS 'E'
#define COMMADE_SET_POSITIONS 'F'

#define COMMANDE_POSITIONS_COURANTES 'P'

#define COMMANDE_FIN 'Z'

/* declarations de types */
typedef unsigned char TypeBuffer[LONGUEUR_MAX_MESSAGE];


typedef struct {
	char typeDonnee;
	char car;
	unsigned short longueur;
	TypeBuffer data;
	}  TypeMessage;


/*   Fonctions d ouverture de sockets */


/* retourne -1 si Pb de creation du socket */
int ouvrirSocketServer(int numeroPort);

/*   Fonctions d ecriture sur le socket */

void envoiMessage ();
void ecrireCaractere (char caractere);
void ecrireChaine (char caractere, char *chaine);
void ecrireEntier (char caractere, short int entier);
void ecrireTableauEntiers (char caractere,short tbl[],short Nbelements);
void ecrireReel (char caractere,  float reel);
void ecrireTableauReels (char caractere,float tbl[],short Nbelements);

/*   Fonctions de lecture sur le socket cote serveur */

 /* attente d un message, retourne le type de donnee recue */
 char lectureMessage();

 /* recuperation des donnees recues */
 char retourneCaractereLu();
 char *retourneChaineLue();
 short int retourneEntierLu();
 void retourneTableauEntiersLu(short int tableauEntiers[],short *nbEntiersLus);
 float retourneReelLu();
 void retourneTableauReelsLu(float tableauReels[],short *nbReelsLus);


void fermetureSocket();

#endif
