SHELL=cmd

OBJS=main.o
PORTN=$(shell type COMPORT.inc)

GCC_DIR =  C:/Programs/msp430_gcc/bin
SUPPORT_DIR = C:/Programs/msp430_gcc/include

DEVICE  = msp430g2553
CC      = $(GCC_DIR)/msp430-elf-gcc
LD      = $(GCC_DIR)/msp430-elf-ld
OBJCOPY = $(GCC_DIR)/msp430-elf-objcopy
ODDJUMP = $(GCC_DIR)/msp430-elf-objdump

CCFLAGS = -I $(SUPPORT_DIR) -mmcu=$(DEVICE) -O2 -g
LDFLAGS = -L $(SUPPORT_DIR)

ShowCalibration.elf: $(OBJS)
	$(CC) $(OBJS) $(CCFLAGS) $(LDFLAGS) -o ShowCalibration.elf
	$(OBJCOPY) -O ihex ShowCalibration.elf ShowCalibration.hex
	@echo done!

main.o: main.c
	$(CC) -c $(CCFLAGS) main.c -o main.o
	
clean:
	del $(OBJS) *.elf

program: ShowCalibration.hex
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	..\programmer\MSP430_prog -p -r ShowCalibration.hex
	cmd /c start putty -serial $(PORTN) -sercfg 115200,8,n,1,N -v
	
putty:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	cmd /c start putty -serial $(PORTN) -sercfg 115200,8,n,1,N -v

dummy: main.lst
	@echo Hello!

