#include <iostream>
using namespace std;

class base {
public:
    void display1()
    {
	cout << "Base class content."<<endl;
    }
};
class derived : public base {
public:
    void display2()
    {
	base::display1();
	cout << "1st derived class content."<<endl;
    }
};

class derived2 : public derived {
public:
    void display3()
    {
	derived::display1();
	derived::display2();
	cout << "2nd Derived class content."<<endl;
    }
};

int main()
{
    derived2 D;
    D.display3();
    cout<<endl;
    D.display2();
    cout<<endl;
    D.display1();
}
