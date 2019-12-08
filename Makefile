all: main
main: main.o func.o 
		gcc -o main main.o func.o

so.o: main.c func.h
		gcc -c func.c 

main.o: main.c func.h
		gcc -c main.c 

clean:
		rm -rf *.o
mrproper:S
		rm -rf main