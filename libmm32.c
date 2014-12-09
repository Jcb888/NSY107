/************************************************************)
(*                     S U P A E R O                        *)
(*                                                          *)
(*              Application roue a reaction                 *)
(*                   Os = linux+xenomai                     *)
(*                                                          *)
(*                        libmm32.c                         *)
(*                                                          *)
(*  Fonctions de gestion de la carte E/S analogiques DMM32  *)
(*                                                          *)
(************************************************************/

#include "libmm32.h"
#include "libUtilRoue.h"
                
#define analog_config BASE_MM32+11
#define vout_pfaible  BASE_MM32+4
#define vout_pfort    BASE_MM32+5
#define DAC_pret      BASE_MM32+4


#define misc_ctrl_reg   BASE_MM32+8

#define timer_op_reg    BASE_MM32+9
#define timer_ctrl_reg  BASE_MM32+10
#define timer_count_0   BASE_MM32+12
#define timer_ctrl_w    BASE_MM32+15

#define pia_ctrl_IO    BASE_MM32+15
#define piaPortA      BASE_MM32+12
#define piaPortB      BASE_MM32+13
#define piaPortC      BASE_MM32+14
#define auxiliaryDigitalOutput     BASE_MM32+1

#define low_chan_reg    BASE_MM32+2
#define hig_chan_reg    BASE_MM32+3
#define adc_ctrl_reg    BASE_MM322+0
#define adc_msb_reg     BASE_MM32+1
#define adc_lsb_reg     BASE_MM32+0
#define adc_data_reg    BASE_MM32+0
#define adc_rback_reg   BASE_MM32+11
#define adc_start_reg   BASE_MM32+0
#define adc_conv_reg    BASE_MM32+8


/*---------------------------------------------------*/
/*  Initialisation des entrees sorties analogiques   */
/*---------------------------------------------------*/

void initAcquisitionAnalogique(int periodeSCAN , int plageCAN ,int gainCAN)
{
	unsigned char masque;
	masque = gainCAN;
	switch (plageCAN)
	{
		case CAN_M5P5_MM32 :
		{
			masque=(masque & 0xF3);
		} break;
		case CAN_M10P10_MM32 :
		{
			masque=(masque | 0x08);
		} break;
		case CAN_0P10_MM32 :
		{
			masque=(masque | 0x0C);
		}
	}   /*  fin du switch   */ 
	
	switch (periodeSCAN)
	{
		case SCAN_20_MICROS_MM32 :
		{
			/* les bits 4 et 5 sont a 0 */
		}   break;
		case SCAN_15_MICROS_MM32 :
		{
			masque=(masque | 0x10);
		}   break;
		case SCAN_10_MICROS_MM32 :
		{
			masque=(masque | 0x20);
		}   break;
		case SCAN_5_MICROS_MM32 :
		{
			masque=(masque | 0x30);
		}
	}   /*  fin du switch   */    
	sysOutByte(analog_config,masque);
}

/*--------------------------------------------------------*/
void attenteCAN(void)
{
    while ((sysInByte(DAC_pret) & 0x80) != 0)
    {  /* attendre   */ }
}
													  

/*--------------------------------------------------------*/
void  ecritureVoieAnalogique (int numeroVoie, short code)
{
unsigned char octet_faible;
unsigned char octet_fort  ;
unsigned char octet       ;
unsigned short mot        ;

  attenteCAN();
 
  mot = (code & 0x00ff);
  octet_faible = mot;
  sysOutByte(vout_pfaible,octet_faible);
  attenteCAN();
 
  mot = (code /256);
  mot = (mot + (numeroVoie * 64));
  octet_fort = mot & 0x00ff ;
  sysOutByte(vout_pfort,octet_fort);
  attenteCAN();

  octet=sysInByte(vout_pfort);
}


/*--------------------------------------------------------*/
void attenteMuxADC(void)
{
 while ((sysInByte(adc_rback_reg) & 0x80) == 0x80)
      { /* attente bit 8 a 0 */}
}      
/*--------------------------------------------------------*/
void attenteConvADC(void)
{

 while ((sysInByte(adc_conv_reg) & 0x80) == 0x80)
      { /* attente bit 8 a 0 */}
}      
/*--------------------------------------------------------------*/

 short acquisitionVoieAnalogique(unsigned char numeroVoie)
{

 unsigned char pipo;
 short mot;  

  attenteConvADC(); 
  attenteMuxADC();

 sysOutByte(low_chan_reg,numeroVoie);     /*   selection de la voie debut */          
 sysOutByte(hig_chan_reg,numeroVoie);	  /*   delection de la voie fin*/

 attenteMuxADC();			  /* Attente Mux pret*/
 pipo=0;
 sysOutByte(adc_start_reg,pipo);    /* Lancement de la conversion */       
 attenteConvADC();		  /* Attente fin de correction*/


 mot=sysInWord(adc_data_reg);
return(mot);

 
}

/*--------------------------------------------------------------*/

float acquisitionVolt(unsigned char numVoie)
{
short codeEntree;
float voltEntree;

  codeEntree = acquisitionVoieAnalogique(numVoie)/16;
  voltEntree = codeEntree * (10.0/2048.0);
  return(voltEntree);
}

/*--------------------------------------------------------*/
void ecritureBitDout1(unsigned char etatBit)
/* ecriture du bit Dout1 sur J3 pin 43   bit independant initialise en sortie */
/*--------------------------------------------------------*/
{
   if (etatBit==0) sysOutByte(auxiliaryDigitalOutput,0);
   else
   sysOutByte(auxiliaryDigitalOutput,2);
  
}


/*--------------------------------------------------------*/
void selectionPage1(void)
{
 sysOutByte(misc_ctrl_reg,   01);              /* selection page 1 */        
}


/*--------------------------------------------------------*/
void configurationPorts(int portAOut,int portBOut,int portC03Out,int portC47Out)

{
unsigned char motControle;
    motControle = 0x80;
    
    if (portAOut   == FALSE) { motControle = (motControle | 0x10); }
    if (portBOut   == FALSE) { motControle = (motControle | 0x02); }
    if (portC03Out == FALSE) { motControle = (motControle | 0x01); }
    if (portC47Out == FALSE) { motControle = (motControle | 0x08); }

    selectionPage1();
    sysOutByte(pia_ctrl_IO  ,   motControle);    /* Operation sur le 8255  */
} 

/*--------------------------------------------------------*/

void ecriturePortA( unsigned char valeur)
{
    selectionPage1();
    sysOutByte(piaPortA,valeur);
}
/*--------------------------------------------------------*/

void ecriturePortB(unsigned char valeur)
{
    selectionPage1();
    sysOutByte(piaPortB,valeur);
}
/*--------------------------------------------------------*/

void ecriturePortC03(unsigned char valeur)
{
    selectionPage1();
    sysOutByte(piaPortC,(valeur & 0x0f));
}
/*--------------------------------------------------------*/
										  
void ecriturePortC47(unsigned char valeur)
{
    selectionPage1();
    sysOutByte(piaPortC,(valeur *16 ));
}

/*--------------------------------------------------------*/
unsigned char lecturePortA(void)
{
unsigned char codeIn;

    selectionPage1();
    codeIn = sysInByte(piaPortA);
    return (codeIn);
}

/*--------------------------------------------------------*/
unsigned char lecturePortB(void)
{
unsigned char codeIn;

    selectionPage1();
    codeIn = sysInByte(piaPortB);
    return (codeIn);
}    


/*--------------------------------------------------------*/
unsigned char lecturePortC03(void)
{
unsigned char codeIn;

    selectionPage1();
    codeIn = sysInByte(piaPortC);
    codeIn = (codeIn & 0x0f);
    return (codeIn);
}

/*--------------------------------------------------------*/
unsigned char lecturePortC47(void)
{
unsigned char codeIn;

    selectionPage1();
    codeIn = sysInByte(piaPortC);
    codeIn = (codeIn / 16);
    return (codeIn);
}

/*--------------------------------------------------------*/

/*--------------------------------------------------------*/
void setBitPort(unsigned char port,unsigned char numBit)
{
unsigned char valeurIn;
unsigned char masque;
unsigned char valeurOut;

    masque = (01 << numBit);
    switch (port)
    {
    case PORTA : {
                  valeurIn  =  lecturePortA();
                  valeurOut = (valeurIn | masque);
                  ecriturePortA(valeurOut);
                  break;}
    case PORTB : {
                  valeurIn  =  lecturePortB();
                  valeurOut = (valeurIn | masque);
                  ecriturePortB(valeurOut);
                  break;}
    case PORTC : {
                     if (numBit < 4)
                     {
                     valeurIn  =  lecturePortC03();
                     valeurOut = (valeurIn | masque);
                     ecriturePortC03(valeurOut);
                     }
                     else
                     {
                     valeurIn  =  lecturePortC47();
                     valeurIn  = (valeurIn << 4);
                     valeurOut = (valeurIn | masque);
                     valeurOut = (valeurOut >> 4);
                     ecriturePortC47(valeurOut);
                     }      
                  break;}
    }
}

/*--------------------------------------------------------*/
void clearBitPort(unsigned char port,unsigned char numBit)
{
unsigned char valeurIn;
unsigned char masque;
unsigned char valeurOut;

    masque = (01 << numBit);
    masque = ~(masque);
    switch (port)
    {
    case PORTA : {
                  valeurIn  =  lecturePortA();
                  valeurOut = (valeurIn & masque);
                  ecriturePortA(valeurOut);
                  break;}
    case PORTB : {
                  valeurIn  =  lecturePortB();
                  valeurOut = (valeurIn & masque);
                  ecriturePortB(valeurOut);
                  break;}
    case PORTC : {
                     if (numBit < 4)
                     {
                     valeurIn  =  lecturePortC03();
                     valeurOut = (valeurIn & masque);
                     ecriturePortC03(valeurOut);
                     }
                     else
                     {
                     valeurIn  =  lecturePortC47();
                     valeurIn  = (valeurIn << 4);
                     valeurOut = (valeurIn & masque);
                     valeurOut = (valeurOut >> 4);
                     ecriturePortC47(valeurOut);
                     }      
                  break;}
    }
}

/*--------------------------------------------------------*/
unsigned char testBitPort (unsigned char port,unsigned char numBit)
{
unsigned char valeurIn;
unsigned char masque;
unsigned char valeurOut;

    masque = (01 << numBit);
    switch (port)
    {
    case PORTA : {
                  valeurIn  =  lecturePortA();
                  break;}
    case PORTB : {
                  valeurIn  =  lecturePortB();
                  break;}
    case PORTC : {
                     if (numBit < 4)
                     {
                     valeurIn  =  lecturePortC03();
                     }
                     else
                     {
                     valeurIn  =  lecturePortC47();
                     valeurIn  = (valeurIn << 4);
                     }      
                  break;}
    }
     valeurOut = (valeurIn & masque);  /* Isoler le bit concerne */ 

     if(valeurOut != 0)
            { valeurOut = 1;}

     return(valeurOut);

}


/*--------------------------------------------------------*/
void inverseBitPort (unsigned char port,unsigned char numBit)
{
unsigned char valeurBit;

  valeurBit = testBitPort(port,numBit);

     if (valeurBit == 0)  setBitPort(port,numBit);  
     if (valeurBit == 1)  clearBitPort(port,numBit);  
    
}



