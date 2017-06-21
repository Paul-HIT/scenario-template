/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "ndn-kite-upload-server.h"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"

#include "helper/ndn-fib-helper.hpp"

NS_LOG_COMPONENT_DEFINE("ndn.kite.KiteUploadServer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(KiteUploadServer);

TypeId
KiteUploadServer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::KiteUploadServer")
      .SetGroupName("Ndn")
      .SetParent<Consumer>()
      .AddConstructor<KiteUploadServer>()

      .AddAttribute("ServerPrefix", "Prefix of this stationary server", StringValue("/"),
                    MakeNameAccessor(&KiteUploadServer::m_serverPrefix), MakeNameChecker())

      .AddAttribute("TracingInterestLifeTime", "LifeTime for tracing Interest packet", StringValue("2.5s"),
                    MakeTimeAccessor(&KiteUploadServer::m_tracingInterestLifeTime), MakeTimeChecker())

      .AddAttribute("MaxSeq", "Maximum sequence number to request",
                    IntegerValue(std::numeric_limits<uint32_t>::max()),
                    MakeIntegerAccessor(&KiteUploadServer::m_seqMax), MakeIntegerChecker<uint32_t>())

    ;

  return tid;
}

KiteUploadServer::KiteUploadServer()
{
  NS_LOG_FUNCTION_NOARGS();
  m_seq = 0;
  m_seqMax = std::numeric_limits<uint32_t>::max();
}

// inherited from Application base class.
void
KiteUploadServer::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  FibHelper::AddRoute(GetNode(), m_serverPrefix, m_face, 0);
}

void
KiteUploadServer::OnInterest(shared_ptr<const Interest> interest)
{
  App::OnInterest(interest); // tracing inside
  NS_LOG_INFO ("Server: m_interestName: " << m_interestName);

  NS_LOG_FUNCTION(this << interest);

  if (!m_active)
    return;

  if (interest->hasTraceName()) {
    NS_LOG_INFO("node(" << GetNode()->GetId() << ") received Interest for: " << interest->getName()
                      << ", trace name: " << interest->getTraceName() << ", trace flag: " << int(interest->getTraceFlag())
    );
  }
  else {
    NS_LOG_INFO("node(" << GetNode()->GetId() << ") received Interest for: " << interest->getName()
                      << " without TraceName, trace flag: " << int(interest->getTraceFlag())
    );
  }
  

  if (interest->getTraceFlag() == 1) {
    SendInterest(interest, 2); // send out a traceOnly Interest packet
  }
  else{
    //Return Data to the interest-requester.
    Name dataName = (interest->getName());

    auto data = make_shared<Data>();
    data->setName(dataName);
    data->setFreshnessPeriod(::ndn::time::milliseconds(0));  //Freshness of data packets, if 0, then unlimited freshness.

    data->setContent(make_shared< ::ndn::Buffer>(1024));

    Signature signature;
    SignatureInfo signatureInfo(static_cast< ::ndn::tlv::SignatureTypeValue>(255));

    signature.setInfo(signatureInfo);
    signature.setValue(::ndn::makeNonNegativeIntegerBlock(::ndn::tlv::SignatureValue, 0));

    data->setSignature(signature);

    NS_LOG_INFO("node(" << GetNode()->GetId() << ") responding with Data: " << data->getName());

    //to create real wire encoding
    data->wireEncode();

    m_transmittedDatas(data, this, m_face);
    m_appLink->onReceiveData(*data);
  }
}

void
KiteUploadServer::SendInterest(shared_ptr<const Interest> tracedInterest, uint8_t traceFlag)
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();

  NS_LOG_INFO("node(" << GetNode()->GetId() << ") received IFI with name: " << tracedInterest->getName() << ", Nonce: " << tracedInterest->getNonce());

  /*uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid

  while (m_retxSeqs.size()) {
    seq = *m_retxSeqs.begin();
    m_retxSeqs.erase(m_retxSeqs.begin());
    break;
  }

  if (seq == std::numeric_limits<uint32_t>::max()) {
    if (m_seqMax != std::numeric_limits<uint32_t>::max()) {
      if (m_seq >= m_seqMax) {
        return; // we are totally done
      }
    }

    seq = m_seq++;
  }*/
  uint32_t seq = m_seq++;

  shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);
  nameWithSequence->appendSequenceNumber(seq);

  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(*nameWithSequence);
  interest->setTraceName(tracedInterest->getName());
  //time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  time::milliseconds interestLifeTime(m_tracingInterestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  if (traceFlag) {
    interest->setTraceFlag(traceFlag);
  }

  NS_LOG_INFO("> Interest for " << seq << ", Name: " << interest->getName() << ", TraceName: " << interest->getTraceName());

  WillSendOutInterest(seq);

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);

  ScheduleNextPacket();
}

std::set<std::string> s;

void 
KiteUploadServer::OnData(shared_ptr<const Data> data)
{
  NS_LOG_INFO("Server: receive Data: " << data->getName());
  
  s.insert(data->getName().toUri());
  NS_LOG_INFO("CURRENT Data ammount: "<< data->getName().toUri() << ": " << s.size());
}

} // namespace ndn
} // namespace ns3
