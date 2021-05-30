#include <iostream>
using namespace std;
int main(){
    unsigned int a = 15;
    // unsigned int b = 8;
    if (a>>2 & 1){
        cout << "true";
    };
    cout << a;
    return 0 ;
}