#include<iostream>
#include<list>
#include<vector>
using namespace std;
int main(){
    vector<int> v = {1,2,3,4};
    list<int> l={1,2,3,4};
    list<int>::iterator it=l.begin();
    for(auto t=v.begin(); t!=v.end(); t++){
	cout<<&(*t)<<endl;
    }
    cout<<"#############"<<endl;
    for(int i=0;i<l.size();i++){
	advance(it, i);
	cout<<&(*it)<<endl;
	it=l.begin();
    }
}


