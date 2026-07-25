#ifndef CLICK_HASHELEM_HH
#define CLICK_HASHELEM_HH
#include <click/element.hh>
#include <click/vector.hh>
#include <click/multithread.hh>
#include <click/flow/flowelement.hh>
#include <click/atomic.hh>
#include <rte_hash.h>
#include <rte_jhash.h>

CLICK_DECLS

/**
 * HashElem - based on SourceCounter
 * - Uses 4-byte UDP src,dst tuple instead of IPv4 src as hash key
 * - Performs hash table lookup and write on push, not new_flow() (i.e., follows the
 *   no-cache behaviour of SourceCounter)
 */

// alignas(CLICK_CACHE_LINE_SIZE)
struct HashElemState {
    atomic_uint32_t count;
};

class HashElem : public FlowStateElement<HashElem,int>
{
public:
    HashElem() CLICK_COLD;

    const char *class_name() const override        { return "HashElem"; }
    const char *port_count() const override        { return PORTS_1_1; }
    const char *processing() const override        { return PUSH; }

    int configure(Vector<String> &, ErrorHandler *) override CLICK_COLD;

    // 14B Ethernet header + 20B IPv4 header
    const static uint32_t _offset = 34;

    /* FlowStateElement interface: timeout, release_flow(), push_flow(), new_flow() */
    void release_flow(int* fcb) {
    }

    const static int timeout = 10000;

    void push_flow(int port, int* fcb, PacketBatch*);

#if FLOW_PUSH_BATCH
    inline void push_flow_batch(int port, int** fcb, PacketBatch *head);
#endif

    inline bool new_flow(int* state, Packet* p) {
#ifdef DEBUG
        printf("HashElem: new_flow\n");
#endif
        return true;
    }

    void add_handlers() override CLICK_COLD;
protected:

    struct local_flowID {
        uint32_t udp_ports;
    };

    static String read_handler(Element *, void *) CLICK_COLD;
    static int write_handler(const String &, Element *, void *, ErrorHandler *) CLICK_COLD;

    uint32_t _capacity;
    HashElemState *_local_fcbs_struct;
    atomic_uint32_t _insertions;
    uint32_t _verbose;

    void *_table;
};

CLICK_ENDDECLS
#endif
