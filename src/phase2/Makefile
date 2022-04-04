PREFIX = mipsel-linux-gnu-
CC = $(PREFIX)gcc
LD = $(PREFIX)ld

UDIR = /usr/include/umps3
SDIR = /usr/share/umps3
CFLAGS = -ffreestanding -ansi -mips1 -mabi=32 -std=gnu99 -mno-gpopt -EL -G 0 -mno-abicalls -fno-pic -mfp32 -I$(UDIR) -Wall -O0
LFLAGS = -G 0 -nostdlib -T $(SDIR)/umpscore.ldscript -m elf32ltsmip
OBJECTS = p2test.o asl.o pcb.o initialization.o libumps.o crtso.o scheduler.o exception.o

all: kernel.core.umps

.PHONY: all clean 

kernel.core.umps: kernel
	umps3-elf2umps -k kernel

kernel: $(OBJECTS)
	$(LD) -o kernel $(OBJECTS) $(LFLAGS)

p2test.o: p2test.c
	$(CC) $(CFLAGS) -c -o p2test.o p2test.c

crtso.o: $(SDIR)/crtso.S
	$(CC) $(CFLAGS) -c -o crtso.o $(SDIR)/crtso.S

exception.o: exception.c
	$(CC) $(CFLAGS) -c -o exception.o exception.c

scheduler.o: scheduler.c
	$(CC) $(CFLAGS) -c -o scheduler.o scheduler.c

initialization.o: initialization.c
	$(CC) $(CFLAGS) -c -o initialization.o initialization.c

pcb.o: ../phase1/pcb.c
	$(CC) $(CFLAGS) -c -o pcb.o ../phase1/pcb.c

asl.o: ../phase1/asl.c
	$(CC) $(CFLAGS)	-c -o asl.o ../phase1/asl.c

libumps.o: $(SDIR)/libumps.S
	$(CC) $(CFLAGS) -c -o libumps.o $(SDIR)/libumps.S

clean:
	rm kernel *.o *.umps
