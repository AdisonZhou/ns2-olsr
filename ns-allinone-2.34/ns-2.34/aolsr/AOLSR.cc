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
 /// \file	AOLSR.cc
 /// \brief	Implementation of AOLSR agent and related classes.
 ///
 /// This is the main file of this software because %AOLSR's behaviour is
 /// implemented here.
 ///

#include <aolsr/AOLSR.h>
#include <aolsr/AOLSR_pkt.h>
#include <aolsr/AOLSR_printer.h>
#include <math.h>
#include <limits.h>
#include <address.h>
#include <ip.h>
#include <cmu-trace.h>
#include <map>

/// Length (in bytes) of UDP header.
#define UDP_HDR_LEN	8

///
/// \brief Function called by MAC layer when cannot deliver a packet.
///
/// \param p Packet which couldn't be delivered.
/// \param arg AOLSR agent passed for a callback.
///
static void
aolsr_mac_failed_callback(Packet *p, void *arg) {
	((AOLSR*)arg)->mac_failed(p);
}


/********** TCL Hooks **********/


int AOLSR_pkt::offset_;
static class AOLSRHeaderClass : public PacketHeaderClass {
public:
	AOLSRHeaderClass() : PacketHeaderClass("PacketHeader/AOLSR", sizeof(AOLSR_pkt)) {
		bind_offset(&AOLSR_pkt::offset_);
	}
} class_rtProtoAOLSR_hdr;

static class AOLSRClass : public TclClass {
public:
	AOLSRClass() : TclClass("Agent/AOLSR") {}
	TclObject* create(int argc, const char*const* argv) {
		// argv has the following structure:
		// <tcl-object> <tcl-object> Agent/AOLSR create-shadow <id>
		// e.g: _o17 _o17 Agent/AOLSR create-shadow 0
		// argv[4] is the address of the node
		assert(argc == 5);
		return new AOLSR((nsaddr_t)Address::instance().str2addr(argv[4]));
	}
} class_rtProtoAOLSR;

///
/// \brief Interface with TCL interpreter.
///
/// From your TCL scripts or shell you can invoke commands on this AOLSR
/// routing agent thanks to this function. Currently you can call "start",
/// "print_rtable", "print_linkset", "print_nbset", "print_nb2hopset",
/// "print_mprset", "print_mprselset" and "print_topologyset" commands.
///
/// \param argc Number of arguments.
/// \param argv Arguments.
/// \return TCL_OK or TCL_ERROR.
///
int
AOLSR::command(int argc, const char*const* argv) {
	if (argc == 2) {
		// Starts all timers
		if (strcasecmp(argv[1], "start") == 0) {
			hello_timer_.resched(0.0);
			tc_timer_.resched(0.0);
			mid_timer_.resched(0.0);

			return TCL_OK;
		}
		// Prints routing table
		else if (strcasecmp(argv[1], "print_rtable") == 0) {
			if (logtarget_ != NULL) {
				sprintf(logtarget_->pt_->buffer(), "P %f _%d_ Routing Table",
					CURRENT_TIME,
					AOLSR::node_id(ra_addr()));
				logtarget_->pt_->dump();
				rtable_.print(logtarget_);
			}
			else {
				fprintf(stdout, "%f _%d_ If you want to print this routing table "
					"you must create a trace file in your tcl script",
					CURRENT_TIME,
					AOLSR::node_id(ra_addr()));
			}
			return TCL_OK;
		}
		// Prints link set
		else if (strcasecmp(argv[1], "print_linkset") == 0) {
			if (logtarget_ != NULL) {
				sprintf(logtarget_->pt_->buffer(), "P %f _%d_ Link Set",
					CURRENT_TIME,
					AOLSR::node_id(ra_addr()));
				logtarget_->pt_->dump();
				AOLSR_printer::print_linkset(logtarget_, linkset());
			}
			else {
				fprintf(stdout, "%f _%d_ If you want to print this link set "
					"you must create a trace file in your tcl script",
					CURRENT_TIME,
					AOLSR::node_id(ra_addr()));
			}
			return TCL_OK;
		}
		// Prints neighbor set
		else if (strcasecmp(argv[1], "print_nbset") == 0) {
			if (logtarget_ != NULL) {
				sprintf(logtarget_->pt_->buffer(), "P %f _%d_ Neighbor Set",
					CURRENT_TIME,
					AOLSR::node_id(ra_addr()));
				logtarget_->pt_->dump();
				AOLSR_printer::print_nbset(logtarget_, nbset());
			}
			else {
				fprintf(stdout, "%f _%d_ If you want to print this neighbor set "
					"you must create a trace file in your tcl script",
					CURRENT_TIME,
					AOLSR::node_id(ra_addr()));
			}
			return TCL_OK;
		}
		// Prints 2-hop neighbor set
		else if (strcasecmp(argv[1], "print_nb2hopset") == 0) {
			if (logtarget_ != NULL) {
				sprintf(logtarget_->pt_->buffer(), "P %f _%d_ Neighbor2hop Set",
					CURRENT_TIME,
					AOLSR::node_id(ra_addr()));
				logtarget_->pt_->dump();
				AOLSR_printer::print_nb2hopset(logtarget_, nb2hopset());
			}
			else {
				fprintf(stdout, "%f _%d_ If you want to print this neighbor2hop set "
					"you must create a trace file in your tcl script",
					CURRENT_TIME,
					AOLSR::node_id(ra_addr()));
			}
			return TCL_OK;
		}
		// Prints MPR set
		else if (strcasecmp(argv[1], "print_mprset") == 0) {
			if (logtarget_ != NULL) {
				sprintf(logtarget_->pt_->buffer(), "P %f _%d_ MPR Set",
					CURRENT_TIME,
					AOLSR::node_id(ra_addr()));
				logtarget_->pt_->dump();
				AOLSR_printer::print_mprset(logtarget_, mprset());
			}
			else {
				fprintf(stdout, "%f _%d_ If you want to print this mpr set "
					"you must create a trace file in your tcl script",
					CURRENT_TIME,
					AOLSR::node_id(ra_addr()));
			}
			return TCL_OK;
		}
		// Prints MPR selector set
		else if (strcasecmp(argv[1], "print_mprselset") == 0) {
			if (logtarget_ != NULL) {
				sprintf(logtarget_->pt_->buffer(), "P %f _%d_ MPR Selector Set",
					CURRENT_TIME,
					AOLSR::node_id(ra_addr()));
				logtarget_->pt_->dump();
				AOLSR_printer::print_mprselset(logtarget_, mprselset());
			}
			else {
				fprintf(stdout, "%f _%d_ If you want to print this mpr selector set "
					"you must create a trace file in your tcl script",
					CURRENT_TIME,
					AOLSR::node_id(ra_addr()));
			}
			return TCL_OK;
		}
		// Prints topology set
		else if (strcasecmp(argv[1], "print_topologyset") == 0) {
			if (logtarget_ != NULL) {
				sprintf(logtarget_->pt_->buffer(), "P %f _%d_ Topology Set",
					CURRENT_TIME,
					AOLSR::node_id(ra_addr()));
				logtarget_->pt_->dump();
				AOLSR_printer::print_topologyset(logtarget_, topologyset());
			}
			else {
				fprintf(stdout, "%f _%d_ If you want to print this topology set "
					"you must create a trace file in your tcl script",
					CURRENT_TIME,
					AOLSR::node_id(ra_addr()));
			}
			return TCL_OK;
		}
	}
	else if (argc == 3) {
		// Obtains the corresponding dmux to carry packets to upper layers
		if (strcmp(argv[1], "port-dmux") == 0) {
			dmux_ = (PortClassifier*)TclObject::lookup(argv[2]);
			if (dmux_ == NULL) {
				fprintf(stderr, "%s: %s lookup of %s failed\n", __FILE__, argv[1], argv[2]);
				return TCL_ERROR;
			}
			return TCL_OK;
		}
		// Obtains the corresponding tracer
		else if (strcmp(argv[1], "log-target") == 0 || strcmp(argv[1], "tracetarget") == 0) {
			logtarget_ = (Trace*)TclObject::lookup(argv[2]);
			if (logtarget_ == NULL)
				return TCL_ERROR;
			return TCL_OK;
		}
		else if (strcmp(argv[1], "cl-mac") == 0){
		     clmac = (Mac802_11 *) TclObject::lookup(argv[2]);
		     if (clmac == 0){
		         fprintf(stderr, "MESPAgent: %s lookup %s failed.\n", argv[1], argv[2]);
		         return TCL_ERROR;
		     }
		     printf("Get Node mac bss_id:%d \n", clmac->bss_id());
		     return TCL_OK;
                }
               else if (strcmp(argv[1], "add_ll") == 0){
                        TclObject *obj;
                        obj=TclObject::lookup(argv[2]);
                         if (obj==NULL)
                            return TCL_ERROR;
                         ll=(NsObject*)obj;
                         return TCL_OK;
                }
               else if (strcmp(argv[1], "add_ifq") == 0){
                       TclObject *obj;
                       obj=TclObject::lookup(argv[2]);
                       if(obj==NULL)
                           return TCL_ERROR;
                       ifq=(CMUPriQueue*)obj;
                       return TCL_OK;
                }
        }
	// Pass the command up to the base class
	return Agent::command(argc, argv);
}


/********** Timers **********/


///
/// \brief Sends a HELLO message and reschedules the HELLO timer.
/// \param e The event which has expired.
///
void
AOLSR_HelloTimer::expire(Event* e) {
	agent_->send_hello();
int n=agent_->clmac-> getqlen();
//n=agent_->ifq->prq_length();
//printf("222length :%d\n",n);
	//\D6\D8\D0¼\C6\CB\E3htime \D2Լ\B0ival_hello
	agent_->MS2() = agent_->MS1();//added \D6ܼҼ\D2
	agent_->MS1() = agent_->MS();//added \D6ܼҼ\D2
	agent_->h_ival_2() = agent_->h_ival_1();//added \D6ܼҼ\D2
	agent_->h_ival_1() = agent_->hello_ival();//added \D6ܼҼ\D2
	double DI = 0.7*agent_->MS1() / agent_->h_ival_1() + 0.3*agent_->MS2() / agent_->h_ival_2();
	printf("DI=%f ",DI);
if (DI>50)
DI=DI/100;
else if (DI>20)
DI=DI/40;
else if (DI>10)
DI=DI/20;
else if(DI>5)
DI=DI/10;
else if (DI>3)
DI=DI/6;
/*else if (DI>2)
DI=DI/4;
else if (DI>1)
DI=DI/1.5;*/
	if (DI >= 0 && DI <= 1)
	{
		double temp = agent_->h_ival_max() + (agent_->h_ival_min() - agent_->h_ival_max())*DI;
		//int n = round(temp);
		agent_->h_ival_current() =temp;

	}
	else
	{
		agent_->h_ival_current() = agent_->h_ival_min();
	}

//// zjj here
//agent_->h_ival_current()=2;
/// zjj here 
	agent_->vtime() = 3 * agent_->h_ival_current();
	agent_->MS()=0;//added \D6ܼҼ\D2
       // printf("%lf",DI);
       printf("%f\n",agent_->h_ival_current());


	agent_->set_hello_timer();
}

///
/// \brief Sends a TC message (if there exists any MPR selector) and reschedules the TC timer.
/// \param e The event which has expired.
///
void
AOLSR_TcTimer::expire(Event* e) {
	if (agent_->mprselset().size() > 0)
		agent_->send_tc();
	if (agent_->t_q_1()>1)
	{
		agent_->t_ival_1() = agent_->t_ival(); // added \D6ܼҼ\D2
		agent_->t_ival() = agent_->t_ival_min();// added \D6ܼҼ\D2
		agent_->t_vtime() = 3 * agent_->t_ival_min();// added \D6ܼҼ\D2
                agent_->t_q_1()=0;
                agent_->t_q_2()=0;
	}
	else if (agent_->t_q_2()>1)
	{
		if (agent_->t_ival_1() == agent_->t_ival_max())
		{
			agent_->t_ival_1() = agent_->t_ival();// added \D6ܼҼ\D2
			agent_->t_ival() = agent_->t_ival_min();// added \D6ܼҼ\D2
			agent_->t_vtime() = 3 * agent_->t_ival_min();// added \D6ܼҼ\D2
                        agent_->t_q_1()=0;
                        agent_->t_q_2()=0;
		}
		else
		{
			if (agent_->t_ival_1() == agent_->t_ival_min())
			{
				agent_->t_ival_1() = agent_->t_ival();// added \D6ܼҼ\D2
				agent_->t_ival() = agent_->t_ival_mid();// added \D6ܼҼ\D2
				agent_->t_vtime() = 3 * agent_->t_ival_mid();// added \D6ܼҼ\D2
                                agent_->t_q_1()=0;
                                agent_->t_q_2()=0;
			}
			else
			{
				agent_->t_ival_1() = agent_->t_ival();// added \D6ܼҼ\D2
				agent_->t_ival() = agent_->t_ival_max();// added \D6ܼҼ\D2
				agent_->t_vtime() = 3 * agent_->t_ival_max();// added \D6ܼҼ\D2
                                agent_->t_q_1()=0;
                                agent_->t_q_2()=0;
		 	}
		}
	}
	else
	{
		if (agent_->t_ival_1() == agent_->t_ival_min())
		{
			agent_->t_ival_1() = agent_->t_ival();// added \D6ܼҼ\D2
			agent_->t_ival() = agent_->t_ival_mid();// added \D6ܼҼ\D2
			agent_->t_vtime() = 3 * agent_->t_ival_mid();// added \D6ܼҼ\D2
                        agent_->t_q_1()=0;
                        agent_->t_q_2()=0;
		}
		else
		{
			agent_->t_ival_1() = agent_->t_ival();// added \D6ܼҼ\D2
			agent_->t_ival() = agent_->t_ival_max();// added \D6ܼҼ\D2
			agent_->t_vtime() = 3 * agent_->t_ival_max();// added \D6ܼҼ\D2
                        agent_->t_q_1()=0;
                        agent_->t_q_2()=0;
		}
	}
/// zjj here
       // agent_->t_ival() =5;// added \D6ܼҼ\D2
	//agent_->t_vtime() =15;// added \D6ܼҼ\D2
      // printf("TC=%d",agent_->t_ival());
/// zjj here
        printf("TC_time=%d\n",agent_->t_ival());
	agent_->set_tc_timer();
}

///
/// \brief Sends a MID message (if the node has more than one interface) and resets the MID timer.
/// \warning Currently it does nothing because there is no support for multiple interfaces.
/// \param e The event which has expired.
///
void
AOLSR_MidTimer::expire(Event* e) {
#ifdef MULTIPLE_IFACES_SUPPORT
	agent_->send_mid();
	agent_->set_mid_timer();
#endif
}

///
/// \brief Removes tuple_ if expired. Else timer is rescheduled to expire at tuple_->time().
///
/// The task of actually removing the tuple is left to the AOLSR agent.
///
/// \param e The event which has expired.
///
void
AOLSR_DupTupleTimer::expire(Event* e) {
	if (tuple_->time() < CURRENT_TIME) {
		agent_->rm_dup_tuple(tuple_);
		delete tuple_;
		delete this;
	}
	else
		resched(DELAY(tuple_->time()));
}

///
/// \brief Removes tuple_ if expired. Else if symmetric time
/// has expired then it is assumed a neighbor loss and agent_->nb_loss()
/// is called. In this case the timer is rescheduled to expire at
/// tuple_->time(). Otherwise the timer is rescheduled to expire at
/// the minimum between tuple_->time() and tuple_->sym_time().
///
/// The task of actually removing the tuple is left to the AOLSR agent.
///
/// \param e The event which has expired.
///
void
AOLSR_LinkTupleTimer::expire(Event* e) {
	double now = CURRENT_TIME;

	if (tuple_->time() < now) {
		agent_->rm_link_tuple(tuple_);
		delete tuple_;
		delete this;
	}
	else if (tuple_->sym_time() < now) {
		if (first_time_)
			first_time_ = false;
		else
		{
			agent_->nb_loss(tuple_);
			agent_->t_q_2() =agent_->t_q_2()+1;//added \D6ܼҼ\D2
		}
		resched(DELAY(tuple_->time()));
	}
	else
		resched(DELAY(MIN(tuple_->time(), tuple_->sym_time())));
}

///
/// \brief Removes tuple_ if expired. Else the timer is rescheduled to expire at tuple_->time().
///
/// The task of actually removing the tuple is left to the AOLSR agent.
///
/// \param e The event which has expired.
///
void
AOLSR_Nb2hopTupleTimer::expire(Event* e) {
	if (tuple_->time() < CURRENT_TIME) {
		agent_->rm_nb2hop_tuple(tuple_);
		delete tuple_;
		delete this;
	}
	else
		resched(DELAY(tuple_->time()));
}

///
/// \brief Removes tuple_ if expired. Else the timer is rescheduled to expire at tuple_->time().
///
/// The task of actually removing the tuple is left to the AOLSR agent.
///
/// \param e The event which has expired.
///
void
AOLSR_MprSelTupleTimer::expire(Event* e) {
	if (tuple_->time() < CURRENT_TIME) {
		agent_->rm_mprsel_tuple(tuple_);
		delete tuple_;
		delete this;
	}
	else
		resched(DELAY(tuple_->time()));
}

///
/// \brief Removes tuple_ if expired. Else the timer is rescheduled to expire at tuple_->time().
///
/// The task of actually removing the tuple is left to the AOLSR agent.
///
/// \param e The event which has expired.
///
void
AOLSR_TopologyTupleTimer::expire(Event* e) {
	if (tuple_->time() < CURRENT_TIME) {
		agent_->rm_topology_tuple(tuple_);
		delete tuple_;
		delete this;
	}
	else
		resched(DELAY(tuple_->time()));
}

///
/// \brief Removes tuple_ if expired. Else timer is rescheduled to expire at tuple_->time().
/// \warning Actually this is never invoked because there is no support for multiple interfaces.
/// \param e The event which has expired.
///
void
AOLSR_IfaceAssocTupleTimer::expire(Event* e) {
	if (tuple_->time() < CURRENT_TIME) {
		agent_->rm_ifaceassoc_tuple(tuple_);
		delete tuple_;
		delete this;
	}
	else
		resched(DELAY(tuple_->time()));
}

///
/// \brief Sends a control packet which must bear every message in the AOLSR agent's buffer.
///
/// The task of actually sending the packet is left to the AOLSR agent.
///
/// \param e The event which has expired.
///
void
AOLSR_MsgTimer::expire(Event* e) {
	agent_->send_pkt();
	delete this;
}


/********** AOLSR class **********/


///
/// \brief Creates necessary timers, binds TCL-available variables and do
/// some more initializations.\B4\B4\BD\A8\B1\D8Ҫ\B5ļ\C6ʱ\C6\F7\A3\AC\B0\F3\B6\A8TCL\BF\C9\D3ñ\E4\C1\BF\B2\A2\BD\F8\D0и\FC\B6\E0\B3\F5ʼ\BB\AF\A1\A3
/// \param id Identifier for the AOLSR agent. It will be used as the address
/// of this routing agent.
///
AOLSR::AOLSR(nsaddr_t id) : Agent(PT_AOLSR),
hello_timer_(this),
tc_timer_(this),
mid_timer_(this) {

	// Enable usage of some of the configuration variables from Tcl.
	//\C6\F4\D3ö\D4Tcl\D6\D0ĳЩ\C5\E4\D6ñ\E4\C1\BF\B5\C4ʹ\D3á\A3
	// Note: Do NOT change the values of these variables in the constructor
	// after binding them! The desired default values should be set in
	// ns-X.XX/tcl/lib/ns-default.tcl instead.
	bind("willingness_", &willingness_);
	bind("hello_ival_", &hello_ival_);
	bind("tc_ival_", &tc_ival_); // \D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC\D6\DC
	bind("mid_ival_", &mid_ival_);
	bind_bool("use_mac_", &use_mac_);

	// Do some initializations
	ra_addr_ = id;
	pkt_seq_ = AOLSR_MAX_SEQ_NUM;
	msg_seq_ = AOLSR_MAX_SEQ_NUM;
	ansn_ = AOLSR_MAX_SEQ_NUM;
       Hmax_ = 0; /// added by ÖÜŒÒŒÒ
	Hmin_ = 10; /// added by ÖÜŒÒŒÒ
	 Pmax_ = -0.001; /// added by ÖÜŒÒŒÒ
	 Pmin_ = 1000; /// added by ÖÜŒÒŒÒ
	// ÒÆ¶¯µÃ·Ö£¬×îœü2žöHELLOÖÜÆÚµÄÒÆ¶¯µÃ·Ö¡£
	 MS1_ = 0; /// added by ÖÜŒÒŒÒ
	 MS2_ = 0; /// added by ÖÜŒÒŒÒ
	 MS_ = 0;/// added by ÖÜŒÒŒÒ Í³ŒÆµ±Ç°ÖÜÆÚµÄµÃ·Ö
	 h_ival_1_ = 2;/// added by ÖÜŒÒŒÒ µ±Ç°µÄhelloŒäžô
	 h_ival_2_ = 2;/// added by ÖÜŒÒŒÒ ÉÏÒ»Ê±¿ÌµÄhelloŒäžô
	/// helloÏûÏ¢·¢ËÍŒäžôµÄ×îŽóÖµ£¬×îÐ¡Öµ£¬ÒÔŒ°Ä¬ÈÏÖµ
	h_ival_max_ = 4;/// added by ÖÜŒÒŒÒ
	h_ival_min_ = 1.5;/// added by ÖÜŒÒŒÒ
	h_ival_mid_ = 2;/// added by ÖÜŒÒŒÒ
	h_ival_current_ = 2;/// added by ÖÜŒÒŒÒ
	///hello ÏûÏ¢ÓÐÐ§Ê±Œä
	vtime_ = 6;/// added by ÖÜŒÒŒÒ
	t_q_a_ = false;//ÊÇ·ñ¿ìËÙ·¢ËÍtcÏûÏ¢1¡£
	t_q_b_ = false;//ÊÇ·ñ¿ìËÙ·¢ËÍtcÏûÏ¢2¡£
        t_q_1_=0;
        t_q_2_=0;
	 t_ival_max_ = 6;// added by ÖÜŒÒŒÒ ×îŽótcŒäžô
	 t_ival_min_ = 4;// added by ÖÜŒÒŒÒ ×îÐ¡tcŒäžô
	t_ival_mid_ = 5;// added by ÖÜŒÒŒÒ Ä¬ÈÏtcŒäžô
	t_ival_1_ = 5;// added by ÖÜŒÒŒÒ ÉÏÒ»Ê±¿ÌtcŒäžô
	t_ival_ = 5;// added by ÖÜŒÒŒÒ µ±Ç°tcŒäžô
	t_vtime_ = 15;// added by ÖÜŒÒŒÒ ÍØÆËÓÐÐ§Ê±Œä

      

}

///
/// \brief	This function is called whenever a packet is received. It identifies
///		the type of the received packet and process it accordingly.
///
/// If it is an %AOLSR packet then it is processed. In other case, if it is a data packet
/// then it is forwarded.
///
/// \param	p the received packet.
/// \param	h a handler (not used).
///
void
AOLSR::recv(Packet* p, Handler* h) {
	struct hdr_cmn* ch = HDR_CMN(p);
	struct hdr_ip* ih = HDR_IP(p);

	if (ih->saddr() == ra_addr()) {
		// If there exists a loop, must drop the packet
		if (ch->num_forwards() > 0) {
			drop(p, DROP_RTR_ROUTE_LOOP);
			return;
		}
		// else if this is a packet I am originating, must add IP header 
		// \C8\E7\B9\FB\D5\E2\CA\C7\CEҷ\A2\B3\F6\B5\C4\CA\FD\BEݰ\FC\A3\AC\D4\F2\B1\D8\D0\EB\CC\ED\BC\D3IP\B1\EAͷ
		else if (ch->num_forwards() == 0)
			ch->size() += IP_HDR_LEN;
	}

	// If it is an AOLSR packet, must process it
	if (ch->ptype() == PT_AOLSR)
		recv_aolsr(p);
	// Otherwise, must forward the packet (unless TTL has reached zero)
	else {
		ih->ttl_--;
		if (ih->ttl_ == 0) {
			drop(p, DROP_RTR_TTL);
			return;
		}
		forward_data(p);
	}
}

///
/// \brief Processes an incoming %AOLSR packet following RFC 3626 specification.
/// \param p received packet.
///
void
AOLSR::recv_aolsr(Packet* p) {
	struct hdr_ip* ih = HDR_IP(p);
	AOLSR_pkt* op = PKT_AOLSR(p);

	double recv_power = 10*log10(p->txinfo_.RxPr)+100; // added by \D6ܼҼ\D2 \BD\D3\CAյ\BD\CA\FD\BEݰ\FC\C4\DC\C1\BFǿ\B6ȡ\A3
	if (recv_power < 0)
	{
		recv_power = 0; //added \D6ܼҼ\D2
	}
	else if (recv_power > 255)
	{
		recv_power = 255;// added \D6ܼҼ\D2
	}
	Pmax() = MAX(Pmax(), recv_power);/// added by \D6ܼҼ\D2
	Pmin() = MIN(Pmin(), recv_power);/// added by \D6ܼҼ\D2
	// All routing messages are sent from and to port RT_PORT,
	// so we check it.
	assert(ih->sport() == RT_PORT);
	assert(ih->dport() == RT_PORT);

	// If the packet contains no messages must be silently discarded.
	// There could exist a message with an empty body, so the size of
	// the packet would be pkt-hdr-size + msg-hdr-size.
	if (op->pkt_len() < AOLSR_PKT_HDR_SIZE + AOLSR_MSG_HDR_SIZE) {
		Packet::free(p);
		return;
	}

	assert(op->count >= 0 && op->count <= AOLSR_MAX_MSGS);
	for (int i = 0; i < op->count; i++) {
		AOLSR_msg& msg = op->msg(i);

		// If ttl is less than or equal to zero, or
		// the receiver is the same as the originator,
		// the message must be silently dropped
		if (msg.ttl() <= 0 || msg.orig_addr() == ra_addr())
			continue;

		// If the message has been processed it must not be
		// processed again
		bool do_forwarding = true;
		AOLSR_dup_tuple* duplicated = state_.find_dup_tuple(msg.orig_addr(), msg.msg_seq_num());
		if (duplicated == NULL) {
			// Process the message according to its type
			if (msg.msg_type() == AOLSR_HELLO_MSG)
				process_hello(msg, ra_addr(), ih->saddr(), recv_power); /// added \D6ܼҼ\D2
			else if (msg.msg_type() == AOLSR_TC_MSG)
				process_tc(msg, ih->saddr());
			else if (msg.msg_type() == AOLSR_MID_MSG)
				process_mid(msg, ih->saddr());
			else {
				debug("%f: Node %d can not process AOLSR packet because does not "
					"implement AOLSR type (%x)\n",
					CURRENT_TIME,
					AOLSR::node_id(ra_addr()),
					msg.msg_type());
			}
		}
		else {
			// If the message has been considered for forwarding, it should
			// not be retransmitted again
			for (addr_list_t::iterator it = duplicated->iface_list().begin();
				it != duplicated->iface_list().end();
				it++) {
				if (*it == ra_addr()) {
					do_forwarding = false;
					break;
				}
			}
		}

		if (do_forwarding) {
			// HELLO messages are never forwarded.
			// TC and MID messages are forwarded using the default algorithm.
			// Remaining messages are also forwarded using the default algorithm.
			if (msg.msg_type() != AOLSR_HELLO_MSG)
				forward_default(p, msg, duplicated, ra_addr());
		}

	}

	// After processing all AOLSR messages, we must recompute routing table
	rtable_computation();    //\BC\C6\CB\E3·\D3ɱ\ED

	// Release resources
	Packet::free(p);    ///\CAͷ\C5\D7\CAԴ
}

///
/// \brief Computates MPR set of a node following RFC 3626 hints.
/// \BC\C6\CB\E3MPR\BC\AF\BA\CF
void
AOLSR::mpr_computation() {
	// MPR computation should be done for each interface. See section 8.3.1
	// (RFC 3626) for details.

	state_.clear_mprset();

	nbset_t N; nb2hopset_t N2;
	// N is the subset of neighbors of the node, which are
	// neighbor "of the interface I"
	for (nbset_t::iterator it = nbset().begin(); it != nbset().end(); it++)
		if ((*it)->status() == AOLSR_STATUS_SYM) // I think that we need this check
			N.push_back(*it);///һ\CC\F8\C1ھӼ\AF\BA\CF

	// N2 is the set of 2-hop neighbors reachable from "the interface
	// I", excluding: N2\CAǴӡ\B0\BDӿ\DAI\A1\B1\BFɵ\BD\B4\EF\B5\C42\CC\F8\C1ھӼ\AF\BAϣ\AC\B2\BB\B0\FC\C0\A8\A3\BA
	// (i)   the nodes only reachable by members of N with willingness WILL_NEVER
	/// ֻ\D3\D0N\B5ĳ\C9Ա\D2\E2ԸΪWILL_NEVER\B2\C5\C4ܵ\BD\B4\EF\B5Ľڵ\E3
	// (ii)  the node performing the computation
	/// \B5\B1ǰ\D4ڼ\C6\CB\E3\B5Ľڵ\E3
	// (iii) all the symmetric neighbors: the nodes for which there exists a symmetric
	//       link to this node on some interface.
	/// \CB\F9\D3жԳ\C6\C1ھӣ\BA\D4\DAĳ\B8\F6\BDӿ\DA\C9\CF\D3\EB\C6\E4\B4\E6\D4ڶԳ\C6\C1\B4\BDӵĽڵ㡣
	for (nb2hopset_t::iterator it = nb2hopset().begin(); it != nb2hopset().end(); it++) {
		AOLSR_nb2hop_tuple* nb2hop_tuple = *it;
		bool ok = true;
		AOLSR_nb_tuple* nb_tuple = state_.find_sym_nb_tuple(nb2hop_tuple->nb_main_addr());
		if (nb_tuple == NULL)
			ok = false;
		else {
			nb_tuple = state_.find_nb_tuple(nb2hop_tuple->nb_main_addr(), AOLSR_WILL_NEVER);
			if (nb_tuple != NULL)
				ok = false;
			else {
				nb_tuple = state_.find_sym_nb_tuple(nb2hop_tuple->nb2hop_addr());
				if (nb_tuple != NULL)
					ok = false;
			}
		}

		if (ok)
			N2.push_back(nb2hop_tuple);///2\CC\F8\C1ھӼ\AF\BA\CF
	}

	// 1. Start with an MPR set made of all members of N with
	// N_willingness equal to WILL_ALWAYS
	for (nbset_t::iterator it = N.begin(); it != N.end(); it++) {
		AOLSR_nb_tuple* nb_tuple = *it;
		if (nb_tuple->willingness() == AOLSR_WILL_ALWAYS)
			state_.insert_mpr_addr(nb_tuple->nb_main_addr());
	}

	// 2. Calculate D(y), where y is a member of N, for all nodes in N.
	// We will do this later.

	// 3. Add to the MPR set those nodes in N, which are the *only*
	// nodes to provide reachability to a node in N2. Remove the
	// nodes from N2 which are now covered by a node in the MPR set.
	mprset_t foundset;
	std::set<nsaddr_t> deleted_addrs;
	for (nb2hopset_t::iterator it = N2.begin(); it != N2.end(); it++) {
		AOLSR_nb2hop_tuple* nb2hop_tuple1 = *it;

		mprset_t::iterator pos = foundset.find(nb2hop_tuple1->nb2hop_addr());
		if (pos != foundset.end())
			continue;

		bool found = false;
		for (nbset_t::iterator it2 = N.begin(); it2 != N.end(); it2++) {
			if ((*it2)->nb_main_addr() == nb2hop_tuple1->nb_main_addr()) {
				found = true;
				break;
			}
		}
		if (!found)
			continue;

		found = false;
		for (nb2hopset_t::iterator it2 = it + 1; it2 != N2.end(); it2++) {
			AOLSR_nb2hop_tuple* nb2hop_tuple2 = *it2;
			if (nb2hop_tuple1->nb2hop_addr() == nb2hop_tuple2->nb2hop_addr()) {
				foundset.insert(nb2hop_tuple1->nb2hop_addr());// \C1\BD\CC\F8\BDڵ\E3\BF\C9\D3ɶ\E0\CC\F5·\BE\B6\B5\BD\B4\D4\DAfound\BC\AF\BA\CF\D6м\D3\C8\EB\B8ýڵ\E3\B5\D8ַ\A1\A3
				found = true;// \CC\F8\B3\F6ѭ\BB\B7\A1\A3
				break;
			}
		}
		if (!found) {
			state_.insert_mpr_addr(nb2hop_tuple1->nb_main_addr());

			for (nb2hopset_t::iterator it2 = it + 1; it2 != N2.end(); it2++) {
				AOLSR_nb2hop_tuple* nb2hop_tuple2 = *it2;
				if (nb2hop_tuple1->nb_main_addr() == nb2hop_tuple2->nb_main_addr()) {
					deleted_addrs.insert(nb2hop_tuple2->nb2hop_addr());
					it2 = N2.erase(it2);
					it2--;
				}
			}
			it = N2.erase(it);
			it--;
		}

		for (std::set<nsaddr_t>::iterator it2 = deleted_addrs.begin();
			it2 != deleted_addrs.end();
			it2++) {
			for (nb2hopset_t::iterator it3 = N2.begin();
				it3 != N2.end();
				it3++) {
				if ((*it3)->nb2hop_addr() == *it2) {
					it3 = N2.erase(it3);
					it3--;
					// I have to reset the external iterator because it
					// may have been invalidated by the latter deletion
					it = N2.begin();
					it--;
				}
			}
		}
		deleted_addrs.clear();
	}

	// 4. While there exist nodes in N2 which are not covered by at
	// least one node in the MPR set:
	while (N2.begin() != N2.end()) {
		// 4.1. For each node in N, calculate the reachability, i.e., the
		// number of nodes in N2 which are not yet covered by at
		// least one node in the MPR set, and which are reachable
		// through this 1-hop neighbor
		map<int, std::vector<AOLSR_nb_tuple*> > reachability;
		set<int> rs;
		for (nbset_t::iterator it = N.begin(); it != N.end(); it++) {
			AOLSR_nb_tuple* nb_tuple = *it;
			int r = 0;
			for (nb2hopset_t::iterator it2 = N2.begin(); it2 != N2.end(); it2++) {
				AOLSR_nb2hop_tuple* nb2hop_tuple = *it2;
				if (nb_tuple->nb_main_addr() == nb2hop_tuple->nb_main_addr())
					r++;
			}
			rs.insert(r);
			reachability[r].push_back(nb_tuple);
		}

		// 4.2. Select as a MPR the node with highest N_willingness among
		// the nodes in N with non-zero reachability. In case of
		// multiple choice select the node which provides
		// reachability to the maximum number of nodes in N2. In
		// case of multiple nodes providing the same amount of
		// reachability, select the node as MPR whose D(y) is
		// greater. Remove the nodes from N2 which are now covered
		// by a node in the MPR set.
		AOLSR_nb_tuple* max = NULL;
		int max_r = 0;
		for (set<int>::iterator it = rs.begin(); it != rs.end(); it++) {
			int r = *it;
			if (r > 0) {
				for (std::vector<AOLSR_nb_tuple*>::iterator it2 = reachability[r].begin();
					it2 != reachability[r].end();
					it2++) {
					AOLSR_nb_tuple* nb_tuple = *it2;
					if (max == NULL || nb_tuple->willingness() > max->willingness()) {
						max = nb_tuple;
						max_r = r;
					}
					else if (nb_tuple->willingness() == max->willingness()) {
						if (r > max_r) {
							max = nb_tuple;
							max_r = r;
						}
						else if (r == max_r) {
							if (degree(nb_tuple) > degree(max)) {
								max = nb_tuple;
								max_r = r;
							}
						}
					}
				}
			}
		}
		if (max != NULL) {
			state_.insert_mpr_addr(max->nb_main_addr());
			std::set<nsaddr_t> nb2hop_addrs;
			for (nb2hopset_t::iterator it = N2.begin(); it != N2.end(); it++) {
				AOLSR_nb2hop_tuple* nb2hop_tuple = *it;
				if (nb2hop_tuple->nb_main_addr() == max->nb_main_addr()) {
					nb2hop_addrs.insert(nb2hop_tuple->nb2hop_addr());
					it = N2.erase(it);
					it--;
				}
			}
			for (nb2hopset_t::iterator it = N2.begin(); it != N2.end(); it++) {
				AOLSR_nb2hop_tuple* nb2hop_tuple = *it;
				std::set<nsaddr_t>::iterator it2 =
					nb2hop_addrs.find(nb2hop_tuple->nb2hop_addr());
				if (it2 != nb2hop_addrs.end()) {
					it = N2.erase(it);
					it--;
				}
			}
		}
	}
}

///
/// \brief Creates the routing table of the node following RFC 3626 hints.
///  \BC\C6\CB\E3·\D3ɱ\ED
void
AOLSR::rtable_computation() {
	// 1.\C7\E5\B3\FD·\D3ɱ\ED\CC\F5Ŀ
	rtable_.clear();
// 2. The new routing entries are added starting with the
	// symmetric neighbors (h=1) as the destination nodes.
		for (nbset_t::iterator it = nbset().begin(); it != nbset().end(); it++) {
		AOLSR_nb_tuple* nb_tuple = *it;
		if (nb_tuple->status() == AOLSR_STATUS_SYM) {
			bool nb_main_addr = false;
			AOLSR_link_tuple* lt = NULL;
			for (linkset_t::iterator it2 = linkset().begin(); it2 != linkset().end(); it2++) {
				AOLSR_link_tuple* link_tuple = *it2;
				if (get_main_addr(link_tuple->nb_iface_addr()) == nb_tuple->nb_main_addr() && link_tuple->time() >= CURRENT_TIME) {
					lt = link_tuple;
					rtable_.add_entry(link_tuple->nb_iface_addr(),
							link_tuple->nb_iface_addr(),
							link_tuple->local_iface_addr(),
							1,
							(u_int32_t)link_tuple->link_score());
					if (link_tuple->nb_iface_addr() == nb_tuple->nb_main_addr())
						nb_main_addr = true;
				}
			}
			if (!nb_main_addr && lt != NULL) {
				rtable_.add_entry(nb_tuple->nb_main_addr(),
						lt->nb_iface_addr(),
						lt->local_iface_addr(),
						1,
						(u_int32_t)lt->link_score());
			}
		}
	}
	
	// N2 is the set of 2-hop neighbors reachable from this node, excluding:
	// (i)   the nodes only reachable by members of N with willingness WILL_NEVER
	// (ii)  the node performing the computation
	// (iii) all the symmetric neighbors: the nodes for which there exists a symmetric
	//       link to this node on some interface.
	for (nb2hopset_t::iterator it = nb2hopset().begin(); it != nb2hopset().end(); it++) {
		AOLSR_nb2hop_tuple* nb2hop_tuple = *it;
		bool ok = true;
		AOLSR_nb_tuple* nb_tuple = state_.find_sym_nb_tuple(nb2hop_tuple->nb_main_addr());
		if (nb_tuple == NULL)
			ok = false;
		else {
			nb_tuple = state_.find_nb_tuple(nb2hop_tuple->nb_main_addr(), AOLSR_WILL_NEVER);
			if (nb_tuple != NULL)
				ok = false;
			else {
				nb_tuple = state_.find_sym_nb_tuple(nb2hop_tuple->nb2hop_addr());
				if (nb_tuple != NULL)
					ok = false;
			}
		}

		// 3. For each node in N2 create a new entry in the routing table
		if (ok) {
			AOLSR_rt_entry* entry = rtable_.lookup(nb2hop_tuple->nb_main_addr());
			assert(entry != NULL);
			u_int32_t score1=entry->rt_cost();
			//score1=score1+nb2hop_tuple->link_score();
			AOLSR_rt_entry* entry1 = rtable_.lookup(nb2hop_tuple->nb2hop_addr());
			if(entry1!=NULL)
                       {
                       u_int32_t score2=entry1->rt_cost();
                        if (score1>score2&&entry1->dist()==1)
                       {
                         rtable_.rm_entry(nb2hop_tuple->nb2hop_addr());
			 rtable_.add_entry(nb2hop_tuple->nb2hop_addr(),
					entry->next_addr(),
					entry->iface_addr(),
					2,score1);
                       }
                       }
                        else 
                        rtable_.add_entry(nb2hop_tuple->nb2hop_addr(),
					entry->next_addr(),
					entry->iface_addr(),
					2,score1);
		}
	}
	
	for (u_int32_t h = 2; ; h++) {
		bool added = false;
		
		/// 对于拓扑表中的每个拓扑条目，如果其T_dest_addr与
		/// 路由表中任何路由条目的R_dest_addr不对应，并且其
		/// T_last_addr与R_dist等于h的路由条目的R_dest_addr
		/// 相对应，则必须记录新的路由条目在路由表中（如果尚不存在）
		for (topologyset_t::iterator it = topologyset().begin();
			it != topologyset().end();
			it++) {
			AOLSR_topology_tuple* topology_tuple = *it;
			//u_int32_t score_=topology_tuple->link_score();
			AOLSR_rt_entry* entry1 = rtable_.lookup(topology_tuple->dest_addr());
			AOLSR_rt_entry* entry2 = rtable_.lookup(topology_tuple->last_addr());
			if (entry1 == NULL && entry2 != NULL && entry2->dist() == h) {
				//score_=score_+entry2->rt_cost();
				rtable_.add_entry(topology_tuple->dest_addr(),
						entry2->next_addr(),
						entry2->iface_addr(),
						h+1,0);
				added = true;
			}
//else if(entry1 != NULL && entry2 != NULL && entry1->dist() == (h+1))
//{
//score_=score_+entry2->rt_cost();
//u_int32_t score2=entry1->rt_cost();
//if (score2<score_)
//{
//printf("%d,%d,\n",score2,score_);
//rtable_.rm_entry(topology_tuple->dest_addr());
//rtable_.add_entry(topology_tuple->dest_addr(),
//						entry2->next_addr(),
//						entry2->iface_addr(),
//						h+1,score_);
//}
//}
		}
		


		if (!added)
			break;
	}


}

///
/// \brief Processes a HELLO message following RFC 3626 specification.
///
/// Link sensing and population of the Neighbor Set, 2-hop Neighbor Set and MPR
/// Selector Set are performed.
///
/// \param msg the %AOLSR message which contains the HELLO message.
/// \param receiver_iface the address of the interface where the message was received from.
/// \param sender_iface the address of the interface where the message was sent from.
///
void
AOLSR::process_hello(AOLSR_msg& msg, nsaddr_t receiver_iface, nsaddr_t sender_iface,double recv_power) {
	assert(msg.msg_type() == AOLSR_HELLO_MSG); /// added \D6ܼҼ\D2
	link_sensing(msg, receiver_iface, sender_iface,recv_power);
	populate_nbset(msg);
	populate_nb2hopset(msg);
	mpr_computation();
	populate_mprselset(msg);
}

///
/// \brief Processes a TC message following RFC 3626 specification.
///
/// The Topology Set is updated (if needed) with the information of
/// the received TC message.
///
/// \param msg the %AOLSR message which contains the TC message.
/// \param sender_iface the address of the interface where the message was sent from.
///
void
AOLSR::process_tc(AOLSR_msg& msg, nsaddr_t sender_iface) {
	assert(msg.msg_type() == AOLSR_TC_MSG);
	double now = CURRENT_TIME;
	AOLSR_tc& tc = msg.tc();

	// 1. If the sender interface of this message is not in the symmetric
	// 1-hop neighborhood of this node, the message MUST be discarded.
	// \B8\C3\CA\FD\BEݰ\FC\B7\C7һ\CC\F8\B6Գ\C6\C1ھӷ\A2\C0\B4\D4\F2\B6\AA\C6\FA
	AOLSR_link_tuple* link_tuple = state_.find_sym_link_tuple(sender_iface, now);
	if (link_tuple == NULL)
		return;

	// 2. If there exist some tuple in the topology set where:
	// 	T_last_addr == originator address AND
	// 	T_seq       >  ANSN,
	// then further processing of this TC message MUST NOT be
	// performed.
	// \D4\DA\CD\D8\C6\CB\D6д\E6\D4\DAһ\B8\F6\CD\D8\C6˽ṹ\C2\FA\D7㣬\B8\C3\CD\D8\C6˵\C4\C9\CFһ\CC\F8\CA\C7\D5\E2\B8\F6\BDڵ\E3\B5\D8ַ\A1\A3
	AOLSR_topology_tuple* topology_tuple =
		state_.find_newer_topology_tuple(msg.orig_addr(), tc.ansn());
	if (topology_tuple != NULL)
		return;

	// 3. All tuples in the topology set where:
	//	T_last_addr == originator address AND
	//	T_seq       <  ANSN
	// MUST be removed from the topology set.
	// ɾ\B3\FD\B2\BB\C2\FA\D7\E3\CC\F5\BC\FE\B5\C4\CD\D8\C6ˡ\A3
	state_.erase_older_topology_tuples(msg.orig_addr(), tc.ansn());

	// 4. For each of the advertised neighbor main address received in
	// the TC message:
	// TC\CF\FBϢ\D6е\C4\C1ھӽڵ\E3\B5\D8ַ
	for (int i = 0; i < tc.count; i++) {
		assert(i >= 0 && i < AOLSR_MAX_ADDRS);
		nsaddr_t addr = tc.nb_main_addr(i);
		//u_int8_t score_t = tc.link_score(i);///added \D6ܼҼ\D2

		// 4.1. If there exist some tuple in the topology set where:
		// 	T_dest_addr == advertised neighbor main address, AND
		// 	T_last_addr == originator address,
		// then the holding time of that tuple MUST be set to:
		// 	T_time      =  current time + validity time.
		// \C8\E7\B9\FB\D2Ѵ\E6\D4ڸ\C3\CD\D8\C6ˣ\AC\D4\F2\B8\FC\D0\C2ʱ\BC\E4\D2Լ\B0\C1\B4·\B5÷\D6
		AOLSR_topology_tuple* topology_tuple =
			state_.find_topology_tuple(addr, msg.orig_addr());
		if (topology_tuple != NULL)
		{
			topology_tuple->time() = now + AOLSR::emf_to_seconds(msg.vtime());
			//topology_tuple->link_score() = score_t;/// added \D6ܼҼ\D2
		}
		// 4.2. Otherwise, a new tuple MUST be recorded in the topology
		// set where:
		//	T_dest_addr = advertised neighbor main address,
		//	T_last_addr = originator address,
		//	T_seq       = ANSN,
		//	T_time      = current time + validity time.
		// \B7\F1\D4\F2\A3\AC\CC\ED\BC\D3\D0µ\C4\CD\D8\C6ˡ\A3
		else {
			AOLSR_topology_tuple* topology_tuple = new AOLSR_topology_tuple;
			topology_tuple->dest_addr() = addr;
			topology_tuple->last_addr() = msg.orig_addr();
			topology_tuple->seq() = tc.ansn();
			topology_tuple->time() = now + AOLSR::emf_to_seconds(msg.vtime());
			//topology_tuple->link_score() = score_t;/// added \D6ܼҼ\D2
			add_topology_tuple(topology_tuple);
			// Schedules topology tuple deletion
			AOLSR_TopologyTupleTimer* topology_timer =
				new AOLSR_TopologyTupleTimer(this, topology_tuple);
			topology_timer->resched(DELAY(topology_tuple->time()));
		}
	}
}

///
/// \brief Processes a MID message following RFC 3626 specification.
///
/// The Interface Association Set is updated (if needed) with the information
/// of the received MID message.
///
/// \param msg the %AOLSR message which contains the MID message.
/// \param sender_iface the address of the interface where the message was sent from.
///
void
AOLSR::process_mid(AOLSR_msg& msg, nsaddr_t sender_iface) {
	assert(msg.msg_type() == AOLSR_MID_MSG);
	double now = CURRENT_TIME;
	AOLSR_mid& mid = msg.mid();

	// 1. If the sender interface of this message is not in the symmetric
	// 1-hop neighborhood of this node, the message MUST be discarded.
	AOLSR_link_tuple* link_tuple = state_.find_sym_link_tuple(sender_iface, now);
	if (link_tuple == NULL)
		return;

	// 2. For each interface address listed in the MID message
	for (int i = 0; i < mid.count; i++) {
		bool updated = false;
		for (ifaceassocset_t::iterator it = ifaceassocset().begin();
			it != ifaceassocset().end();
			it++) {
			AOLSR_iface_assoc_tuple* tuple = *it;
			if (tuple->iface_addr() == mid.iface_addr(i)
				&& tuple->main_addr() == msg.orig_addr()) {
				tuple->time() = now + AOLSR::emf_to_seconds(msg.vtime());
				updated = true;
			}
		}
		if (!updated) {
			AOLSR_iface_assoc_tuple* tuple = new AOLSR_iface_assoc_tuple;
			tuple->iface_addr() = msg.mid().iface_addr(i);
			tuple->main_addr() = msg.orig_addr();
			tuple->time() = now + AOLSR::emf_to_seconds(msg.vtime());
			add_ifaceassoc_tuple(tuple);
			// Schedules iface association tuple deletion
			AOLSR_IfaceAssocTupleTimer* ifaceassoc_timer =
				new AOLSR_IfaceAssocTupleTimer(this, tuple);
			ifaceassoc_timer->resched(DELAY(tuple->time()));
		}
	}
}

///
/// \brief AOLSR's default forwarding algorithm.
///
/// See RFC 3626 for details.
///
/// \param p the %AOLSR packet which has been received.
/// \param msg the %AOLSR message which must be forwarded.
/// \param dup_tuple NULL if the message has never been considered for forwarding,
/// or a duplicate tuple in other case.
/// \param local_iface the address of the interface where the message was received from.
///
void
AOLSR::forward_default(Packet* p, AOLSR_msg& msg, AOLSR_dup_tuple* dup_tuple, nsaddr_t local_iface) {
	double now = CURRENT_TIME;
	struct hdr_ip* ih = HDR_IP(p);

	// If the sender interface address is not in the symmetric
	// 1-hop neighborhood the message must not be forwarded
	AOLSR_link_tuple* link_tuple = state_.find_sym_link_tuple(ih->saddr(), now);
	if (link_tuple == NULL)
		return;

	// If the message has already been considered for forwarding,
	// it must not be retransmitted again
	if (dup_tuple != NULL && dup_tuple->retransmitted()) {
		debug("%f: Node %d does not forward a message received"
			" from %d because it is duplicated\n",
			CURRENT_TIME,
			AOLSR::node_id(ra_addr()),
			AOLSR::node_id(dup_tuple->addr()));
		return;
	}

	// If the sender interface address is an interface address
	// of a MPR selector of this node and ttl is greater than 1,
	// the message must be retransmitted
	bool retransmitted = false;
	if (msg.ttl() > 1) {
		AOLSR_mprsel_tuple* mprsel_tuple =
			state_.find_mprsel_tuple(get_main_addr(ih->saddr()));
		if (mprsel_tuple != NULL) {
			AOLSR_msg& new_msg = msg;
			new_msg.ttl()--;
			new_msg.hop_count()++;
			// We have to introduce a random delay to avoid
			// synchronization with neighbors.
			enque_msg(new_msg, JITTER);
			retransmitted = true;
		}
	}

	// Update duplicate tuple...
	if (dup_tuple != NULL) {
		dup_tuple->time() = now + AOLSR_DUP_HOLD_TIME;
		dup_tuple->retransmitted() = retransmitted;
		dup_tuple->iface_list().push_back(local_iface);
	}
	// ...or create a new one
	else {
		AOLSR_dup_tuple* new_dup = new AOLSR_dup_tuple;
		new_dup->addr() = msg.orig_addr();
		new_dup->seq_num() = msg.msg_seq_num();
		new_dup->time() = now + AOLSR_DUP_HOLD_TIME;
		new_dup->retransmitted() = retransmitted;
		new_dup->iface_list().push_back(local_iface);
		add_dup_tuple(new_dup);
		// Schedules dup tuple deletion
		AOLSR_DupTupleTimer* dup_timer =
			new AOLSR_DupTupleTimer(this, new_dup);
		dup_timer->resched(DELAY(new_dup->time()));
	}
}

///
/// \brief Forwards a data packet to the appropiate next hop indicated by the routing table.
///
/// \param p the packet which must be forwarded.
///
void
AOLSR::forward_data(Packet* p) {
	struct hdr_cmn* ch = HDR_CMN(p);
	struct hdr_ip* ih = HDR_IP(p);

	if (ch->direction() == hdr_cmn::UP &&
		((u_int32_t)ih->daddr() == IP_BROADCAST || ih->daddr() == ra_addr())) {
		dmux_->recv(p, 0);
		return;
	}
	else {
		ch->direction() = hdr_cmn::DOWN;
		ch->addr_type() = NS_AF_INET;
		if ((u_int32_t)ih->daddr() == IP_BROADCAST)
			ch->next_hop() = IP_BROADCAST;
		else {
			AOLSR_rt_entry* entry = rtable_.lookup(ih->daddr());
			if (entry == NULL) {
				debug("%f: Node %d can not forward a packet destined to %d\n",
					CURRENT_TIME,
					AOLSR::node_id(ra_addr()),
					AOLSR::node_id(ih->daddr()));
				drop(p, DROP_RTR_NO_ROUTE);
				return;
			}
			else {
				entry = rtable_.find_send_entry(entry);
				assert(entry != NULL);
				ch->next_hop() = entry->next_addr();
				if (use_mac()) {
					ch->xmit_failure_ = aolsr_mac_failed_callback;
					ch->xmit_failure_data_ = (void*)this;
				}
			}
		}

		Scheduler::instance().schedule(target_, p, 0.0);
	}
}

///
/// \brief Enques an %AOLSR message which will be sent with a delay of (0, delay].
///
/// This buffering system is used in order to piggyback several %AOLSR messages in
/// a same %AOLSR packet.
/// \B7\A2\B3\F6\A3\A5AOLSR\CF\FBϢ\A3\AC\B8\C3\CF\FBϢ\BD\AB\D2ԣ\A80\A3\ACdelay]\B5\C4\D1ӳٷ\A2\CB͡\A3
/// \B4˻\BA\B3\E5ϵͳ\D3\C3\D3\DA\D4\DAͬһ\A3\A5AOLSR\CA\FD\BEݰ\FC\D6и\BD\B4\F8\B6\E0\B8\F6\A3\A5AOLSR\CF\FBϢ\A1\A3
/// \param msg the %AOLSR message which must be sent.
/// \param delay maximum delay the %AOLSR message is going to be buffered.
///
void
AOLSR::enque_msg(AOLSR_msg& msg, double delay) {
	assert(delay >= 0);

	msgs_.push_back(msg);
	AOLSR_MsgTimer* timer = new AOLSR_MsgTimer(this);
	timer->resched(delay);
}

///
/// \brief Creates as many %AOLSR packets as needed in order to send all buffered
/// %AOLSR messages.
///
/// Maximum number of messages which can be contained in an %AOLSR packet is
/// dictated by AOLSR_MAX_MSGS constant.
///
void
AOLSR::send_pkt() {
	int num_msgs = msgs_.size();
	if (num_msgs == 0)
		return;

	// Calculates the number of needed packets
	int num_pkts = (num_msgs%AOLSR_MAX_MSGS == 0) ? num_msgs / AOLSR_MAX_MSGS :
		(num_msgs / AOLSR_MAX_MSGS + 1);

	for (int i = 0; i < num_pkts; i++) {
		Packet* p = allocpkt();
		struct hdr_cmn* ch = HDR_CMN(p);
		struct hdr_ip* ih = HDR_IP(p);
		AOLSR_pkt* op = PKT_AOLSR(p);

		op->pkt_len() = AOLSR_PKT_HDR_SIZE;
		op->pkt_seq_num() = pkt_seq();

		int j = 0;
		for (std::vector<AOLSR_msg>::iterator it = msgs_.begin(); it != msgs_.end(); it++) {
			if (j == AOLSR_MAX_MSGS)
				break;

			op->pkt_body_[j++] = *it;
			op->count = j;
			op->pkt_len() += (*it).size();

			it = msgs_.erase(it);
			it--;
		}

		ch->ptype() = PT_AOLSR;
		ch->direction() = hdr_cmn::DOWN;
		ch->size() = IP_HDR_LEN + UDP_HDR_LEN + op->pkt_len();
		ch->error() = 0;
		ch->next_hop() = IP_BROADCAST;
		ch->addr_type() = NS_AF_INET;
		if (use_mac()) {
			ch->xmit_failure_ = aolsr_mac_failed_callback;
			ch->xmit_failure_data_ = (void*)this;
		}

		ih->saddr() = ra_addr();
		ih->daddr() = IP_BROADCAST;
		ih->sport() = RT_PORT;
		ih->dport() = RT_PORT;
		ih->ttl() = IP_DEF_TTL;

		Scheduler::instance().schedule(target_, p, 0.0);
	}
}

///
/// \brief Creates a new %AOLSR HELLO message which is buffered to be sent later on.
///
void
AOLSR::send_hello() {
	AOLSR_msg msg;
	double now = CURRENT_TIME;
	msg.msg_type() = AOLSR_HELLO_MSG;
	msg.vtime() = AOLSR::seconds_to_emf(vtime());///added \D6ܼҼ\D2
	msg.orig_addr() = ra_addr();
	msg.ttl() = 1;
	msg.hop_count() = 0;
	msg.msg_seq_num() = msg_seq();

	msg.hello().reserved() = 0;
	msg.hello().htime() = AOLSR::seconds_to_emf(hello_ival());
	msg.hello().willingness() = willingness();
	msg.hello().count = 0;

	map<u_int8_t, int> linkcodes_count;
	for (linkset_t::iterator it = linkset().begin(); it != linkset().end(); it++) {
		AOLSR_link_tuple* link_tuple = *it;
		if (link_tuple->local_iface_addr() == ra_addr() && link_tuple->time() >= now) {
			u_int8_t link_type, nb_type, link_code;

			// Establishes link type
			if (use_mac() && link_tuple->lost_time() >= now)
				link_type = AOLSR_LOST_LINK;
			else if (link_tuple->sym_time() >= now)
				link_type = AOLSR_SYM_LINK;
			else if (link_tuple->asym_time() >= now)
				link_type = AOLSR_ASYM_LINK;
			else
				link_type = AOLSR_LOST_LINK;
			// Establishes neighbor type.
			if (state_.find_mpr_addr(get_main_addr(link_tuple->nb_iface_addr())))
				nb_type = AOLSR_MPR_NEIGH;
			else {
				bool ok = false;
				for (nbset_t::iterator nb_it = nbset().begin();
					nb_it != nbset().end();
					nb_it++) {
					AOLSR_nb_tuple* nb_tuple = *nb_it;
					if (nb_tuple->nb_main_addr() == link_tuple->nb_iface_addr()) {
						if (nb_tuple->status() == AOLSR_STATUS_SYM)
							nb_type = AOLSR_SYM_NEIGH;
						else if (nb_tuple->status() == AOLSR_STATUS_NOT_SYM)
							nb_type = AOLSR_NOT_NEIGH;
						else {
							fprintf(stderr, "There is a neighbor tuple"
								" with an unknown status!\n");
							exit(1);
						}
						ok = true;
						break;
					}
				}
				if (!ok) {
					fprintf(stderr, "Link tuple has no corresponding"
						" Neighbor tuple\n");
					exit(1);
				}
			}

			int count = msg.hello().count;
			link_code = (link_type & 0x03) | ((nb_type << 2) & 0x0f);
			map<u_int8_t, int>::iterator pos = linkcodes_count.find(link_code);
			if (pos == linkcodes_count.end()) {
				linkcodes_count[link_code] = count;
				assert(count >= 0 && count < AOLSR_MAX_HELLOS);
				msg.hello().hello_msg(count).count = 0;
				msg.hello().hello_msg(count).link_code() = link_code;
				msg.hello().hello_msg(count).reserved() = 0;
				msg.hello().count++;
			}
			else
				count = (*pos).second;

			int i = msg.hello().hello_msg(count).count;
			assert(count >= 0 && count < AOLSR_MAX_HELLOS);
			assert(i >= 0 && i < AOLSR_MAX_ADDRS);

			msg.hello().hello_msg(count).nb_iface_addr(i) =
				link_tuple->nb_iface_addr();
	
		//	msg.hello().hello_msg(count).link_score(i) =link_tuple->link_score(); ///added \D6ܼҼ\D2
		//	msg.hello().hello_msg(count).reserved2(i) =0; ///added \D6ܼҼ\D2
		//	msg.hello().hello_msg(count).reserved3(i) = 0; ///added \D6ܼҼ\D2

	
			msg.hello().hello_msg(count).count++;
			msg.hello().hello_msg(count).link_msg_size() =
				msg.hello().hello_msg(count).size();
		}
	}

	msg.msg_size() = msg.size();

	enque_msg(msg, JITTER);
}

///
/// \brief Creates a new %AOLSR TC message which is buffered to be sent later on.
///
void
AOLSR::send_tc() {
	AOLSR_msg msg;
	msg.msg_type() = AOLSR_TC_MSG;
	msg.vtime() = AOLSR::seconds_to_emf(AOLSR::t_vtime());
	msg.orig_addr() = ra_addr();
	msg.ttl() = 255;
	msg.hop_count() = 0;
	msg.msg_seq_num() = msg_seq();

	msg.tc().ansn() = ansn_;
	msg.tc().reserved() = 0;
	msg.tc().count = 0;
	AOLSR_link_tuple*	temp_link = new AOLSR_link_tuple; ///added \D6ܼҼ\D2

	for (mprselset_t::iterator it = mprselset().begin(); it != mprselset().end(); it++) {
		AOLSR_mprsel_tuple* mprsel_tuple = *it;
		int count = msg.tc().count;

		assert(count >= 0 && count < AOLSR_MAX_ADDRS);
		msg.tc().nb_main_addr(count) = mprsel_tuple->main_addr();
		
		temp_link= state_.find_link_tuple(mprsel_tuple->main_addr());/// added \D6ܼҼ\D2
		//msg.tc().link_score(count) = temp_link->link_score(); ///added \D6ܼҼ\D2
		//msg.tc().reserved2(count) =0; ///added \D6ܼҼ\D2
		//msg.tc().reserved3(count) = 0; ///added \D6ܼҼ\D2

		msg.tc().count++;
	}

	msg.msg_size() = msg.size();

	enque_msg(msg, JITTER);
}

///
/// \brief Creates a new %AOLSR MID message which is buffered to be sent later on.
/// \warning This message is never invoked because there is no support for multiple interfaces.
///
void
AOLSR::send_mid() {
	AOLSR_msg msg;
	msg.msg_type() = AOLSR_MID_MSG;
	msg.vtime() = AOLSR::seconds_to_emf(AOLSR_MID_HOLD_TIME);
	msg.orig_addr() = ra_addr();
	msg.ttl() = 255;
	msg.hop_count() = 0;
	msg.msg_seq_num() = msg_seq();

	msg.mid().count = 0;
	//foreach iface in this_node do
	//	msg.mid().iface_addr(i) = iface
	//	msg.mid().count++
	//done

	msg.msg_size() = msg.size();

	enque_msg(msg, JITTER);
}

///
/// \brief	Updates Link Set according to a new received HELLO message (following RFC 3626
///		specification). Neighbor Set is also updated if needed.
///
/// \param msg the AOLSR message which contains the HELLO message.
/// \param receiver_iface the address of the interface where the message was received from.
/// \param sender_iface the address of the interface where the message was sent from.
///
void
AOLSR::link_sensing(AOLSR_msg& msg, nsaddr_t receiver_iface, nsaddr_t sender_iface,double recv_power) {
	AOLSR_hello& hello = msg.hello(); /// added by \D6ܼҼ\D2
	double now = CURRENT_TIME;
	bool updated = false;
	bool created = false;


	double h_time_ = emf_to_seconds(hello.htime());//// added \D6ܼҼ\D2
	Hmax() = MAX(Hmax(), h_time_); /// added \D6ܼҼ\D2
	Hmin() = MIN(Hmin(), h_time_); /// added \D6ܼҼ\D2
	u_int8_t l_score = get_link_value(recv_power,h_time_);/// added \D6ܼҼ\D2
	AOLSR_link_tuple* link_tuple = state_.find_link_tuple(sender_iface);
	if (link_tuple == NULL) {
		// We have to create a new tuple
		link_tuple = new AOLSR_link_tuple;
		link_tuple->nb_iface_addr() = sender_iface;
		link_tuple->local_iface_addr() = receiver_iface;
		link_tuple->sym_time() = now - 1;
		link_tuple->lost_time() = 0.0;
		link_tuple->time() = now + AOLSR::emf_to_seconds(msg.vtime());
		link_tuple->link_score() = l_score; //added by \D6ܼҼ\D2
		add_link_tuple(link_tuple, hello.willingness());
		created = true;
	}
	else
		updated = true;

	link_tuple->link_score() = l_score; //added by \D6ܼҼ\D2
	link_tuple->asym_time() = now + AOLSR::emf_to_seconds(msg.vtime());
	assert(hello.count >= 0 && hello.count <= AOLSR_MAX_HELLOS);
	for (int i = 0; i < hello.count; i++) {
		AOLSR_hello_msg& hello_msg = hello.hello_msg(i);
		int lt = hello_msg.link_code() & 0x03;
		int nt = hello_msg.link_code() >> 2;

		// We must not process invalid advertised links
		if ((lt == AOLSR_SYM_LINK && nt == AOLSR_NOT_NEIGH) ||
			(nt != AOLSR_SYM_NEIGH && nt != AOLSR_MPR_NEIGH
				&& nt != AOLSR_NOT_NEIGH))
			continue;

		assert(hello_msg.count >= 0 && hello_msg.count <= AOLSR_MAX_ADDRS);
		for (int j = 0; j < hello_msg.count; j++) {
			if (hello_msg.nb_iface_addr(j) == receiver_iface) {
				if (lt == AOLSR_LOST_LINK) {
					link_tuple->sym_time() = now - 1;
					updated = true;
				}
				else if (lt == AOLSR_SYM_LINK || lt == AOLSR_ASYM_LINK) {
					link_tuple->sym_time() =
						now + AOLSR::emf_to_seconds(msg.vtime());
					link_tuple->time() =
						link_tuple->sym_time() + AOLSR::seconds_to_emf(msg.vtime());// added \D6ܼҼ\D2
					link_tuple->lost_time() = 0.0;
					updated = true;
				}
				break;
			}
		}

	}
	link_tuple->time() = MAX(link_tuple->time(), link_tuple->asym_time());

	if (updated)
		updated_link_tuple(link_tuple);

	// Schedules link tuple deletion
	if (created && link_tuple != NULL) {
		AOLSR_LinkTupleTimer* link_timer =
			new AOLSR_LinkTupleTimer(this, link_tuple);
		link_timer->resched(DELAY(MIN(link_tuple->time(), link_tuple->sym_time())));
	}
}

///
/// \brief	Updates the Neighbor Set according to the information contained in a new received
///		HELLO message (following RFC 3626).
///
/// \param msg the %AOLSR message which contains the HELLO message.
///
void
AOLSR::populate_nbset(AOLSR_msg& msg) {
	AOLSR_hello& hello = msg.hello();

	AOLSR_nb_tuple* nb_tuple = state_.find_nb_tuple(msg.orig_addr());
	if (nb_tuple != NULL)
		nb_tuple->willingness() = hello.willingness();
}

///
/// \brief	Updates the 2-hop Neighbor Set according to the information contained in a new
///		received HELLO message (following RFC 3626).
///
/// \param msg the %AOLSR message which contains the HELLO message.
///
void
AOLSR::populate_nb2hopset(AOLSR_msg& msg) {
	double now = CURRENT_TIME;
	AOLSR_hello& hello = msg.hello();

	for (linkset_t::iterator it_lt = linkset().begin(); it_lt != linkset().end(); it_lt++) {
		AOLSR_link_tuple* link_tuple = *it_lt;
		if (get_main_addr(link_tuple->nb_iface_addr()) == msg.orig_addr()) {
			if (link_tuple->sym_time() >= now) {
				assert(hello.count >= 0 && hello.count <= AOLSR_MAX_HELLOS);
				for (int i = 0; i < hello.count; i++) {
					AOLSR_hello_msg& hello_msg = hello.hello_msg(i);
					int nt = hello_msg.link_code() >> 2;
					assert(hello_msg.count >= 0 &&
						hello_msg.count <= AOLSR_MAX_ADDRS);

					for (int j = 0; j < hello_msg.count; j++) {
						nsaddr_t nb2hop_addr = hello_msg.nb_iface_addr(j);
						//u_int8_t link_score_t = hello_msg.link_score(j);// added \D6ܼҼ\D2
						if (nt == AOLSR_SYM_NEIGH || nt == AOLSR_MPR_NEIGH) {
							// if the main address of the 2-hop
							// neighbor address = main address of
							// the receiving node: silently
							// discard the 2-hop neighbor address
							if (nb2hop_addr != ra_addr()) {
								// Otherwise, a 2-hop tuple is created
								AOLSR_nb2hop_tuple* nb2hop_tuple =
									state_.find_nb2hop_tuple(msg.orig_addr(), nb2hop_addr);
								if (nb2hop_tuple == NULL) {
									nb2hop_tuple =
										new AOLSR_nb2hop_tuple;
									nb2hop_tuple->nb_main_addr() =
										msg.orig_addr();
									nb2hop_tuple->nb2hop_addr() =
										nb2hop_addr;
									//nb2hop_tuple->link_score() = link_score_t;//added \D6ܼҼ\D2
									add_nb2hop_tuple(nb2hop_tuple);
									nb2hop_tuple->time() =
										now + AOLSR::emf_to_seconds(msg.vtime());
									// Schedules nb2hop tuple
									// deletion
									AOLSR_Nb2hopTupleTimer* nb2hop_timer =
										new AOLSR_Nb2hopTupleTimer(this, nb2hop_tuple);
									nb2hop_timer->resched(DELAY(nb2hop_tuple->time()));
								}
								else {
									nb2hop_tuple->time() =
										now + AOLSR::emf_to_seconds(msg.vtime());
									//nb2hop_tuple->link_score() = link_score_t;//added \D6ܼҼ\D2
								}

							}
						}
						else if (nt == AOLSR_NOT_NEIGH) {
							// For each 2-hop node listed in the HELLO
							// message with Neighbor Type equal to
							// NOT_NEIGH all 2-hop tuples where:
							// N_neighbor_main_addr == Originator
							// Address AND N_2hop_addr  == main address
							// of the 2-hop neighbor are deleted.
							state_.erase_nb2hop_tuples(msg.orig_addr(),
								nb2hop_addr);
						}
					}
				}
			}
		}
	}
}

///
/// \brief	Updates the MPR Selector Set according to the information contained in a new
///		received HELLO message (following RFC 3626).
///
/// \param msg the %AOLSR message which contains the HELLO message.
///
void
AOLSR::populate_mprselset(AOLSR_msg& msg) {
	double now = CURRENT_TIME;
	AOLSR_hello& hello = msg.hello();

	assert(hello.count >= 0 && hello.count <= AOLSR_MAX_HELLOS);
	for (int i = 0; i < hello.count; i++) {
		AOLSR_hello_msg& hello_msg = hello.hello_msg(i);
		int nt = hello_msg.link_code() >> 2;
		if (nt == AOLSR_MPR_NEIGH) {
			assert(hello_msg.count >= 0 && hello_msg.count <= AOLSR_MAX_ADDRS);
			for (int j = 0; j < hello_msg.count; j++) {
				if (hello_msg.nb_iface_addr(j) == ra_addr()) {
					// We must create a new entry into the mpr selector set
					AOLSR_mprsel_tuple* mprsel_tuple =
						state_.find_mprsel_tuple(msg.orig_addr());
					if (mprsel_tuple == NULL) {
						mprsel_tuple = new AOLSR_mprsel_tuple;
						mprsel_tuple->main_addr() = msg.orig_addr();
						mprsel_tuple->time() =
							now + AOLSR::emf_to_seconds(msg.vtime());
						add_mprsel_tuple(mprsel_tuple);
						int temp=AOLSR::t_q_1();
						AOLSR::t_q_1() =temp+1;//added \D6ܼҼ\D2
						// Schedules mpr selector tuple deletion
						AOLSR_MprSelTupleTimer* mprsel_timer =
							new AOLSR_MprSelTupleTimer(this, mprsel_tuple);
						mprsel_timer->resched(DELAY(mprsel_tuple->time()));
					}
					else
						mprsel_tuple->time() =
						now + AOLSR::emf_to_seconds(msg.vtime());
				}
			}
		}
	}
}

///
/// \brief	Drops a given packet because it couldn't be delivered to the corresponding
///		destination by the MAC layer. This may cause a neighbor loss, and appropiate
///		actions are then taken.
/// \B6\AA\C6\FA\B8\F8\B6\A8\B5\C4\CA\FD\BEݰ\FC\A3\AC\D2\F2Ϊ\CB\FC\CE޷\A8\B1\BBMAC\B2㴫\B5ݵ\BD\CF\E0Ӧ\B5\C4Ŀ\B5ĵء\A3 
/// \D5\E2\BF\C9\C4ܻᵼ\D6\C2\C1ھ\D3\CB\F0ʧ\A3\ACȻ\BA\F3\B2\C9ȡ\CAʵ\B1\B5Ĵ\EBʩ\A1\A3
/// \param p the packet which couldn't be delivered by the MAC layer.
///
void
AOLSR::mac_failed(Packet* p) {
	double now = CURRENT_TIME;
	struct hdr_ip* ih = HDR_IP(p);
	struct hdr_cmn* ch = HDR_CMN(p);

	debug("%f: Node %d MAC Layer detects a breakage on link to %d\n",
		now,
		AOLSR::node_id(ra_addr()),
		AOLSR::node_id(ch->next_hop()));

	if ((u_int32_t)ih->daddr() == IP_BROADCAST) {
		drop(p, DROP_RTR_MAC_CALLBACK);
		return;
	}

	AOLSR_link_tuple* link_tuple = state_.find_link_tuple(ch->next_hop());
	if (link_tuple != NULL) {
		link_tuple->lost_time() = now + AOLSR::vtime();//changed \D6ܼҼ\D2
		link_tuple->time() = now + AOLSR::vtime();//changed \D6ܼҼ\D2
		nb_loss(link_tuple);
		AOLSR::t_q_2() =AOLSR::t_q_2()+1;//added \D6ܼҼ\D2
	}
	drop(p, DROP_RTR_MAC_CALLBACK);
}

///
/// \brief Schedule the timer used for sending HELLO messages.
///
void
AOLSR::set_hello_timer() {
	hello_timer_.resched((double)(hello_ival() - JITTER));
}

///
/// \brief Schedule the timer used for sending TC messages.
///
void
AOLSR::set_tc_timer() {
	tc_timer_.resched((double)(tc_ival() - JITTER));
}

///
/// \brief Schedule the timer used for sending MID messages.
///
void
AOLSR::set_mid_timer() {
	mid_timer_.resched((double)(mid_ival() - JITTER));
}

///
/// \brief Performs all actions needed when a neighbor loss occurs.
///
/// Neighbor Set, 2-hop Neighbor Set, MPR Set and MPR Selector Set are updated.
///
/// \param tuple link tuple with the information of the link to the neighbor which has been lost.
///
void
AOLSR::nb_loss(AOLSR_link_tuple* tuple) {
	debug("%f: Node %d detects neighbor %d loss\n",
		CURRENT_TIME,
		AOLSR::node_id(ra_addr()),
		AOLSR::node_id(tuple->nb_iface_addr()));

	updated_link_tuple(tuple);
	state_.erase_nb2hop_tuples(get_main_addr(tuple->nb_iface_addr()));
	state_.erase_mprsel_tuples(get_main_addr(tuple->nb_iface_addr()));

	mpr_computation();
	rtable_computation();
}

///
/// \brief Adds a duplicate tuple to the Duplicate Set.
///
/// \param tuple the duplicate tuple to be added.
///
void
AOLSR::add_dup_tuple(AOLSR_dup_tuple* tuple) {
	/*debug("%f: Node %d adds dup tuple: addr = %d seq_num = %d\n",
		CURRENT_TIME,
		AOLSR::node_id(ra_addr()),
		AOLSR::node_id(tuple->addr()),
		tuple->seq_num());*/

	state_.insert_dup_tuple(tuple);
}

///
/// \brief Removes a duplicate tuple from the Duplicate Set.
///
/// \param tuple the duplicate tuple to be removed.
///
void
AOLSR::rm_dup_tuple(AOLSR_dup_tuple* tuple) {
	/*debug("%f: Node %d removes dup tuple: addr = %d seq_num = %d\n",
		CURRENT_TIME,
		AOLSR::node_id(ra_addr()),
		AOLSR::node_id(tuple->addr()),
		tuple->seq_num());*/

	state_.erase_dup_tuple(tuple);
}

///
/// \brief Adds a link tuple to the Link Set (and an associated neighbor tuple to the Neighbor Set).
///
/// \param tuple the link tuple to be added.
/// \param willingness willingness of the node which is going to be inserted in the Neighbor Set.
///
void
AOLSR::add_link_tuple(AOLSR_link_tuple* tuple, u_int8_t  willingness) {
	double now = CURRENT_TIME;

	debug("%f: Node %d adds link tuple: nb_addr = %d\n",
		now,
		AOLSR::node_id(ra_addr()),
		AOLSR::node_id(tuple->nb_iface_addr()));

	state_.insert_link_tuple(tuple);
	// Creates associated neighbor tuple
	AOLSR_nb_tuple* nb_tuple = new AOLSR_nb_tuple;
	nb_tuple->nb_main_addr() = get_main_addr(tuple->nb_iface_addr());
	nb_tuple->willingness() = willingness;
	if (tuple->sym_time() >= now)
		nb_tuple->status() = AOLSR_STATUS_SYM;
	else
		nb_tuple->status() = AOLSR_STATUS_NOT_SYM;
	add_nb_tuple(nb_tuple);
	AOLSR::MS() = AOLSR::MS() + 2;//added \D6ܼҼ\D2
	
	printf("added link_tuple\n");
}

///
/// \brief Removes a link tuple from the Link Set.
///
/// \param tuple the link tuple to be removed.
///
void
AOLSR::rm_link_tuple(AOLSR_link_tuple* tuple) {
	nsaddr_t nb_addr = get_main_addr(tuple->nb_iface_addr());
	double now = CURRENT_TIME;

	debug("%f: Node %d removes link tuple: nb_addr = %d\n",
		now,
		AOLSR::node_id(ra_addr()),
		AOLSR::node_id(tuple->nb_iface_addr()));
	// Prints this here cause we are not actually calling rm_nb_tuple() (efficiency stuff)
	debug("%f: Node %d removes neighbor tuple: nb_addr = %d\n",
		now,
		AOLSR::node_id(ra_addr()),
		AOLSR::node_id(nb_addr));

	state_.erase_link_tuple(tuple);

	AOLSR_nb_tuple* nb_tuple = state_.find_nb_tuple(nb_addr);
	state_.erase_nb_tuple(nb_tuple);
	AOLSR::MS() = AOLSR::MS() + 1;//added \D6ܼҼ\D2
	//printf("delete tuple\n");
	delete nb_tuple;
}

///
/// \brief	This function is invoked when a link tuple is updated. Its aim is to
///		also update the corresponding neighbor tuple if it is needed.
///
/// \param tuple the link tuple which has been updated.
///
void
AOLSR::updated_link_tuple(AOLSR_link_tuple* tuple) {
	double now = CURRENT_TIME;

	// Each time a link tuple changes, the associated neighbor tuple must be recomputed
	AOLSR_nb_tuple* nb_tuple =
		state_.find_nb_tuple(get_main_addr(tuple->nb_iface_addr()));
	
	if (nb_tuple != NULL) {
		u_int8_t nb_state = nb_tuple->status(); //added \D6ܼҼ\D2
		if (use_mac() && tuple->lost_time() >= now)
			{
			nb_tuple->status() = AOLSR_STATUS_NOT_SYM;
			//printf("use mac() && tuple->lost_time() >= now\n");
			}
		else if (tuple->sym_time() >= now)
		{
			nb_tuple->status() = AOLSR_STATUS_SYM;
			//printf("tuple->lost_time() >= now\n");
			}
		else
		{
			nb_tuple->status() = AOLSR_STATUS_NOT_SYM;   ///problem is here
			printf("others\n");
			}
		if (nb_state != nb_tuple->status())
		{
			//added \D6ܼҼ\D2
			AOLSR::MS() = AOLSR::MS() + 1;//added \D6ܼҼ\D2
			printf("state changed from %d to %d \n",nb_state,nb_tuple->status());
		}
		debug("%f: Node %d has updated link tuple: nb_addr = %d status = %s\n",
			now,
			AOLSR::node_id(ra_addr()),
			AOLSR::node_id(tuple->nb_iface_addr()),
			((nb_tuple->status() == AOLSR_STATUS_SYM) ? "sym" : "not_sym"));
	}
}

///
/// \brief Adds a neighbor tuple to the Neighbor Set.
///
/// \param tuple the neighbor tuple to be added.
///
void
AOLSR::add_nb_tuple(AOLSR_nb_tuple* tuple) {
	debug("%f: Node %d adds neighbor tuple: nb_addr = %d status = %s\n",
		CURRENT_TIME,
		AOLSR::node_id(ra_addr()),
		AOLSR::node_id(tuple->nb_main_addr()),
		((tuple->status() == AOLSR_STATUS_SYM) ? "sym" : "not_sym"));

	state_.insert_nb_tuple(tuple);
}

///
/// \brief Removes a neighbor tuple from the Neighbor Set.
///
/// \param tuple the neighbor tuple to be removed.
///
void
AOLSR::rm_nb_tuple(AOLSR_nb_tuple* tuple) {
	debug("%f: Node %d removes neighbor tuple: nb_addr = %d status = %s\n",
		CURRENT_TIME,
		AOLSR::node_id(ra_addr()),
		AOLSR::node_id(tuple->nb_main_addr()),
		((tuple->status() == AOLSR_STATUS_SYM) ? "sym" : "not_sym"));

	state_.erase_nb_tuple(tuple);
}

///
/// \brief Adds a 2-hop neighbor tuple to the 2-hop Neighbor Set.
///
/// \param tuple the 2-hop neighbor tuple to be added.
///
void
AOLSR::add_nb2hop_tuple(AOLSR_nb2hop_tuple* tuple) {
	debug("%f: Node %d adds 2-hop neighbor tuple: nb_addr = %d nb2hop_addr = %d\n",
		CURRENT_TIME,
		AOLSR::node_id(ra_addr()),
		AOLSR::node_id(tuple->nb_main_addr()),
		AOLSR::node_id(tuple->nb2hop_addr()));

	state_.insert_nb2hop_tuple(tuple);
}

///
/// \brief Removes a 2-hop neighbor tuple from the 2-hop Neighbor Set.
///
/// \param tuple the 2-hop neighbor tuple to be removed.
///
void
AOLSR::rm_nb2hop_tuple(AOLSR_nb2hop_tuple* tuple) {
	debug("%f: Node %d removes 2-hop neighbor tuple: nb_addr = %d nb2hop_addr = %d\n",
		CURRENT_TIME,
		AOLSR::node_id(ra_addr()),
		AOLSR::node_id(tuple->nb_main_addr()),
		AOLSR::node_id(tuple->nb2hop_addr()));

	state_.erase_nb2hop_tuple(tuple);
}

///
/// \brief Adds an MPR selector tuple to the MPR Selector Set.
///
/// Advertised Neighbor Sequence Number (ANSN) is also updated.
///
/// \param tuple the MPR selector tuple to be added.
///
void
AOLSR::add_mprsel_tuple(AOLSR_mprsel_tuple* tuple) {
	debug("%f: Node %d adds MPR selector tuple: nb_addr = %d\n",
		CURRENT_TIME,
		AOLSR::node_id(ra_addr()),
		AOLSR::node_id(tuple->main_addr()));

	state_.insert_mprsel_tuple(tuple);
	ansn_ = (ansn_ + 1) % (AOLSR_MAX_SEQ_NUM + 1);
}

///
/// \brief Removes an MPR selector tuple from the MPR Selector Set.
///
/// Advertised Neighbor Sequence Number (ANSN) is also updated.
///
/// \param tuple the MPR selector tuple to be removed.
///
void
AOLSR::rm_mprsel_tuple(AOLSR_mprsel_tuple* tuple) {
	debug("%f: Node %d removes MPR selector tuple: nb_addr = %d\n",
		CURRENT_TIME,
		AOLSR::node_id(ra_addr()),
		AOLSR::node_id(tuple->main_addr()));

	state_.erase_mprsel_tuple(tuple);
	AOLSR::t_q_2() =AOLSR::t_q_2()+1;//added  \D6ܼҼ\D2
	ansn_ = (ansn_ + 1) % (AOLSR_MAX_SEQ_NUM + 1);
}

///
/// \brief Adds a topology tuple to the Topology Set.
///
/// \param tuple the topology tuple to be added.
///
void
AOLSR::add_topology_tuple(AOLSR_topology_tuple* tuple) {
	debug("%f: Node %d adds topology tuple: dest_addr = %d last_addr = %d seq = %d\n",
		CURRENT_TIME,
		AOLSR::node_id(ra_addr()),
		AOLSR::node_id(tuple->dest_addr()),
		AOLSR::node_id(tuple->last_addr()),
		tuple->seq());

	state_.insert_topology_tuple(tuple);
}

///
/// \brief Removes a topology tuple from the Topology Set.
///
/// \param tuple the topology tuple to be removed.
///
void
AOLSR::rm_topology_tuple(AOLSR_topology_tuple* tuple) {
	debug("%f: Node %d removes topology tuple: dest_addr = %d last_addr = %d seq = %d\n",
		CURRENT_TIME,
		AOLSR::node_id(ra_addr()),
		AOLSR::node_id(tuple->dest_addr()),
		AOLSR::node_id(tuple->last_addr()),
		tuple->seq());

	state_.erase_topology_tuple(tuple);
}

///
/// \brief Adds an interface association tuple to the Interface Association Set.
///
/// \param tuple the interface association tuple to be added.
///
void
AOLSR::add_ifaceassoc_tuple(AOLSR_iface_assoc_tuple* tuple) {
	debug("%f: Node %d adds iface association tuple: main_addr = %d iface_addr = %d\n",
		CURRENT_TIME,
		AOLSR::node_id(ra_addr()),
		AOLSR::node_id(tuple->main_addr()),
		AOLSR::node_id(tuple->iface_addr()));

	state_.insert_ifaceassoc_tuple(tuple);
}

///
/// \brief Removes an interface association tuple from the Interface Association Set.
///
/// \param tuple the interface association tuple to be removed.
///
void
AOLSR::rm_ifaceassoc_tuple(AOLSR_iface_assoc_tuple* tuple) {
	debug("%f: Node %d removes iface association tuple: main_addr = %d iface_addr = %d\n",
		CURRENT_TIME,
		AOLSR::node_id(ra_addr()),
		AOLSR::node_id(tuple->main_addr()),
		AOLSR::node_id(tuple->iface_addr()));

	state_.erase_ifaceassoc_tuple(tuple);
}

///
/// \brief Gets the main address associated with a given interface address.
///
/// \param iface_addr the interface address.
/// \return the corresponding main address.
///
nsaddr_t
AOLSR::get_main_addr(nsaddr_t iface_addr) {
	AOLSR_iface_assoc_tuple* tuple =
		state_.find_ifaceassoc_tuple(iface_addr);

	if (tuple != NULL)
		return tuple->main_addr();
	return iface_addr;
}

///
/// \brief Determines which sequence number is bigger (as it is defined in RFC 3626).
///
/// \param s1 a sequence number.
/// \param s2 a sequence number.
/// \return true if s1 > s2, false in other case.
///
bool
AOLSR::seq_num_bigger_than(u_int16_t s1, u_int16_t s2) {
	return (s1 > s2 && s1 - s2 <= AOLSR_MAX_SEQ_NUM / 2)
		|| (s2 > s1 && s2 - s1 > AOLSR_MAX_SEQ_NUM / 2);
}

///
/// \brief This auxiliary function (defined in RFC 3626) is used for calculating the MPR Set.
///
/// \param tuple the neighbor tuple which has the main address of the node we are going to calculate its degree to.
/// \return the degree of the node.
///
int
AOLSR::degree(AOLSR_nb_tuple* tuple) {
	int degree = 0;
	for (nb2hopset_t::iterator it = nb2hopset().begin(); it != nb2hopset().end(); it++) {
		AOLSR_nb2hop_tuple* nb2hop_tuple = *it;
		if (nb2hop_tuple->nb_main_addr() == tuple->nb_main_addr()) {
			AOLSR_nb_tuple* nb_tuple =
				state_.find_nb_tuple(nb2hop_tuple->nb_main_addr());
			if (nb_tuple == NULL)
				degree++;
		}
	}
	return degree;
}

///
/// \brief Converts a decimal number of seconds to the mantissa/exponent format.
///
/// \param seconds decimal number of seconds we want to convert.
/// \return the number of seconds in mantissa/exponent format.
///
u_int8_t
AOLSR::seconds_to_emf(double seconds) {
	// This implementation has been taken from unik-aolsrd-0.4.5 (mantissa.c),
	// licensed under the GNU Public License (GPL)

	int a, b = 0;
	while (seconds / AOLSR_C >= pow((double)2, (double)b))
		b++;
	b--;

	if (b < 0) {
		a = 1;
		b = 0;
	}
	else if (b > 15) {
		a = 15;
		b = 15;
	}
	else {
		a = (int)(16 * ((double)seconds / (AOLSR_C*(double)pow(2, b)) - 1));
		while (a >= 16) {
			a -= 16;
			b++;
		}
	}

	return (u_int8_t)(a * 16 + b);
}

//// \BC\C6\CB\E3\C1\B4·\B5÷\D6 \B7\B6Χ0~255\A3\BB
u_int8_t
AOLSR::get_link_value(double recv_p, double recv_t_time) // added \D6ܼҼ\D2
{
double hello_max=Hmax();
double hello_min=Hmin();
double power_max=Pmax();
double power_min=Pmin();
hello_max=4;
hello_min=1;
//printf("recv_p=%lf",recv_p);;
//printf("recv_time=%d,",recv_t_time);
	if ((abs(hello_max-hello_min)>0.00001)&&(abs(power_max-power_min)>0.00001))
	{
                double power_score_ = (100 * (recv_p - power_min)) / (power_max - power_min);
		int power_score = int(power_score_); // added \D6ܼҼ\D2
                int h_score_ = (100 * (hello_max - recv_t_time)) / (hello_max - hello_min);
		int h_score = int(h_score_);// added \D6ܼҼ\D2
		int score = power_score + h_score;// added \D6ܼҼ\D2
              //  printf("score=%d\n",score);
		return (u_int8_t)score;// added \D6ܼҼ\D2
	}
	else
	{
		int a = 20;// added \D6ܼҼ\D2
                // printf("score=%d\n",a);
		return (u_int8_t)a;// added \D6ܼҼ\D2
	}

}



///
/// \brief Converts a number of seconds in the mantissa/exponent format to a decimal number.
///
/// \param aolsr_format number of seconds in mantissa/exponent format.
/// \return the decimal number of seconds.
///
double
AOLSR::emf_to_seconds(u_int8_t aolsr_format) {
	// This implementation has been taken from unik-aolsrd-0.4.5 (mantissa.c),
	// licensed under the GNU Public License (GPL)
	int a = aolsr_format >> 4;
	int b = aolsr_format - a * 16;
	return (double)(AOLSR_C*(1 + (double)a / 16)*(double)pow(2, b));
}

///
/// \brief Returns the identifier of a node given the address of the attached AOLSR agent.
///
/// \param addr the address of the AOLSR routing agent.
/// \return the identifier of the node.
///
int
AOLSR::node_id(nsaddr_t addr) {
	// Preventing a bad use for this function
	if ((u_int32_t)addr == IP_BROADCAST)
		return addr;
	// Getting node id
	Node* node = Node::get_node_by_address(addr);
	assert(node != NULL);
	return node->nodeid();
}
