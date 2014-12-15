

/*     Communication par socket TCP  communication simple 1 serveur / 1 client     */

#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>


#include "socketsTCP.h"

/*variables globales */
int connectedSD ;
TypeMessage message;
int sd;
int EnEcoute=0; /* 1 si ecoute, 0 si client connecte */ 

/* retourne -1 si Pb de creation du socket */
int ouvrirSocketServer(int numeroPort) {
	struct sockaddr_in  serverAddr;    /* server's socket address */ 
	struct sockaddr_in  clientAddr;    /* client's socket address */ 
	int                 sockAddrSize;  /* size of socket address structure */
	int optval; 
    	EnEcoute = 0;

	if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == ERROR) { 
		printf ("Erreur a l ouverture du socket\n");
		return (-1);
	}
	optval = 1;
	setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,(char *)&optval,sizeof(optval));
    
	/* initialisation de l adresse locale */
 
	sockAddrSize = sizeof (struct sockaddr_in); 
	bzero ((char *) &serverAddr, sizeof(serverAddr)); 
	serverAddr.sin_family = AF_INET; 
/*     serverAddr.sin_len = (u_char) sockAddrSize; */
	serverAddr.sin_port = htons (numeroPort); 
	serverAddr.sin_addr.s_addr = htonl (INADDR_ANY);
   	if (bind(sd, (struct sockaddr *) (&serverAddr), sockAddrSize) == ERROR) { 	
		printf ("Erreur au niveau du bind\n");
		if (sd) close(sd);
		return (-1);
	}
	if (listen(sd, 5) < 0) { 
		printf("listen failed\n");
		close(sd);
		return (-1);
	}
          
	EnEcoute=1;
   	printf ("Attente de connexion client sur port %d \n",numeroPort);
	if ((connectedSD = accept(sd, (struct sockaddr *) (&clientAddr), &sockAddrSize)) == ERROR) { 
		printf("erreur accept ou deja un client connecte\n");
		close(sd);
		return (-1);
	}
	printf ("Client connecte  \n");
	EnEcoute = 0;
	close(sd); 
	return(0);    
}


/*procedure d'ecriture sur le socket*/
void envoiMessage () {
	int snd,lg;
	unsigned char *pt;
	unsigned char tmp;
	lg=message.longueur; 
	/* remise en ordre des 2 octets de longueur */
	pt=(unsigned char*)&message.longueur;
	tmp= *pt;
	*pt=*(pt+1);
	*(pt+1)=tmp;
	if ((snd=send(connectedSD,(char *)&message, lg+4, 0)) == ERROR) {
		 printf("erreur d envoi de packet\n");
	}
}


void ecrireCaractere (char caractere) {
	message.typeDonnee = TYPE_CAR;
	message.longueur=0;
	message.car=(unsigned char)caractere;
	envoiMessage ();
}

void ecrireChaine (char caractere, char *chaine) {
	int i;
	unsigned char *pt;
	pt=(unsigned char *)chaine;
	message.typeDonnee = TYPE_CHAINE;
	message.car=caractere;
	message.longueur=strlen(chaine);
	for(i=0;i<message.longueur;i++) {
		message.data[i]=*pt++;
		}	
	envoiMessage ();
}

void ecrireEntier (char caractere, short int entier) {
	unsigned char *et;
	et=(unsigned char*)&entier;
	message.car=caractere;
	message.typeDonnee=TYPE_ENTIER;
	message.longueur=	2;
	message.data[1]=*et++;
	message.data[0]=*et;
	envoiMessage ();
}

void ecrireTableauEntiers (char caractere,short tbl[],short Nbelements) {

	int i;
	unsigned char *tt;
	tt=(unsigned char*)tbl;
	message.typeDonnee = TYPE_TABLEAU_ENTIERS;
	message.car=caractere;
	message.longueur=(Nbelements*2);
	for(i=0;i<Nbelements;i++) {
		message.data[(i*2)+1]=*tt++;	
		message.data[(i*2)]=*tt++;	
		}
	envoiMessage ();
}

void ecrireReel (char caractere,  float reel) {
	unsigned char *pt;
	pt=(unsigned char *)&reel;
	message.car=caractere;
	message.typeDonnee=TYPE_REEL;
	message.longueur=	4;
	message.data[3]=*(pt);
	message.data[2]=*(pt+1);
	message.data[1]=*(pt+2);
	message.data[0]=*(pt+3);
	envoiMessage ();
  }

 void ecrireTableauReels (char caractere,float tbl[],short int Nbelements) {
	int i;
	unsigned char *tt;
	tt=(unsigned char*)tbl;
	message.typeDonnee =TYPE_TABLEAU_REELS ;
	message.car=caractere;
	message.longueur=Nbelements*4;			
	for(i=0;i<Nbelements;i++) {
		message.data[(i*4)+3]=*tt++;
		message.data[(i*4)+2]=*tt++;
		message.data[(i*4)+1]=*tt++;	
		message.data[(i*4)+0]=*tt++;	
		}
	envoiMessage ();
}



/* attend un message et retourne le type de donnees
   les attributs de "message" sont actualises
*/
/**************************************/
  char lectureMessage()
/**************************************/
 
  {
	  int NbOctetsLus;
	unsigned char *ptc;
	unsigned char tmp;
	  NbOctetsLus = recv(connectedSD ,(char *)&message, sizeof(message), 0);
	  if (NbOctetsLus <=0)
		{ 
		 printf("erreur de reception\n");
		 return(-1); 
		}
	/* remise en ordre des 2 octets de longueur */
	ptc=(unsigned char*)&message.longueur;
	tmp= *ptc;
	*ptc=*(ptc+1);
	*(ptc+1)=tmp;
	return  message.typeDonnee;
  }


char retourneCaractereLu() {
  /* restitue le caractere (message.car) */
	return  message.car;
	}

char * retourneChaineLue() {
  /* restitue la chaine contenue dans message.data */
	message.data[message.longueur]='\0';
	return message.data;
	}

short int retourneEntierLu() {
  /* restitue l entier contenu dans message.data */
	unsigned char entierLu[2];
	entierLu[0]=message.data[1];
	entierLu[1]=message.data[0];
	return(*((short int *)entierLu));
	}


void retourneTableauEntiersLu(short int tableauEntiers[],short int *nbEntiersLus) {
  /* restitue le tableau contenu dans message.data */
	unsigned char entierLu[2];
	short int lg;
	int i;		
	lg=message.longueur;
	*nbEntiersLus=lg/2;
	for(i=0;i<lg/2;i++) {
		entierLu[0]=message.data[(i*2)+1];
		entierLu[1]=message.data[(i*2)+0];
		tableauEntiers[i]=*((short int *)entierLu);
		}
	}

float retourneReelLu() {
	/* restitue le reel contenu dans message.data */
	unsigned char reelLu[4];
	/*affectation des 4 octets du reel */
	reelLu[0]=message.data[3];
	reelLu[1]=message.data[2];
	reelLu[2]=message.data[1];
	reelLu[3]=message.data[0];
	return(*((float *)reelLu));
	}


void retourneTableauReelsLu(float tableauReels[],short *nbReelsLus) {
/* restitue le tableau contenu dans message.data */
unsigned char reelLu[4];
	short int lg;
	int i;		
	lg=message.longueur;
	*nbReelsLus=lg/4;
	for(i=0;i<lg/4;i++)
	{
	reelLu[3]=message.data[(i*4)];   
	reelLu[2]=message.data[(i*4)+1];
	reelLu[1]=message.data[(i*4)+2];
	reelLu[0]=message.data[(i*4)+3];  
	tableauReels[i]=*((float *)reelLu);
	}
}


  void fermetureSocket()
  {
	if (EnEcoute==0)
	  {
	   close(connectedSD);
	   printf(" socket dialogue ferme \n");
	   }
	else
		{   
	   close(sd);
	   printf(" socket ecoute ferme \n");
	   }
       
  }





