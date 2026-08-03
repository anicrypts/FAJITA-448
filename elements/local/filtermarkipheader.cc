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
#include <clicknet/ether.h>
#include <clicknet/ip.h>

#define FILTER_MARK_IP_HEADER_DEBUG 0

CLICK_DECLS

FilterMarkIPHeader::FilterMarkIPHeader() : _ip_offset(0)
{
    _pkt_count = 0;
    _kill_pkt_count = 0;
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
    const click_ether *ethh = (const click_ether *)p->mac_header();
    uint16_t ether_type = ntohs(ethh->ether_type);

    uint8_t dhost_0 = ethh->ether_dhost[0];
    if (dhost_0 == 0xff) {
        p->kill();
	 return p;
     }

    if (likely(ether_type == 0x0800)) {
	// Set ip header
	 const click_ip *ip = reinterpret_cast<const click_ip *>(p->data() + _ip_offset);
         p->set_ip_header(ip, ip->ip_hl << 2);
         uint8_t transport_protocol = ip->ip_p;
	 if (likely(transport_protocol == IP_PROTO_UDP)) {
 	   // TODO:
	 } else {

	 } 
    } else {
	p->kill();
	return p;
    }
    return p;


/*    const click_ip *ip = reinterpret_cast<const click_ip *>(p->data() + _ip_offset);

    p->set_ip_header(ip, ip->ip_hl << 2); // This also sets the transport header pointer
    IPAddress src_ip = IPAddress(ip->ip_src);
    IPAddress dst_ip = IPAddress(ip->ip_dst);
    if (src_ip != _expected_src_ip || dst_ip != _expected_dst_ip) {
        // click_chatter("Dropping packet with unexpected IP addr(s). src %s dst %s", src_ip.unparse().c_str(), dst_ip.unparse().c_str());
        p->kill();
        return 0;
    }
    
    const click_udp *udp = p->udp_header();
    uint16_t udp_sport = ntohs(udp->uh_sport);
    uint16_t udp_dport = ntohs(udp->uh_dport);
    if (udp_sport < 1024 || udp_dport < 1024) {
        // click_chatter("Dropping packet with unexpected UDP port(s). src %d dst %d", udp_sport, udp_dport);
        p->kill();
        return 0;
    }
    // click_chatter("Allowing packet with expected UDP port(s). src %d dst %d", udp_sport, udp_dport);
 
    
    return p;*/
}


int FilterMarkIPHeader::mark_packets(Packet *p)
{
    const click_ether *ethh = (const click_ether *)p->mac_header();
    uint16_t ether_type = ntohs(ethh->ether_type);

    _pkt_count += 1;
    if (FILTER_MARK_IP_HEADER_DEBUG > 0 && _pkt_count % 1000000 == 0) {
        printf("Pkt count %d kill pkt count %d\n", _pkt_count, _kill_pkt_count);
    }

    uint8_t dhost_0 = ethh->ether_dhost[0];
    if (dhost_0 == 0xff) { // broadcast traffic
        return kill_and_update(p);
    }

    if (unlikely(ether_type != ETHERTYPE_IP)) {
        return kill_and_update(p);
    }

	const click_ip *ip = reinterpret_cast<const click_ip *>(p->data() + _ip_offset);
    p->set_ip_header(ip, ip->ip_hl << 2);
    uint8_t transport_protocol = ip->ip_p;
    if (unlikely(transport_protocol != IP_PROTO_UDP)) {
        return kill_and_update(p);
    }

    // Check IP src, dst
    IPAddress src_ip = IPAddress(ip->ip_src);
    IPAddress dst_ip = IPAddress(ip->ip_dst);
    if (unlikely(src_ip != _expected_src_ip || dst_ip != _expected_dst_ip)) {
        return kill_and_update(p);
    }

    // Check UDP portnos
    const click_udp *udp = p->udp_header();
    uint16_t udp_sport = ntohs(udp->uh_sport);
    uint16_t udp_dport = ntohs(udp->uh_dport);
    if (unlikely(udp_sport < 1024 || udp_dport < 1024)) {
        return kill_and_update(p);
    }

    return 0;
}

void
FilterMarkIPHeader::push_batch(int port, PacketBatch* batch) {
    CLASSIFY_EACH_PACKET_IGNORE(1, mark_packets, batch, checked_output_push_batch);
}

CLICK_ENDDECLS
EXPORT_ELEMENT(FilterMarkIPHeader)
ELEMENT_MT_SAFE(FilterMarkIPHeader)
