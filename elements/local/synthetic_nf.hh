#ifndef CLICK_SYNTHETICNF_HH
#define CLICK_SYNTHETICNF_HH
#include <clicknet/ether.h>
#include <click/config.h>
#include <click/element.hh>
#include <click/batchelement.hh>

CLICK_DECLS

struct SyntheticNFState {
    atomic_uint32_t count;
};

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

        struct local_flowID {
            uint32_t ip_src;
        };

        // Configurable parameters
        uint64_t _ops;
	    uint64_t _nread_ratio; // An int in the range [0,100]
        uint32_t _capacity;

        // State-saving members for synthetic processing loads
        uint64_t _accumulator;
        volatile uint8_t _sink;

        // Hash table
        void *_table;
        SyntheticNFState *_states;
        atomic_uint32_t _insertions;

        // Offset into a raw Ethernet/IPv4 frame for UDP src,dst portno
        // (14B Ethernet header + 20B IPv4 header)
        static const uint32_t FLOW_ID_OFFSET = 34;

        void _update_flow_table(Packet *p);

};

CLICK_ENDDECLS
#endif
