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

#include "ndn-kite-upload-mobile.h"
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

NS_LOG_COMPONENT_DEFINE("ndn.kite.KiteUploadMobile");

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED(KiteUploadMobile);

TypeId
KiteUploadMobile::GetTypeId(void)
{
  static TypeId tid =
    TypeId("ns3::ndn::KiteUploadMobile")
      .SetGroupName("Ndn")
      .SetParent<Producer>()
      .AddConstructor<KiteUploadMobile>()

      .AddAttribute("Randomize",
                    "Type of send time randomization: none (default), uniform, exponential",
                    StringValue("none"),
                    MakeStringAccessor(&KiteUploadMobile::SetRandomize, &KiteUploadMobile::GetRandomize),
                    MakeStringChecker())

      .AddAttribute("ServerPrefix", "Name of the server", StringValue("/"),
                    MakeNameAccessor(&KiteUploadMobile::m_serverPrefix), MakeNameChecker())
      .AddAttribute("MobilePrefix", "Name of this node", StringValue("/"),
                    MakeNameAccessor(&KiteUploadMobile::m_mobilePrefix), MakeNameChecker())
      .AddAttribute("InterestLifeTime", "LifeTime for traced Interest packet", StringValue("2s"),
                    MakeTimeAccessor(&KiteUploadMobile::m_interestLifeTime), MakeTimeChecker())
    ;
  return tid;
}

KiteUploadMobile::KiteUploadMobile()
  : m_rand(CreateObject<UniformRandomVariable>())
  , m_seq(0) 
{
  NS_LOG_FUNCTION_NOARGS();
}

// inherited from Application base class.
void
KiteUploadMobile::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  Producer::StartApplication();
  
  SendTrace();
}

void
KiteUploadMobile::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();

  App::StopApplication();
}

void
KiteUploadMobile::SendTrace()
{

  NS_LOG_FUNCTION_NOARGS();

  shared_ptr<Name> name = make_shared<Name>(m_serverPrefix); // consumer is actually a stationary server under upload scenario

  shared_ptr<Interest> interest = make_shared<Interest>();
  interest->setNonce(m_rand->GetValue(0, std::numeric_limits<uint32_t>::max()));
  interest->setName(*name);
  interest->setTraceFlag(1);
  time::milliseconds interestLifeTime(m_interestLifeTime.GetMilliSeconds());
  interest->setInterestLifetime(interestLifeTime);

  NS_LOG_INFO("> Traced Interest Name: " << name->toUri() << ", sent to Face: " << *m_face);

  Simulator::Schedule(Seconds(2.1), &KiteUploadMobile::SendTrace, this); // Send out trace at intervals equal to lifetime of trace

  m_transmittedInterests(interest, this, m_face);
  m_appLink->onReceiveInterest(*interest);
  //Send upload request.
}

void
KiteUploadMobile::OnInterest(shared_ptr<const Interest> interest)
{
  if (interest->hasTraceName()) {
    NS_LOG_INFO("Mobile: Receive tracing Interest: " << interest->getName() << ", TraceName: " << interest->getTraceName());
  }
  else {

    NS_LOG_INFO("Mobile: Receive normal Interest: " << interest->getName());
  }

  //Producer::OnInterest(interest);
}

void
KiteUploadMobile::SetRandomize(const std::string& value)
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
KiteUploadMobile::GetRandomize() const
{
  return m_randomType;
}

} // namespace ndn
} // namespace ns3
