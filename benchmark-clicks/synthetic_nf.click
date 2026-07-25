fd :: FromDPDKDevice(0000:ca:00.0, BURST 32, NDESC 4096,  VERBOSE 3, PROMISC 1,  CLEAR 1);
nf :: SyntheticNF(OPS 100, NREAD 50, TABLE_SIZE 1024);
fc :: SourceCounter(CAPACITY 4096, VERBOSE 1);
td :: ToDPDKDevice(0000:ca:00.0, BURST 32, IQUEUE 8192);

fd -> nf -> fc -> td;

