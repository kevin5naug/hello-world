#include<iostream> 
using namespace std; 

class b
{
public:
virtual void show()
{
cout<<"\n  Showing base class...."<<endl;
}
void display()
{
cout<<"\n  Displaying base class...." <<endl;
}
};

class d:public b
{
public:
void display()
{
cout<<"\n  Displaying derived class...."<<endl;
}
void show()
{
cout<<"\n  Showing derived class...."<<endl;
}
};

int main()
{
b B;
b *ptr;
cout<<"\n\t P points to base:\n" ; ptr=&B; ptr->display();
ptr->show();
cout<<"\n\n\t P points to drive:\n"; d D; ptr=&D; ptr->display();
ptr->show();
D.show();
D.display();
}
