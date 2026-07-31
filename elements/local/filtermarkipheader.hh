#ifndef CLICK_FILTERMARKIPHEADER_HH
#define CLICK_FILTERMARKIPHEADER_HH
#include <click/batchelement.hh>
#include <click/ipaddress.hh>
CLICK_DECLS

/*
 * =c
 * FilterMarkIPHeader([OFFSET])
 * =s ip
 * sets IP header annotation
 * =d
 *
 * Marks packets as IP packets by setting the IP Header annotation. The IP
 * header starts OFFSET bytes into the packet. Default OFFSET is 0.
 *
 * Does not check length fields for sanity, shorten packets to the IP length,
 * or set the destination IP address annotation. Use CheckIPHeader or
 * CheckIPHeader2 for that.
 *
 * =a CheckIPHeader, CheckIPHeader2, StripIPHeader */

class FilterMarkIPHeader : public SimpleElement<FilterMarkIPHeader> {
    public:
        FilterMarkIPHeader () CLICK_COLD;
        ~FilterMarkIPHeader() CLICK_COLD;

        const char *class_name() const override { return "FilterMarkIPHeader"; }
        const char *port_count() const override { return PORTS_1_1; }
        int configure(Vector<String> &, ErrorHandler *) CLICK_COLD;

        Packet *simple_action(Packet *p);

    private:
        int _ip_offset;
        IPAddress _expected_src_ip = IPAddress(String("192.168.0.1"));
        IPAddress _expected_dst_ip = IPAddress(String("192.168.0.2"));
};

CLICK_ENDDECLS
#endif
