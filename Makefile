# Big Data Makefile

all: cc

cc:
	gcc main.c card.c -std=c99 -o main

clean:
	rm main output.csv
