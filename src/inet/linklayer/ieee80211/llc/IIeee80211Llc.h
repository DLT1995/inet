//
// Copyright (C) 2018 Raphael Riebl, TH Ingolstadt
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

#ifndef __INET_IIEEE80211LLC_H
#define __INET_IIEEE80211LLC_H

#include "inet/common/Protocol.h"

namespace inet {
namespace ieee80211 {

class INET_API IIeee80211Llc
{
  public:
    virtual const Protocol *getProtocol() const = 0;
};

} // namespace ieee80211
} // namespace inet

#endif // ifndef __INET_IIEEE80211LLC_H
