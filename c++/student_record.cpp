#include <iostream>
using namespace std;
class student
{
    public:
	char name[30], clas[10];
	int rol, age;

    void enter()
    {
	cout<<"Enter Student Name: ";
	cin>>name;
	cout<<"Enter Student Age: ";
	cin>>age;
	cout<<"Enter Student Roll Number: ";
	cin>>rol;
	cout<<"Enter Student Class: ";
	cin>>clas;
    }

    void display()
    {
	cout<<"\n Age \t Name \t R.No. \t Class";
	cout<<"\n"<<age<<"\t"<<name<<"\t"<<rol<<"\t"<<clas;
    }
};

int main()
{
    class student s;
    s.enter();
    s.display();
    cout<<endl;
}
