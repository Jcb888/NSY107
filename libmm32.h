/************************************************************)
(*                     S U P A E R O                        *)
(*                                                          *)
(*              Application roue a reaction                 *)
(*                   Os = linux+xenomai                     *)
(*                                                          *)
(*                        libmm32.h                         *)
(*                                                          *)
(*  Fonctions de gestion de la carte E/S analogiques DMM32  *)
(*                                                          *)
(************************************************************/
#if !defined(_LIBMM32_H)
#define  _LIBMM32_H

/* Definition de la constante FALSE */
#define FALSE -1

/* Definition de l'adresse de base de la carte */
#define BASE_MM32  0x340

/* SCAN intervalle (bits 5 et 4 du Analog Configuration Register) */

#define SCAN_20_MICROS_MM32 0
#define SCAN_15_MICROS_MM32 1
#define SCAN_10_MICROS_MM32 2
#define SCAN_5_MICROS_MM32 3
                   
/* Plage CAN (bits 3 et 2 du Analog Configuration Register) */   

#define CAN_M5P5_MM32   0
#define CAN_M10P10_MM32 1
#define CAN_0P10_MM32   2

/* gain CAN (bits  1 et 0 du Analog Configuration Register)  */  

#define GAIN_CAN_1_MM32 0
#define GAIN_CAN_2_MM32 1
#define GAIN_CAN_4_MM32 2
#define GAIN_CAN_8_MM32 3

#define PORTA 0
#define PORTB 1
#define PORTC 2


/* Declarations des fonctions prototypes */ 
				    
void initAcquisitionAnalogique(int periodeSCAN , int plageCAN ,int gainCAN);
void  ecritureVoieAnalogique (int numeroVoie, short code);
short acquisitionVoieAnalogique(unsigned char numeroVoie);
float acquisitionVolt(unsigned char numVoie);

void ecritureBitDout1(unsigned char etatBit);

void configurationPorts(int portAOut,int portBOut,int portC03Out,int portC47Out);
void ecriturePortA( unsigned char valeur);
void ecriturePortB( unsigned char valeur);
void ecriturePortC03( unsigned char valeur);
void ecriturePortC47( unsigned char valeur);
unsigned char lecturePortA( void);
unsigned char lecturePortB( void);
unsigned char lecturePortC03( void);
unsigned char lecturePortC47( void);
void setBitPort(unsigned char port,unsigned char numBit);
void clearBitPort(unsigned char port,unsigned char numBit);
unsigned char  testBitPort(unsigned char port,unsigned char numBit);
void inverseBitPort (unsigned char port,unsigned char numBit);

#endif //!defined(_LIBMM32_H) 
