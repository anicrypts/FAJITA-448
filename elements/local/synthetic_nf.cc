/* 
 * syntheticnf.{cc,hh}
 */

#include <click/config.h>
#include <click/element.hh>
#include <click/packet.hh>
#include <click/args.hh>
#include <click/etheraddress.hh>
#include <clicknet/ether.h>
#include <click/error.hh>
#include "synthetic_nf.hh"

CLICK_DECLS

SyntheticNF::SyntheticNF() : _ops(0), _accumulator(0)
{
}

SyntheticNF::~SyntheticNF()
{
}

int SyntheticNF::configure(Vector<String> &conf, ErrorHandler *errh) {
    if (Args(conf, this, errh)
            .read("OPS", _ops)
            .complete() < 0)
        return -1;
    printf("SyntheticNF: configured ops to %d\n", _ops);
    return 0;
}

Packet * SyntheticNF::simple_action(Packet *p) {
    //printf("SyntheticNF: executing simple_action\n");
    // Ensure packet is writable
    WritablePacket *q = p->uniqueify();
    if (!q) {
        // drop if cannot make writable
        p->kill();
        return 0;
    }

    // Basic sanity check: must be at least Ethernet header size
    if (q->length() < (int)sizeof(click_ether)) {
       // output(0).push(q);
        return 0;
    }

    // Swap MAC addresses in-place
    click_ether *ethh = reinterpret_cast<click_ether *>(q->data());
    uint8_t tmp_mac[6];
    memcpy(tmp_mac, ethh->ether_dhost, 6);
    memcpy(ethh->ether_dhost, ethh->ether_shost, 6);
    memcpy(ethh->ether_shost, tmp_mac, 6);

    // Perform OPS dummy operations to emulate processing load
    // Use an accumulator member to prevent compiler optimizing the loop away
    uint64_t local_acc = _accumulator;
    for (unsigned int i = 0; i < _ops; ++i) {
        // simple arithmetic and bit-mix using packet pointer to vary work
        local_acc += (uint64_t)i ^ (uint64_t)(uintptr_t)q;
        local_acc = (local_acc << 1) | (local_acc >> 63);
        local_acc ^= 0x9e3779b97f4a7c15ULL;
    }
    _accumulator = local_acc;
    
    return q;
}


#if HAVE_BATCH
PacketBatch *
SyntheticNF::simple_action_batch(PacketBatch *batch)
{
#ifdef CLICK_NOINDIRECT
    FOR_EACH_PACKET(batch, p)   {
        SyntheticNF::simple_action(p);
    }
#else
    EXECUTE_FOR_EACH_PACKET_DROPPABLE(SyntheticNF::simple_action, batch, [](Packet*){});
#endif
    return batch;
}
#endif


CLICK_ENDDECLS
EXPORT_ELEMENT(SyntheticNF)
