define ($RSS 4)


fd :: FromDPDKDevice(0000:ca:00.0, BURST 32, NDESC 4096, VERBOSE 3, PROMISC 0, CLEAR 0,   MINQUEUES $RSS, MAXQUEUES $RSS);
mark_ip :: FilterMarkIPHeader(IP_OFFSET 14)
fm :: FlowIPManager_DPDK(CAPACITY 16384, TIMEOUT 1, NCHECK 100);
nf :: FlowIPLoadBalancer(DST 10.0.0.1, VIP  10.0.0.2);
td :: ToDPDKDevice(0000:ca:00.0, BURST 32, IQUEUE 8192);

fd -> mark_ip -> fm -> nf -> EtherMirror() ->  td;
