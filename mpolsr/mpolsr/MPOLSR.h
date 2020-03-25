/*              Copyright (C) 2010
	*             Multipath Extension by Jiazi Yi,                            *
 	*                   2007   Ecole Polytech of Nantes, France               * 
 	*                   jiazi.yi@univ-nantes.fr				   *
 	****************************************************************************
 	*    	This program is distributed in the hope that it will be useful,				*
	*    	but WITHOUT ANY WARRANTY; without even the implied warranty of				*
	*    	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 					*
 	**************************************************************************
*/
/***************************************************************************
 *   Copyright (C) 2004 by Francisco J. Ros                                *
 *   fjrm@dif.um.es                                                        *
 *                                                                         *
 *             Multipath Extension by Jiazi Yi,                            *
 *                   2007   Ecole Polytech of Nantes, France               * 
 *                   jiazi.yi@univ-nantes.fr				   *
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
/// \file	MPOLSR.h
/// \brief	Header file for MPOLSR agent and related classes.
///
/// Here are defined all timers used by MPOLSR, including those for managing internal
/// state and those for sending messages. Class MPOLSR is also defined, therefore this
/// file has signatures for the most important methods. Lots of constants are also
/// defined.
///

#ifndef __MPOLSR_h__
#define __MPOLSR_h__

#include "MPOLSR_pkt.h"
#include "MPOLSR_state.h"
#include "MPOLSR_rtable.h"
#include "MPOLSR_m_rtable.h"
#include "MPOLSR_repositories.h"
#include "trace.h"
#include "classifier-port.h"
#include "agent.h"
#include "packet.h"
#include "timer-handler.h"
#include "random.h"
#include "vector"
#include <dsr-priqueue.h>

/********** Useful macros **********/

/// Returns maximum of two numbers.
#ifndef MAX
#define	MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

/// Returns minimum of two numbers.
#ifndef MIN
#define	MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

/// Defines NULL as zero, used for pointers.
#ifndef NULL
#define NULL 0
#endif

/// Gets current time from the scheduler.
#define CURRENT_TIME	Scheduler::instance().clock()

///
/// \brief Gets the delay between a given time and the current time.
///
/// If given time is previous to the current one, then this macro returns
/// a number close to 0. This is used for scheduling events at a certain moment.
///
#define DELAY(time) (((time) < (CURRENT_TIME)) ? (0.000001) : \
	(time - CURRENT_TIME + 0.000001))

/// Scaling factor used in RFC 3626.
#define MPOLSR_C		0.0625


/********** Intervals **********/

/// HELLO messages emission interval.
#define MPOLSR_HELLO_INTERVAL	 2

/// TC messages emission interval.
#define MPOLSR_TC_INTERVAL	 5

/// MID messages emission interval.
#define MPOLSR_MID_INTERVAL	MPOLSR_TC_INTERVAL

///
/// \brief Period at which a node must cite every link and every neighbor.
///
/// We only use this value in order to define MPOLSR_NEIGHB_HOLD_TIME.
///
#define MPOLSR_REFRESH_INTERVAL	2


/********** Holding times **********/

/// Neighbor holding time.
#define MPOLSR_NEIGHB_HOLD_TIME	3*MPOLSR_REFRESH_INTERVAL
/// Top holding time.
#define MPOLSR_TOP_HOLD_TIME	3*MPOLSR_TC_INTERVAL
/// Dup holding time.
#define MPOLSR_DUP_HOLD_TIME	30
/// MID holding time.
#define MPOLSR_MID_HOLD_TIME	3*MPOLSR_MID_INTERVAL


/********** Link types **********/

/// Unspecified link type.
#define MPOLSR_UNSPEC_LINK	0
/// Asymmetric link type.
#define MPOLSR_ASYM_LINK		1
/// Symmetric link type.
#define MPOLSR_SYM_LINK		2
/// Lost link type.
#define MPOLSR_LOST_LINK		3

/********** Neighbor types **********/

/// Not neighbor type.
#define MPOLSR_NOT_NEIGH		0
/// Symmetric neighbor type.
#define MPOLSR_SYM_NEIGH		1
/// Asymmetric neighbor type.
#define MPOLSR_MPR_NEIGH		2


/********** Willingness **********/

/// Willingness for forwarding packets from other nodes: never.
#define MPOLSR_WILL_NEVER		0
/// Willingness for forwarding packets from other nodes: low.
#define MPOLSR_WILL_LOW		1
/// Willingness for forwarding packets from other nodes: medium.
#define MPOLSR_WILL_DEFAULT	3
/// Willingness for forwarding packets from other nodes: high.
#define MPOLSR_WILL_HIGH		6
/// Willingness for forwarding packets from other nodes: always.
#define MPOLSR_WILL_ALWAYS	7


/********** Miscellaneous constants **********/

/// Maximum allowed jitter.
#define MPOLSR_MAXJITTER		MPOLSR_HELLO_INTERVAL/4
/// Maximum allowed sequence number.
#define MPOLSR_MAX_SEQ_NUM	65535
/// Used to set status of an MPOLSR_nb_tuple as "not symmetric".
#define MPOLSR_STATUS_NOT_SYM	0
/// Used to set status of an MPOLSR_nb_tuple as "symmetric".
#define MPOLSR_STATUS_SYM		1
/// Random number between [0-MPOLSR_MAXJITTER] used to jitter MPOLSR packet transmission.
#define JITTER			(Random::uniform()*MPOLSR_MAXJITTER)

/*****************Type of Dijsktra node***********/
#define P_TYPE 1
#define T_TYPE 0

#define MAX_ROUTE 3

#define USE_MAC TRUE

class MPOLSR;			// forward declaration


/********** Timers **********/


/// Timer for sending an enqued message.
class MPOLSR_MsgTimer : public TimerHandler {
public:
	MPOLSR_MsgTimer(MPOLSR* agent) : TimerHandler() {
		agent_	= agent;
	}
protected:
	MPOLSR*	agent_;			///< MPOLSR agent which created the timer.
	virtual void expire(Event* e);
};

/// Timer for sending HELLO messages.
class MPOLSR_HelloTimer : public TimerHandler {
public:
	MPOLSR_HelloTimer(MPOLSR* agent) : TimerHandler() { agent_ = agent; }
protected:
	MPOLSR*	agent_;			///< MPOLSR agent which created the timer.
	virtual void expire(Event* e);
};


/// Timer for sending TC messages.
class MPOLSR_TcTimer : public TimerHandler {
public:
	MPOLSR_TcTimer(MPOLSR* agent) : TimerHandler() { agent_ = agent; }
protected:
	MPOLSR*	agent_;			///< MPOLSR agent which created the timer.
	virtual void expire(Event* e);
};


/// Timer for sending MID messages.
class MPOLSR_MidTimer : public TimerHandler {
public:
	MPOLSR_MidTimer(MPOLSR* agent) : TimerHandler() { agent_ = agent; }
protected:
	MPOLSR*	agent_;			///< MPOLSR agent which created the timer.
	virtual void expire(Event* e);
};


/// Timer for removing duplicate tuples: MPOLSR_dup_tuple.
class MPOLSR_DupTupleTimer : public TimerHandler {
public:
	MPOLSR_DupTupleTimer(MPOLSR* agent, MPOLSR_dup_tuple* tuple) : TimerHandler() {
		agent_ = agent;
		tuple_ = tuple;
	}
protected:
	MPOLSR*		agent_;	///< MPOLSR agent which created the timer.
	MPOLSR_dup_tuple*	tuple_;	///< MPOLSR_dup_tuple which must be removed.
	
	virtual void expire(Event* e);
};


/// Timer for removing link tuples: MPOLSR_link_tuple.
class MPOLSR_LinkTupleTimer : public TimerHandler {
	///
	/// \brief A flag which tells if the timer has expired (at least) once or not.
	///
	/// When a link tuple has been just created, its sym_time is expired but this
	/// does not mean a neighbor loss. Thus, we use this flag in order to be able
	/// to distinguish this situation.
	///
	bool			first_time_;
public:
	MPOLSR_LinkTupleTimer(MPOLSR* agent, MPOLSR_link_tuple* tuple) : TimerHandler() {
		agent_		= agent;
		tuple_		= tuple;
		first_time_	= true;
	}
protected:
	MPOLSR*			agent_;	///< MPOLSR agent which created the timer.
	MPOLSR_link_tuple*	tuple_;	///< MPOLSR_link_tuple which must be removed.
	
	virtual void expire(Event* e);
};


/// Timer for removing nb2hop tuples: MPOLSR_nb2hop_tuple.
class MPOLSR_Nb2hopTupleTimer : public TimerHandler {
public:
	MPOLSR_Nb2hopTupleTimer(MPOLSR* agent, MPOLSR_nb2hop_tuple* tuple) : TimerHandler() {
		agent_ = agent;
		tuple_ = tuple;
	}
protected:
	MPOLSR*			agent_;	///< MPOLSR agent which created the timer.
	MPOLSR_nb2hop_tuple*	tuple_;	///< MPOLSR_nb2hop_tuple which must be removed.
	
	virtual void expire(Event* e);
};


/// Timer for removing MPR selector tuples: MPOLSR_mprsel_tuple.
class MPOLSR_MprSelTupleTimer : public TimerHandler {
public:
	MPOLSR_MprSelTupleTimer(MPOLSR* agent, MPOLSR_mprsel_tuple* tuple) : TimerHandler() {
		agent_ = agent;
		tuple_ = tuple;
	}
protected:
	MPOLSR*			agent_;	///< MPOLSR agent which created the timer.
	MPOLSR_mprsel_tuple*	tuple_;	///< MPOLSR_mprsel_tuple which must be removed.
	
	virtual void expire(Event* e);
};


/// Timer for removing topology tuples: MPOLSR_topology_tuple.
class MPOLSR_TopologyTupleTimer : public TimerHandler {
public:
	MPOLSR_TopologyTupleTimer(MPOLSR* agent, MPOLSR_topology_tuple* tuple) : TimerHandler() {
		agent_ = agent;
		tuple_ = tuple;
	}
protected:
	MPOLSR*			agent_;	///< MPOLSR agent which created the timer.
	MPOLSR_topology_tuple*	tuple_;	///< MPOLSR_topology_tuple which must be removed.
	
	virtual void expire(Event* e);
};


/// Timer for removing interface association tuples: MPOLSR_iface_assoc_tuple.
class MPOLSR_IfaceAssocTupleTimer : public TimerHandler {
public:
	MPOLSR_IfaceAssocTupleTimer(MPOLSR* agent, MPOLSR_iface_assoc_tuple* tuple) : TimerHandler() {
		agent_ = agent;
		tuple_ = tuple;
	}
protected:
	MPOLSR*			agent_;	///< MPOLSR agent which created the timer.
	MPOLSR_iface_assoc_tuple*	tuple_;	///< MPOLSR_iface_assoc_tuple which must be removed.
	
	virtual void expire(Event* e);
};


/********** MPOLSR Agent **********/


///
/// \brief Routing agent which implements %MPOLSR protocol following RFC 3626.
///
/// Interacts with TCL interface through command() method. It implements all
/// functionalities related to sending and receiving packets and managing
/// internal state.
///
class MPOLSR : public Agent {
	// Makes some friends.
	friend class MPOLSR_HelloTimer;
	friend class MPOLSR_TcTimer;
	friend class MPOLSR_MidTimer;
	friend class MPOLSR_DupTupleTimer;
	friend class MPOLSR_LinkTupleTimer;
	friend class MPOLSR_Nb2hopTupleTimer;
	friend class MPOLSR_MprSelTupleTimer;
	friend class MPOLSR_TopologyTupleTimer;
	friend class MPOLSR_IfaceAssocTupleTimer;
	friend class MPOLSR_MsgTimer;
	
	/// Address of the routing agent.
	nsaddr_t	ra_addr_;
	
	/// Packets sequence number counter.
	u_int16_t	pkt_seq_;
	/// Messages sequence number counter.
	u_int16_t	msg_seq_;
	/// Advertised Neighbor Set sequence number.
	u_int16_t	ansn_;
	
	/// HELLO messages' emission interval.
	int		hello_ival_;
	/// TC messages' emission interval.
	int		tc_ival_;
	/// MID messages' emission interval.
	int		mid_ival_;
	/// Willingness for forwarding packets on behalf of other nodes.
	int		willingness_;
	/// Determines if layer 2 notifications are enabled or not.
	int		use_mac_;
	
	/// Routing table.
	MPOLSR_rtable		rtable_;

	/// Multipath routing table
	MPOLSR_m_rtable		m_rtable_;

	
	/// Internal state with all needed data structs.
	MPOLSR_state		state_;
	/// A list of pending messages which are buffered awaiting for being sent.
	std::vector<MPOLSR_msg>	msgs_;

	///packet count
	int 		packet_count_;
	
protected:
	PortClassifier*	dmux_;		///< For passing packets up to agents.
	Trace*		logtarget_;	///< For logging.
	
	MPOLSR_HelloTimer	hello_timer_;	///< Timer for sending HELLO messages.
	MPOLSR_TcTimer	tc_timer_;	///< Timer for sending TC messages.
	MPOLSR_MidTimer	mid_timer_;	///< Timer for sending MID messages.
	
	/// Increments packet sequence number and returns the new value.
	inline u_int16_t	pkt_seq() {
		pkt_seq_ = (pkt_seq_ + 1)%(MPOLSR_MAX_SEQ_NUM + 1);
		return pkt_seq_;
	}
	/// Increments message sequence number and returns the new value.
	inline u_int16_t	msg_seq() {
		msg_seq_ = (msg_seq_ + 1)%(MPOLSR_MAX_SEQ_NUM + 1);
		return msg_seq_;
	}
	
	inline nsaddr_t&	ra_addr()	{ return ra_addr_; }
	
	inline int&		hello_ival()	{ return hello_ival_; }
	inline int&		tc_ival()	{ return tc_ival_; }
	inline int&		mid_ival()	{ return mid_ival_; }
	inline int&		willingness()	{ return willingness_; }
	inline int&		use_mac()	{ return use_mac_; }
	
	inline linkset_t&	linkset()	{ return state_.linkset(); }
	inline mprset_t&	mprset()	{ return state_.mprset(); }
	inline mprselset_t&	mprselset()	{ return state_.mprselset(); }
	inline nbset_t&		nbset()		{ return state_.nbset(); }
	inline nb2hopset_t&	nb2hopset()	{ return state_.nb2hopset(); }
	inline topologyset_t&	topologyset()	{ return state_.topologyset(); }
	inline dupset_t&	dupset()	{ return state_.dupset(); }
	inline ifaceassocset_t&	ifaceassocset()	{ return state_.ifaceassocset(); }
	
	void		recv_mpolsr(Packet*);
	
	void		mpr_computation();
	void		rtable_computation();
	void 		m_rtable_computation(Packet*);

	void		process_hello(MPOLSR_msg&, nsaddr_t, nsaddr_t);
	void		process_tc(MPOLSR_msg&, nsaddr_t);
	void		process_mid(MPOLSR_msg&, nsaddr_t);
	
	void		forward_default(Packet*, MPOLSR_msg&, MPOLSR_dup_tuple*, nsaddr_t);
	void		forward_data(Packet*);
	void 		m_forward_data(Packet*);
	void		resend_data(Packet*);
	
	void		enque_msg(MPOLSR_msg&, double);
	void		send_hello();
	void		send_tc();
	void		send_mid();
	void		send_pkt();
	
	void		link_sensing(MPOLSR_msg&, nsaddr_t, nsaddr_t);
	void		populate_nbset(MPOLSR_msg&);
	void		populate_nb2hopset(MPOLSR_msg&);
	void		populate_mprselset(MPOLSR_msg&);

	void		set_hello_timer();
	void		set_tc_timer();
	void		set_mid_timer();

	void		nb_loss(MPOLSR_link_tuple*);
	void		add_dup_tuple(MPOLSR_dup_tuple*);
	void		rm_dup_tuple(MPOLSR_dup_tuple*);
	void		add_link_tuple(MPOLSR_link_tuple*, u_int8_t);
	void		rm_link_tuple(MPOLSR_link_tuple*);
	void		updated_link_tuple(MPOLSR_link_tuple*);
	void		add_nb_tuple(MPOLSR_nb_tuple*);
	void		rm_nb_tuple(MPOLSR_nb_tuple*);
	void		add_nb2hop_tuple(MPOLSR_nb2hop_tuple*);
	void		rm_nb2hop_tuple(MPOLSR_nb2hop_tuple*);
	void		add_mprsel_tuple(MPOLSR_mprsel_tuple*);
	void		rm_mprsel_tuple(MPOLSR_mprsel_tuple*);
	void		add_topology_tuple(MPOLSR_topology_tuple*);
	void		rm_topology_tuple(MPOLSR_topology_tuple*);
	void		add_ifaceassoc_tuple(MPOLSR_iface_assoc_tuple*);
	void		rm_ifaceassoc_tuple(MPOLSR_iface_assoc_tuple*);
	
	nsaddr_t	get_main_addr(nsaddr_t);
	int		degree(MPOLSR_nb_tuple*);

	static bool	seq_num_bigger_than(u_int16_t, u_int16_t);
	float 		tuple_weight(float a);

	NsObject *ll;		        // our link layer output 
	CMUPriQueue *ifq;		// output interface queue
	Mac *mac_;

public:
	MPOLSR(nsaddr_t);
	int	command(int, const char*const*);
	void	recv(Packet*, Handler*);
	void	mac_failed(Packet*);
	
	static double		emf_to_seconds(u_int8_t);
	static u_int8_t		seconds_to_emf(double);
	static int		node_id(nsaddr_t);
};

#endif
