/*
 * markipheader.{cc,hh} -- element sets IP Header annotation
 * Eddie Kohler
 *
 * Computational batching support
 * by Georgios Katsikas
 *
 * Copyright (c) 1999-2000 Massachusetts Institute of Technology
 * Copyright (c) 2016 KTH Royal Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, subject to the conditions
 * listed in the Click LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the Click LICENSE file; the license in that file is
 * legally binding.
 */

#include <click/config.h>
#include "filtermarkipheader.hh"
#include <click/args.hh>
#include <clicknet/ip.h>
CLICK_DECLS

FilterMarkIPHeader::FilterMarkIPHeader() : _ip_offset(0)
{
}

FilterMarkIPHeader::~FilterMarkIPHeader()
{
}

int
FilterMarkIPHeader::configure(Vector<String> &conf, ErrorHandler *errh)
{
    return Args(conf, this, errh)
        .read_p("IP_OFFSET", _ip_offset)
        .complete();
}

Packet *
FilterMarkIPHeader::simple_action(Packet *p)
{
    const click_ip *ip = reinterpret_cast<const click_ip *>(p->data() + _ip_offset);
    if (ip->ip_v == 0b0110) {
        // click_chatter("Dropping IPv6 packet");
        p->kill();
        return 0;
    }

    p->set_ip_header(ip, ip->ip_hl << 2); // This also sets the transport header pointer
    IPAddress src_ip = IPAddress(ip->ip_src);
    IPAddress dst_ip = IPAddress(ip->ip_dst);
    if (src_ip != _expected_src_ip || dst_ip != _expected_dst_ip) {
        // click_chatter("Dropping packet with unexpected IP addr(s). src %s dst %s", src_ip.unparse().c_str(), dst_ip.unparse().c_str());
        p->kill();
        return 0;
    }
    
    // p->set_transport_header(p->data() + _ip_offset + (ip->ip_hl << 2));
    const click_udp *udp = p->udp_header();
    uint16_t udp_sport = ntohs(udp->uh_sport);
    uint16_t udp_dport = ntohs(udp->uh_dport);
    if (udp_sport < 1024 || udp_dport < 1024) {
        // click_chatter("Dropping packet with unexpected UDP port(s). src %d dst %d", udp_sport, udp_dport);
        p->kill();
        return 0;
    }
    // click_chatter("Allowing packet with expected UDP port(s). src %d dst %d", udp_sport, udp_dport);
    return p;
}

CLICK_ENDDECLS
EXPORT_ELEMENT(FilterMarkIPHeader)
ELEMENT_MT_SAFE(FilterMarkIPHeader)
