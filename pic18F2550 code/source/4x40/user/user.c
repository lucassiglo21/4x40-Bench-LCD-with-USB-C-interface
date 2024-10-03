/*********************************************************************
 *
 *                Microchip USB C18 Firmware Version 1.0
 *
 *********************************************************************
 * FileName:        user.c
 * Dependencies:    See INCLUDES section below
 * Processor:       PIC18
 * Compiler:        C18 2.30.01+
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the ?Company?) for its PICmicro® Microcontroller is intended and
 * supplied to you, the Company?s customer, for use solely and
 * exclusively on Microchip PICmicro Microcontroller products. The
 * software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN ?AS IS? CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 * Author               Date        Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Rawin Rojvanit       11/19/04    Original.
 ********************************************************************/

/******************************************************************************
 * CDC RS-232 Emulation Tutorial Instructions:
 ******************************************************************************
 * Refer to Application Note AN956 for explanation of the CDC class.
 * 
 * First take a look at Exercise_Example() and study how functions are called.
 * 
 * There are five exercises, each one has a solution in the CDC\user\solutions.
 * Scroll down and look for Exercise_01,_02,_03,_04, and _05.
 * Instructions on what to do is inside each function.
 * 
 *****************************************************************************/

/** I N C L U D E S **********************************************************/
#include <p18f2550.h>
#include "system\typedefs.h"
#include "system\usb\usb.h"

#include "io_cfg.h"             // I/O pin mapping
#include "user\user.h"


/** V A R I A B L E S ********************************************************/
#pragma udata

char input_buffer_pointer;
char input_buffer_size;
char input_buffer_char;

char input_buffer[64];
char output_buffer[64];

//extern
//char cdc_rx_len;

char i;

char savecontroller;
char controller;
char transient;
char LCDready;
char LCDsetupstate;
char nextchar;
char onebyte;
char customdone;
char col;
char row;
char custom[9];
char backlight;
char contrast;
char position;

char getnextbyte();


/** P R I V A T E  P R O T O T Y P E S ***************************************/

char getnextbyte(void);
char getnewbuffer(void);
char getsUSBUSART(char *buffer, char len);

void whatcommand(void);
void setupLCD(void);
void sortCMD(void);
void LCDw(char onebyte);
void LCDch(char onebyte);
void LCDposition(char col, char row);


char LCDbusy(void);
char LCDcustom(void);


/** D E C L A R A T I O N S **************************************************/
#pragma code
void UserInit(void)
{
	TRISEN1=0;
	TRISEN2=0;
	TRISRS=0;
	TRISRW=0;
	TRISBL=0;
	TRISCT=0;
	TRISDATA=0;
	EN1=0;
	EN2=0;
	RS=0;
	RW=0;
	BL=0;
	CT=0;
	DATA=0;
	ADCON1=0x0F; //ALL DIGITAL
	CMCON=0x07;  //PORTA COMPARATORS OFF

	//SETUP HARDWARE PWM
	T2CON=0b0000111;
	TMR2=0;
	PR2=0xFF;
	CCP1CON=0b001100; //SET PWM lsb TO 11 AND CCP1 AS PWM
	CCP2CON=0b001100; //FOR CCP2
	CCPR1L=0b00000000; //10-BIT RESOLUTION
	CCPR2L=0b11111111;
	//END SETUP PWM

	customdone = 9;
	nextchar = 0;
	savecontroller = 0;
	controller = 3;
	backlight = 128;
	LCDsetupstate = 0;
	LCDready = 0;
}

/******************************************************************************
 * Overview:        This function is a place holder for other user routines.
 *                  It is a mixture of both USB and non-USB tasks.
 *****************************************************************************/
void ProcessIO(void)
{   
    if ((usb_device_state < CONFIGURED_STATE)||(UCONbits.SUSPND==1)){return;}
	if (!LCDready){setupLCD(); return;}
	if (LCDbusy()){return;}
	if (!LCDcustom()){return;}
	if (getnextbyte()){sortCMD();}
	return;
}
//end ProcessIO


void setupLCD(void)
{
	switch(LCDsetupstate)
	{
		case 0 : LCDsetupstate++; break;

		//config 8 bit mode, 2 line, display on
		case 1 : LCDw(0b00111100); LCDsetupstate++; break;
        // set VFD brightness
        case 2 : if(!LCDbusy()){LCDch(0b00000000); LCDsetupstate++;} break;
		//config cursor shifts right
		case 3 : if(!LCDbusy()){LCDw(0b00010100); LCDsetupstate++;} break;

		//config display on, cursor off, blinking off
		case 4 : if(!LCDbusy()){LCDw(0b00001100); LCDsetupstate++;} break;

		// clear display
		case 5 : if(!LCDbusy()){LCDw(0b00000001); LCDsetupstate++;} break;

		// return home
		case 6 : if(!LCDbusy()){LCDw(0b00000010); LCDsetupstate++;} break;

		// write my data
		case 7 : if(!LCDbusy()){LCDch(77); LCDsetupstate++;} break;
		case 8 : if(!LCDbusy()){LCDch(97); LCDsetupstate++;} break;
		case 9 : if(!LCDbusy()){LCDch(100); LCDsetupstate++;} break;
		case 10 : if(!LCDbusy()){LCDch(68); LCDsetupstate++;} break;
		case 11 : if(!LCDbusy()){LCDch(79); LCDsetupstate++;} break;
		case 12 : if(!LCDbusy()){LCDch(67); LCDsetupstate++;} break;
		//singnal LCDisSetup
		case 13 : LCDready=1; controller=1; break;
	}
}

void sortCMD(void)
{
	switch(nextchar)
	{
		case 0 : if(input_buffer_char!=254){LCDch(input_buffer_char); break;}
				 if(input_buffer_char=254){nextchar++; break;}

		case 1 : whatcommand(); break;

		case 2 : col=input_buffer_char; nextchar++; break;
		case 3 : row=input_buffer_char; LCDposition(col,row); nextchar=0; break;

		case 4 : custom[0]=input_buffer_char; nextchar++; break;
		case 5 : custom[1]=input_buffer_char; nextchar++; break;
		case 6 : custom[2]=input_buffer_char; nextchar++; break;
		case 7 : custom[3]=input_buffer_char; nextchar++; break;
		case 8 : custom[4]=input_buffer_char; nextchar++; break;
		case 9 : custom[5]=input_buffer_char; nextchar++; break;
		case 10 : custom[6]=input_buffer_char; nextchar++; break;
		case 11 : custom[7]=input_buffer_char; nextchar++; break;
		case 12 : custom[8]=input_buffer_char; customdone=0; nextchar=0; savecontroller=controller; controller=3; break;
        case 13 : CCPR1L=(255-input_buffer_char); nextchar=0; break; //contrast adjust
        case 14 : CCPR2L=input_buffer_char;  //brightness adjust
            savecontroller=controller; controller=3;
            LCDw(0b00111100);
            if (input_buffer_char <64)
            {
                LCDch(0b00000011);
            }
            else if(input_buffer_char >= 64 && input_buffer_char < 128)
            {
                LCDch(0b00000010);
            }
            else if(input_buffer_char >= 128 && input_buffer_char < 192)
            {
                LCDch(0b00000001);
            }  
            else if(input_buffer_char >= 192)
            {
                LCDch(0b00000000);
            }
            nextchar=0; 
            controller=savecontroller;
            break;
            
		case 98 : nextchar++; break;
		case 99 : nextchar=0; break; //discard 1 byte
	}
}

void whatcommand(void)
{
	switch(input_buffer_char)
	{
		//66[n] = backlight on for n min.
		case 66 : CCPR2L = backlight; nextchar=99; break; //discard minutes

		//70 = backlight off
		case 70 : CCPR2L = 0; nextchar=0; break;

		//71[col][row] = goto position
		case 71 : nextchar=2; break;

		//72 = goto home
		case 72 : controller=1; LCDw(128); nextchar=0; break;

		//74 = cursor on
		//case 74 : controller=3; LCDw(); nextchar=0; break;

		//75 = cursor off
		//case  :  nextchar=0; break;

		//78[n][8bytes] = define custom character
		case 78 : nextchar=4; break;

		//80[n] = contrast
		//case 80 : CCPR1L=input_buffer_char; nextchar=0; break;
        case 80 : nextchar=13; break;

		//83 = blink on ; 84 off
		//case  :  nextchar=0; break;

		//86[n] = gpo off; 87 on		//DISCARDED
		case 86 : nextchar=99; break;
		case 87 : nextchar=99; break;

		//88 = clear screen
		case 88 : controller=1; LCDw(128); nextchar=0; break; //doesn't clear display but is faster!!! 400x faster
		//case 88 : controller=3; LCDw(1); controller=1; nextchar=0; break;

		//102[n][pwm] = gpo pwm controll	//DISCARDED
		case 102 : nextchar=98; break;

		//145[n] = set & save contrast		//NOT SAVED
		//case 145 : CCPR1L=input_buffer_char; nextchar=0; break;
        case 145 : nextchar=13; break;

		//152[n] = set backlight PWM
		case 152 : nextchar=14; break;

		//153[n] = set & save backlight PWM	//NOT SAVED
		case 153 : nextchar=14; break;

		//254 = maybe it should be sent after all as character
		//case 254 : LCDch(254); nextchar=0; break;

		//discard other commands!
		default : nextchar=0; break;
	}
}

char LCDcustom()
{
	switch(customdone)
	{
		case 0 : LCDw(64+(custom[0]*8)); customdone++; break;
		case 1 : LCDch(custom[1]); customdone++; break;
		case 2 : LCDch(custom[2]); customdone++; break;
		case 3 : LCDch(custom[3]); customdone++; break;
		case 4 : LCDch(custom[4]); customdone++; break;
		case 5 : LCDch(custom[5]); customdone++; break;
		case 6 : LCDch(custom[6]); customdone++; break;
		case 7 : LCDch(custom[7]); customdone++; break;
		case 8 : LCDch(custom[8]); customdone++; controller = savecontroller; break;
		case 9 : return 1; break;
	}
	return 0;
}


/*** I/O with LCD ****************************************/

char LCDbusy(void)
{
	DATA=0;
	transient=0;
	TRISDATA=0xFF;
	RS=0;
	RW=1;
	Nop(); Nop(); Nop();
	EN1=1;
	Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop();
	transient=DATA;
	EN1=0;
	Nop(); Nop(); Nop();
	EN2=1;
	Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop();
	transient|=DATA;

	TRISDATA=0x00;
	EN1=0;
	EN2=0;
	Nop(); Nop();
	RW=0;
	if(transient<128){return 0;};
	return 1;
}

void LCDw(char onebyte)
{
	RS=0;
	RW=0;
	switch (controller)
	{
		case 1 : EN1=1; break;
		case 2 : EN2=1; break;
		case 3 : EN1=1; EN2=1; break;
	}
	DATA=onebyte;
	Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop();
	EN1=0;
	EN2=0;
}


void LCDch(char onebyte)
{
	RS=1;
	RW=0;
	Nop(); Nop(); Nop(); Nop();
	switch (controller)
	{
		case 1 : EN1=1; break;
		case 2 : EN2=1; break;
		case 3 : EN1=1; EN2=1; break;
	}
	DATA=onebyte;
	Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop(); Nop();
	EN1=0;
	EN2=0;
	Nop(); Nop();
	RS=0;
}

void LCDposition(char col, char row)
{
	switch (row)
	{
		case 1 : position=127; controller=1; break;
		case 2 : position=191; controller=1; break;
		case 3 : position=127; controller=2; break;
		case 4 : position=191; controller=2; break;
	}
	position+=(col);
	LCDw(position);
}


/*** USB Buffering ***************************************/

char getnewbuffer(void)
{
	if(getsUSBUSART(input_buffer,64))
	{
		input_buffer_size=cdc_rx_len;
		input_buffer_pointer=0;
		return 1;
	}
	return 0;
}

char getnextbyte(void)
{
	//if it's at the end of the buffer it needs to get a new one
	if(input_buffer_size<=input_buffer_pointer)
	{
		if(!getnewbuffer()){return 0;}	//couldn't get a new char
		input_buffer_pointer=0;			//could, so reset pointer
	}
	input_buffer_char=input_buffer[input_buffer_pointer];
	input_buffer_pointer++;
	return 1;
}

/** EOF user.c ***************************************************************/
