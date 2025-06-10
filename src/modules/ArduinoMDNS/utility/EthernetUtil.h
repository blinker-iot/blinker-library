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

#if !defined(__ETHERNET_UTIL_H__)
#define __ETHERNET_UTIL_H__

#define __ETHERNET_UTIL_BONJOUR__

#include <inttypes.h>

extern uint16_t ethutil_htons(unsigned short hostshort);
extern uint32_t ethutil_htonl(unsigned long hostlong);
extern uint16_t ethutil_ntohs(unsigned short netshort);
extern uint32_t ethutil_ntohl(unsigned long netlong);

#endif // __ETHERNET_UTIL_H__