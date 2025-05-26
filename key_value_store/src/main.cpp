#include <iostream>
#include "store.h"
#include <string>
#include "utility"
using namespace std;
int main(){
    std::string st="../../data";
    Store store(st);
    std::pair<std::string,std::string> p;
    std::string t,x;
    while(true){
        cin>>t;
        if(t=="exit")break;
        else if(t=="delete"){
            cin>>x;
            cout<<(store.DELETE(x)).ToString()<<'\n';
            std::cout<<"Deleted\n";
        }
        else if(t=="get"){
            cin>>x;
            cout<<(store.GET(x,t)).ok()<<'\n';
            std::cout<<t<<'\n';
        }
        else{
            cin>>x;
            p={t,x};
            
            cout<<(store.PUT(p)).ToString()<<'\n';
            std::cout<<x<<'\n';
        }
    }
}