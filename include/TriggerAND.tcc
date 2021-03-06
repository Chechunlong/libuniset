/* This file is part of the UniSet project
 * Copyright (c) 2002 Free Software Foundation, Inc.
 * Copyright (c) 2002 Pavel Vainerman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
// --------------------------------------------------------------------------
/*! \file
 *  \author Pavel Vainerman
*/
// --------------------------------------------------------------------------
#include "TriggerAND.h"
//---------------------------------------------------------------------------
template<class Caller, typename InputType>
TriggerAND<Caller,InputType>::TriggerAND(Caller* c, Action a):
cal(c),
act(a)
{
}

template<class Caller, typename InputType>
TriggerAND<Caller,InputType>::~TriggerAND()
{
}

//---------------------------------------------------------------------------
template<class Caller, typename InputType>
bool TriggerAND<Caller,InputType>::commit(InputType num, bool state)
{
	typename InputMap::iterator it=inputs.find(num);
	if( it!=inputs.end() )
	{
		inputs[num] = state;
		check();	
		return true;
	}
	
	return false;
}

//---------------------------------------------------------------------------
template<class Caller, typename InputType>
void TriggerAND<Caller,InputType>::add(InputType num, bool state)
{
	inputs[num] = state;
	check();
}

//---------------------------------------------------------------------------
template<class Caller, typename InputType>
void TriggerAND<Caller,InputType>::remove(InputType num)
{
	typename InputMap::iterator it=inputs.find(num);
	if( it!=inputs.end() )
		inputs.erase(it);

	check();		
}

//---------------------------------------------------------------------------
template<class Caller, typename InputType>
bool TriggerAND<Caller,InputType>::getState(InputType num)
{
	typename InputMap::iterator it=inputs.find(num);
	if( it!=inputs.end() )
		return it->second;

	return false; // throw NotFound
}
//---------------------------------------------------------------------------
template<class Caller, typename InputType>
void TriggerAND<Caller,InputType>::check()
{
	bool old = out;
	for( typename InputMap::iterator it=inputs.begin(); it!=inputs.end(); ++it )
	{
		if( !it->second )
		{
			// если хоть один вход "0" на выходе "0"
			// и прекращаем дальнейший поиск
			out = false;
			if( old != out )
			{
//				try
//				{
					(cal->*act)(out);
//				}
//				catch(...){}
			}
			return;
		}
	}
	
	out = true;
	if( old != out )
	{
//		try
//		{
			(cal->*act)(out);
//		}
//		catch(...){}
	}
}
//---------------------------------------------------------------------------
template<class Caller, typename InputType>
void TriggerAND<Caller,InputType>::update()
{
	(cal->*act)(out);
}
//---------------------------------------------------------------------------
template<class Caller, typename InputType>
void TriggerAND<Caller,InputType>::reset()
{
	out = false;
	(cal->*act)(out);
}
//---------------------------------------------------------------------------
