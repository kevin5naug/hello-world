#include <iostream>
using namespace std;

class MinusOverload{
    private:
	int a;
	int b;
    public:
	void Distance(){
	    a=0;
	    b=0;
	}
	MinusOverload(int f, int i){
	    int c;
	    a=f;
	    b=i;
	    c=a-b;
	    cout<<"C:"<<c<<endl;
	}
	void display(){
	    cout<<"A: "<<a<<" B: "<<b<<endl;
	}
	MinusOverload operator-(){
	    a=-a;
	    b=-b;
	    return MinusOverload(a,b);
	}
};

int main(){
    MinusOverload M1(6,8), M2(-3,-4), temp(0,0);
    temp=-M1;
    temp.display();
    M1.display();
    temp=-M2;
    temp.display();
    M2.display();
    return 0;
}
