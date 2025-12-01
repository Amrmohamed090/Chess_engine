all:
	gcc -oFast bbc.c -o bbc.exe
#  	X86_64-w64-mingw32-gcc -oFast bbc.c -o bbc.exe


debug:
	gcc bbc.c -o bbc.exe
#  	X86_64-w64-mingw32-gcc bbc.c -o bbc.exe



