
/*********************************************************************************************
 * LED_MATRIX_RS485
 * MAIN file for LED Controller RS485 Version
 * 4-22-19: Works great with both 16x32 and 32x32 panels.
 * 4-28-19: SYS_FREQ 80000000, Color depth = 7 but MSB = 0 to prevent ghosting, timer rollover = 250
 *          Also, general cleanup - eliminated unused methods.
 *          Added blanking bit, Increased color depth to 8 bits
 *********************************************************************************************/
#define SYS_FREQ 80000000
#define BOARD_ID (6 * 16)  // $$$$
#define ALL_BOARDS '0'

#include "Defs.h"
#include "Delay.h"
#include <plib.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#pragma config UPLLEN   = ON            // USB PLL Enabled
#pragma config FPLLMUL  = MUL_20        // PLL Multiplier
#pragma config UPLLIDIV = DIV_2         // USB PLL Input Divider
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider
#pragma config FPLLODIV = DIV_1         // PLL Output Divider
#pragma config FPBDIV   = DIV_1         // Peripheral Clock divisor
#pragma config FWDTEN   = OFF           // Watchdog Timer
#pragma config WDTPS    = PS1           // Watchdog Timer Postscale
#pragma config FCKSM    = CSDCMD        // Clock Switching & Fail Safe Clock Monitor
#pragma config OSCIOFNC = OFF           // CLKO Enable
#pragma config POSCMOD  = HS            // Primary Oscillator
#pragma config IESO     = OFF           // Internal/External Switch-over
#pragma config FSOSCEN  = OFF           // Secondary Oscillator Enable (KLO was off)
#pragma config FNOSC    = PRIPLL        // Oscillator Selection
#pragma config CP       = OFF           // Code Protect
#pragma config BWP      = OFF           // Boot Flash Write Protect
#pragma config PWP      = OFF           // Program Flash Write Protect
#pragma config ICESEL   = ICS_PGx2      // ICE/ICD Comm Channel Select


/** V A R I A B L E S ********************************************************/
unsigned char USBpanelData[MAX_PANEL_DATASIZE];
union {
    unsigned char b[2];
    unsigned short integer;
} convert;

#define HOSTuart UART2
#define HOSTbits U2STAbits
#define HOST_VECTOR _UART_2_VECTOR 
unsigned char HOSTTxBuffer[MAXHOSTBUFFER];
unsigned char HOSTRxBuffer[MAXHOSTBUFFER];
unsigned char HOSTRxData[MAXHOSTBUFFER];
unsigned short HOSTTxLength;
unsigned short HOSTRxLength;

#define RS485uart UART5
#define RS485bits U5STAbits
#define RS485_VECTOR _UART_5_VECTOR 
unsigned char RS485TxBuffer[MAXBUFFER];
unsigned char RS485RxBuffer[MAXRS485BUFFER];
unsigned char RS485RxBufferCopy[MAXRS485BUFFER];
unsigned char RS485RxData[MAXRS485BUFFER];
unsigned char tempPanel[PANELSIZE];
unsigned short RS485TxLength;
unsigned short RS485RxLength;
unsigned char RS485BufferFull = false;

#define MAXPOTS 4

unsigned char DmaIntFlag = false;
unsigned char command = 0;
unsigned char arrPots[MAXPOTS];
unsigned char displayMode = TRUE;

// extern unsigned long colorWheel[MAXCOLOR];
unsigned short matrixOutData[(NUMWRITES*(COLORDEPTH+1)*MAXLINE)];
unsigned short line = 0;
unsigned short dataOffset = 0;
unsigned char colorPlane = 0;
unsigned short latchHigh[1] = {LATCH_HIGH};

unsigned long Delaymultiplier = TIMER_ROLLOVER;

unsigned long redAdjust, greenAdjust, blueAdjust, brightAdjust;
extern BOOL CRCcheck(unsigned char *ptrPacket, short length);
extern UINT16 CRCcalculate(unsigned char *ptrPacket, short length, BOOL addCRCtoPacket);
unsigned long ConvertColorToLong (unsigned char byteColor);
void InitializeDMA();


unsigned long matrix[MAXROW][MAXCOL];
unsigned long panelData[NUMPANELS][PANELROWS][PANELCOLS];
unsigned char panelUpdateBytes[PANELSIZE];
long HOSTuartActualBaudrate, RS485uartActualBaudrate;
unsigned long colorWheel[MAXCOLOR] = {MAGENTA, PURPLE, CYAN, LIME, YELLOW, ORANGE, RED, GREEN, BLUE, PINK, LAVENDER, TURQUOISE, GRAY, DARKGRAY, WHITE, BLACK};

/** P R I V A T E  P R O T O T Y P E S ***************************************/
static void InitializeSystem(void);
void ProcessIO(void);
void USBDeviceTasks(void);
void YourHighPriorityISRCode();
void YourLowPriorityISRCode();
void USBCBSendResume(void);
void BlinkUARTstatus(void);
void UserInit(void);

unsigned short decodePacket(unsigned char *ptrPacket, unsigned char *ptrBitmap);
unsigned short processPanelData(unsigned char *ptrReceivedData, unsigned short packetLength);
void loadMatrix(void);
void updateOutputBuffer(void);
void ConfigAd(void);
unsigned long getRGBColor(unsigned short crossfade, unsigned short potVal);
unsigned char updateOutputPanel(unsigned char panelNumber);
unsigned char updateOutputPanelPtr(unsigned char panelNumber, unsigned char *ptrPanelData);
unsigned char uncompressData(unsigned char *ptrCompressedData, unsigned char *ptrPanelData);
short packetSize = 0;
unsigned short packetCounter = 0;
unsigned char BoardAlias = 0;

void resetTimer(void) {
    line = 0;
    dataOffset = 0;
    colorPlane = 0;
    Delaymultiplier = TIMER_ROLLOVER;
    mT23ClearIntFlag();
    OpenTimer23(T23_ON | T23_SOURCE_INT | T23_PS_1_1, TIMER_ROLLOVER);
}

/*
void initMatrix(unsigned long color) {
    unsigned short i, j;

    for (i = 0; i < (MAXROW); i++) {
        for (j = 0; j < MAXCOL; j++) {
            // matrixPrev[i][j] = matrix[i][j] = color;
            // matrixBlackout[MAXROW][MAXCOL] = 0;
            matrix[i][j] = color;
        }
    }
}
*/

void initPanels(unsigned long color)
{
    unsigned short panel, row, col;
    int i;

    for (panel = 0; panel < NUMPANELS; panel++)
        for (row = 0; row < PANELROWS; row++)
            for (col = 0; col < PANELCOLS; col++)
                panelData[panel][row][col] = color;
        
    for (i = 0; i < NUMPANELS; i++) updateOutputPanel(i);        
}

#define BARWIDTH (MAXCOL/MAXCOLOR)

void panelColorBars(void) 
{
    unsigned short i, row, col;
    unsigned char colorIndex = 0, k = BARWIDTH;
    unsigned long color;

    i = 0;
    do {        
        for (col = 0; col < PANELCOLS; col++) {
            for (row = 0; row < PANELROWS; row++) {
                if (colorIndex < MAXCOLOR) 
                {
                    color = colorWheel[colorIndex];
                    panelData[i][row][col] = color;
                    panelData[i + 1][row][col] = color;
                }
            }
            if (k)k--;
            if (!k) 
            {
                colorIndex++;
                k = BARWIDTH;
            }
        }
        i = i + 2;
    } while (i < (PANELS_ACROSS * 2));
    for (i = 0; i < NUMPANELS; i++) updateOutputPanel(i);
}


void ClearRxBuffer()
{
    int i;
    for (i = 0; i < MAXHOSTBUFFER; i++) HOSTRxBuffer[i] = '\0';
}

int main(void) {
    unsigned long i;    
    int dataLength;
    unsigned char command, subCommand;
    unsigned char CRCcheckOK = false;
    unsigned char PanelComplete = false;
    short PanelNumber, BoardNumber;
    short row, column, colorValue;
    
    BoardAlias = (BOARD_ID / 16) + '0';
    
    for (i = 0; i < (NUMWRITES * (COLORDEPTH+1) * MAXLINE); i++) matrixOutData[i] = (unsigned short) BLACK;
    // panelColorBars();
    initPanels(BLACK);
    InitializeSystem();
    
    DisableRS485_TX();

    // TESTupdateOutputPanel(0);
    // ColorBars();
    
    DelayMs(100);    
    
    //printf("\r\rHost Baud rate: %d", HOSTuartActualBaudrate);
    //printf("\rRS485 baud rate: %d", RS485uartActualBaudrate);    
    printf("\rBoard #%d Start up...", (BOARD_ID/16));    

    while(1) 
    {                
        if (DmaIntFlag)
        {
            DmaIntFlag = false;  
            dataLength = decodePacket(RS485RxBufferCopy, RS485RxData);
            if (dataLength != 0)
            {           
                CRCcheckOK = CRCcheck(RS485RxData, dataLength);
                if (CRCcheckOK) processPanelData(RS485RxData, dataLength-2);                    
                else printf("\r#%d >CRC ERROR", packetCounter++);
                    
                command = RS485RxData[1]; // $$$$
                subCommand = RS485RxData[0] & 0xF0;
                PanelComplete = RS485RxData[0] & 0x08;
                PanelNumber = RS485RxData[0] & 0x07;
                BoardNumber = (RS485RxData[0] / 16);
                
                if (CRCcheckOK)
                {
                    #ifdef PANEL32X32
                        if (subCommand == BOARD_ID) 
                        {
                            if (PanelComplete) printf("\r>>#%d: Board: %d, Panel: %d, TOP", packetCounter++, BoardNumber, PanelNumber);
                            else printf("\r>#%d: Board: %d, Panel: %d, BOTTOM", packetCounter++, BoardNumber, PanelNumber);
                        }
                        else
                        {
                            printf("\r#%d: Board: %d, Panel: %d", packetCounter++, BoardNumber, PanelNumber);                                                        
                        }
                    #else
                        if (subCommand == BOARD_ID) printf("\r>#%d: Board: %d, Panel: %d", packetCounter++, BoardNumber, PanelNumber);
                        else printf("\r#%d: Board: %d, Panel: %d", packetCounter++, BoardNumber, PanelNumber);
                    #endif
                }                
                else if ((RS485RxData[0] == BoardAlias || RS485RxData[0] == ALL_BOARDS) && command != 0)
                {                    
                    printf("\rCommand: %c, Board #%c", command, subCommand);
                    
                    if (command == '?')      
                        printf("\r???");
                    else if (command == 'C')
                    {
                        printf("\rColor bars");
                        panelColorBars(); 
                    }
                    else if (command == ' ')
                    {
                        printf("\rCommand: BLACKOUT");                        
                        initPanels(BLACK);
                    }
                    else if (command == 'P')
                    {
                        for (row = 0; row < PANELROWS; row++)
                        {
                            printf("\rROW %d: ", row);
                            for (column = 0; column < PANELCOLS; column++)    
                            {
                                colorValue = panelData[3][row][column];
                                printf("%d ", colorValue);
                            }
                        }
                    }
                    command = 0;
                }
            }    
            else printf ("\rERROR: No data!");            
        }        
        
        if (HOSTRxLength) 
        {
            printf("\rRECEIVED: %s", HOSTRxBuffer);
            HOSTRxLength = 0;            
        }
        
        if (RS485RxLength) 
        {
            printf("\rRECEIVED RS485: %s", RS485RxBuffer);
            RS485RxLength = 0;            
        }
        DelayMs(1);

    }//end while
}//end main


unsigned short getShort(unsigned char b0, unsigned char b1) {

    union {
        unsigned char b[2];
        unsigned short integer;
    } convert;

    convert.b[0] = b0;
    convert.b[1] = b1;
    return (convert.integer);
}


/*
unsigned long getLongInteger(unsigned char b0, unsigned char b1, unsigned char b2, unsigned char b3) {

    union {
        unsigned b[4];
        unsigned long lngInteger;
    } convert;

    convert.b[0] = b0;
    convert.b[1] = b1;
    convert.b[2] = b2;
    convert.b[3] = b3;

    return (convert.lngInteger);
}
*/


/*
void ConfigAd(void) {

    mPORTBSetPinsAnalogIn(BIT_0);
    mPORTBSetPinsAnalogIn(BIT_1);
    mPORTBSetPinsAnalogIn(BIT_2);
    mPORTBSetPinsAnalogIn(BIT_3);

    // ---- configure and enable the ADC ----

    // ensure the ADC is off before setting the configuration
    CloseADC10();

    // define setup parameters for OpenADC10
    //                 Turn module on | ouput in integer | trigger mode auto | enable autosample
#define PARAM1  ADC_MODULE_ON | ADC_FORMAT_INTG | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_ON

    // ADC ref external    | disable offset test    | enable scan mode | perform  samples | use dual buffers | use only mux A
#define PARAM2  ADC_VREF_AVDD_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_ON | ADC_SAMPLES_PER_INT_4 | ADC_ALT_BUF_ON | ADC_ALT_INPUT_OFF

    //                   use ADC internal clock | set sample time
#define PARAM3  ADC_CONV_CLK_INTERNAL_RC | ADC_SAMPLE_TIME_31

    //  set inputs to analog
#define PARAM4    ENABLE_AN0_ANA | ENABLE_AN1_ANA| ENABLE_AN2_ANA | ENABLE_AN3_ANA

    // Only scan AN0, AN1, AN2, AN3 for now
#define PARAM5   SKIP_SCAN_AN4 |SKIP_SCAN_AN5 |SKIP_SCAN_AN6 |SKIP_SCAN_AN7 |\
                    SKIP_SCAN_AN8 |SKIP_SCAN_AN9 |SKIP_SCAN_AN10 |\
                      SKIP_SCAN_AN11 | SKIP_SCAN_AN12 |SKIP_SCAN_AN13 |SKIP_SCAN_AN14 |SKIP_SCAN_AN15

    // set negative reference to Vref for Mux A
    SetChanADC10(ADC_CH0_NEG_SAMPLEA_NVREF);

    // open the ADC
    OpenADC10(PARAM1, PARAM2, PARAM3, PARAM4, PARAM5);

    ConfigIntADC10(ADC_INT_PRI_2 | ADC_INT_SUB_PRI_2 | ADC_INT_ON);

    // clear the interrupt flag
    mAD1ClearIntFlag();

    // Enable the ADC
    EnableADC10();

}*/

void __ISR(_ADC_VECTOR, ipl6) AdcHandler(void) {
    unsigned short offSet;
    unsigned char i;

    mAD1IntEnable(INT_DISABLED);
    mAD1ClearIntFlag();

    // Determine which buffer is idle and create an offset
    offSet = 8 * ((~ReadActiveBufferADC10() & 0x01));

    for (i = 0; i < MAXPOTS; i++)
        arrPots[i] = (unsigned char) (ReadADC10(offSet + i) / 4); // read the result of channel 0 conversion from the idle buffer

}

void __ISR(_TIMER_23_VECTOR, ipl2) Timer23Handler(void) 
{
    static unsigned char line = 0;
    static unsigned char colorPlane = 0;
    static unsigned long Delaymultiplier = TIMER_ROLLOVER;
    static unsigned short dataOffset = 0;
    static unsigned char testFlag = FALSE;

    mT23ClearIntFlag(); // Clear interrupt flag    

    if (colorPlane == COLORDEPTH) TEST_OUT = 0;
    else if ((colorPlane % 2) == 0) TEST_OUT = 1;
    else TEST_OUT = 0;
    
    WritePeriod23(Delaymultiplier);
    PORTD = line;
#ifdef REV3BOARD
    LATC = LATCH_LOW;
#else
    PORTA = LATCH_LOW;
#endif
    DmaChnSetTxfer(DMA_CHANNEL1, &matrixOutData[dataOffset], (void*) &PMDIN, NUMWRITES * 2, 2, 2);
    DmaChnEnable(DMA_CHANNEL1);
    DmaChnEnable(DMA_CHANNEL2);    
    dataOffset = dataOffset + NUMWRITES;
    Delaymultiplier = Delaymultiplier << 1;
    colorPlane++;
    if (colorPlane > COLORDEPTH) 
    {
        colorPlane = 0;
        Delaymultiplier = TIMER_ROLLOVER;
        line++;
        if (line >= MAXLINE) 
        {
            line = 0;
            dataOffset = 0;
        }
    }
    else if (colorPlane == COLORDEPTH)
        Delaymultiplier = TIMER_ROLLOVER;
}



    
unsigned short decodePacket(unsigned char *ptrInPacket, unsigned char *ptrData) 
{
    unsigned short i, j;
    unsigned char escapeFlag = FALSE;
    unsigned char startFlag = false;
    unsigned char ch;

    j = 0;
    for (i = 0; i < MAXRS485BUFFER; i++) 
    {
        ch = ptrInPacket[i];
        // Escape flag not active
        if (!escapeFlag) 
        {
            if (ch == STX) 
            {
                if (!startFlag) 
                {
                    startFlag = true;
                    j = 0;
                }
                else return (0);
            } 
            else if (ch == ETX) 
                return (j);
            else if (ch == DLE)
                escapeFlag = TRUE;
            else if (j < MAXRS485BUFFER)
                ptrData[j++] = ch;
            else return (0);
        } 
        // Escape flag active
        else 
        {
            escapeFlag = FALSE;
            if (ch == ETX-1) ch = ETX;            
            if (j < MAXRS485BUFFER)
                ptrData[j++] = ch;
            else return(0);
        }
    }
    return (j);
}







// RS485 UART interrupt handler it is set at priority level 2
/*
void __ISR(RS485_VECTOR, ipl2) IntRS485UartHandler(void) 
{
    static unsigned short RS485RxIndex = 0;
    unsigned char ch;

    if (RS485bits.OERR || RS485bits.FERR) {
        if (UARTReceivedDataIsAvailable(RS485uart))
            ch = UARTGetDataByte(RS485uart);
        RS485bits.OERR = 0;
        RS485RxIndex = 0;
    } else if (INTGetFlag(INT_SOURCE_UART_RX(RS485uart))) {
        INTClearFlag(INT_SOURCE_UART_RX(RS485uart));
        if (UARTReceivedDataIsAvailable(RS485uart)) {
            ch = UARTGetDataByte(RS485uart);
            if (ch != 0 && ch != '\n') {
                if (RS485RxIndex < MAXRS485BUFFER - 2)
                    RS485RxBuffer[RS485RxIndex++] = ch;
                if (ch == '\r') {
                    RS485RxBuffer[RS485RxIndex] = '\0';
                    RS485RxLength = RS485RxIndex;
                    RS485RxIndex = 0;
                }
            }
            // UARTtimeout=UART_TIMEOUT;
        }
    }    
    if (INTGetFlag(INT_SOURCE_UART_TX(RS485uart))) 
        INTClearFlag(INT_SOURCE_UART_TX(RS485uart));
}
*/

// HOST UART interrupt handler it is set at priority level 2
void __ISR(HOST_VECTOR, ipl2) IntHostUartHandler(void) 
{
    static unsigned short HOSTRxIndex = 0;
    static unsigned char TxIndex = 0;
    unsigned char ch;

    if (HOSTbits.OERR || HOSTbits.FERR) {
        if (UARTReceivedDataIsAvailable(HOSTuart))
            ch = UARTGetDataByte(HOSTuart);
        HOSTbits.OERR = 0;
        HOSTRxIndex = 0;
    } else if (INTGetFlag(INT_SOURCE_UART_RX(HOSTuart))) {
        INTClearFlag(INT_SOURCE_UART_RX(HOSTuart));
        if (UARTReceivedDataIsAvailable(HOSTuart)) {
            ch = UARTGetDataByte(HOSTuart);
            if (ch != 0 && ch != '\n') {
                if (HOSTRxIndex < MAXHOSTBUFFER - 2)
                    HOSTRxBuffer[HOSTRxIndex++] = ch;
                if (ch == '\r') {
                    HOSTRxBuffer[HOSTRxIndex] = '\0';
                    HOSTRxLength = HOSTRxIndex;
                    HOSTRxIndex = 0;
                }
            }
            // UARTtimeout=UART_TIMEOUT;
        }
    }
    if (INTGetFlag(INT_SOURCE_UART_TX(HOSTuart))) {
        INTClearFlag(INT_SOURCE_UART_TX(HOSTuart));
        if (HOSTTxLength) {
            if (TxIndex < MAXHOSTBUFFER) {
                ch = HOSTTxBuffer[TxIndex++];
                if (TxIndex <= HOSTTxLength) {
                    while (!UARTTransmitterIsReady(HOSTuart));
                    UARTSendDataByte(HOSTuart, ch);
                } else {
                    while (!UARTTransmitterIsReady(HOSTuart));
                    HOSTTxLength = false;
                    TxIndex = 0;
                }
            } else {
                TxIndex = 0;
                HOSTTxLength = false;
                INTEnable(INT_SOURCE_UART_TX(HOSTuart), INT_DISABLED);
            }
        } else INTEnable(INT_SOURCE_UART_TX(HOSTuart), INT_DISABLED);
    }
}

// handler for the DMA channel 1 interrupt
void __ISR(_DMA0_VECTOR, IPL5SOFT) DmaHandler0(void)
{
    int i;
    unsigned char ch;
	int	evFlags;				// event flags when getting the interrupt

	INTClearFlag(INT_SOURCE_DMA(DMA_CHANNEL0));	// release the interrupt in the INT controller, we're servicing int

	evFlags=DmaChnGetEvFlags(DMA_CHANNEL0);	// get the event flags

    if(evFlags&DMA_EV_BLOCK_DONE)
    { // just a sanity check. we enabled just the DMA_EV_BLOCK_DONE transfer done interrupt
        i = 0;
        do {
            ch = RS485RxBuffer[i];
            RS485RxBufferCopy[i++] = ch;            
        } while (i < MAXRS485BUFFER && ch != ETX);
        DmaIntFlag =  true;
        DmaChnClrEvFlags(DMA_CHANNEL0, DMA_EV_BLOCK_DONE);       
        DmaChnEnable(DMA_CHANNEL0);
        resetTimer();
    }
}


/*
unsigned long ConvertColorToLong (unsigned char byteColor)
{    
    unsigned long red, green, blue;
    unsigned long rred, ggreen, bblue;
    unsigned long lngColor;
    
    red = (unsigned long) ((byteColor & 0b11100000) >> 5);
    green = (unsigned long) ((byteColor & 0b00011100) >> 2);
    blue = (unsigned long) (byteColor & 00000011);
    
    rred = ((red + 1) * 32) - 1;
    ggreen = ((green + 1) * 32) - 1;
    bblue = ((blue + 1) * 64) - 1;
    
    lngColor = (rred << 16) + (ggreen << 8) + bblue;
    return lngColor;
}
*/

static void InitializeSystem(void) {
    SYSTEMConfigPerformance(80000000); // was 60000000
    UserInit();

    //variables to known states.
}//end InitializeSystem


void UserInit(void) {
    PORTSetPinsDigitalOut(IOPORT_E, BIT_0 | BIT_1 | BIT_2 | BIT_3);

    // Turn off JTAG so we get the pins back
    mJTAGPortEnable(false);

#ifdef REV3BOARD 
    PORTSetPinsDigitalOut(IOPORT_A, BIT_0);
#else
    PORTSetPinsDigitalOut(IOPORT_A, BIT_6 | BIT_7); // `B
#endif

    PORTD = 0x0000; // Set matrix output enable high. All other outputs low.
#ifdef REV3BOARD
    OEpin = 1;
#else    
    OEpin = 0;
#endif    

#ifdef PANEL32X32    
    PORTSetPinsDigitalOut(IOPORT_D, BIT_0 | BIT_1 | BIT_2 | BIT_3); // For 32x32 matrix
#else
    // PORTSetPinsDigitalOut(IOPORT_D, BIT_0 | BIT_1 | BIT_2 | BIT_13); // For 16x32 matrix
    PORTSetPinsDigitalOut(IOPORT_D, BIT_0 | BIT_1 | BIT_2); // For 16x32 matrix
#endif    
    // Set up Timer 2/3 as a single 32 bit timer with interrupt priority of 2
    // Use internal clock, 1:1 prescale
    // If Postscale = 1600 interrupts occur about every 33 uS
    // This yields a refresh rate of about 59 hz for entire display
    // The flicker seems pretty minimal at this rate
    // 1 / 33 us x 8 x 2^6 = 59
    // 8 lines x 32 columns x 6 panels x 6 bit resolution = 9216 writes to PORT D for each refresh!       
    ConfigIntTimer23(T2_INT_ON | T2_INT_PRIOR_2);
    OpenTimer23(T23_ON | T23_SOURCE_INT | T23_PS_1_1, TIMER_ROLLOVER);

    PORTSetPinsDigitalOut(IOPORT_B, BIT_0 | BIT_1 | BIT_2 | BIT_12 | BIT_15);    
    PORTSetBits(IOPORT_B, BIT_0 | BIT_1 | BIT_2 | BIT_12);
    PORTClearBits(IOPORT_B, BIT_12);  // RS485 control set to disable TX
    XBEE_SLEEP_OFF;

    // Set up Port C outputs:
#ifdef REV3BOARD
    // RtccShutdown();
    PORTSetPinsDigitalOut(IOPORT_C, BIT_3 | BIT_13 | BIT_14 | BIT_4);
#else
    PORTSetPinsDigitalOut(IOPORT_C, BIT_3 | BIT_4);
#endif
    // Set up Port G outputs:
    PORTSetPinsDigitalOut(IOPORT_G, BIT_0);
    RS485_CTRL = 0;

#ifdef REV3BOARD
    LATC = LATCH_LOW;
#else    
    PORTA = LATCH_LOW;
#endif    
    
    // CONFIGURE DMA CHANNEL 0 FOR UART 5 INTERRUPT ON MATCH    
    ClearRxBuffer();
    
    DmaChnOpen(DMA_CHANNEL0, 1, DMA_OPEN_MATCH);
    
    DmaChnSetMatchPattern(DMA_CHANNEL0, ETX);	// set < as ending character

    // Set the transfer event control: what event is to start the DMA transfer    
	// We want the UART2 rx interrupt to start our transfer
	// also we want to enable the pattern match: transfer stops upon detection of CR
	DmaChnSetEventControl(DMA_CHANNEL0, DMA_EV_START_IRQ_EN|DMA_EV_MATCH_EN|DMA_EV_START_IRQ(_UART5_RX_IRQ));    
        
	// Set the transfer source and dest addresses, source and dest sizes and the cell size
	DmaChnSetTxfer(DMA_CHANNEL0, (void*)&U5RXREG, RS485RxBuffer, 1, MAXRS485BUFFER, 1);    

    // Enable the transfer done event flag:
    DmaChnSetEvEnableFlags(DMA_CHANNEL0, DMA_EV_BLOCK_DONE);		// enable the transfer done interrupt: pattern match or all the characters transferred

	INTSetVectorPriority(INT_VECTOR_DMA(DMA_CHANNEL0), INT_PRIORITY_LEVEL_5);		// set INT controller priority
	INTSetVectorSubPriority(INT_VECTOR_DMA(DMA_CHANNEL0), INT_SUB_PRIORITY_LEVEL_3);		// set INT controller sub-priority
	INTEnable(INT_SOURCE_DMA(DMA_CHANNEL0), INT_ENABLED);		// enable the chn interrupt in the INT controller    

    // Once we configured the DMA channel we can enable it
    DmaChnEnable(DMA_CHANNEL0);
    // DmaChnForceTxfer(DMA_CHANNEL0);
    

#define PMP_CONTROL	(PMP_ON | PMP_MUX_OFF | PMP_READ_WRITE_EN | PMP_WRITE_POL_HI) 

#define PMP_MODE        (PMP_IRQ_READ_WRITE | PMP_MODE_MASTER1 | PMP_DATA_BUS_16 | PMP_WAIT_BEG_1 | PMP_WAIT_MID_0 | PMP_WAIT_END_1)
#define PMP_PORT_PINS	PMP_PEN_ALL
#define PMP_INTERRUPT	PMP_INT_OFF

    // setup the PMP
    mPMPOpen(PMP_CONTROL, PMP_MODE, PMP_PORT_PINS, PMP_INTERRUPT);


    // CONFIGURE DMA CHANNEL 1 - SEND DATA
    DmaChnOpen(DMA_CHANNEL1, 3, DMA_OPEN_DEFAULT);

    // Set the transfer event control: what event is to start the DMA transfer
    DmaChnSetEventControl(DMA_CHANNEL1, DMA_EV_START_IRQ(_PMP_IRQ));

    // Set up transfer destination: PMDIN Parallel Port Data Input Register
    DmaChnSetTxfer(DMA_CHANNEL1, &matrixOutData[0], (void*) &PMDIN, NUMWRITES, 2, 2);

    // Enable the transfer done event flag:
    DmaChnSetEvEnableFlags(DMA_CHANNEL1, DMA_EV_BLOCK_DONE);


    // Once we configured the DMA channel we can enable it
    DmaChnEnable(DMA_CHANNEL1);

    DmaChnForceTxfer(DMA_CHANNEL1);

    // CONFIGURE DMA CHANNEL 2 - LATCH DATA
    DmaChnOpen(DMA_CHANNEL2, 2, DMA_OPEN_DEFAULT);

    // Set the transfer event control: what event is to start the DMA transfer
    DmaChnSetEventControl(DMA_CHANNEL2, DMA_EV_START_IRQ(_DMA1_IRQ));

    // Set up transfer: source & destination, source & destination size, number of bytes trasnferred per event
#ifdef REV3BOARD
    DmaChnSetTxfer(DMA_CHANNEL2, latchHigh, (void*) &LATC, sizeof (latchHigh), 2, 2);
#else
    DmaChnSetTxfer(DMA_CHANNEL2, latchHigh, (void*) &LATA, sizeof (latchHigh), 2, 2);
#endif
    // Enable the transfer done event flag:
    DmaChnSetEvEnableFlags(DMA_CHANNEL2, DMA_EV_CELL_DONE);

    // Once we configured the DMA channel we can enable it
    DmaChnEnable(DMA_CHANNEL2);
    
    // Set up RS485 UART #5
    UARTConfigure(RS485uart, UART_ENABLE_HIGH_SPEED | UART_ENABLE_PINS_TX_RX_ONLY);
    UARTSetLineControl(RS485uart, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);

    RS485uartActualBaudrate = (long) UARTSetDataRate(RS485uart, SYS_FREQ, 921600); // Was 230400
    UARTEnable(RS485uart, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));

    // Configure RS485 UART Interrupts
    INTEnable(INT_U5TX, INT_DISABLED);
    INTEnable(INT_SOURCE_UART_RX(RS485uart), INT_DISABLED);
    INTSetVectorPriority(INT_VECTOR_UART(RS485uart), INT_PRIORITY_LEVEL_2);
    

    // Set up HOST UART #2
    UARTConfigure(HOSTuart, UART_ENABLE_HIGH_SPEED | UART_ENABLE_PINS_TX_RX_ONLY);
    UARTSetLineControl(HOSTuart, UART_DATA_SIZE_8_BITS | UART_PARITY_NONE | UART_STOP_BITS_1);

    HOSTuartActualBaudrate = (long) UARTSetDataRate(HOSTuart, SYS_FREQ, 921600); // Was 230400
    UARTEnable(HOSTuart, UART_ENABLE_FLAGS(UART_PERIPHERAL | UART_RX | UART_TX));

    // Configure HOST UART Interrupts
    INTEnable(INT_U2TX, INT_DISABLED);
    INTEnable(INT_SOURCE_UART_RX(HOSTuart), INT_ENABLED);
    INTSetVectorPriority(INT_VECTOR_UART(HOSTuart), INT_PRIORITY_LEVEL_2);
    // INTSetVectorSubPriority(INT_VECTOR_UART(HOSTuart), INT_SUB_PRIORITY_LEVEL_0);        

    // Turn on the interrupts
    INTEnableSystemMultiVectoredInt();
    
    
    PORTSetPinsDigitalOut(IOPORT_B, BIT_0 | BIT_1 | BIT_2 | BIT_12 | BIT_15);    
    PORTSetBits(IOPORT_B, BIT_0 | BIT_1 | BIT_2 | BIT_12);


}//end UserInit


// RS485 VERSION
unsigned short processPanelData(unsigned char *ptrData, unsigned short dataLength)
{
    unsigned char lowByte, highByte, command, subCommand, panelNumber;
    unsigned short lengthCheck, packetCheck;
    unsigned short i, j, row, column;

    packetCheck = dataLength;

    command = ptrData[1]; 
    subCommand = ptrData[0];
    if ((subCommand & 0xF0) != BOARD_ID) return (0);     
    
    panelNumber = subCommand & 0x07;    
    
    lowByte = ptrData[2];
    highByte = ptrData[3];

    lengthCheck = getShort(lowByte, highByte);
    if (lengthCheck != (dataLength - 4))
        return (UART_LENGTH_ERROR);

    if (panelNumber >= NUMPANELS) return (UART_COMMAND_ERROR);    
    
#ifdef PANEL32X32    
    i = 4;
    if (panelNumber == 0)
    {     
        panelNumber = 3;
        for (row = (0); row < (PANELROWS/2); row++)
            for (column = 0; column < PANELCOLS; column++)          
                panelData[panelNumber][row][column] = colorWheel[ptrData[i++]];               
    }        
    else if (panelNumber == 1) 
    {
        panelNumber = 3;
        for (row = (PANELROWS/2); row < (PANELROWS); row++)
            for (column = 0; column < PANELCOLS; column++)          
                panelData[panelNumber][row][column] = colorWheel[ptrData[i++]];            
    }        
    else if (panelNumber == 2)
    {     
        panelNumber = 2;
        for (row = 0; row < (PANELROWS/2); row++)
            for (column = 0; column < PANELCOLS; column++)          
                panelData[panelNumber][row][column] = colorWheel[ptrData[i++]];    
    }    
    else if (panelNumber == 3)
    {
        panelNumber = 2;
        for (row = PANELROWS/2; row < PANELROWS; row++)
            for (column = 0; column < PANELCOLS; column++)          
                panelData[panelNumber][row][column] = colorWheel[ptrData[i++]];            
    }           
    updateOutputPanel(panelNumber);
#else     
    i = 4;
    for (row = 0; row < PANELROWS; row++)
        for (column = 0; column < PANELCOLS; column++)          
            panelData[panelNumber][row][column] = colorWheel[ptrData[i++]];    
    updateOutputPanel(panelNumber);    
#endif 
    return (UART_PACKET_OK);
}


unsigned char updateOutputPanel(unsigned char panelNumber) 
{
    unsigned short line, col;
    unsigned char PWMbit;
    unsigned short outDataIndex, offset;
    unsigned short outputData;
    unsigned long matrixData, REDmask, GREENmask, BLUEmask;
    unsigned char evenFlag;

    if ((panelNumber % 2) == 0) evenFlag = TRUE;
    else evenFlag = FALSE;

    offset = (PANELS_ACROSS - 1) * PANELCOLS;
    outDataIndex = (panelNumber / 2) * PANELCOLS;

    for (line = 0; line < MAXLINE; line++) 
    {
        REDmask = RED_LSB;
        GREENmask = GREEN_LSB;
        BLUEmask = BLUE_LSB;
        for (PWMbit = 0; PWMbit <= COLORDEPTH; PWMbit++) 
        {
            for (col = 0; col < PANELCOLS; col++) 
            {
                if (PWMbit < COLORDEPTH)
                {
                    if (evenFlag) 
                    {                    
                        outputData = matrixOutData[outDataIndex] & EVEN_MASK;
                        matrixData = panelData[panelNumber][line][col];                    
                    
                        if (REDmask & matrixData) outputData = outputData | R1bit;
                        if (GREENmask & matrixData) outputData = outputData | G1bit;
                        if (BLUEmask & matrixData) outputData = outputData | B1bit;

                        matrixData = panelData[panelNumber][line + MAXLINE][col];

                        if (REDmask & matrixData) outputData = outputData | R2bit;
                        if (GREENmask & matrixData) outputData = outputData | G2bit;
                        if (BLUEmask & matrixData) outputData = outputData | B2bit;

                        matrixOutData[outDataIndex] = outputData;                    
                    } 
                    else 
                    {
                        outputData = matrixOutData[outDataIndex] & ODD_MASK;
                        matrixData = panelData[panelNumber][line][col];                    
                    
                        if (REDmask & matrixData) outputData = outputData | M2R1bit;
                        if (GREENmask & matrixData) outputData = outputData | M2G1bit;
                        if (BLUEmask & matrixData) outputData = outputData | M2B1bit;
                    
                        matrixData = panelData[panelNumber][line + MAXLINE][col];                    

                        if (REDmask & matrixData) outputData = outputData | M2R2bit;
                        if (GREENmask & matrixData) outputData = outputData | M2G2bit;
                        if (BLUEmask & matrixData) outputData = outputData | M2B2bit;

                        matrixOutData[outDataIndex] = outputData;
                    }
                }
                outDataIndex++;
            }
            outDataIndex = outDataIndex + offset;
            REDmask = REDmask << 1;
            GREENmask = GREENmask << 1;
            BLUEmask = BLUEmask << 1;
        }
    }
}

