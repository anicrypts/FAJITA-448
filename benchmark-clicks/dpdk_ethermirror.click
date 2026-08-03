fd :: FromDPDKDevice(0000:03:00.0, BURST 32, NDESC 4096,  VERBOSE 3, PROMISC 0,  CLEAR 1);
td :: ToDPDKDevice(0000:03:00.0, BURST 32, IQUEUE 8192);

fd -> EtherMirror() -> td;
