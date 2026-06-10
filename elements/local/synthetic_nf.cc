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

//#define DEBUG

CLICK_DECLS

SyntheticNF::SyntheticNF()
    : _ops(0), _nread_ratio(0), _capacity(0), _accumulator(0), _sink(0),
    _table(nullptr), _local_fcbs_struct(nullptr)
{
}

SyntheticNF::~SyntheticNF()
{
}

int SyntheticNF::configure(Vector<String> &conf, ErrorHandler *errh) {
    if (Args(conf, this, errh)
        .read("OPS", _ops)
	    .read("NREAD", _nread_ratio)
        .read_or_set("TABLE_SIZE", _capacity, 0)
        .complete() < 0)
        return -1;

    printf("SyntheticNF: configured ops=%d nread_ratio=%d table_size=%u\n",
           _ops, _nread_ratio, _capacity);

    if (_capacity == 0)
        return 0; // hash table disabled

    // Build hash table if enabled
    struct rte_hash_parameters hash_params = {0};
    char buf[64];
    sprintf(buf, "%d-%s", click_random(), name().c_str());
    hash_params.name = buf;
    hash_params.entries = _capacity;
    hash_params.key_len = sizeof(local_flowID);
    hash_params.hash_func = ipv4_hash_crc_src_ip;
    hash_params.hash_func_init_val = 0;
    hash_params.extra_flag = RTE_HASH_EXTRA_FLAGS_RW_CONCURRENCY
                           | RTE_HASH_EXTRA_FLAGS_MULTI_WRITER_ADD;
    
    _table = rte_hash_create(&hash_params);
    if (!_table)
        return errh->error("SyntheticNF: could not init flow table %s: "
                           "error %d (%s)",
                           name().c_str(), rte_errno, rte_strerror(rte_errno));

    // Allocate flat state array
    size_t stateAlignedSize = (sizeof(SyntheticNFState) + 63) & ~63;
    _local_fcbs_struct = (SourceCounterState*) CLICK_ALIGNED_ALLOC(stateAlignedSize * _capacity);
    CLICK_ASSERT_ALIGNED(_local_fcbs_struct);
    bzero(_local_fcbs_struct, stateAlignedSize * _capacity);
    if (!_local_fcbs_struct)
        return errh->error("SyntheticNF: could not init state array for "
                           "table %s!", name().c_str());
    return 0;
}


void SyntheticNF::_update_flow_table(Packet *p)
{
    auto *table = reinterpret_cast<rte_hash *>(_table);

    // Read the 32-bits of UDP src port, dst port
    local_flowID *ifid = (local_flowID *)(p->data() + FLOW_ID_OFFSET);

    int idx = rte_hash_lookup(table, ifid);
    if (idx < 0) {
        // New flow
        idx = rte_hash_add_key(table, ifid);
        if (unlikely(idx < 0)) {
            click_chatter("SyntheticNF: problem inserting data! %d", idx);
            return;
        }
    }

    _states[pos].count++;
}

Packet * SyntheticNF::simple_action(Packet *p) {

#ifdef DEBUG    
	printf("SyntheticNF: executing simple_action\n");
#endif
    // Ensure packet is writable
    WritablePacket *q = p->uniqueify();
    if (!q) {
        // drop if cannot make writable
        p->kill();
        return 0;
    }

    // Basic sanity check: must be at least Ethernet header size
    if (q->length() < (int)sizeof(click_ether)) {
        return 0;
    }

    // Read a fraction of the bytes of the received packet
    float ratio = _nread_ratio / 100.0;
    unsigned int nread = q->length() * ratio;

#ifdef DEBUG
    printf("reading %d bytes of a %d-byte packet\n", nread, q->length());
#endif

    // Accumulate packet data into a volatile sink so the compiler cannot prove the result is unused
    volatile uint8_t sink = 0;
    const volatile uint8_t *data = reinterpret_cast<const volatile uint8_t *>(q->data());
    unsigned int i = 0;
    for (; i < nread; ++i) {
        sink ^= data[i];
    }
    _sink = sink;
    
    // Dummy compute load
    uint64_t local_acc = _accumulator;
    for (i = 0; i < _ops; ++i) {
        local_acc += (uint64_t)i ^ (uint64_t)(uintptr_t)q;
    }
    _accumulator = local_acc;

    // Hash (flow) table update
    if (_table)
        _update_flow_table(q);

    // Swap MAC addresses in-place
    click_ether *ethh = reinterpret_cast<click_ether *>(q->data());
    uint8_t tmp_mac[6];
    memcpy(tmp_mac, ethh->ether_dhost, 6);
    memcpy(ethh->ether_dhost, ethh->ether_shost, 6);
    memcpy(ethh->ether_shost, tmp_mac, 6);

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
