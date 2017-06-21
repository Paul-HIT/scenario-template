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

#include "pull-server.h"
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

NS_LOG_COMPONENT_DEFINE("ndn.kite.KitePullServer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(KitePullServer);

TypeId
KitePullServer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::KitePullServer")
      .SetGroupName("Ndn")
      .SetParent<Consumer>()
      .AddConstructor<KitePullServer>()

      .AddAttribute("TraceNamePrefix", "Prefix of this anchor node", StringValue("/"),
                    MakeNameAccessor(&KitePullServer::m_traceNamePrefix), MakeNameChecker())

      .AddAttribute("ServerPrefix", "Prefix of this stationary server", StringValue("/"),
                    MakeNameAccessor(&KitePullServer::m_serverPrefix), MakeNameChecker())

      .AddAttribute("TracingInterestLifeTime", "LifeTime for tracing Interest packet", StringValue("2s"),
                    MakeTimeAccessor(&KitePullServer::m_tracingInterestLifeTime), MakeTimeChecker())

      .AddAttribute("MaxSeq", "Maximum sequence number to request",
                    IntegerValue(std::numeric_limits<uint32_t>::max()),
                    MakeIntegerAccessor(&KitePullServer::m_seqMax), MakeIntegerChecker<uint32_t>())

    ;

  return tid;
}

KitePullServer::KitePullServer()
{
  NS_LOG_FUNCTION_NOARGS();
  m_seq = 0;
  m_seqMax = std::numeric_limits<uint32_t>::max();
}

// inherited from Application base class.
void
KitePullServer::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  FibHelper::AddRoute(GetNode(), m_serverPrefix, m_face, 0);


  SendInterest();
}

void
KitePullServer::SendInterest()
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();

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

  shared_ptr<Name> nameWithSequence = make_shared<Name>(m_interestName);
  nameWithSequence->appendSequenceNumber(seq);

  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(*nameWithSequence);
  interest->setTraceName(m_traceNamePrefix);
  //time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  time::milliseconds interestLifeTime(m_tracingInterestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  interest->setTraceFlag(2);

  NS_LOG_INFO("> Server: Interest for " << seq << ", Name: " << interest->getName() << ", TraceName: " << interest->getTraceName());

  WillSendOutInterest(seq);

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);

  Simulator::Schedule(Seconds(5), &KitePullServer::SendInterest, this);
}

void 
KitePullServer::OnData(shared_ptr<const Data> data)
{
  NS_LOG_INFO("Server: receive Data: " << data->getName());
}

} // namespace ndn
} // namespace ns3
