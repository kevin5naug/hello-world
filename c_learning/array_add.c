#include<stdio.h>

void array_add(int *a, int size)
{
    for (int i=0; i<size;i++){
        *a=*a+1;
        a++;
    }
}
void main()
{
    int a[]={1,2,3,4,5};
    int *test=a;
    array_add(test, sizeof(a)/sizeof(int));
    printf("[%d, %d, %d, %d, %d]\n", a[0],a[1],a[2],a[3],a[4]);
}
