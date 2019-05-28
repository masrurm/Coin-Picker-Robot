// FT230XS SPI in synchronous bit bang mode to program
// the MSP430G2553 microcontroller. Connect as follows:
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

#define BAUDRATE (9600/4)
#define CLOCKSPERBIT (16/4)
#define CHUNK 32

#define BSL_TXD   0x01  // FT230XS TXD pin
#define NOT_USED1 0x02  // FT230XS RXD pin
#define NOT_USED2 0x04  // FT230XS RTS pin
#define BSL_RXD   0x08  // FT230XS CTS pin
#define OUTPUTS (BSL_RXD)

unsigned char bitloc[]={0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

#define BUFFSIZE 0x10000

FT_HANDLE handle;
DWORD bytes;

unsigned char BitBangBuff[BUFFSIZE]; // Buffer used to transmit and receive bit-banged bits
DWORD BitBangBuff_cnt;
unsigned char Transmit_Serial[BUFFSIZE/(11*CLOCKSPERBIT)]; // Bytes to encode and transmit
unsigned char Received_Serial[BUFFSIZE/(11*CLOCKSPERBIT)]; // Received and decoded input bytes
DWORD Received_Serial_cnt;

#define MEMSIZE 0x10000
unsigned char Flash_Buffer[MEMSIZE];
char HexName[MAX_PATH]="";

clock_t startm, stopm;
#define START if ( (startm = clock()) == -1) {printf("Error calling clock");exit(1);}
#define STOP if ( (stopm = clock()) == -1) {printf("Error calling clock");exit(1);}
#define PRINTTIME printf( "%.1f seconds.", ((double)stopm-startm)/CLOCKS_PER_SEC);

int Selected_Device=-1;
BOOL b_program=FALSE;
BOOL b_verify=FALSE;
BOOL b_run=FALSE;
int m_Time2Start;

void GetOut (int val)
{
	if(handle!=NULL)
	{
		FT_SetBitMode(handle, 0x00, FT_BITMODE_CBUS_BITBANG); // Set CBUS3 and CBUS0 as input
		FT_SetBitMode(handle, 0x0, FT_BITMODE_RESET); // Back to serial port mode
		FT_Close(handle);
	}
	exit(val);
}

void Load_Byte (unsigned char val)
{
	int j, k, parity=0;
	
	if((BitBangBuff_cnt+(CLOCKSPERBIT*11))>=BUFFSIZE)
	{
		printf("ERROR: Unable to load %d bytes in buffer.  Max is %d.\n", BitBangBuff_cnt+(CLOCKSPERBIT*11), BUFFSIZE);
		GetOut(0);
	}
	
	// Load start bit
	for(k=0; k<CLOCKSPERBIT; k++)	BitBangBuff[BitBangBuff_cnt++]=0;
	
	// Load 8 data bits
	for(j=0; j<8; j++)
	{
		if((val&bitloc[j]))
		{
			for(k=0; k<CLOCKSPERBIT; k++)	BitBangBuff[BitBangBuff_cnt++]=BSL_RXD;
			parity++;
		}
		else
		{
			for(k=0; k<CLOCKSPERBIT; k++)	BitBangBuff[BitBangBuff_cnt++]=0;
		}
	}
	
	// Load parity bit
	for(k=0; k<CLOCKSPERBIT; k++)	BitBangBuff[BitBangBuff_cnt++]=(parity & 0x01)?BSL_RXD:0;
	
	// Load stop bit
	for(k=0; k<CLOCKSPERBIT; k++)	BitBangBuff[BitBangBuff_cnt++]=BSL_RXD;
}

int Decode_BitBangBuff (int skip, int total)
{
	int j, k, parity=0;
	int time2start=0;
		
	for(j=skip; j<total; j++)
	{
		if ((BitBangBuff[j]&BSL_TXD)!=BSL_TXD)
		{
			// Got a start bit
			if(time2start==0) time2start=j; // Save the position of the first start bit
			
			j+=(CLOCKSPERBIT+(CLOCKSPERBIT/2)); // Skip start bit and point to the middle of bit0
			Received_Serial[Received_Serial_cnt]=0;
			parity=0;
			// Decode 8 data bits.  LSB first.
			for(k=0; k<8; k++)
			{
				if((BitBangBuff[j]&BSL_TXD)==BSL_TXD)
				{
					Received_Serial[Received_Serial_cnt]|=bitloc[k];
					parity++;
				}
				j+=CLOCKSPERBIT; // Point to the middle of next bit
			}
			Received_Serial_cnt++;
			// Get the parity bit
			if((BitBangBuff[j]&BSL_TXD)==BSL_TXD) parity++;
			
			if((parity&1)==0)
			{
				//printf("Parity is even\n");
			}
			else
			{
				printf("Parity Error\n");
				fflush(stdout);
			}

			j+=(CLOCKSPERBIT/2); // Skip to stop bit
		}
	}
	return time2start;
}

void Reset_BitBangBuff(void)
{
	int j;
	BitBangBuff_cnt=0;
	Received_Serial_cnt=0;
	memset(BitBangBuff, 0x0f, BUFFSIZE); // Load buffer with default value
}

void Send_BitBangBuff (void)
{
    FT_Write(handle, BitBangBuff, BitBangBuff_cnt, &bytes);
    FT_Read(handle, BitBangBuff, BitBangBuff_cnt, &bytes);
	//Decode_BitBangBuff(BitBangBuff_cnt);
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
				if(recordtype==0)
				{
					if((address+j)<MEMSIZE)
					{
						Flash_Buffer[address+j]=value;
						TotalBytes++;
					}
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
	       "%s -p -v -r somefile.hex   --- program, verify, and run.\n",
	       prn);
}

unsigned char Default_Password[]=
{
	0x80, 0x10, 0x24, 0x24, 0x00, 0x00, 0x00, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x00, 0x00 // Place holders for checksum
	//0x5B, 0xCB // Actual checksum, but let us calculate it
};

/*
The upper 16 bytes of the boot-ROM (0FF0h to 0FFFh) hold information about the device and BSL
version number in BCD representation. This is common for all devices and BSL versions:
	0FF0h to 0FF1h: Chip identification (for example, F413h for an F41x device).
	0FFAh to 0FFBh: BSL version number (for example, 0130h for BSL version V1.30).
*/

unsigned char BSL_Version[]=
{
	0x80, 0x14, 0x04, 0x04, 0xF0, 0x0F, 0x10, 0x00, 0xFF, 0xFF
};

unsigned char Mass_erase[]=
{
	0x80, 0x18, 0x04, 0x04, 0x00, 0x00, 0x06, 0xA5, 0x00, 0x00
};

void Calculate_Check_Sum (unsigned char * TXbuffer, unsigned int Length)
{
	int k;
	int temp1 = TXbuffer[0];
	int temp2 = TXbuffer[1];
	
	for (k = 2; k < Length-2 ; k += 2) // bytes to encode for checksum 
	{               
	    temp1 ^= TXbuffer[k];
	    temp2 ^= TXbuffer[k+1];
	}
	TXbuffer[Length-2]= (unsigned char)~temp1; // fill in the checksum low
	TXbuffer[Length-1]= (unsigned char)~temp2; // fill in the checksum high
	//printf("CHKH:0x%02X  CHKL:0x%02X\n", TXbuffer[Length-2], TXbuffer[Length-1]);
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
					GetOut(0);
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
					GetOut(0);
				}
			}
		}
	}
}

void Activate_BSL (void)
{
	// Bits 7 down to 4 set CBUSx as either input or ouput.  Bits 3 down to 0 is the actual output.
	// For example 0x90 configures CBUS3 and CBUS0 as outputs and set their values to zero.
	unsigned char Seq[7]={0x90, 0x91, 0x90, 0x91, 0x99, 0x98, 0x00};
	unsigned char c;

    FT_SetBaudRate(handle, BAUDRATE);

	// Set the RXD_BSL pin to high before activating BSL
	FT_SetBitMode(handle, OUTPUTS, FT_BITMODE_SYNC_BITBANG);
	c=0xff;
    FT_Write(handle, &c, 1, &bytes);
    FT_Read(handle, &c, 1, &bytes);
	
	FTDI_Set_CBUS_Mode(1); // Make sure both CBUS3 and CBUS0 are in "CBUS IOMODE"
	
	for(c=0; c<7; c++)
	{
		FT_SetBitMode(handle, Seq[c], FT_BITMODE_CBUS_BITBANG); // RESET=0, TEST=0
		Sleep(1); // Sleep mili seconds
	}
	
	FT_Purge(handle, FT_PURGE_RX | FT_PURGE_TX);
	FT_SetBitMode(handle, OUTPUTS, FT_BITMODE_SYNC_BITBANG);
	FT_SetLatencyTimer (handle, 2); // 2 ms is the minimum allowed...

	Sleep(10);
}

void Display_Received (void)
{
	int j;
	printf("Number of bytes in buffer: %d\n", Received_Serial_cnt);
	for(j=0; j<Received_Serial_cnt; j++)
	{
		printf("%02x ", Received_Serial[j]);
	}
	printf("\n");
	fflush(stdout);
}

void Send_Command (unsigned char * Buffer, int nTX, int nRX)
{
	int j;
	
	Reset_BitBangBuff();
	Load_Byte(0x80);
	
	// Add space between the synchronization and the actual command.  About two milliseconds...
	for(j=0; j<(CLOCKSPERBIT*11*2); j++) BitBangBuff_cnt++;

	Calculate_Check_Sum(Buffer, nTX);
	for(j=0; j<nTX; j++) Load_Byte(Buffer[j]);
	// Send the command and acquire the response
    FT_Write(handle, BitBangBuff, BitBangBuff_cnt+(CLOCKSPERBIT*11*(nRX+1)), &bytes);
    // Get the response
    FT_Read(handle, BitBangBuff, BitBangBuff_cnt+(CLOCKSPERBIT*11*(nRX+1)), &bytes);
    // Decode the response
	m_Time2Start=Decode_BitBangBuff(BitBangBuff_cnt, BitBangBuff_cnt+(CLOCKSPERBIT*11*(nRX+1)));
	Display_Received();
}

void Send_Command_ne (unsigned char * Buffer, int nTX, int nRX)
{
	int j;
	
	Reset_BitBangBuff();
	Load_Byte(0x80);
	
	// Add space between the synchronization and the actual command.  About two milliseconds...
	for(j=0; j<(CLOCKSPERBIT*11*2); j++) BitBangBuff_cnt++;

	Calculate_Check_Sum(Buffer, nTX);
	for(j=0; j<nTX; j++) Load_Byte(Buffer[j]);
	// Send the command and acquire the response
    FT_Write(handle, BitBangBuff, BitBangBuff_cnt+(CLOCKSPERBIT*11*(nRX+1)), &bytes);
    // Get the response
    FT_Read(handle, BitBangBuff, BitBangBuff_cnt+(CLOCKSPERBIT*11*(nRX+1)), &bytes);
    // Decode the response
	m_Time2Start=Decode_BitBangBuff(BitBangBuff_cnt, BitBangBuff_cnt+(CLOCKSPERBIT*11*(nRX+1)));
}

void Flash_MSP430 (void)
{
    int j, k, dot;
	unsigned char buff[256];
	BOOL Data_is_0xff;

	START;
	    
	Activate_BSL();

	if(b_program)
	{
		printf("Erasing memory.  Please wait.\n");
		fflush(stdout);
		Send_Command_ne(Mass_erase, sizeof(Mass_erase), 600); // It takes about 0.5 seconds to erase the flash.  Assume 1 ms per byte...
		if(Received_Serial[0]==0x90)
		{
			printf("'Mass erase' command completed in %f seconds\n", m_Time2Start/(9600.0*CLOCKSPERBIT));
			fflush(stdout);
		}
		else
		{
			printf("Mass erase ERROR.\n");
			fflush(stdout);
			GetOut(5);
		}
		
		Send_Command_ne(Default_Password, sizeof(Default_Password), 1);
		if(Received_Serial[0]==0x90)
		{
			printf("Password ok.\n");
			fflush(stdout);
		}
		else
		{
			printf("Password ERROR.  Probalby 'mass erase' command failed.\n");
			fflush(stdout);
			GetOut(5);
		}
	}
	else if (b_verify) // Send the vector table from the file (this is the password)
	{
		buff[0]=0x80;
		buff[1]=0x10;
		buff[2]=32+4;
		buff[3]=32+4;
		buff[4]=0;
		buff[5]=0;
		buff[6]=0;
		buff[7]=0;
		for(k=0; k<32; k++) buff[8+k]=Flash_Buffer[0xFFE0+k];
		buff[8+32]=0;
		buff[8+32+1]=0;
		Send_Command_ne(buff, 8+32+2, 1);
		if(Received_Serial[0]==0x90)
		{
			printf("Password ok.\n");
			fflush(stdout);
		}
		else
		{
			printf("Password ERROR.  Verification failed.\n");
			fflush(stdout);
			GetOut(5);
		}
	}	
	
	Send_Command_ne(BSL_Version, sizeof(BSL_Version), 22);
	printf("MSP430x%02x%02x detected\n", Received_Serial[4], Received_Serial[5]);
	printf("BSL Version: %d.%02X\n", Received_Serial[14], Received_Serial[15]);
	fflush(stdout);

	if(b_program)
	{
		// Load the flash
		printf("Loading flash memory: \n");
		fflush(stdout);
		dot=0;
		for(j=0; j<0x10000; j+=CHUNK)
		{
			//RX data block: 80 12 n n AL AH n–4 0 D1 D2 … Dn–4 CKL CKH
			buff[0]=0x80;
			buff[1]=0x12;
			buff[2]=CHUNK+4;
			buff[3]=CHUNK+4;
			buff[4]=(unsigned char)(j%0x100);
			buff[5]=(unsigned char)(j/0x100);
			buff[6]=CHUNK;
			buff[7]=0;
			Data_is_0xff=TRUE;
			for(k=0; k<CHUNK; k++)
			{
				buff[8+k]=Flash_Buffer[j+k];
				if(Flash_Buffer[j+k]!=0xff) Data_is_0xff=FALSE;
			}
			buff[8+CHUNK]=0x00;
			buff[8+CHUNK+1]=0x00;
			
			if(Data_is_0xff==FALSE)
			{
				Send_Command_ne(buff, 8+CHUNK+2, 1);
				
				dot++;
				if(dot==64)
				{
					dot=0;
					printf("\n");	
				}
				if(Received_Serial[0]==0x90)
				{
					printf(".");
					fflush(stdout);
				}
				else
				{
					printf("\nProgram ERROR at %04x.\n", j);
					fflush(stdout);
					GetOut(0);
				}
			}		
		}
		printf("\nDone\n");
	}

	if(b_verify)
	{
		// Load the flash
		printf("Verifying flash memory: \n");
		fflush(stdout);
		dot=0;
		for(j=0; j<0x10000; j+=CHUNK)
		{			
			Data_is_0xff=TRUE;
			for(k=0; k<CHUNK; k++)
			{
				if(Flash_Buffer[j+k]!=0xff) Data_is_0xff=FALSE;
			}
			
			if(Data_is_0xff==FALSE)
			{				
				//0x80, 0x14, 0x04, 0x04, 0x0C, 0xC0, 0x10, 0x00, 0x75, 0xE0
				buff[0]=0x80;
				buff[1]=0x14; // Read command
				buff[2]=4;
				buff[3]=4;
				buff[4]=(unsigned char)(j%0x100);
				buff[5]=(unsigned char)(j/0x100);
				buff[6]=CHUNK;
				buff[7]=0;
				buff[8]=0;
				buff[9]=0;
				Send_Command_ne(buff, 10, CHUNK+6);

				dot++;
				if(dot==64)
				{
					dot=0;
					printf("\n");	
				}

				printf(".");
				fflush(stdout);
				
				for(k=0; k<CHUNK; k++)
				{
					if(Flash_Buffer[j+k]!=Received_Serial[4+k]) break;
				}
				if(k!=CHUNK) break;
			}		
		}
		if(j!=0x10000)
		{
			printf("\nVerify ERROR at %04x.\n", j);
			fflush(stdout);
			GetOut(0);
		}
		else
		{
			printf("\nDone.\n", j);
			fflush(stdout);
		}
	}

	if(b_run)
	{
		printf("Running program... ");
		fflush(stdout);
		
		FT_SetBitMode(handle, 0x90, FT_BITMODE_CBUS_BITBANG); // RESET=0, TEST=0

		printf("Done\n");
		fflush(stdout);
	}
	
    printf("Actions completed in ");
    STOP;
	PRINTTIME;
	printf("\n"); fflush(stdout);
}

int main(int argc, char ** argv)
{
    int j;
	unsigned char buff[32];
	LONG lComPortNumber;

    if(argc<2)
    {
    	printf("Need a filename to proceed.  ");
    	print_help(argv[0]);
    	return 1;
    }
    
    for(j=1; j<argc; j++)
    {		
		if(stricmp("-p", argv[j])==0) b_program=TRUE;
		else if(stricmp("-v", argv[j])==0) b_verify=TRUE;
		else if(stricmp("-r", argv[j])==0) b_run=TRUE;

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
   
    if(Read_Hex_File(HexName)<0)
    {
    	return 2;
    }

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
    
	Flash_MSP430();    
    
	GetOut(0);
		
    return 0;
}
