#include <systemc.h>
#include "full_adder.h"

SC_MODULE(csa) {
    sc_in<bool> A, B;
    sc_in<bool> Sin, Cin;
    sc_out<bool> Sout, Cout;

    sc_signal<bool> AB;

    full_adder* fa;

    void and_gate() {
        AB.write(A.read() & B.read());
    }

    SC_CTOR(csa) {
        fa = new full_adder("fa");
        fa->A(AB);
        fa->B(Sin);
        fa->Cin(Cin);
        fa->Sum(Sout);
        fa->Cout(Cout);

        SC_METHOD(and_gate);
        sensitive << A << B;
    }

    ~csa() { delete fa; }
};
