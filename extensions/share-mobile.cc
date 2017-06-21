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

#include "share-mobile.h"
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

NS_LOG_COMPONENT_DEFINE("ndn.kite.KiteShareMobile");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(KiteShareMobile);

TypeId
KiteShareMobile::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::KiteShareMobile")
      .SetGroupName("Ndn")
      .SetParent<Producer>()
      .AddConstructor<KiteShareMobile>()

      .AddAttribute("Randomize",
                    "Type of send time randomization: none (default), uniform, exponential",
                    StringValue("none"),
                    MakeStringAccessor(&KiteShareMobile::SetRandomize, &KiteShareMobile::GetRandomize),
                    MakeStringChecker())
      
      .AddAttribute("ChatRoomPrefix", "Name of the chat room", StringValue("/"),
                    MakeNameAccessor(&KiteShareMobile::m_chatRoomPrefix), MakeNameChecker())
      .AddAttribute("DataPrefix", "Name of the wanted date prefix in Interest", StringValue("/"),
                    MakeNameAccessor(&KiteShareMobile::m_dataPrefix), MakeNameChecker())
      .AddAttribute("InterestLifeTime", "LifeTime for traced Interest packet", StringValue("5s"),
                    MakeTimeAccessor(&KiteShareMobile::m_interestLifeTime), MakeTimeChecker())
      .AddAttribute("TracingInterestLifeTime", "LifeTime for traced Interest packet", StringValue("3s"),
                    MakeTimeAccessor(&KiteShareMobile::m_tracingInterestLifeTime), MakeTimeChecker())
    ;
  return tid;
}

KiteShareMobile::KiteShareMobile()
  : m_rand(CreateObject<UniformRandomVariable>())
  , m_seq(0) 
{
  NS_LOG_FUNCTION_NOARGS();
}

// inherited from Application base class.
void
KiteShareMobile::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  Producer::StartApplication();
  
  SendTrace();
}

void
KiteShareMobile::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  App::StopApplication();
}

void
KiteShareMobile::SendTrace()
{

  NS_LOG_FUNCTION_NOARGS();

  // periodly send traced Interest with chat room prefix to set up a from-anchor-to-mobile trace.
  shared_ptr<Name> name = make_shared<Name>(m_chatRoomPrefix); 

  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(*name);
  interest->setTraceFlag(1);
  interest->setTraceName(m_chatRoomPrefix);
  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  NS_LOG_INFO("> Traced Interest Name (same as TraceName): " << name->toUri() << ", sent to Face: " << *m_face);

  Simulator::Schedule(Seconds(5), &KiteShareMobile::SendTrace, this); // Send out trace at intervals equal to lifetime of trace

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);
  //Send upload request.
}

void
KiteShareMobile::OnInterest(shared_ptr<const Interest> interest)
{
  if (interest->hasTraceName() && interest->getTraceFlag() == 1) {
    // Trace has been set up.
    NS_LOG_INFO("Mobile: Receive traced Interest: " << interest->getName() << ", TraceName: " << interest->getTraceName());
    SendTracingInterest();
  }
  else if (interest->hasTraceName() && interest->getTraceFlag() == 2) {
    // An Interest that wants data.
    NS_LOG_INFO("Mobile: Receive tracing Interest: " << interest->getName() << ", TraceName: " << interest->getTraceName());
    NS_LOG_INFO("Mobile: Will send Data.");
    Producer::OnInterest(interest);
  }
  else {
    NS_LOG_INFO("Mobile: Receive normal Interest: " << interest->getName());
    NS_LOG_INFO("Mobile: Will send Data.");

    // Producer::OnInterest(interest);
  }
}

void
KiteShareMobile::OnData(shared_ptr<const Data> data) {
  NS_LOG_INFO("Receive Data Named: " << data->getName());
}

void
KiteShareMobile::SendTracingInterest()
{
  if (!m_active)
    return;

  NS_LOG_FUNCTION_NOARGS();

  uint32_t seq = 0;
  if (m_seq != std::numeric_limits<uint32_t>::max()) {
    seq = m_seq ++;        //simply add, not consider seq conflict in the transmit window.
  }
  else {
    m_seq = 0;
  }

  //data request with customized nonce.
  std::string data = "/chatroom/next.msg";

  shared_ptr<Name> nameWithSequence = make_shared<Name>(m_dataPrefix);
  nameWithSequence->appendSequenceNumber(seq);

  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(*nameWithSequence);
  interest->setTraceName(m_chatRoomPrefix);
  //time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  time::milliseconds interestLifeTime(m_tracingInterestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  interest->setTraceFlag(2);

  NS_LOG_INFO("> TracingInterest for " << seq << ", Name: " << interest->getName() << ", TraceName: " << interest->getTraceName());

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);

  Simulator::Schedule(Seconds(5), &KiteShareMobile::SendTrace, this);
}

void
KiteShareMobile::SetRandomize(const std::string& value)
{
  // if (value == "uniform") {
  //   m_random = CreateObject<UniformRandomVariable>();
  //   m_random->SetAttribute("Min", DoubleValue(0.0));
  //   m_random->SetAttribute("Max", DoubleValue(2 * 1.0 / m_frequency));
  // }
  // else if (value == "exponential") {
  //   m_random = CreateObject<ExponentialRandomVariable>();
  //   m_random->SetAttribute("Mean", DoubleValue(1.0 / m_frequency));
  //   m_random->SetAttribute("Bound", DoubleValue(50 * 1.0 / m_frequency));
  // }
  // else
  //   m_random = 0;
  
  m_random = 0;

  m_randomType = value;
}

std::string
KiteShareMobile::GetRandomize() const
{
  return m_randomType;
}

} // namespace ndn
} // namespace ns3
