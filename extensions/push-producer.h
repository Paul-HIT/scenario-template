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

#ifndef NDN_KITE_PUSH_PRODUCER_H
#define NDN_KITE_PUSH_PRODUCER_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ns3/random-variable-stream.h"
#include "ns3/nstime.h"
#include "ns3/ptr.h"

#include "ns3/ndnSIM/apps/ndn-producer.hpp"

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief Ndn application that runs as the stationary server in upload scenarios supporting Kite scheme.
 * This one is a server application, it waits for Interest packets from mobile nodes that serve as upload requests,
 * It then sends out Interest towards the mobile node to pull the data it tries to upload.
 * Currently, the name of the uploading mobile node is fixed.
 * Eventually the upload request should include information about the mobile node,
 * and certain verification machanisms should be applied so that this won't be exploited to conduct DDoS attacks.
 */
class KitePushProducer : public Producer {
public:
  static TypeId
  GetTypeId();

  KitePushProducer();
  virtual ~KitePushProducer() {};

  virtual void 
  OnInterest(shared_ptr<const Interest> interest);

  /**
   * @brief Actually send packet, with TraceFlag option
   */
  void
  SendInterest();

protected:
  // from App
  virtual void
  StartApplication();

  /**
   * \brief Actually does nothing.
   */
  virtual void
  ScheduleNextPacket() {};

protected:
  Name m_serverPrefix;
  Name m_traceNamePrefix;
  Time m_tracingInterestLifeTime;

  Ptr<UniformRandomVariable> m_rand; ///< @brief nonce generator
  Ptr<RandomVariableStream> m_random;
  std::string m_randomType;
  
  int m_seq;
};

} // namespace ndn
} // namespace ns3

#endif
