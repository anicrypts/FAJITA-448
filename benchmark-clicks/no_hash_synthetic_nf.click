define ($STATSFILE /home/anikajh/stats/test-stats.txt)
define ($OPS 100000)
define ($STRIDE 16)


fd :: FromDPDKDevice(0000:ca:00.0, BURST 32, NDESC 4096, VERBOSE 3, PROMISC 0, CLEAR 1);
filter_ip ::FilterMarkIPHeader(IP_OFFSET 14)
nf :: SyntheticNF(OPS $OPS, STRIDE $STRIDE);
td :: ToDPDKDevice(0000:ca:00.0, BURST 32);

fd -> filter_ip -> nf -> td;

StatsOnSigterm :: Script(TYPE SIGNAL INT,
    print  >$STATSFILE  "ipackets=$(fd.hw_count)",
    print >>$STATSFILE "opackets=$(td.hw_count)",
    print >>$STATSFILE "ibytes=$(fd.hw_bytes)",
    print >>$STATSFILE "obytes=$(td.hw_bytes)",
    print >>$STATSFILE "imissed=$(fd.hw_dropped)",
    print >>$STATSFILE "ierrors=$(fd.hw_errors)",
    print >>$STATSFILE "oerrors=$(td.hw_errors)",
    print >>$STATSFILE "rx_nombuf=$(fd.nombufs)",
    stop);
