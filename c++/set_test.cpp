#include<iostream>
#include<set>

using namespace std;

int main(){
    set<int> mp;
    mp.insert(30);
    mp.insert(171);
    mp.insert(111);
    mp.insert(2);
    mp.insert(5);
    cout<<"Elements are: \n";
    for(auto it=mp.begin();it!=mp.end();it++){
	cout<<(*it)<<endl;
    }
    auto it=mp.lower_bound(31);
    cout<<"The lowerbound of key 31 is ";
    cout<<(*it)<<endl;
    it=mp.lower_bound(300);
    if(it!=mp.end()){
	cout<<"The lowerbound of key 300 is ";
	cout<<(*it)<<endl;
    }
    return 0;
}
