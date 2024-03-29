PREFIX = mipsel-linux-gnu-
CC = $(PREFIX)gcc
LD = $(PREFIX)ld

UDIR = /usr/include/umps3
SDIR = /usr/share/umps3
CFLAGS = -ffreestanding -ansi -mips1 -mabi=32 -std=gnu99 -mno-gpopt -EL -G 0 -mno-abicalls -fno-pic -mfp32 -I$(UDIR) -Wall -O0
LFLAGS = -G 0 -nostdlib -T $(SDIR)/umpscore.ldscript -m elf32ltsmip
OBJECTS = asl.o pcb.o initialization.o syscall.o interrupt.o exception.o scheduler.o debug.o globals.o libumps.o crtso.o exception_support.o test.o pager.o

all: kernel.core.umps

.PHONY: all clean 

kernel.core.umps: kernel
	umps3-elf2umps -k kernel

kernel: $(OBJECTS)
	$(LD) -o kernel $(OBJECTS) $(LFLAGS)

crtso.o: $(SDIR)/crtso.S
	$(CC) $(CFLAGS) -c -o crtso.o $(SDIR)/crtso.S

exception.o: exception.c
	$(CC) $(CFLAGS) -c -o exception.o exception.c

scheduler.o: scheduler.c
	$(CC) $(CFLAGS) -c -o scheduler.o scheduler.c 

interrupt.o: interrupt.c
	$(CC) $(CFLAGS) -c -o interrupt.o interrupt.c

syscall.o: syscall.c
	$(CC) $(CFLAGS) -c -o syscall.o syscall.c

initialization.o: initialization.c
	$(CC) $(CFLAGS) -c -o initialization.o initialization.c

debug.o: debug.c 
	$(CC) $(CFLAGS) -c -o debug.o debug.c

globals.o: globals.c
	$(CC) $(CFLAGS) -c -o globals.o globals.c

pcb.o: pcb.c
	$(CC) $(CFLAGS) -c -o pcb.o pcb.c

asl.o: asl.c
	$(CC) $(CFLAGS)	-c -o asl.o asl.c

exception_support.o: exception_support.c
	$(CC) $(CFLAGS) -c -o exception_support.o exception_support.c

test.o: test.c
	$(CC) $(CFLAGS) -c -o test.o test.c

pager.o: pager.c
	$(CC) $(CFLAGS) -c -o pager.o pager.c

libumps.o: $(SDIR)/libumps.S
	$(CC) $(CFLAGS) -c -o libumps.o $(SDIR)/libumps.S

clean:
	rm kernel *.o *.umps
