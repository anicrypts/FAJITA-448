// - swaps Ethernet source/destination MACs
// - performs OPS dummy operations per packet (tunable)
// - forwards packet out the same interface

#include <click/config.h>
#include <click/element.hh>
#include <click/packet.hh>
#include <click/args.hh>
#include <click/etheraddress.hh>
#include <click/ether.hh>
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
    return 0;
}

void SyntheticNF::push(int port, Packet *p) {
    // Ensure packet is writable
    WritablePacket *q = p->uniqueify();
    if (!q) {
        // drop if cannot make writable
        p->kill();
        return;
    }

    // Basic sanity check: must be at least Ethernet header size
    if (q->length() < (int)sizeof(click_ether)) {
        output(0).push(q);
        return;
    }

    // Swap MAC addresses in-place
    click_ether *eth = reinterpret_cast<click_ether *>(q->data());
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

    // Forward packet out the same interface (output port 0)
    output(0).push(q);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(SyntheticNF)
