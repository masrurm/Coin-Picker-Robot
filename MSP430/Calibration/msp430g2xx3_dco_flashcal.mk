SHELL=cmd

OBJS=msp430g2xx3_dco_flashcal.o uart.o

GCC_DIR =  C:/Programs/msp430_gcc/bin
SUPPORT_DIR = C:/Programs/msp430_gcc/include

DEVICE  = msp430g2553
CC      = $(GCC_DIR)/msp430-elf-gcc
LD      = $(GCC_DIR)/msp430-elf-ld
OBJCOPY = $(GCC_DIR)/msp430-elf-objcopy

CCFLAGS = -I $(SUPPORT_DIR) -mmcu=$(DEVICE) -O2 -g
LDFLAGS = -L $(SUPPORT_DIR)

msp430g2xx3_dco_flashcal.elf: $(OBJS)
	$(CC) $(OBJS) $(CCFLAGS) $(LDFLAGS) -o msp430g2xx3_dco_flashcal.elf
	$(OBJCOPY) -O ihex msp430g2xx3_dco_flashcal.elf msp430g2xx3_dco_flashcal.hex
	@echo done!

msp430g2xx3_dco_flashcal.o: msp430g2xx3_dco_flashcal.c
	$(CC) -c $(CCFLAGS) msp430g2xx3_dco_flashcal.c -o msp430g2xx3_dco_flashcal.o

uart.o: uart.c
	$(CC) -c $(CCFLAGS) uart.c -o uart.o
	
clean:
	del $(OBJS) *.elf *.hex

program: msp430g2xx3_dco_flashcal.hex
	..\programmer\MSP430_prog -p -r msp430g2xx3_dco_flashcal.hex

program_cal: msp430g2xx3_dco_flashcal.hex Calibration_IC_1.hex
	..\programmer\MSP430_prog -p -r msp430g2xx3_dco_flashcal.hex Calibration_IC_1.hex

