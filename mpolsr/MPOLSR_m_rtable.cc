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

///
///\file MPOLSR_m_rtable.cc
///\brief Implementation of our multipath routing table
///

//#include "mpolsr/MPOLSR.h"
#include "mpolsr/MPOLSR_m_rtable.h"
#include "mpolsr/MPOLSR_repositories.h"
using namespace std;
///
///\brief Creates a new empty routing table
///
MPOLSR_m_rtable::MPOLSR_m_rtable(){
	for (int i = 0;i<MAX_NODE; i ++)
	{
		out_of_date[i] = true;
	}
}

///
///\brief Destroys the routing table and ll its entries
///
MPOLSR_m_rtable::~MPOLSR_m_rtable(){
	for(m_rtable_t::iterator it = m_rt_.begin();it!=m_rt_.end();it++)
		delete (*it).second;
		
}

m_rtable_t* MPOLSR_m_rtable::m_rt(){
	return &m_rt_;
}
void MPOLSR_m_rtable::set_flag(int id, bool flag){
	out_of_date[id] = flag;
}

void MPOLSR_m_rtable::set_flag(bool flag){
	for (int i = 0; i < MAX_NODE; i ++){
		out_of_date[i] = flag;
	}
}

bool MPOLSR_m_rtable::get_flag(int id){
	return out_of_date[id];
}
///
///\brief Destroys the routing table and all its entries
///
void MPOLSR_m_rtable::clear(){
	for (m_rtable_t::iterator it = m_rt_.begin();it != m_rt_.end(); it++)
		delete (*it).second;

	m_rt_.clear();
}


///
///\brief Deletes the entry whose destination is given.
///\param dest	address of the destiantion node.
///
void MPOLSR_m_rtable::rm_entry(nsaddr_t dest){
/*	for (m_rtable_t::iterator it = m_rt_.begin(); it != m_rt_.end(); it++){
		if ( (MPOLSR_m_rt_entry*)((*it).second).end() == dest)
			delete (*it).second;
	}
*/
	m_rt_.erase(dest);
	
}

///
///\brief finds the entries for the specified destination address
///\param dest destination address
///
m_rtable_t::iterator MPOLSR_m_rtable::lookup(nsaddr_t dest){
	return m_rt_.find(dest);
}

///
///\brief add a new entry to the routing table
///\
///
void MPOLSR_m_rtable::add_entry(MPOLSR_m_rt_entry* entry,nsaddr_t addr){
/*
	MPOLSR_m_rt_entry::iterator it = (*entry).end();
	nsaddr_t addr = (*it);
	m_rt_.insert(pair<nsaddr_t,MPOLSR_m_rt_entry*>(addr,entry));*/
//	nsaddr_t addr = (*entry).addr
	m_rt_.insert(pair<nsaddr_t,MPOLSR_m_rt_entry*>(addr,entry));
}
