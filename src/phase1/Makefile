PREFIX = mipsel-linux-gnu-
CC = $(PREFIX)gcc
LD = $(PREFIX)ld

UDIR = /usr/include/umps3
SDIR = /usr/share/umps3
CFLAGS = -ffreestanding -ansi -mips1 -mabi=32 -std=gnu99 -mno-gpopt -EL -G 0 -mno-abicalls -fno-pic -mfp32 -I$(UDIR) -Wall -O0
LFLAGS = -G 0 -nostdlib -T $(SDIR)/umpscore.ldscript -m elf32ltsmip
OBJECTS = p1test.o pcb.o asl.o libumps.o crtso.o

all: kernel.core.umps

kernel.core.umps: kernel
	umps3-elf2umps -k kernel

kernel: $(OBJECTS)
	$(LD) -o kernel $(OBJECTS) $(LFLAGS)

p1test.o: p1test.c
	$(CC) $(CFLAGS) -c -o p1test.o p1test.c

crtso.o: $(SDIR)/crtso.S
	$(CC) $(CFLAGS) -c -o crtso.o $(SDIR)/crtso.S

pcb.o: pcb.c
	$(CC) $(CFLAGS) -c -o pcb.o pcb.c

asl.o: asl.c
	$(CC) $(CFLAGS)	-c -o asl.o asl.c

libumps.o: $(SDIR)/libumps.S
	$(CC) $(CFLAGS) -c -o libumps.o $(SDIR)/libumps.S

clean:
	rm kernel *.o *.umps
