/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2017 Harbin Institute of Technology, China
 *
 * Author: Zhongda Xia <xiazhongda@hit.edu.cn>
 **/

// ndn-simple-kite.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"

#include "share-mobile.h"

#include "fw/kite-trace-strategy.hpp"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE("ndn.kite.WifiUpload");

int
main(int argc, char* argv[])
{
  LogComponentEnable("nfd.KiteTraceStrategy", LOG_LEVEL_INFO);
  // LogComponentEnable("ndn.Producer", LOG_LEVEL_INFO);
  // LogComponentEnable("ndn.Consumer", LOG_LEVEL_INFO);

  // LogComponentEnable("ndn.kite.KiteUploadServer", LOG_LEVEL_INFO);
  // LogComponentEnable("ndn.kite.KiteUploadMobile", LOG_LEVEL_INFO);

  // LogComponentEnable("nfd.TraceTable", LOG_LEVEL_INFO);

  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("20"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(5);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> posAlloc = CreateObject<ListPositionAllocator> ();
  posAlloc->Add (Vector (0.0, 0.0, 0.0));
  posAlloc->Add (Vector (0.0, 200.0, 0.0));
  posAlloc->Add (Vector (100.0, 100.0, 0.0));
  posAlloc->Add (Vector (200.0, 200.0, 0.0));
  posAlloc->Add (Vector (100.0, 300.0, 0.0));
  mobility.SetPositionAllocator (posAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  PointToPointHelper p2p;
  p2p.Install(nodes.Get(0), nodes.Get(2));
  p2p.Install(nodes.Get(1), nodes.Get(2));
  p2p.Install(nodes.Get(2), nodes.Get(3));
  p2p.Install(nodes.Get(4), nodes.Get(3));

  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  //ndn::StrategyChoiceHelper::Install(nodes.Get(0), "/", "/localhost/nfd/strategy/kite-trace");
  //ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/kite-trace");
  ndn::StrategyChoiceHelper::InstallAll<nfd::fw::KiteTraceStrategy>("/");
  //ndn::StrategyChoiceHelper::InstallAll<nfd::fw::KiteTraceStrategy>("/");

  std::string chatRoomPrefix = "/chatroom";
  std::string dataPrefix = "/next.msg";
  std::string mobilePrefix = "/join";

  ndn::FibHelper::AddRoute (nodes.Get(2), chatRoomPrefix, nodes.Get(3), 1);

  // Don't add the next line code. If added, Interest will be drop at anchor node, won't be saved in pit.
  // ndn::FibHelper::AddRoute (nodes.Get(3), chatRoomPrefix, nodes.Get(3), 1);

  // Mobile node
  ndn::AppHelper mobileNodeHelper("ns3::ndn::KiteShareMobile");
  mobileNodeHelper.SetPrefix(mobilePrefix);

  mobileNodeHelper.SetAttribute("ChatRoomPrefix", StringValue(chatRoomPrefix + mobilePrefix));
  mobileNodeHelper.SetAttribute("DataPrefix", StringValue(chatRoomPrefix + dataPrefix));
  mobileNodeHelper.SetAttribute("PayloadSize", StringValue("1024"));
  ApplicationContainer mobileApp1 = mobileNodeHelper.Install(nodes.Get(0)); // mobile joiner node
  mobileApp1.Start(Seconds(0));

  ApplicationContainer mobileApp2 = mobileNodeHelper.Install(nodes.Get(1)); // mobile joiner node
  mobileApp2.Start(Seconds(2));

  ApplicationContainer mobileApp3 = mobileNodeHelper.Install(nodes.Get(4)); // mobile joiner node
  mobileApp3.Start(Seconds(4));

  L2RateTracer::InstallAll("drop-trace.txt", Seconds(5.0));
  ndn::L3RateTracer::InstallAll("rate-trace.txt", Seconds(5.0));

  Simulator::Stop(Seconds(20.0));

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
