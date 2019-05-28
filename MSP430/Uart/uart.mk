SHELL=cmd

OBJS=main.o uart.o
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

main.elf: $(OBJS)
	$(CC) $(OBJS) $(CCFLAGS) $(LDFLAGS) -o main.elf
	$(OBJCOPY) -O ihex main.elf main.hex
	@echo done!

main.o: main.c
	$(CC) -c $(CCFLAGS) main.c -o main.o

uart.o: uart.c
	$(CC) -c $(CCFLAGS) uart.c -o uart.o
	
clean:
	del $(OBJS) *.elf *.hex

program: main.hex
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	..\programmer\MSP430_prog -p -r main.hex
	cmd /c start putty -serial $(PORTN) -sercfg 115200,8,n,1,N -v
	
putty:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	cmd /c start putty -serial $(PORTN) -sercfg 115200,8,n,1,N -v

dummy: main.lst
	@echo Hello!
	
dissasembly:
	$(ODDJUMP) -d main.elf > main.lst
