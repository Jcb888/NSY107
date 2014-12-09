/************************************************************)
(*                     S U P A E R O                        *)
(*                                                          *)
(*              Application roue a reaction                 *)
(*                  Os = linux+xenomai                      *)
(*                                                          *)
(*                     libUtilRoue.c                        *)
(*                                                          *)
(*                  Fonctions communes                      *)
(*                                                          *)
(************************************************************/

#include <stdio.h>
#include <unistd.h>

#include <sys/io.h>

#include "libmm32.h"
#include "libUtilRoue.h"

/* declaration des variables globales */

TypeEtalon infosEtalonnage;

/* fonctions d'acces aux ports I/O */
void sysOutByte(unsigned short adresseIO, unsigned char valeur)
{
	outb( valeur, adresseIO);
}

unsigned char sysInByte(unsigned short adresseIO)
{
	unsigned char val;
	val = inb(adresseIO);
	return val;
}

unsigned short sysInWord(unsigned short adresseIO)
{
	unsigned short val;
	val = inw(adresseIO);
	return val;
}

/* fonctions de conversion */


/****************************************************/
 void miseAJourInfosEtalonnage(float tableauReels[])
/****************************************************/
 {
     
     infosEtalonnage.offsetVoltVitesseRoue     = tableauReels[0];
     infosEtalonnage.gainVitesseRoue           = tableauReels[1];
     infosEtalonnage.offsetVoltVitessePlateau  = tableauReels[2]; 
     infosEtalonnage.gainVitessePlateau        = tableauReels[3]; 
     infosEtalonnage.offsetVoltPositionPlateau = tableauReels[4]; 
     infosEtalonnage.gainPositionPlateau       = tableauReels[5]; 
     infosEtalonnage.offsetVoltCourantMoteur   = tableauReels[6]; 
     infosEtalonnage.gainCourantMoteur         = tableauReels[7];
     infosEtalonnage.offsetCourantMoteur       = tableauReels[8];
     infosEtalonnage.gainCommandeMoteur        = tableauReels[9];
 //    printf("%f, %f, %f, %f, %f,%f, %f, %f, %f, %f\n",tableauReels[0],tableauReels[1],tableauReels[2],tableauReels[3],tableauReels[4],tableauReels[5],tableauReels[6],tableauReels[7],tableauReels[8],tableauReels[9]);
 }



/***************************************************************/
 float codeCANToVolts(int codeCAN)
/***************************************************************/
{
  return (codeCAN*(10.0/2048.0));
}



/***************************************************************/
float voltToVitesseRoue     (float voltVitesseRoue)   
/***************************************************************/
{
   float vitesseRoue;
   
  vitesseRoue = (voltVitesseRoue - infosEtalonnage.offsetVoltVitesseRoue)  * infosEtalonnage.gainVitesseRoue;
  return(vitesseRoue); 
}

/***************************************************************/
float voltToVitessePlateau  (float voltVitessePlateau)
/***************************************************************/
{
   float vitessePlateau;
   
  vitessePlateau = (voltVitessePlateau - infosEtalonnage.offsetVoltVitessePlateau)  * infosEtalonnage.gainVitessePlateau;
  return(vitessePlateau); 
}

/***************************************************************/
float voltToPositionPlateau (float voltPositionPlateau)
/***************************************************************/

{
   float positionPlateau;
   
  positionPlateau = (voltPositionPlateau - infosEtalonnage.offsetVoltPositionPlateau)  * infosEtalonnage.gainPositionPlateau;
  return(positionPlateau); 
}

/***************************************************************/
float voltToCourantMoteur   (float voltCourantMoteur)
/***************************************************************/
{
   float courantMoteur;
   
  courantMoteur = (voltCourantMoteur - infosEtalonnage.offsetVoltCourantMoteur)  * infosEtalonnage.gainCourantMoteur;
  return(courantMoteur); 
}




/***************************************************************/
 void acquisitionEchantillon(TypeEchantillon *echantillon) 
/***************************************************************/
{
 
  
  echantillon->vitesseMoteur   = voltToVitesseRoue     ( acquisitionVolt(VOIE_VITESSE_MOTEUR   ));
  echantillon->vitessePlateau  = voltToVitessePlateau  ( acquisitionVolt(VOIE_VITESSE_PLATEAU  ));
  echantillon->positionPlateau = voltToPositionPlateau ( acquisitionVolt(VOIE_POSITION_PLATEAU ));
  echantillon->courantMoteur   = voltToCourantMoteur   ( acquisitionVolt(VOIE_COURANT_MOTEUR   ));
 
  /*printf("%f\n" ,echantillon->positionPlateau );*/
  
  
}


/***************************************************************/
 void initCarteDMM32()
/***************************************************************/
 {
	 ioperm(BASE_MM32, 16,1); 
   initAcquisitionAnalogique(SCAN_20_MICROS_MM32 , CAN_M10P10_MM32 ,GAIN_CAN_1_MM32);
   ecritureVoieAnalogique(0,2048);
   ecritureVoieAnalogique(1,2048);

 }  


/************************************************/
 void applicationIConsigne(float courantConsigne)    /* en A */
/************************************************/
{
  float voltConsigne;
  int codeConsigne; 
  
  voltConsigne = (courantConsigne - infosEtalonnage.offsetCourantMoteur) * infosEtalonnage.gainCommandeMoteur;
  codeConsigne = ((voltConsigne * 2048.0)/10.0) +2048;
  if (codeConsigne > 4095) codeConsigne=4095;
  if (codeConsigne <    0) codeConsigne=   0;
  
 
  ecritureVoieAnalogique(0,codeConsigne);
  
 /*  printf("Courant : %f   voltConsigne : %f code %d\n",courantConsigne,voltConsigne,codeConsigne);*/
}

