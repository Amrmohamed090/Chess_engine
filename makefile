all:
	gcc -Ofast bbc.c -o bbc.exe


debug:
	gcc bbc.c -o bbc.exe

dll:
	gcc -shared -o bbc.dll bbc.c

