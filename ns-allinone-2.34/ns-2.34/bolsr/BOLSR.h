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
/// \file	BOLSR.h
/// \brief	Header file for BOLSR agent and related classes.
///
/// Here are defined all timers used by BOLSR, including those for managing internal
/// state and those for sending messages. Class BOLSR is also defined, therefore this
/// file has signatures for the most important methods. Lots of constants are also
/// defined.
///

#ifndef __BOLSR_h__
#define __BOLSR_h__

#include <bolsr/BOLSR_pkt.h>
#include <bolsr/BOLSR_state.h>
#include <bolsr/BOLSR_rtable.h>
#include <bolsr/BOLSR_repositories.h>
#include <trace.h>
#include <classifier-port.h>
#include <agent.h>
#include <packet.h>
#include <timer-handler.h>
#include <random.h>
#include <cmu-trace.h> ///added 周家家
#include <priqueue.h> ///added 周家家
#include <classifier/classifier-port.h> ///added 周家家
#include <mac/mac-802_11.h> ///added 周家家
#include <vector>
#include <cmath>

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
#define BOLSR_C		0.0625


/********** Intervals **********/

/// HELLO messages emission interval.
#define BOLSR_HELLO_INTERVAL	2

/// TC messages emission interval.
#define BOLSR_TC_INTERVAL	5

/// MID messages emission interval.
#define BOLSR_MID_INTERVAL	BOLSR_TC_INTERVAL

///
/// \brief Period at which a node must cite every link and every neighbor.
///
/// We only use this value in order to define BOLSR_NEIGHB_HOLD_TIME.
///
#define BOLSR_REFRESH_INTERVAL	2


/********** Holding times **********/

/// Neighbor holding time.
#define BOLSR_NEIGHB_HOLD_TIME	3*BOLSR_REFRESH_INTERVAL
/// Top holding time.
#define BOLSR_TOP_HOLD_TIME	3*BOLSR_TC_INTERVAL
/// Dup holding time.
#define BOLSR_DUP_HOLD_TIME	30
/// MID holding time.
#define BOLSR_MID_HOLD_TIME	3*BOLSR_MID_INTERVAL


/********** Link types **********/

/// Unspecified link type.
#define BOLSR_UNSPEC_LINK	0
/// Asymmetric link type.
#define BOLSR_ASYM_LINK		1
/// Symmetric link type.
#define BOLSR_SYM_LINK		2
/// Lost link type.
#define BOLSR_LOST_LINK		3

/********** Neighbor types **********/

/// Not neighbor type.
#define BOLSR_NOT_NEIGH		0
/// Symmetric neighbor type.
#define BOLSR_SYM_NEIGH		1
/// Asymmetric neighbor type.
#define BOLSR_MPR_NEIGH		2


/********** Willingness **********/

/// Willingness for forwarding packets from other nodes: never.
#define BOLSR_WILL_NEVER		0
/// Willingness for forwarding packets from other nodes: low.
#define BOLSR_WILL_LOW		1
/// Willingness for forwarding packets from other nodes: medium.
#define BOLSR_WILL_DEFAULT	3
/// Willingness for forwarding packets from other nodes: high.
#define BOLSR_WILL_HIGH		6
/// Willingness for forwarding packets from other nodes: always.
#define BOLSR_WILL_ALWAYS	7


/********** Miscellaneous constants **********/

/// Maximum allowed jitter.
#define BOLSR_MAXJITTER		BOLSR_HELLO_INTERVAL/4
/// Maximum allowed sequence number.
#define BOLSR_MAX_SEQ_NUM	65535
/// Used to set status of an BOLSR_nb_tuple as "not symmetric".
#define BOLSR_STATUS_NOT_SYM	0
/// Used to set status of an BOLSR_nb_tuple as "symmetric".
#define BOLSR_STATUS_SYM		1
/// Random number between [0-BOLSR_MAXJITTER] used to jitter BOLSR packet transmission.
#define JITTER			(Random::uniform()*BOLSR_MAXJITTER)


class BOLSR;			// forward declaration


/********** Timers **********/


/// Timer for sending an enqued message.
class BOLSR_MsgTimer : public TimerHandler {
public:
	BOLSR_MsgTimer(BOLSR* agent) : TimerHandler() {
		agent_	= agent;
	}
protected:
	BOLSR*	agent_;			///< BOLSR agent which created the timer.
	virtual void expire(Event* e);
};

/// Timer for sending HELLO messages.
class BOLSR_HelloTimer : public TimerHandler {
public:
	BOLSR_HelloTimer(BOLSR* agent) : TimerHandler() { agent_ = agent; }
protected:
	BOLSR*	agent_;			///< BOLSR agent which created the timer.
	virtual void expire(Event* e);
};


/// Timer for sending TC messages.
class BOLSR_TcTimer : public TimerHandler {
public:
	BOLSR_TcTimer(BOLSR* agent) : TimerHandler() { agent_ = agent; }
protected:
	BOLSR*	agent_;			///< BOLSR agent which created the timer.
	virtual void expire(Event* e);
};


/// Timer for sending MID messages.
class BOLSR_MidTimer : public TimerHandler {
public:
	BOLSR_MidTimer(BOLSR* agent) : TimerHandler() { agent_ = agent; }
protected:
	BOLSR*	agent_;			///< BOLSR agent which created the timer.
	virtual void expire(Event* e);
};


/// Timer for removing duplicate tuples: BOLSR_dup_tuple.
class BOLSR_DupTupleTimer : public TimerHandler {
public:
	BOLSR_DupTupleTimer(BOLSR* agent, BOLSR_dup_tuple* tuple) : TimerHandler() {
		agent_ = agent;
		tuple_ = tuple;
	}
protected:
	BOLSR*		agent_;	///< BOLSR agent which created the timer.
	BOLSR_dup_tuple*	tuple_;	///< BOLSR_dup_tuple which must be removed.
	
	virtual void expire(Event* e);
};


/// Timer for removing link tuples: BOLSR_link_tuple.
class BOLSR_LinkTupleTimer : public TimerHandler {
	///
	/// \brief A flag which tells if the timer has expired (at least) once or not.
	///
	/// When a link tuple has been just created, its sym_time is expired but this
	/// does not mean a neighbor loss. Thus, we use this flag in order to be able
	/// to distinguish this situation.
	///
	bool			first_time_;
public:
	BOLSR_LinkTupleTimer(BOLSR* agent, BOLSR_link_tuple* tuple) : TimerHandler() {
		agent_		= agent;
		tuple_		= tuple;
		first_time_	= true;
	}
protected:
	BOLSR*			agent_;	///< BOLSR agent which created the timer.
	BOLSR_link_tuple*	tuple_;	///< BOLSR_link_tuple which must be removed.
	
	virtual void expire(Event* e);
};


/// Timer for removing nb2hop tuples: BOLSR_nb2hop_tuple.
class BOLSR_Nb2hopTupleTimer : public TimerHandler {
public:
	BOLSR_Nb2hopTupleTimer(BOLSR* agent, BOLSR_nb2hop_tuple* tuple) : TimerHandler() {
		agent_ = agent;
		tuple_ = tuple;
	}
protected:
	BOLSR*			agent_;	///< BOLSR agent which created the timer.
	BOLSR_nb2hop_tuple*	tuple_;	///< BOLSR_nb2hop_tuple which must be removed.
	
	virtual void expire(Event* e);
};


/// Timer for removing MPR selector tuples: BOLSR_mprsel_tuple.
class BOLSR_MprSelTupleTimer : public TimerHandler {
public:
	BOLSR_MprSelTupleTimer(BOLSR* agent, BOLSR_mprsel_tuple* tuple) : TimerHandler() {
		agent_ = agent;
		tuple_ = tuple;
	}
protected:
	BOLSR*			agent_;	///< BOLSR agent which created the timer.
	BOLSR_mprsel_tuple*	tuple_;	///< BOLSR_mprsel_tuple which must be removed.
	
	virtual void expire(Event* e);
};


/// Timer for removing topology tuples: BOLSR_topology_tuple.
class BOLSR_TopologyTupleTimer : public TimerHandler {
public:
	BOLSR_TopologyTupleTimer(BOLSR* agent, BOLSR_topology_tuple* tuple) : TimerHandler() {
		agent_ = agent;
		tuple_ = tuple;
	}
protected:
	BOLSR*			agent_;	///< BOLSR agent which created the timer.
	BOLSR_topology_tuple*	tuple_;	///< BOLSR_topology_tuple which must be removed.
	
	virtual void expire(Event* e);
};


/// Timer for removing interface association tuples: BOLSR_iface_assoc_tuple.
class BOLSR_IfaceAssocTupleTimer : public TimerHandler {
public:
	BOLSR_IfaceAssocTupleTimer(BOLSR* agent, BOLSR_iface_assoc_tuple* tuple) : TimerHandler() {
		agent_ = agent;
		tuple_ = tuple;
	}
protected:
	BOLSR*			agent_;	///< BOLSR agent which created the timer.
	BOLSR_iface_assoc_tuple*	tuple_;	///< BOLSR_iface_assoc_tuple which must be removed.
	
	virtual void expire(Event* e);
};


/********** BOLSR Agent **********/


///
/// \brief Routing agent which implements %BOLSR protocol following RFC 3626.
///
/// Interacts with TCL interface through command() method. It implements all
/// functionalities related to sending and receiving packets and managing
/// internal state.
///
class BOLSR : public Agent {
	// Makes some friends.
	friend class BOLSR_HelloTimer;
	friend class BOLSR_TcTimer;
	friend class BOLSR_MidTimer;
	friend class BOLSR_DupTupleTimer;
	friend class BOLSR_LinkTupleTimer;
	friend class BOLSR_Nb2hopTupleTimer;
	friend class BOLSR_MprSelTupleTimer;
	friend class BOLSR_TopologyTupleTimer;
	friend class BOLSR_IfaceAssocTupleTimer;
	friend class BOLSR_MsgTimer;
	
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
	
	Mac802_11*  clmac; // Mac802.11的指针 added 周家家



        double h_time_max_;//added zjj
        double h_time_min_;//added zjj
        double h_time_mid_;//added zjj
        double h_time_current_;//added zjj
        double link_change_ratio_1_;//added zjj
        double link_change_ratio_2_;//added zjj
        double link_change_ratio_;//added zjj


        double t_time_max_;//added zjj
        double t_time_min_;//added zjj
        double t_time_mid_;// added zjj
        double t_time_current_;//added zjj
        nbset_t nbset_1_;//added zjj last nb_set
        bool quick_tc_;
        int d_tc_;

	int n_nb_add=0; // added 周家家
	int n_nb_lose=0; // added 周家家
        double rssi=0; // added zjj
	/// Routing table.
	BOLSR_rtable		rtable_;
	/// Internal state with all needed data structs.
	BOLSR_state		state_;
	/// A list of pending messages which are buffered awaiting for being sent.
	std::vector<BOLSR_msg>	msgs_;
	
protected:
	PortClassifier*	dmux_;		///< For passing packets up to agents.
	Trace*		logtarget_;	///< For logging.
	
	BOLSR_HelloTimer	hello_timer_;	///< Timer for sending HELLO messages.
	BOLSR_TcTimer	tc_timer_;	///< Timer for sending TC messages.
	BOLSR_MidTimer	mid_timer_;	///< Timer for sending MID messages.
	
	/// Increments packet sequence number and returns the new value.
	inline u_int16_t	pkt_seq() {
		pkt_seq_ = (pkt_seq_ + 1)%(BOLSR_MAX_SEQ_NUM + 1);
		return pkt_seq_;
	}
	/// Increments message sequence number and returns the new value.
	inline u_int16_t	msg_seq() {
		msg_seq_ = (msg_seq_ + 1)%(BOLSR_MAX_SEQ_NUM + 1);
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
	

        inline double& h_time_max() {return h_time_max_;}//added zjj
        inline double& h_time_min() {return h_time_min_;}//added zjj
        inline double& h_time_mid() {return h_time_mid_;}//added zjj
        inline double& link_change_ratio_1() {return link_change_ratio_1_;}//added zjj
        inline double& link_change_ratio_2() {return link_change_ratio_2_;}//added zjj
        inline double& link_change_ratio() {return link_change_ratio_;}//added zjj
        inline double& h_time_current() {return h_time_current_;}//added zjj


        inline double& t_time_max() {return t_time_max_;}//added zjj
        inline double& t_time_min() {return t_time_min_;}//added zjj
        inline double& t_time_mid() {return t_time_mid_;}// added zjj
        inline double& t_time_current() {return t_time_current_;}//added zjj
        inline nbset_t& nbset_1() {return nbset_1_;}//added zjj last nb_set
        inline bool& quick_tc() {return quick_tc_;}// added zjj
        inline int& d_tc() {return d_tc_;}// added zjj


        BOLSR_nb_tuple* find_nb_tuple_1(nsaddr_t main_addr)  //added zjj
       { //added zjj
	for (nbset_t::iterator it = nbset_1_.begin(); it != nbset_1_.end(); it++)  //added zjj
           { //added zjj
		BOLSR_nb_tuple* tuple = *it; //added zjj
		if (tuple->nb_main_addr() == main_addr) //added zjj
			return tuple; //added zjj
	   } //added zjj
	return NULL; //added zjj
       } //added zjj


	void		recv_bolsr(Packet*);
	
	void		mpr_computation();
	void		rtable_computation();
	void		process_hello(BOLSR_msg&, nsaddr_t, nsaddr_t, double);//changed zjj
	void		process_tc(BOLSR_msg&, nsaddr_t);
	void		process_mid(BOLSR_msg&, nsaddr_t);
	
	void		forward_default(Packet*, BOLSR_msg&, BOLSR_dup_tuple*, nsaddr_t);
	void		forward_data(Packet*);
	
	void		enque_msg(BOLSR_msg&, double);
	void		send_hello();
	void		send_tc();
	void		send_mid();
	void		send_pkt();
	
	void		link_sensing(BOLSR_msg&, nsaddr_t, nsaddr_t,double);//changed zjj 
	void		populate_nbset(BOLSR_msg&);
	void		populate_nb2hopset(BOLSR_msg&);
	void		populate_mprselset(BOLSR_msg&);

	void		set_hello_timer();
	void		set_tc_timer();
	void		set_mid_timer();

	void		nb_loss(BOLSR_link_tuple*);
	void		add_dup_tuple(BOLSR_dup_tuple*);
	void		rm_dup_tuple(BOLSR_dup_tuple*);
	void		add_link_tuple(BOLSR_link_tuple*, u_int8_t);
	void		rm_link_tuple(BOLSR_link_tuple*);
	void		updated_link_tuple(BOLSR_link_tuple*);
	void		add_nb_tuple(BOLSR_nb_tuple*);
	void		rm_nb_tuple(BOLSR_nb_tuple*);
	void		add_nb2hop_tuple(BOLSR_nb2hop_tuple*);
	void		rm_nb2hop_tuple(BOLSR_nb2hop_tuple*);
	void		add_mprsel_tuple(BOLSR_mprsel_tuple*);
	void		rm_mprsel_tuple(BOLSR_mprsel_tuple*);
	void		add_topology_tuple(BOLSR_topology_tuple*);
	void		rm_topology_tuple(BOLSR_topology_tuple*);
	void		add_ifaceassoc_tuple(BOLSR_iface_assoc_tuple*);
	void		rm_ifaceassoc_tuple(BOLSR_iface_assoc_tuple*);
	u_int8_t        get_node_score(); //added 周家家
        u_int16_t       get_node_score_2();// added zjj
        void            nb_tuple_resize();// added zjj
        double          get_result(BOLSR_nb_tuple*);// added zjj
	
	nsaddr_t	get_main_addr(nsaddr_t);
	int		degree(BOLSR_nb_tuple*);

	static bool	seq_num_bigger_than(u_int16_t, u_int16_t);

public:
	BOLSR(nsaddr_t);
	int	command(int, const char*const*);
	void	recv(Packet*, Handler*);
	void	mac_failed(Packet*);
	
	static double		emf_to_seconds(u_int8_t);
	static u_int8_t		seconds_to_emf(double);
	static int		node_id(nsaddr_t);
};

#endif
