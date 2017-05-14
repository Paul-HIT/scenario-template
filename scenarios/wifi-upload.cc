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

#include "ndn-kite-upload-server.h"
#include "ndn-kite-upload-mobile.h"

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

  int isKite = 1;
  int gridSize = 3;
  int mobileSize = 1;
  int speed = 100;
  int stopTime = 100;
  int joinTime = 1;

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.AddValue("kite", "enable Kite", isKite);
  cmd.AddValue("speed", "mobile speed m/s", speed);
  cmd.AddValue("size", "# mobile", mobileSize);
  cmd.AddValue("grid", "grid size", gridSize);  
  cmd.AddValue("stop", "stop time", stopTime);  
  cmd.AddValue("join", "join period", joinTime); 
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(4);

  MobilityHelper mobility;
  Ptr<ListPositionAllocator> posAlloc = CreateObject<ListPositionAllocator> ();
  posAlloc->Add (Vector (0.0, 0.0, 0.0));
  posAlloc->Add (Vector (100.0, 0.0, 0.0));
  posAlloc->Add (Vector (200.0, 100.0, 0.0));
  posAlloc->Add (Vector (200.0, -100.0, 0.0));
  mobility.SetPositionAllocator (posAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  PointToPointHelper p2p;
  p2p.Install(nodes.Get(0), nodes.Get(1));
  p2p.Install(nodes.Get(1), nodes.Get(2));
  p2p.Install(nodes.Get(1), nodes.Get(3));

  // Create mobile nodes
  NodeContainer mobileNodes;
  mobileNodes.Create (mobileSize);

  // Setup mobility model
  Ptr<RandomRectanglePositionAllocator> randomPosAlloc = CreateObject<RandomRectanglePositionAllocator> ();
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute ("Min", DoubleValue (200));
  x->SetAttribute ("Max", DoubleValue (250));
  randomPosAlloc->SetX (x);
    Ptr<UniformRandomVariable> y = CreateObject<UniformRandomVariable> ();
  y->SetAttribute ("Min", DoubleValue (-100));
  y->SetAttribute ("Max", DoubleValue (100));
  randomPosAlloc->SetY (y);

  mobility.SetPositionAllocator(randomPosAlloc);
  std::stringstream ss;
  ss << "ns3::UniformRandomVariable[Min=" << speed << "|Max=" << speed << "]";

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
    "Bounds", RectangleValue (Rectangle (200, 250, -100, 100)),
    "Distance", DoubleValue (200),
    "Speed", StringValue (ss.str ()));

  // Make mobile nodes move
  mobility.Install (mobileNodes);

  // Setup initial position of mobile node
  posAlloc = CreateObject<ListPositionAllocator> ();
  posAlloc->Add (Vector (200.0, 30.0, 0.0));
  mobility.SetPositionAllocator (posAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (mobileNodes);

  //apply wifi component on mobile-nodes and constant-nodes
  WifiHelper wifi;
  wifi.SetStandard (WIFI_PHY_STANDARD_80211a);
  // Set to a non-QoS upper mac
  NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
  // Set it to adhoc mode
  wifiMac.SetType ("ns3::AdhocWifiMac");
  // Set Wi-Fi rate manager
  std::string phyMode ("OfdmRate54Mbps");
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",StringValue (phyMode), "ControlMode",StringValue (phyMode));

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  // ns-3 supports RadioTap and Prism tracing extensions for 802.11
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel", "Exponent", DoubleValue (3.0));
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifi.Install (wifiPhy, wifiMac, mobileNodes);
  wifi.Install (wifiPhy, wifiMac, nodes.Get(2));
  wifi.Install (wifiPhy, wifiMac, nodes.Get(3));

  ndn::StackHelper ndnHelper;
  ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  //ndn::StrategyChoiceHelper::Install(nodes.Get(0), "/", "/localhost/nfd/strategy/kite-trace");
  //ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/kite-trace");
  ndn::StrategyChoiceHelper::InstallAll<nfd::fw::KiteTraceStrategy>("/");
  //ndn::StrategyChoiceHelper::InstallAll<nfd::fw::KiteTraceStrategy>("/");

  std::string serverPrefix = "/server";
  std::string mobilePrefix = "/mobile";

  ndn::FibHelper::AddRoute (nodes.Get(1), serverPrefix, nodes.Get(0), 1);

  // Installing applications
  // Stationary server
  ndn::AppHelper serverHelper("ns3::ndn::KiteUploadServer");
  serverHelper.SetPrefix(mobilePrefix);
  serverHelper.SetAttribute("ServerPrefix", StringValue(serverPrefix));
  serverHelper.Install(nodes.Get(0));                        // first node

  // Mobile node
  ndn::AppHelper mobileNodeHelper("ns3::ndn::KiteUploadMobile");
  mobileNodeHelper.SetPrefix(mobilePrefix);
  mobileNodeHelper.SetAttribute("ServerPrefix", StringValue(serverPrefix));
  mobileNodeHelper.SetAttribute("MobilePrefix", StringValue(mobilePrefix));
  mobileNodeHelper.SetAttribute("PayloadSize", StringValue("1024"));
  mobileNodeHelper.Install(mobileNodes.Get(0)); // last node

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
