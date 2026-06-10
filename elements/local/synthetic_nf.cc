/* 
 * syntheticnf.{cc,hh} -- synthetic network function for benchmarking
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
    _table(nullptr), _states(nullptr)
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

    printf("SyntheticNF: configured ops=%lu nread_ratio=%u table_size=%u\n",
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
    _states = (SyntheticNFState*) CLICK_ALIGNED_ALLOC(stateAlignedSize * _capacity);
    CLICK_ASSERT_ALIGNED(_states);
    bzero(_states, stateAlignedSize * _capacity);
    if (!_states)
        return errh->error("SyntheticNF: could not init state array for "
                           "table %s!", name().c_str());
    return 0;
}

bool SyntheticNF::new_flow(SyntheticNFFlowState *state, Packet *p)
{
    // Initialise to sentinel so release_flow() is safe even if we return false
    state->hash_idx = -1;

    if (!_table)
        return true; // table disabled, nothing to insert

    auto *table = reinterpret_cast<rte_hash *>(_table);
    local_flowID *ifid = (local_flowID *)(p->data() + FLOW_ID_OFFSET);

    int idx = rte_hash_add_key(table, ifid);
    if (unlikely(idx < 0)) {
        click_chatter("SyntheticNF: problem inserting key! %d", idx);
        return false;
    }

    state->hash_idx = idx;
    return true;
}

/* -----------------------------------------------------------------------
 * FlowStateElement: release_flow
 * Called by the framework after TIMEOUT ms of inactivity.
 * Deletes the flow's key from the rte_hash using the position stored in the
 * FCB state — no second lookup needed.
 * --------------------------------------------------------------------- */

void SyntheticNF::release_flow(SyntheticNFFlowState *state)
{
    if (!_table || state->hash_idx < 0)
        return;

    auto *table = reinterpret_cast<rte_hash *>(_table);

    // Retrieve the key by position so we can call rte_hash_del_key
    const void *key = nullptr;
    if (rte_hash_get_key_with_position(table, state->hash_idx, &key) == 0) {
        rte_hash_del_key(table, key);
    }

    // Reset the counter slot so it is clean if the position is reused
    _states[state->hash_idx].count = 0;
    state->hash_idx = -1;
}

/* -----------------------------------------------------------------------
 * FlowStateElement: push_flow
 * Called for every packet batch belonging to an active flow.
 * Performs the synthetic load and increments the per-flow counter.
 * --------------------------------------------------------------------- */

void SyntheticNF::push_flow(int, SyntheticNFFlowState *state, PacketBatch *batch)
{
    FOR_EACH_PACKET_SAFE(batch, p) {
        _process_packet(p, state);
    }
    output_push_batch(0, batch);
}


// Per-packet work, called form push_flow for every packet
void SyntheticNF::_process_packet(Packet *p, SyntheticNFFlowState *state)
{
#ifdef DEBUG
    printf("SyntheticNF: executing _process_packet\n");
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

    // Increment per-flow counter in the flat state array
    if (_table && state->hash_pos >= 0)
        _states[state->hash_pos].count++;

    // Swap MAC addresses in-place
    click_ether *ethh = reinterpret_cast<click_ether *>(q->data());
    uint8_t tmp_mac[6];
    memcpy(tmp_mac, ethh->ether_dhost, 6);
    memcpy(ethh->ether_dhost, ethh->ether_shost, 6);
    memcpy(ethh->ether_shost, tmp_mac, 6);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(SyntheticNF)
