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
 /// \file	AOLSR_rtable.h
 /// \brief	Header file for routing table's related stuff.
 ///路由表相关内容的头文件。

#ifndef __AOLSR_rtable_h__
#define __AOLSR_rtable_h__

#include <aolsr/AOLSR_repositories.h>
#include <trace.h>
#include <map>

///        将rtable_t定义为AOLSR_rt_entry的映射，其键为目标地址。
/// \brief Defines rtable_t as a map of AOLSR_rt_entry, whose key is the destination address.
///
/// An %AOLSR's routing table entry. // 路由表条目
///typedef struct AOLSR_rt_entry {
	///nsaddr_t	dest_addr_;	///< Address of the destination node.
	///nsaddr_t	next_addr_;	///< Address of the next hop.
	///nsaddr_t	iface_addr_;	///< Address of the local interface.
	///u_int32_t	dist_;		///< Distance in hops to the destination.

	///inline nsaddr_t&	dest_addr()	{ return dest_addr_; }
	///inline nsaddr_t&	next_addr()	{ return next_addr_; }
	///inline nsaddr_t&	iface_addr()	{ return iface_addr_; }
	///inline u_int32_t&	dist()		{ return dist_; }
///} AOLSR_rt_entry;

///路由表因此被定义为成对的：[目的地地址，条目]。 可以通过“第一”和“第二”成员访问该对中的每个元素。
/// The routing table is thus defined as pairs: [dest address, entry]. Each element
/// of the pair can be accesed via "first" and "second" members.
///
typedef std::map<nsaddr_t, AOLSR_rt_entry*> rtable_t;

///
/// \brief This class is a representation of the AOLSR's Routing Table.
///
class AOLSR_rtable {
	rtable_t	rt_;	///< Data structure for the routing table.

public:

	AOLSR_rtable();
	~AOLSR_rtable();

	void		clear();
	void		rm_entry(nsaddr_t dest);
	AOLSR_rt_entry*	add_entry(nsaddr_t dest, nsaddr_t next, nsaddr_t iface, u_int32_t dist,u_int32_t rt_cost); /// added 周家家
	AOLSR_rt_entry*	lookup(nsaddr_t dest);
	AOLSR_rt_entry*	find_send_entry(AOLSR_rt_entry*);
	u_int32_t	size();
	void		print(Trace*);
};

#endif
