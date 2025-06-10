//  Copyright (C) 2010 Georg Kaindl
//  http://gkaindl.com
//
//  This file is part of Arduino EthernetBonjour.
//
//  EthernetBonjour is free software: you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public License
//  as published by the Free Software Foundation, either version 3 of
//  the License, or (at your option) any later version.
//
//  EthernetBonjour is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with EthernetBonjour. If not, see
//  <http://www.gnu.org/licenses/>.
//

#include "EthernetUtil.h"

#if defined(__ETHERNET_UTIL_BONJOUR__)

uint16_t ethutil_swaps(uint16_t i);
uint32_t ethutil_swapl(uint32_t l);

extern uint16_t ethutil_htons(unsigned short hostshort)
{
#if ( SYSTEM_ENDIAN == _ENDIAN_LITTLE_ )
	return ethutil_swaps(hostshort);
#else
	return hostshort;
#endif
}

extern uint32_t ethutil_htonl(unsigned long hostlong)
{
#if ( SYSTEM_ENDIAN == _ENDIAN_LITTLE_ )
	return ethutil_swapl(hostlong);
#else
	return hostlong;
#endif
}

extern uint16_t ethutil_ntohs(unsigned short netshort)
{
#if ( SYSTEM_ENDIAN == _ENDIAN_LITTLE_ )	
	return ethutil_swaps(netshort);
#else
	return netshort;
#endif  
}

extern uint32_t ethutil_ntohl(unsigned long netlong)
{
#if ( SYSTEM_ENDIAN == _ENDIAN_LITTLE_ )
	return ethutil_swapl(netlong);
#else
	return netlong;
#endif
}

// #pragma mark -
// #pragma mark Private

uint16_t ethutil_swaps(uint16_t i)
{
	uint16_t ret=0;
	ret = (i & 0xFF) << 8;
	ret |= ((i >> 8)& 0xFF);
	return ret;	
}

uint32_t ethutil_swapl(uint32_t l)
{
	uint32_t ret=0;
	ret = (l & 0xFF) << 24;
	ret |= ((l >> 8) & 0xFF) << 16;
	ret |= ((l >> 16) & 0xFF) << 8;
	ret |= ((l >> 24) & 0xFF);
	return ret;
}

#endif // __ETHERNET_UTIL_BONJOUR__