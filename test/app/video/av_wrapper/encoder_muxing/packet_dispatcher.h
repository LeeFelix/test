// by wangh
#ifndef PACKETLIST_H
#define PACKETLIST_H

#include "../dispatcher.h"

class CameraMuxer;

class PacketDispatcher : public Dispatcher<CameraMuxer, EncodedPacket>
{
public:
    void Dispatch(EncodedPacket* pkt);
    // void Dispatch(inbuf, outbuf);
};

#endif // PACKETLIST_H
