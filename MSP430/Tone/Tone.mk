SHELL=cmd

OBJS=tone.o
PORTN=$(shell type COMPORT.inc)

GCC_DIR =  C:/Programs/msp430_gcc/bin
SUPPORT_DIR = C:/Programs/msp430_gcc/include

DEVICE  = msp430g2553
CC      = $(GCC_DIR)/msp430-elf-gcc
LD      = $(GCC_DIR)/msp430-elf-ld
OBJCOPY = $(GCC_DIR)/msp430-elf-objcopy

CCFLAGS = -I $(SUPPORT_DIR) -mmcu=$(DEVICE) -O2 -g
LDFLAGS = -L $(SUPPORT_DIR)

tone.elf: $(OBJS)
	$(CC) $(OBJS) $(CCFLAGS) $(LDFLAGS) -o tone.elf
	$(OBJCOPY) -O ihex tone.elf tone.hex
	@echo done!

tone.o: tone.c
	$(CC) -c $(CCFLAGS) tone.c -otone.o
	
clean:
	del $(OBJS) *.elf *.hex

program: tone.hex
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	..\programmer\MSP430_prog -p -r tone.hex
	cmd /c start putty -serial $(PORTN) -sercfg 115200,8,n,1,N -v
	
putty:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	cmd /c start putty -serial $(PORTN) -sercfg 115200,8,n,1,N -v
