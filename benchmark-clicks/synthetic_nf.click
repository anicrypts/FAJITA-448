fd :: FromDPDKDevice(0000:ca:00.0, BURST 32, NDESC 4096, VERBOSE 3, PROMISC 0, CLEAR 1);
mark_ip :: MarkIPHeader(OFFSET 14)
fm :: FlowIPManager_DPDK(CAPACITY 65536, TIMEOUT 10);
nf :: SyntheticNF(OPS 100, STRIDE 32);
td :: ToDPDKDevice(0000:ca:00.0, BURST 32, IQUEUE 8192);

fd -> mark_ip -> fm -> nf -> td;
