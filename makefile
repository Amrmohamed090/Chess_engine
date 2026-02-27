all:
	gcc -Ofast bbc.c -o bbc.exe


debug:
	gcc bbc.c -o bbc_debug.exe

dll:
	gcc -shared -o bbc.dll bbc.c

ANN:
	gcc ANN.c -o ANN.exe

