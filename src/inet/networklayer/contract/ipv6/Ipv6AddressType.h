//
// Copyright (C) 2013 Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#ifndef __INET_IPV6ADDRESSTYPE_H
#define __INET_IPV6ADDRESSTYPE_H

#include "inet/common/INETDefs.h"

#include "inet/common/Protocol.h"
#include "inet/networklayer/contract/IL3AddressType.h"
#include "inet/networklayer/contract/ipv6/Ipv6Address.h"

namespace inet {

class INET_API Ipv6AddressType : public IL3AddressType
{
  public:
    static Ipv6AddressType INSTANCE;
    static const Ipv6Address ALL_RIP_ROUTERS_MCAST;

  public:
    Ipv6AddressType() {}
    virtual ~Ipv6AddressType() {}

    virtual int getAddressBitLength() const override { return 128; }
    virtual int getMaxPrefixLength() const override { return 128; }
    virtual L3Address getUnspecifiedAddress() const override { return Ipv6Address::UNSPECIFIED_ADDRESS; }
    virtual L3Address getBroadcastAddress() const override { return Ipv6Address::ALL_NODES_1; }
    virtual L3Address getLinkLocalManetRoutersMulticastAddress() const override { return Ipv6Address::LL_MANET_ROUTERS; }
    virtual L3Address getLinkLocalRIPRoutersMulticastAddress() const override { return ALL_RIP_ROUTERS_MCAST; };
    virtual const Protocol *getNetworkProtocol() const override { return &Protocol::ipv6; }
    virtual L3Address getLinkLocalAddress(const InterfaceEntry *ie) const override;
};

} // namespace inet

#endif // ifndef __INET_IPV6ADDRESSTYPE_H

