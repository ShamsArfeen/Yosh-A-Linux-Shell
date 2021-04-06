make: yosh.c yosh.h parse.h parse.c main.c
	gcc -Wall main.c parse.c yosh.c -o yosh -lreadline
	
