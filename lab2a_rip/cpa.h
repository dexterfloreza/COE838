#include <systemc.h>
#include "full_adder.h"

SC_MODULE(cpa) {
    sc_in<bool> A, B, Cin;
    sc_out<bool> Sum, Cout;

    full_adder* fa;

    SC_CTOR(cpa) {
        fa = new full_adder("fa");
        fa->A(A);
        fa->B(B);
        fa->Cin(Cin);
        fa->Sum(Sum);
        fa->Cout(Cout);
    }

    ~cpa() { delete fa; }
};
