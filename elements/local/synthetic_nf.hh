#ifndef CLICK_SYNTHETICNF_HH
#define CLICK_SYNTHETICNF_HH
#include <clicknet/ether.h>
#include <click/config.h>
#include <click/element.hh>
#include <click/flow/flowelement.hh>
#include <click/batchelement.hh>
#include <rte_hash.h>

CLICK_DECLS

// Per-flow FCB state managed by FlowStateElement.
// Stores the hash table position so release_flow() can delete the entry
// without doing a second lookup.
struct SyntheticNFFlowState {
    int hash_idx;  // position returned by rte_hash_add_key; -1 if not inserted
};

// Per-position counter stored in the flat state array.
struct SyntheticNFState {
    atomic_uint32_t count;
};

class SyntheticNF : public FlowStateElement<SyntheticNF, SyntheticNFFlowState>
{
    public:

        SyntheticNF() CLICK_COLD;
        ~SyntheticNF() CLICK_COLD;

        const char *class_name() const override { return "SyntheticNF"; }
        const char *port_count() const override { return PORTS_1_1; }
        const char *processing() const override { return PUSH; }

        int configure(Vector<String> &, ErrorHandler *);

        // FlowStateElement interface
        const static int timeout = 15000;
        bool new_flow(SyntheticNFFlowState *state, Packet *p);
        void release_flow(SyntheticNFFlowState *state);
        void push_flow(int port, SyntheticNFFlowState *state, PacketBatch *batch);
    
    private:

        struct local_flowID {
            uint32_t udp_ports;
        };

        // Configurable parameters
        uint64_t _ops;
	    int _nread_ratio; // An int in the range [0,100]
        uint32_t _capacity;

        // State-saving members for synthetic processing loads
        uint64_t _accumulator;
        volatile uint8_t _sink;

        // Hash table
        void *_table;
        SyntheticNFState *_states;

        // Offset into a raw Ethernet/IPv4 frame for UDP src,dst portno
        // (14B Ethernet header + 20B IPv4 header)
        static const uint32_t FLOW_ID_OFFSET = 34;

        // Per-packet processing shared by push_flow
        void _process_packet(Packet *p, SyntheticNFFlowState *state);

};

CLICK_ENDDECLS
#endif
