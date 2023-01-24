/* AT45DB641.h: Header file for routines for reading and writing to ATMEL AT45DB641 memory
 *
 * 2-11-12 JBS: 	Retooled version with latest corrections
 * 5-01-13 JBS:  	Added write and read line routines.
 * 5-26-14 JBS:		Added initSPI(), WriteAtmelRAM(), ReadAtmelRAM()
 * 12-16-14             Added initAtmelSPI() and ATMEL_SPI_CHANNEL below. 
 *                      Works with LED Controller Board. initSPI() not working.
 *
 * 5-8-15           Renamed Atmel functions to RAM and FLASH
 * 5-13-15          Added Buffer 1 and Buffer 2 reads/writes
 * 6-14-17          Added corrected version of initAtmelSPI()
 *                  Renamed ReadAtmelPageBuffer(), WriteAtmelPageBuffer(), eliminated line routines. 
 *                  Simplified page addressing, renamed routines
 * 6-15-17          Minor corrections and cleanup
 */
#define PAGESIZE 264
#define BUFFER_OVERRRUN 1
#define ARRAY_OVERRRUN 2
#define MAX_PAGE 32768
#define MAX_FRAME 29
#define SECTOR_SIZE 256
#define MAX_SECTOR 31
#define ERROR 1
#define LINESIZE 16
#define MAX_ATMEL_LINE 33

#define ATMEL_WRITE_PROTECT LATBbits.LATB8
#define ATMEL_CS1 LATCbits.LATC1
#define ATMEL_CS2 LATCbits.LATC2
#define ATMEL_CS3 LATCbits.LATC3
#define ATMEL_CS4 LATCbits.LATC4
#define ATMEL_SPI_CHANNEL 2

void initAtmelSPI(void);
int SendReceiveSPI(unsigned char dataOut);
unsigned char ProgramFLASH (unsigned char AtmelSelect, unsigned char RAMbufferNum, unsigned short pageNum);
unsigned char TransferFLASH (unsigned char AtmelSelect, unsigned char bufferNum, unsigned short pageNum);
int ErasePage(unsigned char AtmelSelect, unsigned short pageNum);

int EraseFLASHsector(unsigned char AtmelSelect, unsigned char sector, unsigned char Sector0Select);
int AtmelBusy(unsigned char AtmelSelect, unsigned char waitFlag);
int EraseEntireFLASH(unsigned char AtmelSelect);

int WriteAtmelPageBuffer (unsigned char AtmelSelect, unsigned char bufferNum, unsigned char *buffer);
int ReadAtmelPageBuffer(unsigned char AtmelSelect, unsigned char bufferNum, unsigned char *buffer);

int ReadAtmelBytes (unsigned char AtmelSelect, unsigned char bufferNum, unsigned char *buffer, unsigned short bufferAddress, unsigned short numberOfBytes);
int WriteAtmelBytes (unsigned char AtmelSelect, unsigned char bufferNum, unsigned char *buffer, unsigned short bufferAddress, unsigned short numberOfBytes);

int MainMemoryPageRead (unsigned char AtmelSelect, unsigned char *buffer, unsigned short pageNum, unsigned short bufferAddress, unsigned short numberOfBytes);

unsigned char storeShortToAtmel(unsigned char AtmelSelect, unsigned short pageNum, unsigned short address, unsigned short inData);
unsigned short fetchShortFromAtmel(unsigned char AtmelSelect, unsigned short pageNum, unsigned short address);


