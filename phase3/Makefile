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

exception.o: src/exception.c
	$(CC) $(CFLAGS) -c -o exception.o src/exception.c

scheduler.o: src/scheduler.c
	$(CC) $(CFLAGS) -c -o scheduler.o src/scheduler.c 

interrupt.o: src/interrupt.c
	$(CC) $(CFLAGS) -c -o interrupt.o src/interrupt.c

syscall.o: src/syscall.c
	$(CC) $(CFLAGS) -c -o syscall.o src/syscall.c

initialization.o: src/initialization.c
	$(CC) $(CFLAGS) -c -o initialization.o src/initialization.c

debug.o: src/debug.c 
	$(CC) $(CFLAGS) -c -o debug.o src/debug.c

globals.o: src/globals.c
	$(CC) $(CFLAGS) -c -o globals.o src/globals.c

pcb.o: src/pcb.c
	$(CC) $(CFLAGS) -c -o pcb.o src/pcb.c

asl.o: src/asl.c
	$(CC) $(CFLAGS)	-c -o asl.o src/asl.c

exception_support.o: src/exception_support.c
	$(CC) $(CFLAGS) -c -o exception_support.o src/exception_support.c

test.o: src/test.c
	$(CC) $(CFLAGS) -c -o test.o src/test.c

pager.o: src/pager.c
	$(CC) $(CFLAGS) -c -o pager.o src/pager.c

libumps.o: $(SDIR)/libumps.S
	$(CC) $(CFLAGS) -c -o libumps.o $(SDIR)/libumps.S

clean:
	rm kernel *.o *.umps
