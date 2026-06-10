fd :: FromDPDKDevice(0000:ca:00.0, BURST 32, NDESC 4096,  VERBOSE 3, PROMISC 1,  CLEAR 1);
nf :: SyntheticNF(OPS 100, NREAD 50, TABLE_SIZE 1024);
td :: ToDPDKDevice(0000:ca:00.0, BURST 32, IQUEUE 8192);

fd -> nf -> td;

ControlSocket(tcp, 1024);

// Log stats once per second to a file in CSV format
Script(
    TYPE ACTIVE,
    // CSV header
    print > /home/anikajh/stats/logs1.txt "timestamp,rx_packets,rx_bytes,rx_dropped,rx_errors,rx_nombuf,tx_packets,tx_bytes,tx_errors\n",
    set start $(now),

    label loop,
    // Compute elapsed time
    set t $(sub $(now) $start),
    // Read all stats into variables
    set rxp $(fd.hw_count),
    set rxb $(fd.hw_bytes),
    set rxd $(fd.hw_dropped),
    set rxe $(fd.hw_errors),
    set rxn $(fd.nombufs),
    set txp $(td.hw_count),
    set txb $(td.hw_bytes),
    set txe $(td.hw_errors),
    // Append one CSV row
    print >> /home/anikajh/stats/logs1.txt "$t,$rxp,$rxb,$rxd,$rxe,$rxn,$txp,$txb,$txe\n",
    wait 1s,
    goto loop
);
