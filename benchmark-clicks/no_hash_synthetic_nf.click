define ($STATSFILE /home/anikajh/stats/test-stats.txt)
define ($OPS 100000)
define ($STRIDE 16)
define ($STATS_TIMEOUT 10)

fd :: FromDPDKDevice(0000:ca:00.0, BURST 32, NDESC 4096, VERBOSE 3, PROMISC 0, CLEAR 1, STATSFILE $STATSFILE, STATS_TIMEOUT $STATS_TIMEOUT);
filter_ip ::FilterMarkIPHeader(IP_OFFSET 14)
nf :: SyntheticNF(OPS $OPS, STRIDE $STRIDE);
td :: ToDPDKDevice(0000:ca:00.0, BURST 32);

fd -> filter_ip -> nf -> td;
