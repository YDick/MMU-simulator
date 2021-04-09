# makefile for Virtual Memeory Unit (MMU)
#
# usage: make mmu 

CC=gcc 
CFLAGS=-Wall -std=c99

clean:
	rm -rf *.o
	rm -rf mmu
	rm -rf output*.csv
	
mmu: 
	$(CC) $(CFLAGS) -o mmu mmu.c