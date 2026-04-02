#ifndef CLICK_SYNTHETICNF_HH
#define CLICK_SYNTHETICNF_HH
#include <clicknet/ether.h>
#include <click/config.h>
#include <click/element.hh>
#include <click/batchelement.hh>

CLICK_DECLS

class SyntheticNF : public BatchElement {
    public:

        SyntheticNF() CLICK_COLD;
        ~SyntheticNF() CLICK_COLD;

        const char *class_name() const override { return "SyntheticNF"; }
        const char *port_count() const override { return PORTS_1_1; }

        int configure(Vector<String> &, ErrorHandler *);
        Packet * simple_action(Packet *);

    #if HAVE_BATCH
        PacketBatch *simple_action_batch(PacketBatch *);
    #endif
    
    private:
        unsigned int _ops;
        uint64_t _accumulator;
};

CLICK_ENDDECLS
#endif
