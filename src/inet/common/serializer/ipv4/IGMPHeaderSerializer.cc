//
// Copyright (C) 2005 Christian Dankbar, Irene Ruengeler, Michael Tuexen, Andras Varga
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include "inet/common/packet/SerializerRegistry.h"
#include "inet/common/serializer/ipv4/IGMPHeaderSerializer.h"
#include "inet/networklayer/ipv4/IGMPMessage.h"

#if !defined(_WIN32) && !defined(__WIN32__) && !defined(WIN32) && !defined(__CYGWIN__) && !defined(_WIN64)
#include <netinet/in.h>    // htonl, ntohl, ...
#endif // if !defined(_WIN32) && !defined(__WIN32__) && !defined(WIN32) && !defined(__CYGWIN__) && !defined(_WIN64)

namespace inet {

namespace serializer {

Register_Serializer(IGMPMessage, IGMPHeaderSerializer);

void IGMPHeaderSerializer::serialize(ByteOutputStream& stream, const std::shared_ptr<Chunk>& chunk) const
{
    const auto& igmpMessage = std::static_pointer_cast<const IGMPMessage>(chunk);

    switch (igmpMessage->getType())
    {
        case IGMP_MEMBERSHIP_QUERY: {
            stream.writeByte(IGMP_MEMBERSHIP_QUERY);    // type
            if (auto v3pkt = std::dynamic_pointer_cast<const IGMPv3Query>(igmpMessage)) {
                ASSERT(v3pkt->getMaxRespTime() < 12.8); // TODO: floating point case, see RFC 3376 4.1.1
                stream.writeByte(v3pkt->getMaxRespTime().inUnit((SimTimeUnit)-1));
            }
            else if (auto v2pkt = std::dynamic_pointer_cast<const IGMPv2Query>(igmpMessage))
                stream.writeByte(v2pkt->getMaxRespTime().inUnit((SimTimeUnit)-1));
            stream.writeUint16(igmpMessage->getCrc());
            stream.writeIPv4Address(check_and_cast<const IGMPQuery*>(igmpMessage.get())->getGroupAddress());
            if (auto v3pkt = std::dynamic_pointer_cast<const IGMPv3Query>(igmpMessage))
            {
                ASSERT(v3pkt->getRobustnessVariable() <= 7);
                stream.writeByte((v3pkt->getSuppressRouterProc() ? 0x8 : 0) | v3pkt->getRobustnessVariable());
                stream.writeByte(v3pkt->getQueryIntervalCode());
                unsigned int vs = v3pkt->getSourceList().size();
                stream.writeUint16(vs);
                for (unsigned int i = 0; i < vs; i++)
                    stream.writeIPv4Address(v3pkt->getSourceList()[i]);
            }
            break;
        }

        case IGMPV1_MEMBERSHIP_REPORT:
            stream.writeByte(IGMPV1_MEMBERSHIP_REPORT);    // type
            stream.writeByte(0);    // unused
            stream.writeUint16(igmpMessage->getCrc());
            stream.writeIPv4Address(check_and_cast<const IGMPv1Report*>(igmpMessage.get())->getGroupAddress());
            break;

        case IGMPV2_MEMBERSHIP_REPORT:
            stream.writeByte(IGMPV2_MEMBERSHIP_REPORT);    // type
            stream.writeByte(0);    // code
            stream.writeUint16(igmpMessage->getCrc());
            stream.writeIPv4Address(check_and_cast<const IGMPv2Report*>(igmpMessage.get())->getGroupAddress());
            break;

        case IGMPV2_LEAVE_GROUP:
            stream.writeByte(IGMPV2_LEAVE_GROUP);    // type
            stream.writeByte(0);    // code
            stream.writeUint16(igmpMessage->getCrc());
            stream.writeIPv4Address(check_and_cast<const IGMPv2Leave*>(igmpMessage.get())->getGroupAddress());
            break;

        case IGMPV3_MEMBERSHIP_REPORT: {
            const IGMPv3Report* v3pkt = check_and_cast<const IGMPv3Report*>(igmpMessage.get());
            stream.writeByte(IGMPV3_MEMBERSHIP_REPORT);    // type
            stream.writeByte(0);    // code
            stream.writeUint16(igmpMessage->getCrc());
            stream.writeUint16(0);    // reserved
            unsigned int s = v3pkt->getGroupRecordArraySize();
            stream.writeUint16(s);    // number of groups
            for (unsigned int i = 0; i < s; i++) {
                // serialize one group:
                const GroupRecord& gr = v3pkt->getGroupRecord(i);
                stream.writeByte(gr.recordType);
                stream.writeByte(0);  // aux data len: RFC 3376 Section 4.2.6
                stream.writeUint16(gr.sourceList.size());
                stream.writeIPv4Address(gr.groupAddress);
                for (auto src: gr.sourceList) {
                    stream.writeIPv4Address(src);
                }
                // write auxiliary data
            }
            break;
        }

        default:
            throw cRuntimeError("Can not serialize IGMP packet (%s): type %d not supported.", igmpMessage->getClassName(), igmpMessage->getType());
    }
}

std::shared_ptr<Chunk> IGMPHeaderSerializer::deserialize(ByteInputStream& stream) const
{
    std::shared_ptr<IGMPMessage> packet = nullptr;
    unsigned int startPos = stream.getPosition();
    unsigned char type = stream.readByte();
    unsigned char code = stream.readByte();
    uint16_t chksum = stream.readUint16();

    switch (type) {
        case IGMP_MEMBERSHIP_QUERY:
            if (code == 0) {
                auto pkt = std::make_shared<IGMPv1Query>();
                packet = pkt;
                pkt->setGroupAddress(stream.readIPv4Address());
                pkt->setChunkLength(byte(8));
            }
            else if (stream.getSize() - startPos == 8) {        // RFC 3376 Section 7.1
                auto pkt = std::make_shared<IGMPv2Query>();
                packet = pkt;
                pkt->setMaxRespTime(SimTime(code, (SimTimeUnit)-1));
                pkt->setGroupAddress(stream.readIPv4Address());
                pkt->setChunkLength(byte(8));
            }
            else {
                auto pkt = std::make_shared<IGMPv3Query>();
                packet = pkt;
                ASSERT(code < 128); // TODO: floating point case, see RFC 3376 4.1.1
                pkt->setMaxRespTime(SimTime(code, (SimTimeUnit)-1));
                pkt->setGroupAddress(stream.readIPv4Address());
                unsigned char x = stream.readByte(); //
                pkt->setSuppressRouterProc((x & 0x8) != 0);
                pkt->setRobustnessVariable(x & 7);
                pkt->setQueryIntervalCode(stream.readByte());
                unsigned int vs = stream.readUint16();
                for (unsigned int i = 0; i < vs && !stream.isReadBeyondEnd(); i++)
                    pkt->getSourceList()[i] = stream.readIPv4Address();
                pkt->setChunkLength(byte(stream.getPosition() - startPos));
            }
            break;

        case IGMPV1_MEMBERSHIP_REPORT:
            {
                auto pkt = std::make_shared<IGMPv1Report>();
                packet = pkt;
                pkt->setGroupAddress(stream.readIPv4Address());
                pkt->setChunkLength(byte(8));
            }
            break;

        case IGMPV2_MEMBERSHIP_REPORT:
            {
                auto pkt = std::make_shared<IGMPv2Report>();
                packet = pkt;
                pkt->setGroupAddress(stream.readIPv4Address());
                pkt->setChunkLength(byte(8));
            }
            break;

        case IGMPV2_LEAVE_GROUP:
            {
                auto pkt = std::make_shared<IGMPv2Leave>();
                packet = pkt;
                pkt->setGroupAddress(stream.readIPv4Address());
                pkt->setChunkLength(byte(8));
            }
            break;

        case IGMPV3_MEMBERSHIP_REPORT:
            {
                auto pkt = std::make_shared<IGMPv3Report>();
                packet = pkt;
                stream.readUint16(); //reserved
                unsigned int s = stream.readUint16();
                pkt->setGroupRecordArraySize(s);
                unsigned int i;
                for (i = 0; i < s && !stream.isReadBeyondEnd(); i++) {
                    GroupRecord gr;
                    gr.recordType = stream.readByte();
                    uint16_t auxDataLen = 4 * stream.readByte();
                    unsigned int gac = stream.readUint16();
                    gr.groupAddress = stream.readIPv4Address();
                    for (unsigned int j = 0; j < gac && !stream.isReadBeyondEnd(); j++) {
                        gr.sourceList.push_back(stream.readIPv4Address());
                    }
                    pkt->setGroupRecord(i, gr);
                    stream.seek(stream.getPosition() + auxDataLen);
                }
                if (i < s) {
                    pkt->setGroupRecordArraySize(i);
                }
                pkt->setChunkLength(byte(stream.getPosition() - startPos));
            }
            break;

        default:
            EV_ERROR << "IGMPSerializer: can not create IGMP packet: type " << type << " not supported\n";
            packet = std::make_shared<IGMPMessage>();
            packet->markImproperlyRepresented();
            break;
    }

    ASSERT(packet);
    packet->setCrc(chksum);
    packet->setCrcMode(CRC_COMPUTED);
    return packet;
}

} // namespace serializer

} // namespace inet
