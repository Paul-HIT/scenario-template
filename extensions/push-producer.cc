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

#include "push-producer.h"
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

NS_LOG_COMPONENT_DEFINE("ndn.kite.KitePushProducer");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(KitePushProducer);

TypeId
KitePushProducer::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::KitePushProducer")
      .SetGroupName("Ndn")
      .SetParent<Producer>()
      .AddConstructor<KitePushProducer>()

      .AddAttribute("TraceNamePrefix", "Prefix of this anchor node", StringValue("/"),
                    MakeNameAccessor(&KitePushProducer::m_traceNamePrefix), MakeNameChecker())

      .AddAttribute("ServerPrefix", "Prefix of this stationary server", StringValue("/"),
                    MakeNameAccessor(&KitePushProducer::m_serverPrefix), MakeNameChecker())

      .AddAttribute("TracingInterestLifeTime", "LifeTime for tracing Interest packet", StringValue("2s"),
                    MakeTimeAccessor(&KitePushProducer::m_tracingInterestLifeTime), MakeTimeChecker())

    ;

  return tid;
}

KitePushProducer::KitePushProducer()
  : m_rand(CreateObject<UniformRandomVariable>())
  , m_seq(0) 
{
  NS_LOG_FUNCTION_NOARGS();
}

// inherited from Application base class.
void
KitePushProducer::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  Producer::StartApplication();

  SendInterest();
}

void
KitePushProducer::SendInterest()
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();

  shared_ptr<Name> name = make_shared<Name>(m_serverPrefix);

  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(*name);
  interest->setTraceName(m_traceNamePrefix);
  //time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  time::milliseconds interestLifeTime(m_tracingInterestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  interest->setTraceFlag(2);

  NS_LOG_INFO("> Server: Interest, Name: " << interest->getName() << ", TraceName: " << interest->getTraceName());

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);

  Simulator::Schedule(Seconds(2.1), &KitePushProducer::SendInterest, this);
}

void 
KitePushProducer::OnInterest(shared_ptr<const Interest> interest)
{
  NS_LOG_INFO("Server: receive Interest: " << interest->getName());
  if (!interest->getTraceFlag()) {
    Producer::OnInterest(interest);
  }
}


} // namespace ndn
} // namespace ns3
