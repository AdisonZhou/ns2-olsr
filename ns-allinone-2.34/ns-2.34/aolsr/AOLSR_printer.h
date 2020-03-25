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
 /// \file	AOLSR_printer.h
 /// \brief	Header file which includes all printing functions related to AOLSR.
 ///

#ifndef __AOLSR_printer_h__
#define __AOLSR_printer_h__

#include <aolsr/AOLSR.h>
#include <aolsr/AOLSR_pkt.h>
#include <aolsr/AOLSR_repositories.h>
#include <packet.h>
#include <ip.h>
#include <trace.h>

/// Encapsulates all printing functions for AOLSR data structures and messages.
class AOLSR_printer {
	friend class AOLSR;

protected:
	static void	print_linkset(Trace*, linkset_t&);
	static void	print_nbset(Trace*, nbset_t&);
	static void	print_nb2hopset(Trace*, nb2hopset_t&);
	static void	print_mprset(Trace*, mprset_t&);
	static void	print_mprselset(Trace*, mprselset_t&);
	static void	print_topologyset(Trace*, topologyset_t&);

	static void	print_aolsr_pkt(FILE*, AOLSR_pkt*);
	static void	print_aolsr_msg(FILE*, AOLSR_msg&);
	static void	print_aolsr_hello(FILE*, AOLSR_hello&);
	static void	print_aolsr_tc(FILE*, AOLSR_tc&);
	static void	print_aolsr_mid(FILE*, AOLSR_mid&);

public:
	static void	print_cmn_hdr(FILE*, struct hdr_cmn*);
	static void	print_ip_hdr(FILE*, struct hdr_ip*);
};

#endif
