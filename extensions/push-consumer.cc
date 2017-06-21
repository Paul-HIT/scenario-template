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

#include "push-consumer.h"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"

#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"

#include <memory>
#include <ctime>

NS_LOG_COMPONENT_DEFINE("ndn.kite.KitePushConsumer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(KitePushConsumer);

TypeId
KitePushConsumer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::KitePushConsumer")
      .SetGroupName("Ndn")
      .SetParent<Consumer>()
      .AddConstructor<KitePushConsumer>()
      
      .AddAttribute("AnchorPrefix", "Name of the anchor", StringValue("/"),
                    MakeNameAccessor(&KitePushConsumer::m_anchorPrefix), MakeNameChecker())
      .AddAttribute("ServerPrefix", "Name of the server", StringValue("/"),
                    MakeNameAccessor(&KitePushConsumer::m_serverPrefix), MakeNameChecker())
      .AddAttribute("InterestLifeTime", "LifeTime for traced Interest packet", StringValue("2s"),
                    MakeTimeAccessor(&KitePushConsumer::m_interestLifeTime), MakeTimeChecker())

      .AddAttribute("TraceNamePrefix", "Prefix of this anchor node", StringValue("/"),
                    MakeNameAccessor(&KitePushConsumer::m_traceNamePrefix), MakeNameChecker())

      .AddAttribute("MaxSeq", "Maximum sequence number to request",
                    IntegerValue(std::numeric_limits<uint32_t>::max()),
                    MakeIntegerAccessor(&KitePushConsumer::m_seqMax), MakeIntegerChecker<uint32_t>())
    ;
  return tid;
}

KitePushConsumer::KitePushConsumer()
{
  NS_LOG_FUNCTION_NOARGS();
  m_seq = 0;
  m_seqMax = std::numeric_limits<uint32_t>::max();
}

// inherited from Application base class.
void
KitePushConsumer::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  FibHelper::AddRoute(GetNode(), m_serverPrefix, m_face, 0);
  
  SendTrace();
}

void
KitePushConsumer::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  App::StopApplication();
}

void
KitePushConsumer::SendTrace()
{

  NS_LOG_FUNCTION_NOARGS();

  // periodly send traced Interest with anchor prefix to set up a from-anchor-to-mobile trace.
  shared_ptr<Name> name = make_shared<Name>(m_anchorPrefix); 

  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(*name);
  interest->setTraceFlag(1);
  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  NS_LOG_INFO("> Traced Interest Name: " << name->toUri() << ", sent to Face: " << *m_face);

  Simulator::Schedule(Seconds(1.9), &KitePushConsumer::SendTrace, this); // Send out trace at intervals equal to lifetime of trace

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);
  //Send upload request.
}

void
KitePushConsumer::OnInterest(shared_ptr<const Interest> interest)
{
  if (interest->hasTraceName()) {
    NS_LOG_INFO("Mobile: Receive tracing Interest: " << interest->getName() << ", TraceName: " << interest->getTraceName());

    uint32_t seq = std::numeric_limits<uint32_t>::max(); // invalid

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
    }

    shared_ptr<Name> nameWithSequence = make_shared<Name>(m_serverPrefix);
    nameWithSequence->appendSequenceNumber(seq);

    shared_ptr<Interest> interest = make_shared<Interest>();
    interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
    interest->setName(*nameWithSequence);
    time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
    interest->setInterestLifetime(interestLifeTime);

    NS_LOG_INFO("> Normal Interest Name: " << nameWithSequence->toUri() << ", sent to Face: " << *m_face);

    WillSendOutInterest(seq);

    m_transmittedInterests(interest, this, m_face);
    m_appLink->onReceiveInterest(*interest);
  }
  else {
    NS_LOG_INFO("Mobile: Receive normal Interest: " << interest->getName());
    // Producer::OnInterest(interest);
  }
}

} // namespace ndn
} // namespace ns3
