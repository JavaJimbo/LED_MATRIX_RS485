/* 
 * File:   Defs.h
 * Author: Jim
 *
 * Modified 3-25-16 for Rev 2 LED controller
 *  Modified 4-30-16 for Rev 3 LED controller
 * For DUAL MATRIX option and 32x32 panels
 * 6-7-16: Basic video works great with three serial ports.
 * 4-22-19: Modified for RS485 version
 * 
 */

#ifndef DEFS_H
#define	DEFS_H

#define CR 13
#define	STX '>'
#define	DLE '/'
#define	ETX CR

#define MAXPACKETSIZE 1024
#define MAX_PANEL_DATASIZE (32 * 16 * 3 * 3)
#define NUMCHANNELS 1
#define REV2BOARD 
// #define REV3BOARD  // $$$$

#define PANELS_ACROSS 2
#define PANELS_STACKED 2 
#define NUMPANELS (PANELS_ACROSS * PANELS_STACKED)
#define PANEL32X32

#define UART_STANDBY 0
#define UART_INCOMING 1
#define UART_DONE 2
#define UART_PACKET_OK 3
#define UART_CRC_ERROR 4
#define UART_LENGTH_ERROR 5
#define UART_TIMEOUT_ERROR 7
#define UART_PACKETERROR 8
#define UART_COMMAND_ERROR 9

#define Delayms DelayMs

#define MAXDATASIZE 10000


#ifdef PANEL32X32
#define PANELROWS 32
#define COLORDEPTH 3 // was 5
#define MAXLINE 16   // Equal to number of rows / 2
#define TIMER_ROLLOVER 2000
#define MAXCOL (PANELS_ACROSS*32)
#define MAXROW (PANELS_STACKED*32)  
#else
#define PANELROWS 16
#define COLORDEPTH 8
#define MAXLINE 8
#define TIMER_ROLLOVER 250
#define MAXCOL (PANELS_ACROSS*32)
#define MAXROW (PANELS_STACKED*16)
#endif
 
// NUMWRITES is the number of words written to the output port 
// per each color plane for each line on the matrix = Total columns
#define NUMWRITES MAXCOL

#define PANELCOLS 32
#define PANELSIZE (PANELROWS*PANELCOLS*NUMCHANNELS)
#define COMPRESSED_SIZE (PANELROWS*PANELCOLS*2)
#define BALANCE 48840
#define NUMPOTS 4

#define HOSTuart UART2
#define HOSTbits U2STAbits
#define XBEEuart UART4
#define XBEEbits U4STAbits
#define XBEE_VECTOR _UART_4_VECTOR
#define HOST_VECTOR _UART_2_VECTOR

#define RS485uart UART5

#define XBEE_SLEEP PORTBbits.RB15

#define XBEE_SLEEP_ON PORTSetBits(IOPORT_B, BIT_0)
#define XBEE_SLEEP_OFF PORTClearBits(IOPORT_B, BIT_0)

// #define TEST_OUT LATCbits.LATC4
#define TEST_OUT LATBbits.LATB0
#define RS485_TX_OUT LATBbits.LATB12
#define RS485_TX_ENABLE LATBbits.LATB12
#define DisableRS485_TX() PORTClearBits(IOPORT_B, BIT_12) 
#define EnableRS485_TX() PORTSetBits(IOPORT_B, BIT_12) 
#define START 1

#define MAXBUFFER 256
#define MAXHOSTBUFFER 256
#define MAXRS485BUFFER 4096 // 2048

#define XBEE_SLEEP PORTBbits.RB15

//#define RED_LSB 0b00001000 // (0x01<<(8-COLORDEPTH))
#define RED_LSB (0x01<<(8-COLORDEPTH))
#define GREEN_LSB (RED_LSB << 8)
#define BLUE_LSB  (RED_LSB << 16)

#define OEbit 0x0020
#define LATbit 0x0040

#ifdef REV3BOARD
#define LATCH_LOW 0x0000
#define LATCH_HIGH 0x6000
//#define LATCH_LOW 0x2000
//#define LATCH_HIGH 0x6000
#define OE_OFF() PORTClearBits(IOPORT_C, BIT_13)
#define OE_ON() PORTCSetBits(IOPORT_C, BIT_13)
#endif

#ifdef REV2BOARD
#define LATCH_LOW 0x80  // OE=1, LATCH=0
#define LATCH_HIGH 0x40  // OE=0, LATCH=1
#endif

//#define LATCH_LOW 0xC0  // 0x40
//#define LATCH_HIGH 0x80  
//#endif

// PIN DEFINITIONS FOR REV 2 LED MATRIX CONTROLLER


#ifdef REV3BOARD
#define R1bit 0x0100
#define B1bit 0x0001
#define G1bit 0x0200
#define R2bit 0x0002
#define B2bit 0x0800
#define G2bit 0x0400
#define M2R1bit 0x0080
#define M2B1bit 0x0040
#define M2G1bit 0x0020
#define M2R2bit 0x0004
#define M2B2bit 0x0008
#define M2G2bit 0x0010
#endif

#ifdef REV2BOARD
#define R1bit 0x0001
#define B1bit 0x0002
#define G1bit 0x0010
#define R2bit 0x0004
#define B2bit 0x0008
#define G2bit 0x0020

#define M2R1bit 0x0040
#define M2B1bit 0x0080
#define M2G1bit 0x0400
#define M2R2bit 0x0100
#define M2B2bit 0x0200
#define M2G2bit 0x0800
#endif

#define EVEN_MASK ~(R1bit | B1bit | G1bit | R2bit | B2bit | G2bit)
#define ODD_MASK  ~(M2R1bit | M2B1bit | M2G1bit | M2R2bit | M2B2bit | M2G2bit)


#define CLKOUT  0x0010  // PORTDbits.RD4
// #define OE_ENB  0xFFDF  // PORTDbits.RD5
// #define LATCH   0x0040  // PORTDbits.RD6




//#define DARKEN //& 0x7F7F7F


#define MAGENTA 0x1000FF
#define PURPLE 0x7F0070
#define CYAN 0x705000
#define LIME 0x00FF40
#define YELLOW 0x0070FE
#define ORANGE 0x0020FF
#define RED 0x0000FF
#define GREEN 0x00FF00
#define BLUE 0xFF0000
#define PINK 0x10207F
#define LAVENDER 0xF00030
#define TURQUOISE 0x70FF00
#define WHITE 0xFFFFFF // was 808080
#define GRAY 0x505050
#define DARKGRAY 0x202020
#define BLACK 0

#define MAXCOLOR 16

#define MAXRANDOM (RAND_MAX+1)
#define true	TRUE
#define false 	FALSE

#define RS485_CTRL PORTGbits.RG0

// Pin defs for UBW32:
#define CLKOUT  0x0010  // PORTDbits.RD4
// #define OE_ENB  0xFFDF  // PORTDbits.RD5
//#define LATCH   0x0040  // PORTDbits.RD6

#ifdef REV3BOARD
#define OEpin   LATCbits.LATC13
#define LATCH   LATCbits.LATC14
#else
#define OEpin   PORTDbits.RD5
#endif


#endif