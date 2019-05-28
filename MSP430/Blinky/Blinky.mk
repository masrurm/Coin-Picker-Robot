# Since we are compiling in windows, select 'cmd' as the default shell.  This
# is important because make will search the path for a linux/unix like shell
# and if it finds it will use it instead.  This is the case when cygwin is
# installed.  That results in commands like 'del' and echo that don't work.
SHELL=cmd

OBJS=blinky.o

GCC_DIR =  C:/Programs/msp430_gcc/bin
SUPPORT_DIR = C:/Programs/msp430_gcc/include

DEVICE  = msp430g2553
CC      = $(GCC_DIR)/msp430-elf-gcc
LD      = $(GCC_DIR)/msp430-elf-ld
OBJCOPY = $(GCC_DIR)/msp430-elf-objcopy

CCFLAGS = -I $(SUPPORT_DIR) -mmcu=$(DEVICE) -O2 -g
LDFLAGS = -L $(SUPPORT_DIR)

# The default 'target' (output) is blinky.elf and 'depends' on
# the object files listed in the 'OBJS' assignment above.
# These object files are linked together to create blinky.elf.
# The linked file is converted to hex using program objcopy.
blinky.elf: $(OBJS)
	$(CC) $(OBJS) $(CCFLAGS) $(LDFLAGS) -o blinky.elf
	$(OBJCOPY) -O ihex blinky.elf blinky.hex
	@echo done!

# The object file blinky.o depends on blinky.c. blinky.c is compiled
# to create blinky.o.
blinky.o: blinky.c
	$(CC) -c $(CCFLAGS) blinky.c -o blinky.o
	
clean:
	del $(OBJS) *.elf *.hex

program: blinky.hex
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	..\programmer\MSP430_prog -p -r blinky.hex

verify: blinky.hex
	@Taskkill /IM putty.exe /F 2>NUL | wait 500
	..\programmer\MSP430_prog -v -r blinky.hex
