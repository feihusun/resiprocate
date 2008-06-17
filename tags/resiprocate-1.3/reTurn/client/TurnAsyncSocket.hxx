#ifndef TURNASYNCSOCKET_HXX
#define TURNASYNCSOCKET_HXX

#include <map>
#include <queue>
#include <asio.hpp>
#include <rutil/Data.hxx>
#include <rutil/Mutex.hxx>

#include "../StunTuple.hxx"
#include "../StunMessage.hxx"
#include "../ChannelManager.hxx"
#include "../AsyncSocketBase.hxx"
#include "TurnAsyncSocketHandler.hxx"

namespace reTurn {

class TurnAsyncSocket
{
public:
   static unsigned int UnspecifiedLifetime;
   static unsigned int UnspecifiedBandwidth;
   static unsigned short UnspecifiedPort;
   static asio::ip::address UnspecifiedIpAddress;

   explicit TurnAsyncSocket(asio::io_service& ioService,
                            AsyncSocketBase& asyncSocketBase,
                            TurnAsyncSocketHandler* turnAsyncSocketHandler,
                            const asio::ip::address& address = UnspecifiedIpAddress, 
                            unsigned short port = 0, 
                            bool turnFraming = true);
   virtual ~TurnAsyncSocket();

   virtual void disableTurnAsyncHandler();
   virtual unsigned int getSocketDescriptor() = 0;

   // Note: Shared Secret requests have been deprecated by RFC3489-bis11, and not 
   //       widely implemented in RFC3489 - so not really needed at all
   void requestSharedSecret();

   // Set the username and password for all future requests
   void setUsernameAndPassword(const char* username, const char* password, bool shortTermAuth=false);
   void connect(const std::string& address, unsigned short port, bool turnFraming);

   // Stun Binding Method - use getReflexiveTuple() to get binding info
   void bindRequest();

   // Turn Allocation Methods
   void createAllocation(unsigned int lifetime = UnspecifiedLifetime,
                         unsigned int bandwidth = UnspecifiedBandwidth,
                         unsigned short requestedPortProps = StunMessage::PortPropsNone, 
                         unsigned short requestedPort = UnspecifiedPort,
                         StunTuple::TransportType requestedTransportType = StunTuple::None, 
                         const asio::ip::address &requestedIpAddress = UnspecifiedIpAddress);
   void refreshAllocation(unsigned int lifetime);
   void destroyAllocation();

   // Methods to control active destination
   void setActiveDestination(const asio::ip::address& address, unsigned short port);
   void clearActiveDestination();

   asio::ip::address& getConnectedAddress() { return mAsyncSocketBase.getConnectedAddress(); }
   unsigned short getConnectedPort() { return mAsyncSocketBase.getConnectedPort(); }

   // Turn Send Methods
   virtual void send(const char* buffer, unsigned int size);
   virtual void sendTo(const asio::ip::address& address, unsigned short port, const char* buffer, unsigned int size);

   // Receive Methods
   //asio::error_code receive(char* buffer, unsigned int& size, unsigned int timeout, asio::ip::address* sourceAddress=0, unsigned short* sourcePort=0);
   //asio::error_code receiveFrom(const asio::ip::address& address, unsigned short port, char* buffer, unsigned int& size, unsigned int timeout);

   virtual void close();
   virtual void turnReceive();

protected:

   void handleReceivedData(const asio::ip::address& address, unsigned short port, boost::shared_ptr<DataBuffer>& data);

   asio::io_service& mIOService;
   TurnAsyncSocketHandler* mTurnAsyncSocketHandler;
   bool mTurnFraming;

   // Local Binding Info
   StunTuple mLocalBinding;

   // Authentication Info
   resip::Data mUsername;
   resip::Data mPassword;
   resip::Data mHmacKey;
   resip::Data mRealm;
   resip::Data mNonce;

   // Turn Allocation Properties used in request
   StunTuple::TransportType mRequestedTransportType;

   // Turn Allocation Properties from response
   bool mHaveAllocation;
   StunTuple::TransportType mRelayTransportType;
   unsigned int mLifetime;

   ChannelManager mChannelManager;
   RemotePeer* mActiveDestination;

private:
   AsyncSocketBase& mAsyncSocketBase;
   bool mCloseAfterDestroyAllocationFinishes;

   // Request map (for retransmissions)
   class RequestEntry : public boost::enable_shared_from_this<RequestEntry>
   {
   public:
      RequestEntry(asio::io_service& ioService, TurnAsyncSocket* turnAsyncSocket, StunMessage* requestMessage);
      ~RequestEntry();

      void startTimer();
      void stopTimer();
      void requestTimerExpired(const asio::error_code& e);

      asio::io_service& mIOService;
      TurnAsyncSocket* mTurnAsyncSocket;
      StunMessage* mRequestMessage;
      asio::deadline_timer mRequestTimer;
      unsigned int mRequestsSent;
      unsigned int mTimeout;
   };
   typedef std::map<UInt128, boost::shared_ptr<RequestEntry> > RequestMap;
   RequestMap mActiveRequestMap;
   friend class RequestEntry;
   void requestTimeout(UInt128 tid);
   void clearActiveRequestMap();

   // Async guards - holds shared pointers to AsyncSocketBase so that TurnAsyncSocket 
   // destruction will be delayed if there are outstanding async TurnAsyncSocket calls   
   std::queue<boost::shared_ptr<AsyncSocketBase> > mGuards;
   class GuardReleaser 
   {
   public:
      GuardReleaser(std::queue<boost::shared_ptr<AsyncSocketBase> >&  guards) : mGuards(guards) {}
      ~GuardReleaser() { mGuards.pop(); }
   private:
      std::queue<boost::shared_ptr<AsyncSocketBase> >&  mGuards;
   };

   asio::deadline_timer mAllocationTimer;
   void startAllocationTimer();
   void cancelAllocationTimer();
   void allocationTimerExpired(const asio::error_code& e);

   void doRequestSharedSecret();
   void doSetUsernameAndPassword(resip::Data* username, resip::Data* password, bool shortTermAuth);
   void doBindRequest();
   void doCreateAllocation(unsigned int lifetime = UnspecifiedLifetime,
                           unsigned int bandwidth = UnspecifiedBandwidth,
                           unsigned short requestedPortProps = StunMessage::PortPropsNone, 
                           unsigned short requestedPort = UnspecifiedPort,
                           StunTuple::TransportType requestedTransportType = StunTuple::None, 
                           const asio::ip::address &requestedIpAddress = UnspecifiedIpAddress);
   void doRefreshAllocation(unsigned int lifetime);
   void doDestroyAllocation();
   void doSetActiveDestination(const asio::ip::address& address, unsigned short port);
   void doClearActiveDestination();
   void doSend(boost::shared_ptr<DataBuffer>& data);
   void doSendTo(const asio::ip::address& address, unsigned short port, boost::shared_ptr<DataBuffer>& data);
   void doClose();
   void actualClose();

   StunMessage* createNewStunMessage(UInt16 stunclass, UInt16 method, bool addAuthInfo=true);
   void sendStunMessage(StunMessage* request, bool reTransmission=false);
   void sendTo(RemotePeer& remotePeer, boost::shared_ptr<DataBuffer>& data);
   void send(boost::shared_ptr<DataBuffer>& data);  // Send unframed data
   void send(unsigned short channel, boost::shared_ptr<DataBuffer>& data);  // send with turn framing

   asio::error_code handleStunMessage(StunMessage& stunMessage);
   asio::error_code handleDataInd(StunMessage& stunMessage);
   asio::error_code handleChannelConfirmation(StunMessage &stunMessage);
   asio::error_code handleSharedSecretResponse(StunMessage& stunMessage);
   asio::error_code handleBindRequest(StunMessage& stunMessage);
   asio::error_code handleBindResponse(StunMessage& stunMessage);
   asio::error_code handleAllocateResponse(StunMessage& stunMessage);
   asio::error_code handleRefreshResponse(StunMessage& stunMessage);
};

} 

#endif


/* ====================================================================

 Original contribution Copyright (C) 2007 Plantronics, Inc.
 Provided under the terms of the Vovida Software License, Version 2.0.

 The Vovida Software License, Version 2.0 
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution. 
 
 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.

 ==================================================================== */
