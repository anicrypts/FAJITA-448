#ifndef CLICK_SYNTHETICNF_HH
#define CLICK_SYNTHETICNF_HH
#include <click/batchelement.hh>
CLICK_DECLS

class SyntheticNF : public Element {
    public:

        SyntheticNF() CLICK_COLD;
        ~SyntheticNF() CLICK_COLD;

        const char *class_name() const override { return "SyntheticNF"; }
        const char *port_count() const override { return PORTS_1_1; }

        int configure(Vector<String> &, ErrorHandler *);
        void push(int, Packet *);

    private:
        unsigned int _ops;
        uint64_t _accumulator;

}

CLICK_ENDDECLS
#endif