#include <iostream>
#include <iomanip>
using namespace std;

int main(){
    float basic, ta, da, gs;
    basic=10000;
    ta=800;
    da=5000;
    gs=basic+ta+da;
    cout<<setw(15)<<setfill('*')<<"Basic"<<setw(10)<<basic<<'\n'
	<<setw(15)<<setfill('$')<<"TA"<<setw(10)<<ta<<endl
	<<setw(15)<<"DA"<<setw(10)<<da<<endl
	<<setw(15)<<setfill('#')<<"GS"<<setw(10)<<gs<<endl;
    return 0;
}
