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
 /// \file	AOLSR.h
 /// \brief	Header file for AOLSR agent and related classes.
 ///
 /// Here are defined all timers used by AOLSR, including those for managing internal
 /// state and those for sending messages. Class AOLSR is also defined, therefore this
 /// file has signatures for the most important methods. Lots of constants are also
 /// defined.
 /// 此处定义了AOLSR使用的所有计时器，包括用于管理内部状态的计时器和用于发送消息的计时器。 
 /// 还定义了AOLSR类，因此该文件具有最重要方法的签名。 还定义了许多常量。

#ifndef __AOLSR_h__
#define __AOLSR_h__

#include <aolsr/AOLSR_pkt.h>
#include <aolsr/AOLSR_state.h>
#include <aolsr/AOLSR_rtable.h>
#include <aolsr/AOLSR_repositories.h>
#include <trace.h>
#include <classifier-port.h>
#include <agent.h>
#include <packet.h>
#include <timer-handler.h>
#include <random.h>
#include <vector>
#include <cmath>

#include <cmu-trace.h>
#include <priqueue.h>
#include <classifier/classifier-port.h>
#include <mac/mac-802_11.h>
#include <dsr-priqueue.h>

/********** Useful macros 巨集**********/

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

/// Gets current time from the scheduler. 从调度程序获取当前时间。
#define CURRENT_TIME	Scheduler::instance().clock()

///
/// \brief Gets the delay between a given time and the current time.
///
/// If given time is previous to the current one, then this macro returns
/// a number close to 0. This is used for scheduling events at a certain moment.
///
#define DELAY(time) (((time) < (CURRENT_TIME)) ? (0.000001) : \
	(time - CURRENT_TIME + 0.000001))

/// Scaling factor used in RFC 3626. RFC 3626中使用的比例因子。
#define AOLSR_C		0.0625


/********** Intervals **********/

/// HELLO messages emission interval.
/// HELLO消息发射间隔。
#define AOLSR_HELLO_INTERVAL	2

/// TC messages emission interval.
#define AOLSR_TC_INTERVAL	5

/// MID messages emission interval.
#define AOLSR_MID_INTERVAL	AOLSR_TC_INTERVAL

///
/// \brief Period at which a node must cite every link and every neighbor.
/// We only use this value in order to define AOLSR_NEIGHB_HOLD_TIME.
///节点必须引用每个链接和每个邻居的时间段。 我们仅使用此值来定义AOLSR_NEIGHB_HOLD_TIME。  

#define AOLSR_REFRESH_INTERVAL	2


/********** Holding times **********/

/// Neighbor holding time.
#define AOLSR_NEIGHB_HOLD_TIME	3*AOLSR_REFRESH_INTERVAL
/// Top holding time.
#define AOLSR_TOP_HOLD_TIME	3*AOLSR_TC_INTERVAL
/// Dup holding time.
#define AOLSR_DUP_HOLD_TIME	30
/// MID holding time.
#define AOLSR_MID_HOLD_TIME	3*AOLSR_MID_INTERVAL


/********** Link types **********/

/// Unspecified link type. 未指定的链接类型。
#define AOLSR_UNSPEC_LINK	0
/// Asymmetric link type. 非对称链接类型。
#define AOLSR_ASYM_LINK		1
/// Symmetric link type. 对称链接类型。
#define AOLSR_SYM_LINK		2
/// Lost link type. 链接类型丢失。
#define AOLSR_LOST_LINK		3

/********** Neighbor types **********/

/// Not neighbor type. 非邻居
#define AOLSR_NOT_NEIGH		0
/// Symmetric neighbor type. 对称节点
#define AOLSR_SYM_NEIGH		1
/// Asymmetric neighbor type. 非对称节点
#define AOLSR_MPR_NEIGH		2


/********** Willingness **********/

/// Willingness for forwarding packets from other nodes: never.
#define AOLSR_WILL_NEVER		0
/// Willingness for forwarding packets from other nodes: low.
#define AOLSR_WILL_LOW		1
/// Willingness for forwarding packets from other nodes: medium.
#define AOLSR_WILL_DEFAULT	3
/// Willingness for forwarding packets from other nodes: high.
#define AOLSR_WILL_HIGH		6
/// Willingness for forwarding packets from other nodes: always.
#define AOLSR_WILL_ALWAYS	7


/********** Miscellaneous constants杂常数 **********/

/// Maximum allowed jitter.
#define AOLSR_MAXJITTER		AOLSR_HELLO_INTERVAL/4
/// Maximum allowed sequence number.
#define AOLSR_MAX_SEQ_NUM	65535
/// Used to set status of an AOLSR_nb_tuple as "not symmetric".
#define AOLSR_STATUS_NOT_SYM	0
/// Used to set status of an AOLSR_nb_tuple as "symmetric".
#define AOLSR_STATUS_SYM		1
/// Random number between [0-AOLSR_MAXJITTER] used to jitter AOLSR packet transmission.
#define JITTER			(Random::uniform()*AOLSR_MAXJITTER)


class AOLSR;			// forward declaration


/********** Timers **********/


/// Timer for sending an enqued message. 用于发送入队消息的计时器。
class AOLSR_MsgTimer : public TimerHandler {
public:
	AOLSR_MsgTimer(AOLSR* agent) : TimerHandler() {
		agent_ = agent;
	}
protected:
	AOLSR*	agent_;			///< AOLSR agent which created the timer.
	virtual void expire(Event* e);
};

/// Timer for sending HELLO messages. 发送hello消息的计时器
class AOLSR_HelloTimer : public TimerHandler {
public:
	AOLSR_HelloTimer(AOLSR* agent) : TimerHandler() { agent_ = agent; }
protected:
	AOLSR*	agent_;			///< AOLSR agent which created the timer.
	virtual void expire(Event* e);
};


/// Timer for sending TC messages. 发送tc消息的计时器
class AOLSR_TcTimer : public TimerHandler {
public:
	AOLSR_TcTimer(AOLSR* agent) : TimerHandler() { agent_ = agent; }
protected:
	AOLSR*	agent_;			///< AOLSR agent which created the timer.
	virtual void expire(Event* e);
};


/// Timer for sending MID messages.
class AOLSR_MidTimer : public TimerHandler {
public:
	AOLSR_MidTimer(AOLSR* agent) : TimerHandler() { agent_ = agent; }
protected:
	AOLSR*	agent_;			///< AOLSR agent which created the timer.
	virtual void expire(Event* e);
};


/// Timer for removing duplicate tuples: AOLSR_dup_tuple.
class AOLSR_DupTupleTimer : public TimerHandler {
public:
	AOLSR_DupTupleTimer(AOLSR* agent, AOLSR_dup_tuple* tuple) : TimerHandler() {
		agent_ = agent;
		tuple_ = tuple;
	}
protected:
	AOLSR*		agent_;	///< AOLSR agent which created the timer.
	AOLSR_dup_tuple*	tuple_;	///< AOLSR_dup_tuple which must be removed.

	virtual void expire(Event* e);
};


/// Timer for removing link tuples: AOLSR_link_tuple.
class AOLSR_LinkTupleTimer : public TimerHandler {
	///
	/// \brief A flag which tells if the timer has expired (at least) once or not.
	///
	/// When a link tuple has been just created, its sym_time is expired but this
	/// does not mean a neighbor loss. Thus, we use this flag in order to be able
	/// to distinguish this situation.
	///
	bool			first_time_;
public:
	AOLSR_LinkTupleTimer(AOLSR* agent, AOLSR_link_tuple* tuple) : TimerHandler() {
		agent_ = agent;
		tuple_ = tuple;
		first_time_ = true;
	}
protected:
	AOLSR*			agent_;	///< AOLSR agent which created the timer.
	AOLSR_link_tuple*	tuple_;	///< AOLSR_link_tuple which must be removed.

	virtual void expire(Event* e);
};


/// Timer for removing nb2hop tuples: AOLSR_nb2hop_tuple.
class AOLSR_Nb2hopTupleTimer : public TimerHandler {
public:
	AOLSR_Nb2hopTupleTimer(AOLSR* agent, AOLSR_nb2hop_tuple* tuple) : TimerHandler() {
		agent_ = agent;
		tuple_ = tuple;
	}
protected:
	AOLSR*			agent_;	///< AOLSR agent which created the timer.
	AOLSR_nb2hop_tuple*	tuple_;	///< AOLSR_nb2hop_tuple which must be removed.

	virtual void expire(Event* e);
};


/// Timer for removing MPR selector tuples: AOLSR_mprsel_tuple.
class AOLSR_MprSelTupleTimer : public TimerHandler {
public:
	AOLSR_MprSelTupleTimer(AOLSR* agent, AOLSR_mprsel_tuple* tuple) : TimerHandler() {
		agent_ = agent;
		tuple_ = tuple;
	}
protected:
	AOLSR*			agent_;	///< AOLSR agent which created the timer.
	AOLSR_mprsel_tuple*	tuple_;	///< AOLSR_mprsel_tuple which must be removed.

	virtual void expire(Event* e);
};


/// Timer for removing topology tuples: AOLSR_topology_tuple.
class AOLSR_TopologyTupleTimer : public TimerHandler {
public:
	AOLSR_TopologyTupleTimer(AOLSR* agent, AOLSR_topology_tuple* tuple) : TimerHandler() {
		agent_ = agent;
		tuple_ = tuple;
	}
protected:
	AOLSR*			agent_;	///< AOLSR agent which created the timer.
	AOLSR_topology_tuple*	tuple_;	///< AOLSR_topology_tuple which must be removed.

	virtual void expire(Event* e);
};


/// Timer for removing interface association tuples: AOLSR_iface_assoc_tuple.
class AOLSR_IfaceAssocTupleTimer : public TimerHandler {
public:
	AOLSR_IfaceAssocTupleTimer(AOLSR* agent, AOLSR_iface_assoc_tuple* tuple) : TimerHandler() {
		agent_ = agent;
		tuple_ = tuple;
	}
protected:
	AOLSR*			agent_;	///< AOLSR agent which created the timer.
	AOLSR_iface_assoc_tuple*	tuple_;	///< AOLSR_iface_assoc_tuple which must be removed.

	virtual void expire(Event* e);
};


/********** AOLSR Agent **********/


///
/// \brief Routing agent which implements %AOLSR protocol following RFC 3626.
///
/// Interacts with TCL interface through command() method. It implements all
/// functionalities related to sending and receiving packets and managing
/// internal state.
///
class AOLSR : public Agent {
	// Makes some friends.
	friend class AOLSR_HelloTimer;
	friend class AOLSR_TcTimer;
	friend class AOLSR_MidTimer;
	friend class AOLSR_DupTupleTimer;
	friend class AOLSR_LinkTupleTimer;
	friend class AOLSR_Nb2hopTupleTimer;
	friend class AOLSR_MprSelTupleTimer;
	friend class AOLSR_TopologyTupleTimer;
	friend class AOLSR_IfaceAssocTupleTimer;
	friend class AOLSR_MsgTimer;

	/// Address of the routing agent.
	nsaddr_t	ra_addr_;

	/// Packets sequence number counter.
	u_int16_t	pkt_seq_;
	/// Messages sequence number counter.
	u_int16_t	msg_seq_;
	/// Advertised Neighbor Set sequence number.
	u_int16_t	ansn_;

	/// HELLO messages' emission interval.
	double		hello_ival_;
	/// TC messages' emission interval.
	double		tc_ival_;
	/// MID messages' emission interval.
	int		mid_ival_;
	/// Willingness for forwarding packets on behalf of other nodes.
	int		willingness_;
	/// Determines if layer 2 notifications are enabled or not.
	int		use_mac_;

        double Hmax_; /// added by 周家家
	double Hmin_ ; /// added by 周家家
	double Pmax_ ; /// added by 周家家
	double Pmin_ ; /// added by 周家家
	// 移动得分，最近2个HELLO周期的移动得分。
	double MS1_; /// added by 周家家
	double MS2_ ; /// added by 周家家
	double MS_ ;/// added by 周家家 统计当前周期的得分
	double h_ival_1_ ;/// added by 周家家 当前的hello间隔
	double h_ival_2_ ;/// added by 周家家 上一时刻的hello间隔
	/// hello消息发送间隔的最大值，最小值，以及默认值
	double h_ival_max_ ;/// added by 周家家
	double h_ival_min_ ;/// added by 周家家
	double h_ival_mid_ ;/// added by 周家家
	double h_ival_current_ ;/// added by 周家家
	///hello 消息有效时间
	u_int8_t vtime_ ;/// added by 周家家
	bool t_q_a_ ;//是否快速发送tc消息1。
	bool t_q_b_ ;//是否快速发送tc消息2。
        int t_q_1_;
        int t_q_2_;
	int t_ival_max_;// added by 周家家 最大tc间隔
	int t_ival_min_ ;// added by 周家家 最小tc间隔
	int t_ival_mid_ ;// added by 周家家 默认tc间隔
	int t_ival_1_ ;// added by 周家家 上一时刻tc间隔
	int t_ival_ ;// added by 周家家 当前tc间隔
	int t_vtime_ ;// added by 周家家 拓扑有效时间

Mac802_11*  clmac;

NsObject *ll;
CMUPriQueue *ifq;

	/// Routing table.
	AOLSR_rtable		rtable_;
	/// Internal state with all needed data structs.
	AOLSR_state		state_;
	/// A list of pending messages which are buffered awaiting for being sent.
	std::vector<AOLSR_msg>	msgs_;

protected:
	PortClassifier*	dmux_;		///< For passing packets up to agents.
	Trace*		logtarget_;	///< For logging.

	AOLSR_HelloTimer	hello_timer_;	///< Timer for sending HELLO messages.
	AOLSR_TcTimer	tc_timer_;	///< Timer for sending TC messages.
	AOLSR_MidTimer	mid_timer_;	///< Timer for sending MID messages.

	/// Increments packet sequence number and returns the new value.
	inline u_int16_t	pkt_seq() {
		pkt_seq_ = (pkt_seq_ + 1) % (AOLSR_MAX_SEQ_NUM + 1);
		return pkt_seq_;
	}
	/// Increments message sequence number and returns the new value.
	inline u_int16_t	msg_seq() {
		msg_seq_ = (msg_seq_ + 1) % (AOLSR_MAX_SEQ_NUM + 1);
		return msg_seq_;
	}

	inline nsaddr_t&	ra_addr() { return ra_addr_; }

	inline double&		hello_ival() { return h_ival_current_; } //changed 周家家
	inline int&		tc_ival() { return t_ival_; }//changed 周家家
	inline int&		mid_ival() { return mid_ival_; }
	inline int&		willingness() { return willingness_; }
	inline int&		use_mac() { return use_mac_; }

	inline double& h_ival_current() { return h_ival_current_; }///added 周家家
	inline u_int8_t& vtime() { return vtime_; }/// added by 周家家


	inline double& Hmax() { return Hmax_; } /// added by 周家家
	inline double& Hmin() { return Hmin_; } /// added by 周家家
	inline double& Pmax() { return Pmax_; } /// added by 周家家
	inline double& Pmin() { return Pmin_; } /// added by 周家家
	// 移动得分，最近2个HELLO周期的移动得分。
	inline double& MS1() { return MS1_; } /// added by 周家家
	inline double& MS2() { return MS2_; } /// added by 周家家
	inline double& MS() { return MS_; }/// added by 周家家 统计当前周期的得分
	inline double& h_ival_1() { return h_ival_1_; }/// added by 周家家 当前的hello间隔
	inline double& h_ival_2() { return h_ival_2_; }/// added by 周家家 上一时刻的hello间隔
	inline double& h_ival_max() { return  h_ival_max_; }/// added by 周家家
	inline double& h_ival_min() { return  h_ival_min_; }/// added by 周家家

	inline bool& t_q_a() { return t_q_a_; }//是否快速发送tc消息1。
	inline bool& t_q_b() { return t_q_b_; }//是否快速发送tc消息2。
	inline int& t_ival_max() { return t_ival_max_; }// added by 周家家 最大tc间隔
	inline int& t_ival_min() { return t_ival_min_; }// added by 周家家 最小tc间隔
	inline int& t_ival_mid() { return t_ival_mid_; }// added by 周家家 默认tc间隔
	inline int& t_ival_1() { return t_ival_1_; }// added by 周家家 上一时刻tc间隔
	inline int& t_ival() { return t_ival_; }// added by 周家家 当前tc间隔
	inline int& t_vtime() { return t_vtime_; }// added by 周家家 拓扑有效时间
        inline int& t_q_1(){ return t_q_1_;};
        inline int& t_q_2() {return t_q_2_;};

	inline linkset_t&	linkset() { return state_.linkset(); }
	inline mprset_t&	mprset() { return state_.mprset(); }
	inline mprselset_t&	mprselset() { return state_.mprselset(); }
	inline nbset_t&		nbset() { return state_.nbset(); }
	inline nb2hopset_t&	nb2hopset() { return state_.nb2hopset(); }
	inline topologyset_t&	topologyset() { return state_.topologyset(); }
	inline dupset_t&	dupset() { return state_.dupset(); }
	inline ifaceassocset_t&	ifaceassocset() { return state_.ifaceassocset(); }

	void		recv_aolsr(Packet*);

	void		mpr_computation();
	void		rtable_computation();

	void		process_hello(AOLSR_msg&, nsaddr_t, nsaddr_t,double); /// added 周家家
	void		process_tc(AOLSR_msg&, nsaddr_t);
	void		process_mid(AOLSR_msg&, nsaddr_t);

	void		forward_default(Packet*, AOLSR_msg&, AOLSR_dup_tuple*, nsaddr_t);
	void		forward_data(Packet*);

	void		enque_msg(AOLSR_msg&, double);
	void		send_hello();
	void		send_tc();
	void		send_mid();
	void		send_pkt();

	void		link_sensing(AOLSR_msg&, nsaddr_t, nsaddr_t,double);///added 周家家
	void		populate_nbset(AOLSR_msg&);
	void		populate_nb2hopset(AOLSR_msg&);
	void		populate_mprselset(AOLSR_msg&);

	void		set_hello_timer();
	void		set_tc_timer();
	void		set_mid_timer();

	void		nb_loss(AOLSR_link_tuple*);
	void		add_dup_tuple(AOLSR_dup_tuple*);
	void		rm_dup_tuple(AOLSR_dup_tuple*);
	void		add_link_tuple(AOLSR_link_tuple*, u_int8_t);
	void		rm_link_tuple(AOLSR_link_tuple*);
	void		updated_link_tuple(AOLSR_link_tuple*);
	void		add_nb_tuple(AOLSR_nb_tuple*);
	void		rm_nb_tuple(AOLSR_nb_tuple*);
	void		add_nb2hop_tuple(AOLSR_nb2hop_tuple*);
	void		rm_nb2hop_tuple(AOLSR_nb2hop_tuple*);
	void		add_mprsel_tuple(AOLSR_mprsel_tuple*);
	void		rm_mprsel_tuple(AOLSR_mprsel_tuple*);
	void		add_topology_tuple(AOLSR_topology_tuple*);
	void		rm_topology_tuple(AOLSR_topology_tuple*);
	void		add_ifaceassoc_tuple(AOLSR_iface_assoc_tuple*);
	void		rm_ifaceassoc_tuple(AOLSR_iface_assoc_tuple*);

	nsaddr_t	get_main_addr(nsaddr_t);
	int		degree(AOLSR_nb_tuple*);

	static bool	seq_num_bigger_than(u_int16_t, u_int16_t);

public:
	AOLSR(nsaddr_t);
	int	command(int, const char*const*);
	void	recv(Packet*, Handler*);
	void	mac_failed(Packet*);

	static double		emf_to_seconds(u_int8_t);
	static u_int8_t		seconds_to_emf(double);
	static int		node_id(nsaddr_t);
	u_int8_t get_link_value(double,double); /// added 周家家
};

#endif
