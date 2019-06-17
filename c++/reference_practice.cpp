#include <iostream>  
using namespace std;  
template<class T> T add(T &a,T &b)  
{  
    a = 100;
    T result = a+b;  
    return result;  
      
}  
int main()  
{  
  int i =2;  
  int j =3;  
  float m = 2.3;  
  float n = 1.2;  
  cout<<"Addition of i and j is :"<<add(i,j)<<endl;
  cout<<"final value of i: "<<i<<endl;
  cout<<"Addition of m and n is :"<<add(m,n)<<endl;
  cout<<"final value of m: "<<m<<endl;
  return 0;  
}  
