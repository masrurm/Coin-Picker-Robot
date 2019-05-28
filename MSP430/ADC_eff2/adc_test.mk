SHELL=cmd

OBJS=adc.o
PORTN=$(shell type COMPORT.inc)

GCCPATH =  C:/Programs/msp430_gcc
SUPPORT_DIR = $(GCCPATH)/include

DEVICE  = msp430g2553
CC      = $(GCCPATH)/bin/msp430-elf-gcc
LD      = $(GCCPATH)/bin/msp430-elf-ld
OBJCOPY = $(GCCPATH)/bin/msp430-elf-objcopy
ODDJUMP = $(GCCPATH)/bin/msp430-elf-objdump

CCFLAGS = -I $(SUPPORT_DIR) -mmcu=$(DEVICE) -Os
LDFLAGS = -L $(SUPPORT_DIR) -Wl,-Map,adc_test.map

adc_test.elf: $(OBJS)
	$(CC) $(OBJS) $(CCFLAGS) $(LDFLAGS) -o adc_test.elf
	$(OBJCOPY) -O ihex adc_test.elf adc_test.hex
	@echo done!

adc.o: adc.c
	$(CC) -c $(CCFLAGS) adc.c -o adc.o
	
clean:
	del $(OBJS) *.elf

program: adc_test.hex
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	..\programmer\MSP430_prog -p -r adc_test.hex
	cmd /c start putty -serial $(PORTN) -sercfg 115200,8,n,1,N -v
	
putty:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	cmd /c start putty -serial $(PORTN) -sercfg 115200,8,n,1,N -v
	
dummy: adc_test.map
	@echo :-)