/* AT45DB641.C -    Routines for reading and writing to ATMEL AT45DB641 memory
 * 					Adapted from AT45DB161.c
 *              
 *
 * 02-11-12 JBS:    Retooled version with latest corrections
 * 05-01-13 JBS:    Tested 0-4095 page writes
 *                  Swapped in pointers in place of global arrays
 *                  Added write and read line routines.
 * 5-26-14 JBS:     Adapted for PIC32 & MPLABX.
 *                  Added initSPI(), WriteAtmelRAM(), ReadAtmelRAM()
 * 6-6-14           INVERTED return values so functions return TRUE or FALSE instead of error.
 * 12-16-14         Works with LED COntroller Board - SPI #2
 *                  initSPI() commented out below because it isn't working
 * 5-8-15           Renamed Atmel functions to RAM and FLASH
 * 5-13-15          Eliminated array length check for reading and writing bytes (lines)
 * 5-13-15          Added Buffer 1 and Buffer 2 reads/writes `A
 * 6-14-17          Added corrected version of initAtmelSPI()
 *                  Renamed ReadAtmelPageBuffer(), WriteAtmelPageBuffer(), eliminated line routines.
 *                  Simplified page addressing, renamed routines
 * 6-15-17          Minor corrections and cleanup
 * 5-4-19           
 */

// #include <XC.h>
#include <plib.h>
#include "AT45DB641.h"

#define FALSE 0
#define TRUE !FALSE



union {
    unsigned char byte[2];
    unsigned short integer;
} convert;

#define lowByte byte[0]
#define highByte byte[1]  

// Set SPI port #2 for 8 bit Master Mode
// Sample Phase for the input bit at the end of the data out time.
// Set the Clock Edge reversed: transmit from active to idle.
// Use 80 Mhz / 2 = 40 Mhz clock
void initAtmelSPI(void)
{
   SpiChnOpen(ATMEL_SPI_CHANNEL, SPI_OPEN_MSTEN | SPI_OPEN_MODE8 | SPI_OPEN_CKE_REV | SPI_OPEN_ON | SPI_OPEN_SMP_END, 1000);  
}

// This version uses the PIC 32 SPI port 
int SendReceiveSPI(unsigned char dataOut){
int dataIn;

	SpiChnPutC(ATMEL_SPI_CHANNEL, dataOut);
	dataIn =SpiChnGetC(ATMEL_SPI_CHANNEL);

	return(dataIn);
}


// This function writes an entire 528 byte page to the Atmel memory buffer.
// *buffer points to input array.
int WriteAtmelPageBuffer (unsigned char AtmelSelect, unsigned char bufferNum, unsigned char *buffer)
{
int i;

 	if (buffer==NULL) return(FALSE); // ERROR
    
	AtmelBusy(AtmelSelect, 1);						// Read Status register and make sure Atmel isn't busy with any previous activity.

    if (AtmelSelect == 1) ATMEL_CS1 = 0;
    else if (AtmelSelect == 2) ATMEL_CS2 = 0;
    else if (AtmelSelect == 3) ATMEL_CS3 = 0;
    else if (AtmelSelect == 4) ATMEL_CS4 = 0;
    else ATMEL_CS1 = 0;
    
    if (bufferNum == 1) SendReceiveSPI(0x84);	// Command: Buffer #1 write 
    else SendReceiveSPI(0x87);			// Command: Buffer #2 write 
	SendReceiveSPI(0);				// Address
	SendReceiveSPI(0);
	SendReceiveSPI(0);

	for(i=0; i<PAGESIZE; i++)	// write the data to Atmel buffer
		SendReceiveSPI(buffer[i]);
    
    if (AtmelSelect == 1) ATMEL_CS1 = 1;
    else if (AtmelSelect == 2) ATMEL_CS2 = 1;
    else if (AtmelSelect == 3) ATMEL_CS3 = 1;
    else if (AtmelSelect == 4) ATMEL_CS4 = 1;
    else ATMEL_CS1 = 1;

	return(TRUE); // No errors - write was successful
}

int ReadAtmelPageBuffer(unsigned char AtmelSelect, unsigned char bufferNum, unsigned char *buffer)
{
unsigned char dataIn;
int i;

	AtmelBusy(AtmelSelect, 1);						// Read Status register and make sure Atmel isn't busy with any previous activity.

    if (AtmelSelect == 1) ATMEL_CS1 = 0;
    else if (AtmelSelect == 2) ATMEL_CS2 = 0;
    else if (AtmelSelect == 3) ATMEL_CS3 = 0;
    else if (AtmelSelect == 4) ATMEL_CS4 = 0;
    else ATMEL_CS1 = 0;
	
    if (bufferNum == 1) SendReceiveSPI(0xD4);    // Buffer #1 read command 
    else SendReceiveSPI(0xD6);			// Buffer #2 read command
	SendReceiveSPI(0);
	SendReceiveSPI(0);
	SendReceiveSPI(0);
	SendReceiveSPI(0);				// Additional don't care byte

	for(i=0; i<PAGESIZE; i++){
		dataIn = (unsigned char) SendReceiveSPI(0x00);
		buffer[i] = dataIn;
	}
    
    if (AtmelSelect == 1) ATMEL_CS1 = 1;
    else if (AtmelSelect == 2) ATMEL_CS2 = 1;
    else if (AtmelSelect == 3) ATMEL_CS3 = 1;
    else if (AtmelSelect == 4) ATMEL_CS4 = 1;
    else ATMEL_CS1 = 1;
	
	return(TRUE); // No errors - read was successful
}

// This erases one page on the Atmel.
// The routine returns FALSE if page number is out of bounds, otherwise returns TRUE
int ErasePage(unsigned char AtmelSelect, unsigned short pageNum)
{
unsigned char pageLowAddress, pageHighAddress;

	if (pageNum > MAX_PAGE) return (FALSE);    
    convert.integer = (pageNum << 1) & 0xFFFE;     
    pageLowAddress = convert.byte[0];
    pageHighAddress = convert.byte[1];
    
	AtmelBusy(AtmelSelect, 1);				// Read Status register and make sure Atmel isn't busy with previous activity.

    if (AtmelSelect == 1) ATMEL_CS1 = 0;
    else if (AtmelSelect == 2) ATMEL_CS2 = 0;
    else if (AtmelSelect == 3) ATMEL_CS3 = 0;
    else if (AtmelSelect == 4) ATMEL_CS4 = 0;
    else ATMEL_CS1 = 0;
							
	SendReceiveSPI(0x81);           // Erase page command
	SendReceiveSPI(pageHighAddress);	
	SendReceiveSPI(pageLowAddress);			
	SendReceiveSPI(0x00);			// Don't care byte		

    if (AtmelSelect == 1) ATMEL_CS1 = 1;
    else if (AtmelSelect == 2) ATMEL_CS2 = 1;
    else if (AtmelSelect == 3) ATMEL_CS3 = 1;
    else if (AtmelSelect == 4) ATMEL_CS4 = 1;
    else ATMEL_CS1 = 1;    

    AtmelBusy(AtmelSelect, 1);				// Wait until erasing is done

	return(TRUE);
}


// This erases a sector of 1024 pages on the Atmel.
// Exceptions: sector 0a is 8 pages, sector 0b is 1016 pages
// Any sector can be erased, but if sector is 0,
// then Sector0Select must be 0 for sector 0a or 1 for sector 0b
// The routine returns FALSE if sector number is out of bounds, otherwise returns TRUE
int EraseFLASHsector(unsigned char AtmelSelect, unsigned char sector, unsigned char Sector0Select)
{
unsigned char sectorLowAddress, sectorHighAddress;

	if (sector > MAX_SECTOR) return (FALSE);
    
    if (sector == 0)
    {
        sectorHighAddress = 0x00;
        if (Sector0Select == 0) sectorLowAddress = 0x00;  // Erase sector 0a
        else sectorLowAddress = 0x10;  // Erase sector 0b     
    }
    else // For sectors 1 to 31
    {
        sectorLowAddress = 0x00;
        sectorHighAddress = (sector << 3) & 0xF8;
    }

	AtmelBusy(AtmelSelect, 1);				// Read Status register and make sure Atmel isn't busy with previous activity.

    if (AtmelSelect == 1) ATMEL_CS1 = 0;
    else if (AtmelSelect == 2) ATMEL_CS2 = 0;
    else if (AtmelSelect == 3) ATMEL_CS3 = 0;
    else if (AtmelSelect == 4) ATMEL_CS4 = 0;
    else ATMEL_CS1 = 0;
							
	SendReceiveSPI(0x7C);           // erase sector command
	SendReceiveSPI(sectorHighAddress);	
	SendReceiveSPI(sectorLowAddress);			
	SendReceiveSPI(0x00);			// Don't care byte		

    if (AtmelSelect == 1) ATMEL_CS1 = 1;
    else if (AtmelSelect == 2) ATMEL_CS2 = 1;
    else if (AtmelSelect == 3) ATMEL_CS3 = 1;
    else if (AtmelSelect == 4) ATMEL_CS4 = 1;
    else ATMEL_CS1 = 1;
    

    AtmelBusy(AtmelSelect, 1);				// Wait until erasing is done

	return(TRUE);
}
// This routine erases the entire flash memeory
int EraseEntireFLASH (unsigned char AtmelSelect)
{
	AtmelBusy(AtmelSelect, 1);	// Read Status register and make sure Atmel isn't busy with any previous activity.

    if (AtmelSelect == 1) ATMEL_CS1 = 0;
    else if (AtmelSelect == 2) ATMEL_CS2 = 0;
    else if (AtmelSelect == 3) ATMEL_CS3 = 0;
    else if (AtmelSelect == 4) ATMEL_CS4 = 0;
    else ATMEL_CS1 = 0;
							
	SendReceiveSPI(0xC7);
	SendReceiveSPI(0x94);	
	SendReceiveSPI(0x80);	
	SendReceiveSPI(0x9A);		

    if (AtmelSelect == 1) ATMEL_CS1 = 1;
    else if (AtmelSelect == 2) ATMEL_CS2 = 1;
    else if (AtmelSelect == 3) ATMEL_CS3 = 1;
    else if (AtmelSelect == 4) ATMEL_CS4 = 1;
    else ATMEL_CS1 = 1;    
		
	return(TRUE);
}



// This routine reads the Atmel memory status register
// to check the READY/BUSY flag.
// This flag goes high when a page programming operation 
// has completed. It returns TRUE if complete
// and FALSE otherwise.
//
// If waitFlag is a 1 then it will sit in a loop
// and keep going until the status register
// READY/BUSY flag reads high.
int AtmelBusy (unsigned char AtmelSelect, unsigned char waitFlag)
{	
unsigned char status, inByte;
int i = 0;
		
    if (AtmelSelect == 1) ATMEL_CS1 = 0;
    else if (AtmelSelect == 2) ATMEL_CS2 = 0;
    else if (AtmelSelect == 3) ATMEL_CS3 = 0;
    else if (AtmelSelect == 4) ATMEL_CS4 = 0;
    else ATMEL_CS1 = 0;
					
	SendReceiveSPI(0xD7);	// Send read Status register command		

	// Read status byte from Atmel and mask off MSB. 
	// This is the READ/BUSY bit:
	do {
		inByte = (unsigned char) SendReceiveSPI(0x00);
        // printf("\r#%d: STATUS: %X", i++, inByte);
		status = (0x80 & inByte); 
	} while ((0==status)&&(waitFlag));  // Keep looping until status bit goes high, or quit after one loop if waitFlag is false

    if (AtmelSelect == 1) ATMEL_CS1 = 1;
    else if (AtmelSelect == 2) ATMEL_CS2 = 1;
    else if (AtmelSelect == 3) ATMEL_CS3 = 1;
    else if (AtmelSelect == 4) ATMEL_CS4 = 1;
    else ATMEL_CS1 = 1;

	if(0==status)
		return(FALSE);
	else
		return(TRUE);		// Retuirn TRUE if Atmel isn't busy
}


// This programs the buffer into a previously erased page in flash memory.
// The Atmel memory buffer is #1 or #2, specified by bufferNum
// The pages are numbered 0-4095.
// The routine always returns 0, however it can be modified to return an error code as well.
unsigned char ProgramFLASH(unsigned char AtmelSelect, unsigned char bufferNum, unsigned short pageNum)
{

    convert.integer = (pageNum << 1) & 0xFFFE;

	AtmelBusy(AtmelSelect, 1);					// Read Status register and make sure Atmel isn't busy with any previous activity.	

    if (AtmelSelect == 1) ATMEL_CS1 = 0;
    else if (AtmelSelect == 2) ATMEL_CS2 = 0;
    else if (AtmelSelect == 3) ATMEL_CS3 = 0;
    else if (AtmelSelect == 4) ATMEL_CS4 = 0;
    else ATMEL_CS1 = 0;

    if (bufferNum == 1)  SendReceiveSPI(0x88);       // Buffer 1 page program without built-in erase     
    else SendReceiveSPI(0x89);                          // Buffer 2 page program without built-in erase
    
    //if (bufferNum == 1)  SendReceiveSPI(0x83);       // Buffer 1 page program WITH built-in erase 
    //else SendReceiveSPI(0x86);                          // Buffer 2 page program WITH built-in erase
    
    SendReceiveSPI(convert.highByte);
    SendReceiveSPI(convert.lowByte);
	SendReceiveSPI(0);				// 

    if (AtmelSelect == 1) ATMEL_CS1 = 1;
    else if (AtmelSelect == 2) ATMEL_CS2 = 1;
    else if (AtmelSelect == 3) ATMEL_CS3 = 1;
    else if (AtmelSelect == 4) ATMEL_CS4 = 1;
    else ATMEL_CS1 = 1;    
	
	return(TRUE);
}


// This copies a page into Atmel Buffer 1
// The pages are numbered 0-4095.
// The routine returns FALSE page is out of bounds, otherwise returns TRUE
unsigned char TransferFLASH (unsigned char AtmelSelect, unsigned char bufferNum, unsigned short pageNum)
{

	if (pageNum>MAX_PAGE) return(FALSE);
        
    convert.integer = (pageNum << 1) & 0xFFFE;        

	AtmelBusy(AtmelSelect, 1);					// Read Status register and make sure Atmel isn't busy with any previous activity.	

    if (AtmelSelect == 1) ATMEL_CS1 = 0;
    else if (AtmelSelect == 2) ATMEL_CS2 = 0;
    else if (AtmelSelect == 3) ATMEL_CS3 = 0;
    else if (AtmelSelect == 4) ATMEL_CS4 = 0;
    else ATMEL_CS1 = 0;
			
    if (bufferNum == 1) SendReceiveSPI(0x53);        // Command: FLASH to RAM buffer 1 transfer
    else SendReceiveSPI(0x55);                          // Command: FLASH to RAM buffer 2 transfer $$$$    
    SendReceiveSPI(convert.highByte);
    SendReceiveSPI(convert.lowByte);
	SendReceiveSPI(0);    

    if (AtmelSelect == 1) ATMEL_CS1 = 1;
    else if (AtmelSelect == 2) ATMEL_CS2 = 1;
    else if (AtmelSelect == 3) ATMEL_CS3 = 1;
    else if (AtmelSelect == 4) ATMEL_CS4 = 1;
    else ATMEL_CS1 = 1;	
	
	return(TRUE);
}


// This erases one page in flash memory.
// The pages are numbered 0-4095.
// The routine returns ERROR of page is out of bounds, otherwise returns 0.
unsigned char EraseFLASHpage (unsigned char AtmelSelect, unsigned short pageNum){
    
	if (pageNum>MAX_PAGE) return(FALSE);
        
    convert.integer = (pageNum << 1) & 0xFFFE;

	AtmelBusy(AtmelSelect, 1);					// Read Status register and make sure Atmel isn't busy with any previous activity.	

    if (AtmelSelect == 1) ATMEL_CS1 = 0;
    else if (AtmelSelect == 2) ATMEL_CS2 = 0;
    else if (AtmelSelect == 3) ATMEL_CS3 = 0;
    else if (AtmelSelect == 4) ATMEL_CS4 = 0;
    else ATMEL_CS1 = 0;
				    
    SendReceiveSPI(0x81);		// Page erase
    SendReceiveSPI(convert.highByte);
    SendReceiveSPI(convert.lowByte);
	SendReceiveSPI(0);    

    if (AtmelSelect == 1) ATMEL_CS1 = 1;
    else if (AtmelSelect == 2) ATMEL_CS2 = 1;
    else if (AtmelSelect == 3) ATMEL_CS3 = 1;
    else if (AtmelSelect == 4) ATMEL_CS4 = 1;
    else ATMEL_CS1 = 1;    
    
	return(TRUE);
}



int ReadAtmelBytes (unsigned char AtmelSelect, unsigned char bufferNum, unsigned char *buffer, unsigned short bufferAddress, unsigned short numberOfBytes){
unsigned char dataIn;
int i;

	if ((bufferAddress+numberOfBytes) >= PAGESIZE) return(FALSE); // Make sure we don't overrun the Atmel page buffer	

    convert.integer = bufferAddress & 0x03FF;

	AtmelBusy(AtmelSelect, 1);						// Read Status register and make sure Atmel isn't busy with any previous activity.	
		
    if (AtmelSelect == 1) ATMEL_CS1 = 0;
    else if (AtmelSelect == 2) ATMEL_CS2 = 0;
    else if (AtmelSelect == 3) ATMEL_CS3 = 0;
    else if (AtmelSelect == 4) ATMEL_CS4 = 0;
    else ATMEL_CS1 = 0;
							
	if (bufferNum == 1) SendReceiveSPI(0xD4);			// Buffer #1 read command
    else SendReceiveSPI(0xD6);			// Buffer #1 read command    
	SendReceiveSPI(0);
	SendReceiveSPI(convert.highByte);
	SendReceiveSPI(convert.lowByte);
	SendReceiveSPI(0);				// Additional don't care byte	
	
	for(i=0; i<numberOfBytes; i++){
		dataIn = (unsigned char) SendReceiveSPI(0x00);
		buffer[i] = dataIn;		
	}

    if (AtmelSelect == 1) ATMEL_CS1 = 1;
    else if (AtmelSelect == 2) ATMEL_CS2 = 1;
    else if (AtmelSelect == 3) ATMEL_CS3 = 1;
    else if (AtmelSelect == 4) ATMEL_CS4 = 1;
    else ATMEL_CS1 = 1;
    
	return (TRUE); // No errors - read was successful
}



// This function writes to either Atmel memory buffer #1 or #2, specified by bufferNum
// *buffer points to input array.	
// The "numberOfBytes" input is the number of bytes that will be 
// copied from the AtmelWriteArray[] array to the Atmel.
// If there are no errors, it returns TRUE
int WriteAtmelBytes (unsigned char AtmelSelect, unsigned char bufferNum, unsigned char *buffer, unsigned short bufferAddress, unsigned short numberOfBytes)
{
int i;
 
 	if (buffer==NULL) return(FALSE); // ERROR
 
	if ((bufferAddress+numberOfBytes) >= PAGESIZE) return(BUFFER_OVERRRUN); // Make sure we don't overrun the Atmel page buffer	

    convert.integer = bufferAddress & 0x01FF;

	AtmelBusy(AtmelSelect, 1);						// Read Status register and make sure Atmel isn't busy with any previous activity.	

    if (AtmelSelect == 1) ATMEL_CS1 = 0;
    else if (AtmelSelect == 2) ATMEL_CS2 = 0;
    else if (AtmelSelect == 3) ATMEL_CS3 = 0;
    else if (AtmelSelect == 4) ATMEL_CS4 = 0;
    else ATMEL_CS1 = 0;
	
	if (bufferNum == 1) SendReceiveSPI(0x84);			// Command: Buffer #1 write
    else SendReceiveSPI(0x87);			// Command: Buffer #2 write
	SendReceiveSPI(0);				// Address
	SendReceiveSPI(convert.highByte);
	SendReceiveSPI(convert.lowByte);    
	
	for(i=0; i<numberOfBytes; i++)	// write the data to Atmel buffer	
		SendReceiveSPI(buffer[i]);	

    if (AtmelSelect == 1) ATMEL_CS1 = 1;
    else if (AtmelSelect == 2) ATMEL_CS2 = 1;
    else if (AtmelSelect == 3) ATMEL_CS3 = 1;
    else if (AtmelSelect == 4) ATMEL_CS4 = 1;
    else ATMEL_CS1 = 1;
    
	return(TRUE); // No errors - write was successful
}

int MainMemoryPageRead (unsigned char AtmelSelect, unsigned char *buffer, unsigned short pageNum, unsigned short bufferAddress, unsigned short numberOfBytes){
unsigned char dataIn;
unsigned short lowAddress, midAddress, highAddress;
int i;

    // Make sure we don't overrun the Atmel page buffer
	if ((bufferAddress+numberOfBytes) >= PAGESIZE)  
    
    if (buffer = NULL) return(FALSE);

    convert.integer = bufferAddress;
    lowAddress = convert.lowByte;
    midAddress = convert.highByte & 0x01;

    convert.integer = (pageNum << 1) & 0xFFFE;
    midAddress = midAddress | convert.lowByte;
    highAddress = convert.highByte;
     
    // Make sure Atmel isn't busy with any previous activity.	    
	AtmelBusy(AtmelSelect, 1);
		
    if (AtmelSelect == 1) ATMEL_CS1 = 0;
    else if (AtmelSelect == 2) ATMEL_CS2 = 0;
    else if (AtmelSelect == 3) ATMEL_CS3 = 0;
    else if (AtmelSelect == 4) ATMEL_CS4 = 0;
    else ATMEL_CS1 = 0;
					
    // Send Main Memory Page Read command
    SendReceiveSPI(0xD2);		
    // Send page number combined with buffer address
	SendReceiveSPI((unsigned char) highAddress);
    SendReceiveSPI((unsigned char) midAddress);
    SendReceiveSPI((unsigned char) lowAddress);
    SendReceiveSPI(0);  // Send four don't care bytes
    SendReceiveSPI(0);
    SendReceiveSPI(0);
    SendReceiveSPI(0);
	
    // Now read incoming data one byte at a time
	for(i=0; i<numberOfBytes; i++){
		dataIn = (unsigned char) SendReceiveSPI(0x00);
		buffer[i] = dataIn;		
	}

    if (AtmelSelect == 1) ATMEL_CS1 = 1;
    else if (AtmelSelect == 2) ATMEL_CS2 = 1;
    else if (AtmelSelect == 3) ATMEL_CS3 = 1;
    else if (AtmelSelect == 4) ATMEL_CS4 = 1;
    else ATMEL_CS1 = 1;
    
	return (TRUE); // No errors - read was successful
}



unsigned char storeShortToAtmel(unsigned char AtmelSelect, unsigned short pageNum, unsigned short address, unsigned short inData) 
{
unsigned char AtmelRAM[2];

    if (address < (PAGESIZE - 1)) 
    {
        convert.integer = inData;
        AtmelRAM[0] = convert.lowByte;
        AtmelRAM[1] = convert.highByte;
        // Store flash in RAM, Erase flash, overwrite RAM, program flash:
        //printf("\rCopy flash");
        TransferFLASH (AtmelSelect, 1, pageNum); 
        //printf("\rErase page");
        EraseFLASHpage(AtmelSelect, pageNum);
        //printf("\rWrite bytes");
        WriteAtmelBytes (AtmelSelect, 1, AtmelRAM, address, 2);
        //printf("\rProgram flash");
        ProgramFLASH(AtmelSelect, 1, pageNum);
        return (TRUE);
    } else return (FALSE);
}

unsigned short fetchShortFromAtmel(unsigned char AtmelSelect, unsigned short pageNum, unsigned short address) 
{
unsigned char AtmelRAM[2];

    if (address < (PAGESIZE - 1)) 
    {
        // Read two bytes directly from flash without using RAM buffers:
        TransferFLASH (AtmelSelect, 1, pageNum);
        ReadAtmelBytes (AtmelSelect, 1, AtmelRAM, address, 2);
        // Convert two bytes to integer and return result;
        convert.lowByte = AtmelRAM[0];
        convert.highByte = AtmelRAM[1];        
        return (convert.integer);
    } else return (0);
}

