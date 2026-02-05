#include <systemc.h>

SC_MODULE(full_adder) {
    sc_in<bool> A, B, Cin;
    sc_out<bool> Sum, Cout;

    void compute() {
        bool a = A.read();
        bool b = B.read();
        bool c = Cin.read();
        Sum.write(a ^ b ^ c);
        Cout.write((a & b) | (a & c) | (b & c));
    }

    SC_CTOR(full_adder) {
        SC_METHOD(compute);
        sensitive << A << B << Cin;
    }
};
