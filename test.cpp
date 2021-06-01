#include <iostream>
using namespace std;
int main(){
    unsigned int a = 3;
    bool k = false;
    // unsigned int b = 8;
    if (a>>2 & 1){
        cout << "true";
    }
    else{
        cout <<"false";
        cout << k;
    }
    cout << a;
    return 0 ;
}