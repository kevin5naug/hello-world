#include<stdio.h>
int add(int m, int *n)
{
    return (m+*n);
}
main()
{
    int a=25;
    int b=37;
    int *bb=&b;
    a=add(a, bb);
    printf("the final result of addtion over pointer&int: %d\n", a);
}

