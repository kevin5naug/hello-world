#include<stdio.h>
void main(){
    char a[2][3]={{'t','b','c'},{'d','e', 'f'}};
    char *p0=a;
    char *p1=a[0];
    char *p2=&a[0][0];
    printf("%c for p0=a,%c for p1=a[0], %c for p2=&a[0][0]. ", *p0, *p1, *p2);
}
