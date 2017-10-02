#include<stdio.h>
#define MAXSIZE 500

void swap(char* a, char*b){
    char temp=*a;
    *a=*b;
    *b=temp;
}

int getString(char str[]){
    int c,i;
    for(i=0;i<MAXSIZE-1&&(c=getchar())!=EOF;i++){
        str[i]=c;
    }
    str[i]='\0';
    return i;
}

void reverse(char str[], int len){
    int i=0;
    int j=len-1;
    while(i<j){
        swap(&str[i++],&str[j--]);
    }
}


void show(char str[], int len){
    for(int i=0;i<len;i++){
        putchar(str[i]);
    }
    printf("\n");
}

void main(){
    char str[MAXSIZE];
    int len=getString(str);
    reverse(str,len);
    show(str,len);
}
