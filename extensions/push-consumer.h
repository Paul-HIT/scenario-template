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

#ifndef NDN_KITE_PUSH_CONSUMER_H
#define NDN_KITE_PUSH_CONSUMER_H


#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ns3/ndnSIM/apps/ndn-consumer.hpp"

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief A simple Interest-sink applia simple Interest-sink application
 *
 * A simple Interest-sink.
 * It also sends out traces periodically to update it's location in the network.
 * In pull scenario, this should run on a mobile node, 
 * and the trace will be set up through an anchor,
 * which is a normal forwarding node that states a special prefix in the topology.
 */
class KitePushConsumer : public Consumer {
public:
  static TypeId
  GetTypeId(void);

  KitePushConsumer();
  virtual ~KitePushConsumer() {};

  void SendTrace(); // Periodically send traced Interest packets(IFI)

  virtual void
  OnInterest(shared_ptr<const Interest> interest);

protected:
  // inherited from Application base class.
  virtual void
  StartApplication(); // Called at time specified by Start

  virtual void
  StopApplication(); // Called at time specified by Stop

  /**
   * \brief Actually does nothing.
   */
  virtual void
  ScheduleNextPacket() {};

private:
  Name m_anchorPrefix;
  Name m_serverPrefix;
  Name m_traceNamePrefix;
  Time m_interestLifeTime; // LifeTime for interest packet(IFI)
  
};

} // namespace ndn
} // namespace ns3

#endif // NDN_PRODUCER_H
