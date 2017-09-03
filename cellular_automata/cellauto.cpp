

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>


using namespace std;

string color_code(int cell) {
    switch(cell) {
    case 0: return " "; // ".";
    case 1: return "\033[1;31m1\033[0m";
    case 2: return "\033[1;33m2\033[0m";
    case 3: return "\033[1;34m3\033[0m";;
    default: 
        char buffer[10];
        snprintf(buffer, 10, "%d", cell);
        return buffer;    
    }
}

class automata_rule {
    automata_rule(); // private so nobody uses this one.
public:
    automata_rule(int k0, //int r0,
     int n0, const string& rule_mapping_string) : k(k0), // r(r0),
      n(n0) {

        for(int i = 0; i < rule_mapping_string.length(); i++)
        {
             rule_mapping[i] = rule_mapping_string[n-1-i]-'0';
        }
    }
    virtual ~automata_rule(){}

    int k;
    // TODO: I hardcoded it to 1 just so it runs faster. Finish it with the generic case later.
    //int r;
    int n;
    map<int,int> rule_mapping;
    
    void print() {
        cout << "k " << k << "; r" << 1 // r
         << "; n " << n << "; mapping: ";
        for(int i = n-1; i>=0; --i)
            cout << rule_mapping[i];
        cout << "\n";
        for(int i = 0; i<n; ++i)
            cout << i << "->" << rule_mapping[i] << "\n";
    }

    void run(const string& initial, int num_iterations, int size_of_buffer=200) {
        
        clock_t t0 = clock();
        
        cout << "Running " << initial << "\n----------------\n";
        
        int buffer[2][size_of_buffer];
        int buffer_flag=0;
        int offset = (size_of_buffer-initial.length())/2; 
        
        for(int i = 0; i < size_of_buffer; i++) {
            buffer[0][i] = buffer[1][i] = 0;
        }

        for(int i = 0; i < initial.length(); i++) {
             buffer[buffer_flag][offset+i] = initial[i]-'0';
        }
        
        while(num_iterations--) {
            for(int i=1;i<size_of_buffer-1;i++) {
                cout << color_code(buffer[buffer_flag][i]);
            
                buffer[!buffer_flag][i] = rule_mapping[buffer[buffer_flag][i]
                                                       +buffer[buffer_flag][i-1]
                                                       +buffer[buffer_flag][i+1]];
            }
            cout << "\n";

            buffer_flag=!buffer_flag;
        }
    
        cout << "Done----------------\n";
        clock_t t = clock() - t0;
        cout << "(Took " << (((float)t)/CLOCKS_PER_SEC) << "ms\n";
    }
};

int main(int argc, char *argv[]) {

    cout << "hello!\n";

    //41     10        3311100320
    //  with starting points of 1, 23, 11
    class automata_rule myrule(4,//1,
                        10,"3311100320");
    myrule.print();

     myrule.run("1", 1000);
     myrule.run("23", 1000);
     myrule.run("11", 1000);


    cout << "Bye-bye!\n";

    return 0;
}
