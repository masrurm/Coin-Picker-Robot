SHELL=cmd

OBJS=Servo.o
PORTN=$(shell type COMPORT.inc)

GCC_DIR =  C:\CrossIDE\msp430-gcc-6.2.1.16_win32\bin
SUPPORT_DIR = C:\CrossIDE\msp430-gcc-support-files\include

DEVICE  = msp430g2553
CC      = $(GCC_DIR)/msp430-elf-gcc
LD      = $(GCC_DIR)/msp430-elf-ld
OBJCOPY = $(GCC_DIR)/msp430-elf-objcopy

CCFLAGS = -I $(SUPPORT_DIR) -mmcu=$(DEVICE) -O2 -g
LDFLAGS = -L $(SUPPORT_DIR)

Servo.elf: $(OBJS)
	$(CC) $(OBJS) $(CCFLAGS) $(LDFLAGS) -o Servo.elf
	$(OBJCOPY) -O ihex Servo.elf Servo.hex
	@echo done!

Servo.o: Servo.c
	$(CC) -c $(CCFLAGS) Servo.c -oServo.o
	
Period.elf: $(OBJS)
	$(CC) $(OBJS) $(CCFLAGS) $(LDFLAGS) -o Period.elf
	$(OBJCOPY) -O ihex Period.elf Period.hex
	@echo done!

Period.o: Period.c
	$(CC) -c $(CCFLAGS) Period.c -oPeriod.o
	
clean:
	del $(OBJS) *.elf *.hex

program: Servo.hex
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	C:\CrossIDE\Programmer\MSP430_prog -p -r Servo.hex
	cmd /c start putty -serial $(PORTN) -sercfg 115200,8,n,1,N -v
	
putty:
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	C:\CrossIDE\Servo\putty.exe -serial $(PORTN) -sercfg 115200,8,n,1,N -v