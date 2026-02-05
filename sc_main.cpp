#include <systemc.h>
#include "top.h"

int sc_main(int argc, char* argv[]) {
    top t("top");

    sc_trace_file* tf = sc_create_vcd_trace_file("multiplier_8x8");
    sc_trace(tf, t.A, "A");
    sc_trace(tf, t.B, "B");
    sc_trace(tf, t.P, "P");

    t.A = 3;   t.B = 5;   sc_start(10, SC_NS);
    t.A = 15;  t.B = 15;  sc_start(10, SC_NS);
    t.A = 255; t.B = 255; sc_start(10, SC_NS);

    sc_close_vcd_trace_file(tf);
    return 0;
}
