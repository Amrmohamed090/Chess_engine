#include <stdio.h>
#include <stdlib.h>

int main(){
    int a[5] = {10,20,30,40,50};
    int *p = a+2;
    printf("%d", *(p+1));
    printf("%d", *p+1);
    return 0;
}