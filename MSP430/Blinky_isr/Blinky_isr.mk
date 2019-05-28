SHELL=cmd

OBJS=Blinky_isr.o

GCC_DIR =  C:/Programs/msp430_gcc/bin
SUPPORT_DIR = C:/Programs/msp430_gcc/include

DEVICE  = msp430g2553
CC      = $(GCC_DIR)/msp430-elf-gcc
LD      = $(GCC_DIR)/msp430-elf-ld
OBJCOPY = $(GCC_DIR)/msp430-elf-objcopy

CCFLAGS = -I $(SUPPORT_DIR) -mmcu=$(DEVICE) -O2 -g
LDFLAGS = -L $(SUPPORT_DIR)

Blinky_isr.elf: $(OBJS)
	$(CC) $(OBJS) $(CCFLAGS) $(LDFLAGS) -o Blinky_isr.elf
	$(OBJCOPY) -O ihex Blinky_isr.elf Blinky_isr.hex
	@echo done!

Blinky_isr.o: Blinky_isr.c
	$(CC) -c $(CCFLAGS) Blinky_isr.c -oBlinky_isr.o
	
clean:
	del $(OBJS) *.elf *.hex

program: Blinky_isr.hex
	..\programmer\MSP430_prog -p -r Blinky_isr.hex

verify: Blinky_isr.hex
	..\programmer\MSP430_prog -v -r Blinky_isr.hex
