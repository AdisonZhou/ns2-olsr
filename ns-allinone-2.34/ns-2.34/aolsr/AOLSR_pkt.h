/***************************************************************************
 *   Copyright (C) 2004 by Francisco J. Ros                                *
 *   fjrm@dif.um.es                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

 ///
 /// \file	AOLSR_pkt.h
 /// \brief	This file contains all declarations of %AOLSR packets and messages,
 ///		including all related contants and macros.
 ///

#ifndef __AOLSR_pkt_h__
#define __AOLSR_pkt_h__

#include <packet.h>

/********** Message types **********/

/// %AOLSR HELLO message type.
#define AOLSR_HELLO_MSG		1
/// %AOLSR TC message type.
#define AOLSR_TC_MSG		2
/// %AOLSR MID message type.
#define AOLSR_MID_MSG		3

/********** Packets stuff **********/

/// Accessor to the %AOLSR packet.
#define PKT_AOLSR(p)		AOLSR_pkt::access(p)

///
/// \brief Size of the addresses we are using.
///
/// You should always use this macro when interested in calculate addresses'
/// sizes. By default IPv4 addresses are supposed, but you can change to
/// IPv6 ones by recompiling with AOLSR_IPv6 constant defined.
///
#ifdef AOLSR_IPv6
#define ADDR_SIZE	16
#else
#define ADDR_SIZE	4
#endif

/// Maximum number of messages per packet.
#define AOLSR_MAX_MSGS		4

/// Maximum number of hellos per message (4 possible link types * 3 possible nb types).
#define AOLSR_MAX_HELLOS		12

/// Maximum number of addresses advertised on a message.
#define AOLSR_MAX_ADDRS		64

/// Size (in bytes) of packet header.
#define AOLSR_PKT_HDR_SIZE	4

/// Size (in bytes) of message header.
#define AOLSR_MSG_HDR_SIZE	12

/// Size (in bytes) of hello header.
#define AOLSR_HELLO_HDR_SIZE	4

/// Size (in bytes) of hello_msg header.
#define AOLSR_HELLO_MSG_HDR_SIZE	4

/// Size (in bytes) of tc header.
#define AOLSR_TC_HDR_SIZE	4

/// Auxiliary struct which is part of the %AOLSR HELLO message (struct AOLSR_hello).
typedef struct AOLSR_hello_msg {

	/// Link code.
	u_int8_t	link_code_;
	/// Reserved.
	u_int8_t	reserved_;
	/// Size of this link message.
	u_int16_t	link_msg_size_;
	/// List of interface addresses of neighbor nodes.
	nsaddr_t	nb_iface_addrs_[AOLSR_MAX_ADDRS];
	/// Number of interface addresses contained in nb_iface_addrs_.
	int		count;
	/// 添加邻居的HELLO消息间隔，以及接收到的邻居数据包的能量。

	//u_int8_t link_score_[AOLSR_MAX_ADDRS]; ///added 周家家
	//u_int8_t reserved2_[AOLSR_MAX_ADDRS];/// added 周家家
	//u_int16_t reserved3_[AOLSR_MAX_ADDRS];/// added 周家家


	inline u_int8_t&	link_code() { return link_code_; }
	inline u_int8_t&	reserved() { return reserved_; }
	inline u_int16_t&	link_msg_size() { return link_msg_size_; }
	inline nsaddr_t&	nb_iface_addr(int i) { return nb_iface_addrs_[i]; }
	//inline u_int8_t&	link_score(int i) { return link_score_[i]; } ///added 周家家
	//inline u_int8_t&	reserved2(int i) { return reserved2_[i]; } ///added 周家家
	//inline u_int16_t&	reserved3(int i) { return reserved3_[i]; } ///added 周家家


	inline u_int32_t size() { return AOLSR_HELLO_MSG_HDR_SIZE + count * ADDR_SIZE; }/// changed 周家家

} AOLSR_hello_msg;

/// %AOLSR HELLO message.
typedef struct AOLSR_hello {

	/// Reserved.
	u_int16_t	reserved_;
	/// HELLO emission interval in mantissa/exponent format. hello 消息间隔
	u_int8_t	htime_;
	/// Willingness of a node for forwarding packets on behalf of other nodes.
	u_int8_t	willingness_;
	/// List of AOLSR_hello_msg.
	AOLSR_hello_msg	hello_body_[AOLSR_MAX_HELLOS];
	/// Number of AOLSR_hello_msg contained in hello_body_.
	int		count;

	inline u_int16_t&	reserved() { return reserved_; }
	inline u_int8_t&	htime() { return htime_; }
	inline u_int8_t&	willingness() { return willingness_; }
	inline AOLSR_hello_msg&	hello_msg(int i) { return hello_body_[i]; }

	inline u_int32_t size() {
		u_int32_t sz = AOLSR_HELLO_HDR_SIZE;
		for (int i = 0; i < count; i++)
			sz += hello_msg(i).size();
		return sz;
	}

} AOLSR_hello;

/// %AOLSR TC message.
typedef struct AOLSR_tc {

	/// Advertised Neighbor Sequence Number.
	u_int16_t	ansn_;
	/// Reserved.
	u_int16_t	reserved_;
	/// List of neighbors' main addresses.
	nsaddr_t	nb_main_addrs_[AOLSR_MAX_ADDRS];
	/// Number of neighbors' main addresses contained in nb_main_addrs_.

	//u_int8_t link_score_[AOLSR_MAX_ADDRS]; ///added 周家家
	//u_int8_t reserved2_[AOLSR_MAX_ADDRS];/// added 周家家
	//u_int16_t reserved3_[AOLSR_MAX_ADDRS];/// added 周家家

	int		count;

	inline	u_int16_t&	ansn() { return ansn_; }
	inline	u_int16_t&	reserved() { return reserved_; }
	inline	nsaddr_t&	nb_main_addr(int i) { return nb_main_addrs_[i]; }

	//inline u_int8_t&	link_score(int i) { return link_score_[i]; } ///added 周家家
	///inline u_int8_t&	reserved2(int i) { return reserved2_[i]; } ///added 周家家
	//inline u_int16_t&	reserved3(int i) { return reserved3_[i]; } ///added 周家家

	inline	u_int32_t size() { return AOLSR_TC_HDR_SIZE + count * ADDR_SIZE; }

} AOLSR_tc;

/// %AOLSR MID message.
typedef struct AOLSR_mid {

	/// List of interface addresses.
	nsaddr_t	iface_addrs_[AOLSR_MAX_ADDRS];
	/// Number of interface addresses contained in iface_addrs_.
	int		count;

	inline nsaddr_t&	iface_addr(int i) { return iface_addrs_[i]; }

	inline u_int32_t	size() { return count * ADDR_SIZE; }

} AOLSR_mid;

/// %AOLSR message.
typedef struct AOLSR_msg {

	u_int8_t	msg_type_;	///< Message type.
	u_int8_t	vtime_;		///< Validity time.
	u_int16_t	msg_size_;	///< Message size (in bytes).
	nsaddr_t	orig_addr_;	///< Main address of the node which generated this message.
	u_int8_t	ttl_;		///< Time to live (in hops).
	u_int8_t	hop_count_;	///< Number of hops which the message has taken.
	u_int16_t	msg_seq_num_;	///< Message sequence number.
	union {
		AOLSR_hello	hello_;
		AOLSR_tc		tc_;
		AOLSR_mid	mid_;
	} msg_body_;			///< Message body.

	inline	u_int8_t&	msg_type() { return msg_type_; }
	inline	u_int8_t&	vtime() { return vtime_; }
	inline	u_int16_t&	msg_size() { return msg_size_; }
	inline	nsaddr_t&	orig_addr() { return orig_addr_; }
	inline	u_int8_t&	ttl() { return ttl_; }
	inline	u_int8_t&	hop_count() { return hop_count_; }
	inline	u_int16_t&	msg_seq_num() { return msg_seq_num_; }
	inline	AOLSR_hello&	hello() { return msg_body_.hello_; }
	inline	AOLSR_tc&	tc() { return msg_body_.tc_; }
	inline	AOLSR_mid&	mid() { return msg_body_.mid_; }

	inline u_int32_t size() {
		u_int32_t sz = AOLSR_MSG_HDR_SIZE;
		if (msg_type() == AOLSR_HELLO_MSG)
			sz += hello().size();
		else if (msg_type() == AOLSR_TC_MSG)
			sz += tc().size();
		else if (msg_type() == AOLSR_MID_MSG)
			sz += mid().size();
		return sz;
	}

} AOLSR_msg;

/// %AOLSR packet.
typedef struct AOLSR_pkt {

	u_int16_t	pkt_len_;			///< Packet length (in bytes).
	u_int16_t	pkt_seq_num_;			///< Packet sequence number.
	AOLSR_msg	pkt_body_[AOLSR_MAX_MSGS];	///< Packet body.
	int		count;				///< Number of AOLSR_msg contained in pkt_body_.

	inline	u_int16_t&	pkt_len() { return pkt_len_; }
	inline	u_int16_t&	pkt_seq_num() { return pkt_seq_num_; }
	inline	AOLSR_msg&	msg(int i) { return pkt_body_[i]; }

	static int offset_;
	inline static int& offset() { return offset_; }
	inline static struct AOLSR_pkt* access(const Packet* p) {
		return (struct AOLSR_pkt*)p->access(offset_);
	}

} AOLSR_pkt;

#endif
