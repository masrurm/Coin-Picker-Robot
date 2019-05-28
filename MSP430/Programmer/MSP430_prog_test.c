// FT230XS SPI in synchronous bit bang mode to program
// the Atmega328P microcontroller. Connect as follows:
//
//
//  (c) Jesus Calvino-Fraga (2017)
//
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#pragma comment(lib, "FTD2XX.lib")
#define FTD2XX_STATIC
#include "ftd2xx.h"

#define TXD   0x01  // TXD pin
#define RXD   0x02  // RXD pin
#define TEST  0x04  // RTS pin
#define RESET 0x08  // CTS pin
#define BSL_ENTRY_CONFIG (TXD|TEST|RESET)

unsigned char bitloc[]={0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

#define BUFFSIZE 0x100

FT_HANDLE handle;
DWORD bytes;

unsigned char BitBangBuff[BUFFSIZE]; // Buffer used to transmit and receive SPI bits
DWORD BitBangBuff_cnt;
short int Bit_Location[BUFFSIZE/2]; // Location of input bits in BitBangBuff
DWORD Bit_Location_cnt;
unsigned char Received_SPI[BUFFSIZE/(8*2)]; // Decoded input bits
DWORD Received_SPI_cnt;

#define MEMSIZE 0x10000
unsigned char Flash_Buffer[MEMSIZE];
char HexName[MAX_PATH]="";

#define BIT7 0x80
#define BIT6 0x40
#define BIT5 0x20
#define BIT4 0x10
#define BIT3 0x08
#define BIT2 0x04
#define BIT1 0x02
#define BIT0 0x01

// The fuses with their default values according to the datasheet:
unsigned char Fuse_Low_Bits=0x62;
unsigned char Fuse_High_Bits=0xd9;
unsigned char Fuse_Extended_Bits=0x07;
int b_program=0, b_verify=0;
int Selected_Device=-1;

clock_t startm, stopm;
#define START if ( (startm = clock()) == -1) {printf("Error calling clock");exit(1);}
#define STOP if ( (stopm = clock()) == -1) {printf("Error calling clock");exit(1);}
#define PRINTTIME printf( "%.1f seconds.", ((double)stopm-startm)/CLOCKS_PER_SEC);

void Load_Byte (unsigned char val)
{
	int j;
}

void Decode_BitBangBuff (void)
{
	int i, j;
}

void Reset_BitBangBuff(void)
{
}

void Send_BitBangBuff (void)
{
	BitBangBuff[BitBangBuff_cnt++]=0; //Need to write one more byte in order to get the last bit
    FT_Write(handle, BitBangBuff, BitBangBuff_cnt, &bytes);
    FT_Read(handle, BitBangBuff, BitBangBuff_cnt, &bytes);
	Decode_BitBangBuff();
}

int hex2dec (char hex_digit)
{
   int j;
   j=toupper(hex_digit)-'0';
   if (j>9) j -= 7;
   return j;
}

unsigned char GetByte(char * buffer)
{
	return hex2dec(buffer[0])*0x10+hex2dec(buffer[1]);
}

unsigned short GetWord(char * buffer)
{
	return hex2dec(buffer[0])*0x1000+hex2dec(buffer[1])*0x100+hex2dec(buffer[2])*0x10+hex2dec(buffer[3]);
}

int Read_Hex_File(char * filename)
{
	char buffer[1024];
	FILE * filein;
	int j;
	unsigned char linesize, recordtype, rchksum, value;
	unsigned short address;
	int MaxAddress=0;
	int chksum;
	int line_counter=0;
	int numread=0;
	int TotalBytes=0;

	//Set the flash buffer to its default value
	memset(Flash_Buffer, 0xff, MEMSIZE);

    if ( (filein=fopen(filename, "r")) == NULL )
    {
       printf("Error: Can't open file `%s`.\r\n", filename);
       return -1;
    }

    while(fgets(buffer, sizeof(buffer), filein)!=NULL)
    {
    	numread+=(strlen(buffer)+1);

    	line_counter++;
    	if(buffer[0]==':')
    	{
			linesize = GetByte(&buffer[1]);
			address = GetWord(&buffer[3]);
			recordtype = GetByte(&buffer[7]);
			rchksum = GetByte(&buffer[9]+(linesize*2));
			chksum=linesize+(address/0x100)+(address%0x100)+recordtype+rchksum;

			if (recordtype==1) break; /*End of record*/

			for(j=0; j<linesize; j++)
			{
				value=GetByte(&buffer[9]+(j*2));
				chksum+=value;
				if((address+j)<MEMSIZE)
				{
					Flash_Buffer[address+j]=value;
					TotalBytes++;
				}
			}
			if(MaxAddress<(address+linesize-1)) MaxAddress=(address+linesize-1);

			if((chksum%0x100)!=0)
			{
				printf("ERROR: Bad checksum in file '%s' at line %d\r\n", filename, line_counter);
				return -1;
			}
		}
    }
    fclose(filein);
    printf("%s: Loaded %d bytes.  Highest address is %d.\n", filename, TotalBytes, MaxAddress);

    return MaxAddress;
}

void Check_Status_Bit(void)
{
	do
	{	// Poll RDY/BSY: $F0, $00, $00, data byte out
		Reset_BitBangBuff();
		Load_Byte(0xf0);
		Load_Byte(0x00);
	    Load_Byte(0x00);
	    Load_Byte(0x00);
	    Send_BitBangBuff();
    } while (Received_SPI[3]&0x01); // If the LSB in RDY/BSY data byte out is �1�,
                                    // a programming operation is still pending.
                                    // Wait until this bit returns �0� before the
                                    // next instruction is carried out.
}

void Dump_Received_SPI (void)
{
	int i;

	for(i=0; i<Received_SPI_cnt; i++)
	{
		if((i&0xf)==0) printf("\n%04x: ", i);
		printf(" %02x", Received_SPI[i]);
	}
	printf("\n");
}

int Load_Flash_Atmega328 (void)
{
	int j, k, star_count;
	int sendbuff;
	unsigned char Signature_Byte[3];
	
	START;
	// According to the datasheet: "Apply power between VCC and GND while RESET and SCK
	// are set to 0. In some systems, the programmer can not guarantee that SCK is
	// held low during power-up. In this case, RESET must be given a positive pulse
	// of at least two CPU clock cycles duration after SCK has been set to 0."

	Reset_BitBangBuff();
	//for(j=0; j<100; j++) BitBangBuff[BitBangBuff_cnt++]=SS;
	BitBangBuff[BitBangBuff_cnt++]=0;
	Send_BitBangBuff();
	FT_W32_PurgeComm(handle, PURGE_TXCLEAR|PURGE_RXCLEAR);

	// Send Programming Enable command: $AC, $53, $00, $00
	Reset_BitBangBuff();
	Load_Byte(0xac);
	Load_Byte(0x53);
    Load_Byte(0x00); //0x53 is returned at this point if ISP is enabled.
    Load_Byte(0x00); 
	Send_BitBangBuff();
    if(Received_SPI[2]!=0x53)
    {
        puts("Programming Enable command fail.\n");
        Dump_Received_SPI();
        return -1;
    }
    
    //Read Signature Byte $30, $00, 0000 000aa, data byte out
    for(j=0; j<3; j++)
    {
		Reset_BitBangBuff();
		Load_Byte(0x30);
		Load_Byte(0x00);
	    Load_Byte((unsigned char)j);
	    Load_Byte(0x00); 
		Send_BitBangBuff();
		Signature_Byte[j]=Received_SPI[3];
	}

    // for the ATmega328P the signature bytes are: 0x1E 0x95 0x0F
    if(Signature_Byte[0]==0x1e && Signature_Byte[1]==0x95 && Signature_Byte[2]==0x0f)
    {
		printf("Atmega328P detected.\n"); fflush(stdout);
    }
    else if(Signature_Byte[0]==0x1e && Signature_Byte[1]==0x95 && Signature_Byte[2]==0x14)
    {
		printf("Atmega328 detected.\n"); fflush(stdout);
    }
    else
    {
    	printf("Atmega328/P NOT detected!\n");
		printf("Signature bytes: 0x%02x 0x%02x 0x%02x\n",
				Signature_Byte[0], Signature_Byte[1], Signature_Byte[2]);
		fflush(stdout);
		return -1;
    }

	if(b_program)
	{
	    // Prior to loading a new program we need to erase the flash memory of the Atmega328 
		printf("Erasing flash memory: "); fflush(stdout);
		// Chip Erase (Program Memory/EEPROM): $AC, $80, $00, $00
		Reset_BitBangBuff();
		Load_Byte(0xac);
		Load_Byte(0x80); // Chip erase command
	    Load_Byte(0x00);
	    Load_Byte(0x00);
		Send_BitBangBuff();
		Check_Status_Bit(); // Wait for the erase command to complete
		printf("# Done.\n"); fflush(stdout);
		
		/* From the datasheet:	
		"The Flash is programmed one page at a time. The memory page is loaded one
		byte at a time by supplying the 6 LSB of the address and data together with
		the Load Program Memory Page instruction. To ensure correct loading of the
		page, the data low byte must be loaded before data high byte is applied for
		a given address. The Program Memory Page is stored by loading the Write
		Program Memory Page instruction with the 7 MSB of the address. If polling
		(RDY/BSY) is not used, the user must wait at least tWD_FLASH before issuing
		the next page (See Table 28-18). Accessing the serial programming interface
		before the Flash write operation completes can result in incorrect programming."
		*/
		
		// Load Extended Address byte: $4D, $00, Extended adr, $00
		Reset_BitBangBuff();
		Load_Byte(0x4d);
		Load_Byte(0x00);
	    Load_Byte(0x00);
	    Load_Byte(0x00);
		Send_BitBangBuff();
		
		printf("Loading flash memory: "); fflush(stdout);
		for(j=0; j<MEMSIZE; j+=128) // Each page is 128 bytes or 64 words
		{
			Reset_BitBangBuff();
			sendbuff=0;
		    for(k=0; k<64; k++)
		    {
		    	//Load Program Memory Page, Low byte: $40, $00, adr LSB, low data byte in
				Load_Byte(0x40);
				Load_Byte(0x00);
			    Load_Byte((unsigned char)k);
		    	Load_Byte(Flash_Buffer[j+(k*2)]);
		    	if (Flash_Buffer[j+(k*2)]!=0xff) sendbuff=1;
		    	//Load Program Memory Page, High byte: $48, $00, adr LSB, high data byte in
				Load_Byte(0x48);
				Load_Byte(0x00);
			    Load_Byte((unsigned char)k);
		    	Load_Byte(Flash_Buffer[j+(k*2)+1]);
		    	if (Flash_Buffer[j+(k*2)+1]!=0xff)
		    	{
		    		sendbuff=1;
		    	}
		    }
		    if(sendbuff==1) // Only send pages that are not all 0xff
		    {
				if(star_count==50)
				{
					star_count=0;
					printf("\nLoading flash memory: ");
				}
				printf("#"); fflush(stdout);
				star_count++;
				Send_BitBangBuff(); // First copy to buffer, then write to flash...
				// Write Program Memory Page: $4C, adr MSB, adr LSB, $00
				Reset_BitBangBuff();
				Load_Byte(0x4c);
				Load_Byte((unsigned char)((j/2)/256));
				Load_Byte((unsigned char)((j/2)%256));
				Load_Byte(0x00);
				Send_BitBangBuff();
				Check_Status_Bit(); // Wait for the command to complete
			}
		}
		printf(" Done.\n"); fflush(stdout);
	}

	if(b_verify)
	{
		printf("Verifying flash memory: "); fflush(stdout);
		star_count=0;
		for(j=0; j<MEMSIZE; j+=128) // Each page is 128 bytes or 64 words
		{
			Reset_BitBangBuff();
			sendbuff=0;
		    for(k=0; k<64; k++)
		    {
		    	int memloc=(j/2)+k;
	
				//Read Program Memory, Low byte: $20, adr MSB, adr LSB, low data byte out
				Load_Byte(0x20);
				Load_Byte((unsigned char)(memloc/256));
			    Load_Byte((unsigned char)(memloc%256));
		    	Load_Byte(0x00);
		    	if (Flash_Buffer[j+(k*2)]!=0xff) sendbuff=1;
	
				//Read Program Memory, High byte: $28, adr MSB, adr LSB, high data byte out
				Load_Byte(0x28);
				Load_Byte((unsigned char)(memloc/256));
			    Load_Byte((unsigned char)(memloc%256));
		    	Load_Byte(0x00);
		    	if (Flash_Buffer[j+(k*2)+1]!=0xff) sendbuff=1;
		    }
			if(sendbuff==1)
			{
				if(star_count==50)
				{
					star_count=0;
					printf("\nVerifying flash memory: ");
				}
				printf("#"); fflush(stdout);
				star_count++;
				
				Send_BitBangBuff();
				for(k=0; k<128; k++)
				{
					if(Received_SPI[(k*4)+3]!=Flash_Buffer[j+k])
					{
						printf("\nFlash memory program error at location %04x\n", (j+k)/2); fflush(stdout);
						Reset_BitBangBuff();
						//BitBangBuff[BitBangBuff_cnt++]=SS;
						Send_BitBangBuff();
						Sleep(100); // Wait for the reset pin to stabilize at logic 1
						return -1;
					}
				}
			}
		}
		printf(" Done.\n"); fflush(stdout);
	}

	if(b_program)
	{
		printf("Writing configuration fuses: "); fflush(stdout);
		
		//Write low Fuse bits: $AC, $A0, $00, data byte in
		
		// Bit 7: CKDIV8, Divide clock by 8, 0 (programmed)
		// Bit 6: CKOUT, Clock output, 1 (unprogrammed)
		// Bit 5: SUT1, Select start-up time, 1 (unprogrammed)
		// Bit 4: SUT0, Select start-up time, 0 (programmed)
		// Bit 3: CKSEL3, Select Clock source, 0 (programmed)
		// Bit 2: CKSEL2, Select Clock source, 0 (programmed)
		// Bit 1: CKSEL1, Select Clock source, 1 (unprogrammed)
		// Bit 0: CKSEL0, Select Clock source, 0 (programmed)
	
		Reset_BitBangBuff();
		Load_Byte(0xac);
		Load_Byte(0xa0);
		Load_Byte(0x00);
		Load_Byte(Fuse_Low_Bits);
		Send_BitBangBuff();
		Check_Status_Bit(); // Wait for the command to complete
		printf("#"); fflush(stdout);
		
		//Write Fuse High bits: $AC, $A8, $00, data byte in:
		// Bit 7: RSTDISBL, External Reset Disable, 1 (unprogrammed)
		// Bit 6: DWEN, debugWIRE Enable, 1 (unprogrammed)
		// Bit 5: SPIEN, Enable Serial Program and Data Downloading, 0 (programmed, SPI programming enabled)
		// Bit 4: WDTON, Watchdog Timer Always On, 1 (unprogrammed)
		// Bit 3: EESAVE, EEPROM memory is preserved through the Chip Erase, 1 (unprogrammed), EEPROM not reserved
		// Bit 2: BOOTSZ1, Select Boot Size (see datasheet for details), 0 (programmed)
		// Bit 1: BOOTSZ0, Select Boot Size (see datasheet for details), 0 (programmed)
		// Bit 0: BOOTRST, Select Reset Vector, 1 (unprogrammed)
	
		Reset_BitBangBuff();
		Load_Byte(0xac);
		Load_Byte(0xa8);
		Load_Byte(0x00);
		Load_Byte(Fuse_High_Bits);
		Send_BitBangBuff();
		Check_Status_Bit(); // Wait for the command to complete
		printf("#"); fflush(stdout);
		
		//Write Extended Fuse Bits: $AC, $A4, $00, data byte in
		// Bit 2: BODLEVEL2, Brown-out Detector trigger level, 1 (unprogrammed)
		// Bit 1: BODLEVEL1, Brown-out Detector trigger level, 1 (unprogrammed)
		// Bit 0: BODLEVEL0, Brown-out Detector trigger level, 1 (unprogrammed)
		Reset_BitBangBuff();
		Load_Byte(0xac);
		Load_Byte(0xa4);
		Load_Byte(0x00);
		Load_Byte(Fuse_Extended_Bits);
		Send_BitBangBuff();
		Check_Status_Bit(); // Wait for the command to complete
		printf("#"); fflush(stdout);
		
		printf(" Done.\n"); fflush(stdout);
	}
	
	// Set reset pin to logic 1
	Reset_BitBangBuff();
	//BitBangBuff[BitBangBuff_cnt++]=SS;
	Send_BitBangBuff();
	Sleep(100); // Wait for the reset pin to stabilize at logic 1

    printf("Actions completed in ");
    STOP;
	PRINTTIME;
	printf("\n"); fflush(stdout);

    return 0;
}

int List_FTDI_Devices (void)
{
	FT_STATUS ftStatus;
	FT_HANDLE ftHandleTemp;
	DWORD numDevs;
	DWORD Flags;
	DWORD ID;
	DWORD Type;
	DWORD LocId;
	char SerialNumber[16];
	char Description[64];
	int j, toreturn=0;
	LONG PortNumber;
	
	if (Selected_Device>=0) return Selected_Device;
	
	// create the device information list
	ftStatus = FT_CreateDeviceInfoList(&numDevs);
	if (ftStatus == FT_OK)
	{
		//printf("Number of devices is %d\n",numDevs);
	}
	
	if (numDevs > 1)
	{
		printf("More than one device detected.  Use option -d to select device to use:\n");
		for(j=0; j<numDevs; j++)
		{
			ftStatus = FT_GetDeviceInfoDetail(j, &Flags, &Type, &ID, &LocId, SerialNumber, Description, &ftHandleTemp);
			if (ftStatus == FT_OK)
			{
				printf("-d%d: ", j);
				//printf("Flags=0x%x ",Flags);
				//printf("Type=0x%x ",Type);
				printf("ID=0x%x ",ID);
				//printf("LocId=0x%x ",LocId);
				printf("Serial=%s ",SerialNumber);
				printf("Description='%s' ",Description);
				//printf(" ftHandle=0x%x",ftHandleTemp);
				FT_Open(j, &handle);
				FT_GetComPortNumber(handle, &PortNumber);				
				FT_Close(handle);
				printf("Port=COM%d\n", PortNumber); fflush(stdout);
			}
		}
		fflush(stdout);
		exit(-1);
	}
	
	return toreturn;
}

void print_help (char * prn)
{
	printf("Some examples:\n"
	       "%s -p -v -CRYSTAL somefile.hex   (program/verify configure for external crystal)\n"
	       "%s -p -v -RC somefile.hex        (program/verify configure for internal 8 MHz RC oscillator)\n"
	       "%s -p -v -ARDUINO somefile.hex   (program/verify configure for Arduino bootloader)\n"
	       "Other options available (x must be replaced with either 0 or 1):\n"
	       "   -CKDIV8=x -CKOUT=x -SUT1=x -SUT0=x -CKSEL3=x -CKSEL2=x -CKSEL1=x -CKSEL0=x\n"
	       "   -RSTDISBL=x -DWEN=x -SPIEN=x -WDTON=x -EESAVE=x -BOOTSZ1=x -BOOTSZ0=x -BOOTRST=x\n"
	       "   -BODLEVEL2=x -BODLEVEL2=x -BODLEVEL2=x\n"
	       "   Check the datasheet for the meaning of the above options\n", 
	       prn, prn, prn);
}

void WriteSerial(unsigned char * Buf, int Len)
{
	// setup by FT_W32_CreateFile for overlapped i/o 
	DWORD dwWritten;
    if (FT_Write(handle, Buf, Len, &dwWritten)== FT_OK)
	{
		printf("FT_Write OK\n");
	}
	else
	{
		printf("FT_Write failed\n");
	}
}

int ReadSerial(unsigned char * Buf, int Len)
{
	// setup by FT_W32_CreateFile for overlapped i/o 
	DWORD dwRead;
    if (FT_Read(handle, Buf, Len, &dwRead)== FT_OK)
	{
		printf("FT_Read OK\n");
	}
	else
	{
		printf("FT_Read failed\n");
	}
	return dwRead;
}

void GetOutAfterError (int val)
{
	if(handle!=NULL)
	{
		FT_SetBitMode(handle, 0x00, FT_BITMODE_CBUS_BITBANG); // Set CBUS3 as input
		FT_SetBitMode(handle, 0x0, FT_BITMODE_RESET); // Back to serial port mode
		FT_Close(handle);
	}
	exit(val);
}

// CBUS3 and CBUs0 are used as RESET and TEST respectevely, to activate BSL mode.
// They must be configured for GPIO first.
void FTDI_Set_CBUS_Mode (int pinmode)
{
	FT_STATUS status;
	char Manufacturer[64];
	char ManufacturerId[64];
	char Description[64];
	char SerialNumber[64];
	
	FT_EEPROM_HEADER ft_eeprom_header;
	FT_EEPROM_X_SERIES ft_eeprom_x_series;
	
	ft_eeprom_header.deviceType = FT_DEVICE_X_SERIES; // FTxxxx device type to be accessed
	ft_eeprom_x_series.common = ft_eeprom_header;
	ft_eeprom_x_series.common.deviceType = FT_DEVICE_X_SERIES;
	
	status = FT_EEPROM_Read(handle, &ft_eeprom_x_series, sizeof(ft_eeprom_x_series),
							Manufacturer, ManufacturerId, Description, SerialNumber);
	// FT_X_SERIES_CBUS_IOMODE configure pin to this mode bor bit bang mode
	// FT_X_SERIES_CBUS_SLEEP Factory default setting for CBUS3
	// FT_X_SERIES_CBUS_TXLED Factory default setting for CBUS2
	// FT_X_SERIES_CBUS_RXLED Factory default setting for CBUS1
	// FT_X_SERIES_CBUS_TXDEN Factory default setting for CBUS0
	if (status == FT_OK)
	{
		if(pinmode==1)
		{
			if ((ft_eeprom_x_series.Cbus3!=FT_X_SERIES_CBUS_IOMODE) ||  (ft_eeprom_x_series.Cbus0!=FT_X_SERIES_CBUS_IOMODE))
			{
				ft_eeprom_x_series.Cbus3=FT_X_SERIES_CBUS_IOMODE;
				ft_eeprom_x_series.Cbus0=FT_X_SERIES_CBUS_IOMODE;
				status = FT_EEPROM_Program(handle, &ft_eeprom_x_series, sizeof(ft_eeprom_x_series),
										Manufacturer, ManufacturerId, Description, SerialNumber);
				if (status == FT_OK)
				{
					FT_ResetDevice(handle);
					Sleep(100);
					printf("WARNING: Pins CBUS3 and CBUS0 have been configured as 'IO Mode for CBUS bit-bang'.\n");
					printf("Please unplug/plug the BO230XS board for the changes to take effect\n"
					       "and try again.\n");
					fflush(stdout);
					GetOutAfterError(0);
				}
			}
		}
		else
		{
			if ((ft_eeprom_x_series.Cbus3!=FT_X_SERIES_CBUS_SLEEP) ||  (ft_eeprom_x_series.Cbus0!=FT_X_SERIES_CBUS_TXDEN))
			{
				ft_eeprom_x_series.Cbus3=FT_X_SERIES_CBUS_SLEEP;
				ft_eeprom_x_series.Cbus0=FT_X_SERIES_CBUS_TXDEN;
				status = FT_EEPROM_Program(handle, &ft_eeprom_x_series, sizeof(ft_eeprom_x_series),
										Manufacturer, ManufacturerId, Description, SerialNumber);
				if (status == FT_OK)
				{
					FT_ResetDevice(handle);
					Sleep(100);
					printf("WARNING: Pins CBUS3 and CBUS0 have been configured as factory default.\n");
					printf("Please unplug/plug the BO230XS board for the changes to take effect\n"
					       "and try again.\n");
					fflush(stdout);
					GetOutAfterError(0);
				}
			}
		}
	}
}

void Activate_BSL_old (void)
{
	FTDI_Set_CBUS_Mode(1); // Make sure both CBUS3 and CBUS0 are in "CBUS IOMODE"
	FT_SetBitMode(handle, (unsigned char)0x90, FT_BITMODE_CBUS_BITBANG); // RESET=0, TEST=0
	Sleep(1); // Sleep mili seconds
	FT_SetBitMode(handle, (unsigned char)0x91, FT_BITMODE_CBUS_BITBANG); // RESET=0, TEST=1
	Sleep(1);
	FT_SetBitMode(handle, (unsigned char)0x90, FT_BITMODE_CBUS_BITBANG); // RESET=0, TEST=0
	Sleep(1);
	FT_SetBitMode(handle, (unsigned char)0x91, FT_BITMODE_CBUS_BITBANG); // RESET=0, TEST=1
	Sleep(1);
	FT_SetBitMode(handle, (unsigned char)0x99, FT_BITMODE_CBUS_BITBANG); // RESET=1, TEST=1
	Sleep(1);
	FT_SetBitMode(handle, (unsigned char)0x98, FT_BITMODE_CBUS_BITBANG); // RESET=1, TEST=0	
	Sleep(1);

	FT_SetBitMode(handle, 0x00, FT_BITMODE_CBUS_BITBANG); // Set both CBUS3 and CBUS0 as inputs
	
	FT_SetBitMode(handle, 0x0, FT_BITMODE_RESET); // Back to serial port mode
	FT_Purge(handle, FT_PURGE_RX | FT_PURGE_TX);
	Sleep(10);
}

void Activate_BSL (void)
{
	// Bits 7 down to 4 set CBUSx as either input or ouput.  Bits 3 down to 0 is the actual output.
	// For example 0x90 configures CBUS3 and CBUS0 as outputs and set their values to zero.
	unsigned char Seq[7]={0x90, 0x91, 0x90, 0x91, 0x99, 0x98, 0x00};
	int j;
	
	FTDI_Set_CBUS_Mode(1); // Make sure both CBUS3 and CBUS0 are in "CBUS IOMODE"
	
	for(j=0; j<7; j++)
	{
		FT_SetBitMode(handle, Seq[j], FT_BITMODE_CBUS_BITBANG); // RESET=0, TEST=0
		Sleep(1); // Sleep mili seconds
	}
	
	FT_SetBitMode(handle, 0x0, FT_BITMODE_RESET); // Back to serial port mode
	FT_Purge(handle, FT_PURGE_RX | FT_PURGE_TX);
	Sleep(10);
}


int main(int argc, char ** argv)
{
    int j, k;
	LONG lComPortNumber;
	unsigned char buff[256];
	unsigned char buff2[256];

    if(argc<2)
    {
    	//printf("Need a filename to proceed.  ");
    	//print_help(argv[0]);
    	//return 1;
    }
    
    for(j=1; j<argc; j++)
    {
		     if(stricmp("-CKDIV8=0", argv[j])==0) Fuse_Low_Bits&=(~BIT7);
		else if(stricmp("-CKDIV8=1", argv[j])==0) Fuse_Low_Bits|=BIT7;
		else if(stricmp("-CKOUT=0", argv[j])==0)  Fuse_Low_Bits&=(~BIT6);
		else if(stricmp("-CKOUT=1", argv[j])==0)  Fuse_Low_Bits|=BIT6;
		else if(stricmp("-SUT1=0", argv[j])==0)   Fuse_Low_Bits&=(~BIT5);
		else if(stricmp("-SUT1=1", argv[j])==0)   Fuse_Low_Bits|=BIT5;
		else if(stricmp("-SUT0=0", argv[j])==0)   Fuse_Low_Bits&=(~BIT4);
		else if(stricmp("-SUT0=1", argv[j])==0)   Fuse_Low_Bits|=BIT4;
		else if(stricmp("-CKSEL3=0", argv[j])==0) Fuse_Low_Bits&=(~BIT3);
		else if(stricmp("-CKSEL3=1", argv[j])==0) Fuse_Low_Bits|=BIT3;
		else if(stricmp("-CKSEL2=0", argv[j])==0) Fuse_Low_Bits&=(~BIT2);
		else if(stricmp("-CKSEL2=1", argv[j])==0) Fuse_Low_Bits|=BIT2;
		else if(stricmp("-CKSEL1=0", argv[j])==0) Fuse_Low_Bits&=(~BIT1);
		else if(stricmp("-CKSEL1=1", argv[j])==0) Fuse_Low_Bits|=BIT1;
		else if(stricmp("-CKSEL0=0", argv[j])==0) Fuse_Low_Bits&=(~BIT0);
		else if(stricmp("-CKSEL0=1", argv[j])==0) Fuse_Low_Bits|=BIT0;

		else if(stricmp("-RSTDISBL=0", argv[j])==0) Fuse_Low_Bits&=(~BIT7);
		else if(stricmp("-RSTDISBL=1", argv[j])==0) Fuse_High_Bits|=BIT7;
		else if(stricmp("-DWEN=0", argv[j])==0)     Fuse_High_Bits&=(~BIT6);
		else if(stricmp("-DWEN=1", argv[j])==0)     Fuse_High_Bits|=BIT6;
		else if(stricmp("-SPIEN=0", argv[j])==0)    Fuse_Low_Bits&=(~BIT5);
		else if(stricmp("-SPIEN=1", argv[j])==0)    Fuse_High_Bits|=BIT5;
		else if(stricmp("-WDTON=0", argv[j])==0)    Fuse_High_Bits&=(~BIT4);
		else if(stricmp("-WDTON=1", argv[j])==0)    Fuse_High_Bits|=BIT4;
		else if(stricmp("-EESAVE=0", argv[j])==0)   Fuse_High_Bits&=(~BIT3);
		else if(stricmp("-EESAVE=1", argv[j])==0)   Fuse_High_Bits|=BIT3;
		else if(stricmp("-BOOTSZ1=0", argv[j])==0)  Fuse_High_Bits&=(~BIT2);
		else if(stricmp("-BOOTSZ1=1", argv[j])==0)  Fuse_High_Bits|=BIT2;
		else if(stricmp("-BOOTSZ0=0", argv[j])==0)  Fuse_High_Bits&=(~BIT1);
		else if(stricmp("-BOOTSZ0=1", argv[j])==0)  Fuse_High_Bits|=BIT1;
		else if(stricmp("-BOOTRST=0", argv[j])==0)  Fuse_High_Bits&=(~BIT0);
		else if(stricmp("-BOOTRST=1", argv[j])==0)  Fuse_High_Bits|=BIT0;

		else if(stricmp("-BODLEVEL2=0", argv[j])==0)  Fuse_Extended_Bits&=(~BIT2);
		else if(stricmp("-BODLEVEL2=1", argv[j])==0)  Fuse_Extended_Bits|=BIT2;
		else if(stricmp("-BODLEVEL1=0", argv[j])==0)  Fuse_Extended_Bits&=(~BIT1);
		else if(stricmp("-BODLEVEL1=1", argv[j])==0)  Fuse_Extended_Bits|=BIT1;
		else if(stricmp("-BODLEVEL0=0", argv[j])==0)  Fuse_Extended_Bits&=(~BIT0);
		else if(stricmp("-BODLEVEL0=1", argv[j])==0)  Fuse_Extended_Bits|=BIT0;

		else if(stricmp("-ARDUINO", argv[j])==0) 
		{
			Fuse_Low_Bits=0xff;
			Fuse_High_Bits=0xde;
			Fuse_Extended_Bits=0x05;
		}
		
		else if(stricmp("-CRYSTAL", argv[j])==0) Fuse_Low_Bits=0xff;
		else if(stricmp("-RC", argv[j])==0) Fuse_Low_Bits=0xe2; // Use 0xe2 for internal 8MHz RC oscillator
		
		else if(stricmp("-p", argv[j])==0) b_program=1;
		else if(stricmp("-v", argv[j])==0) b_verify=1;

		else if(stricmp("-h", argv[j])==0)
		{
    		print_help(argv[0]);
    		return 0;
		}
		else if(strnicmp("-d", argv[j], 2)==0)
		{
			Selected_Device=atoi(&argv[j][2]);
		}
		
    	else strcpy(HexName, argv[j]);
    }
    
    printf("MSP430 programmer using BO230X board. (C) Jesus Calvino-Fraga (2017)\n");
   
    //if(Read_Hex_File(HexName)<0)
    //{
    //	return 2;
    //}

    if(FT_Open(List_FTDI_Devices(), &handle) != FT_OK)
    {
        puts("Can not open FTDI adapter.\n");
        return 3;
    }
    
    if (FT_GetComPortNumber(handle, &lComPortNumber) == FT_OK)
    { 
    	if (lComPortNumber != -1)
    	{
    		printf("Connected to COM%d\n", lComPortNumber); fflush(stdout);
    		sprintf(buff,"echo COM%d>COMPORT.inc", lComPortNumber);
    		system(buff);
    	}
    }
    
	Activate_BSL();
	
    FT_SetBaudRate(handle, 9600);
	FT_SetDataCharacteristics(handle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_EVEN);
	FT_SetFlowControl(handle, FT_FLOW_NONE, 0x11, 0x13);
	FT_SetTimeouts(handle, 100, 100); // Set read timeout of 0.1sec, write timeout of 0.1sec

	// Warning:
	// BSL TX is P1.1 (pin 3), which is UART RXD
	// BSL RX is P1.5 (pin 7), which is GPIO / A5
	// which make the whole thing useles!
	// In normal mode:
	// TX is P1.2 (pin 4)
	// RX is P1.1 (pin 3)

	buff[0]=0x80;
	WriteSerial(buff, 1);
	buff2[0]=0;
	ReadSerial(buff2, 1);
	if(buff2[0]==0x90)
	{
		printf("Success!\n");
	}
	else
	{
		printf("Error. Received: %02x\n", buff2[0]);
	}
	FT_Close(handle);
	
	printf("Done\n");
	
    return 0;
}