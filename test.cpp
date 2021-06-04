#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <bitset>
#include <vector>
#include <bits/stdc++.h>

using namespace std;
 
int main()
{
    string k="aaabbbcccdddeeefffggg";
    ofstream testfile("cout.txt");
    for (int i=0;i<k.size();i=i+4){
        testfile << k.substr(i,4) << endl;
    }
    testfile << "new ewwee"<< endl;
    unsigned int p = 23;
    testfile << bitset<32>(p) << endl;
}