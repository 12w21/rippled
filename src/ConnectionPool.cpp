
#include <boost/asio.hpp>
#include <boost/foreach.hpp>

#include "ConnectionPool.h"
#include "Config.h"
#include "Peer.h"
#include "Application.h"
#include "utils.h"


ConnectionPool::ConnectionPool() :
	iConnecting(0)
{ ; }


void ConnectionPool::start()
{
	// XXX Start running policy.
}

// XXX Broken don't send a message to a peer if we got it from the peer.
void ConnectionPool::relayMessage(Peer* fromPeer, PackedMessage::pointer msg)
{
	BOOST_FOREACH(naPeer pair, mConnectedMap)
	{
		Peer::pointer peer	= pair.second;

		if(!fromPeer || !(peer.get() == fromPeer))
			peer->sendPacket(msg);
	}
}

// Inbound connection, false=reject
// Reject addresses we already have in our table.
// XXX Reject, if we have too many connections.
bool ConnectionPool::peerAccepted(Peer::pointer peer, const std::string& strIp, int iPort)
{
	bool	bAccept;
	ipPort	ip	= make_pair(strIp, iPort);

    boost::unordered_map<ipPort, Peer::pointer>::iterator	it;

	boost::mutex::scoped_lock sl(mPeerLock);

	it	= mIpMap.find(ip);

	if (it == mIpMap.end())
	{
		// Did not find it.  Not already connecting or connected.

		std::cerr << "ConnectionPool::peerAccepted: " << ip.first << " " << ip.second << std::endl;
		// Mark as connecting.
		mIpMap[ip]	= peer;
		bAccept		= true;
	}
	else
	{
		// Found it.  Already connected or connecting.

		bAccept	= false;
	}

	return bAccept;
}

bool ConnectionPool::connectTo(const std::string& strIp, int iPort)
{
	ipPort	ip	= make_pair(strIp, iPort);

    boost::unordered_map<ipPort, Peer::pointer>::iterator	it;

	boost::mutex::scoped_lock sl(mPeerLock);

	it	= mIpMap.find(ip);

	if (it == mIpMap.end())
	{
		// Did not find it.  Not already connecting or connected.

		Peer::pointer peer(Peer::create(theApp->getIOService()));

		mIpMap[ip]	= peer;

		peer->connect(strIp, iPort);
	}
	else
	{
		// Found it.  Already connected.
		std::cerr << "ConnectionPool::connectTo: Already connected: "
			<< strIp << " " << iPort << std::endl;
	}

	return true;
}

Json::Value ConnectionPool::getPeersJson()
{
    Json::Value ret(Json::arrayValue);

	BOOST_FOREACH(naPeer pair, mConnectedMap)
	{
		Peer::pointer peer	= pair.second;

		ret.append(peer->getJson());
    }

    return ret;
}

void ConnectionPool::peerConnected(Peer::pointer peer)
{
	std::cerr << "ConnectionPool::peerConnected" << std::endl;
}

void ConnectionPool::peerDisconnected(Peer::pointer peer)
{
	std::cerr << "ConnectionPool::peerDisconnected: " << peer->mIpPort.first << " " << peer->mIpPort.second << std::endl;

	boost::mutex::scoped_lock sl(mPeerLock);

	// XXX Don't access member variable directly.
	if (peer->mPublicKey.isValid())
	{
		boost::unordered_map<NewcoinAddress, Peer::pointer>::iterator itCm;

		itCm	= mConnectedMap.find(peer->mPublicKey);

		if (itCm == mConnectedMap.end())
		{
			// Did not find it.  Not already connecting or connected.
			std::cerr << "Internal Error: peer connection was inconsistent." << std::endl;
			// XXX Bad error.
		}
		else
		{
			// Found it. Delete it.
			mConnectedMap.erase(itCm);
		}
	}

	// XXX Don't access member variable directly.
    boost::unordered_map<ipPort, Peer::pointer>::iterator	itIp;

	itIp	= mIpMap.find(peer->mIpPort);

	if (itIp == mIpMap.end())
	{
		// Did not find it.  Not already connecting or connected.
		std::cerr << "Internal Error: peer wasn't connected." << std::endl;
		// XXX Bad error.
	}
	else
	{
		// Found it. Delete it.
		mIpMap.erase(itIp);
	}
}

#if 0
bool ConnectionPool::isMessageKnown(PackedMessage::pointer msg)
{
	for(unsigned int n=0; n<mBroadcastMessages.size(); n++)
	{
		if(msg==mBroadcastMessages[n].first) return(false);
	}
	return(false);
}
#endif

// vim:ts=4
