/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2017 Harbin Institute of Technology, China
 *
 * Author: Peng Yu
 **/

// topo-upload.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
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
  int speed = 60;        //100
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
  PointToPointHelper p2p;
  PointToPointGridHelper grid (gridSize, gridSize, p2p);
  grid.BoundingBox(0,0,400,400);


  // Create mobile nodes
  NodeContainer wifiNodes;
  NodeContainer mobileNodes;
  mobileNodes.Create (mobileSize);

  // Setup mobility model
  Ptr<RandomRectanglePositionAllocator> randomPosAlloc = CreateObject<RandomRectanglePositionAllocator> ();
  Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable> ();
  x->SetAttribute ("Min", DoubleValue (200));
  x->SetAttribute ("Max", DoubleValue (450));
  randomPosAlloc->SetX (x);
    Ptr<UniformRandomVariable> y = CreateObject<UniformRandomVariable> ();
  y->SetAttribute ("Min", DoubleValue (250));
  y->SetAttribute ("Max", DoubleValue (500));
  randomPosAlloc->SetY (y);

  MobilityHelper mobility;
  mobility.SetPositionAllocator(randomPosAlloc);
  std::stringstream ss;
  ss << "ns3::UniformRandomVariable[Min=" << speed << "|Max=" << speed << "]";

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
    "Bounds", RectangleValue (Rectangle (200, 450, 250, 500)),
    "Distance", DoubleValue (150),
    "Speed", StringValue (ss.str ()));

  // Make mobile nodes move
  mobility.Install (mobileNodes);

  // Setup initial position of mobile node
  Ptr<ListPositionAllocator> posAlloc = CreateObject<ListPositionAllocator> ();
  //posAlloc->Add (Vector (200.0, 30.0, 0.0));
  posAlloc->Add (Vector (450.0, 500.0, 0.0));
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
  wifiChannel.AddPropagationLoss ("ns3::LogDistancePropagationLossModel", "Exponent", DoubleValue (3.35));
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifi.Install (wifiPhy, wifiMac, wifiNodes.GetGlobal());

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

  
  for (int i = 0; i<gridSize; i++)
  {
    for (int j = 0; j<gridSize; j++)
    {
        //ndnHelper.Install (grid.GetNode (i, j));
        if (i ==0 && j==0) continue;
        int m = i>j ? (i - 1) : i;
        int n = i>j ? j : (j - 1);
        ndn::FibHelper::AddRoute (grid.GetNode (i, j), serverPrefix, grid.GetNode (m, n), 1);
    }
  }

  // Installing applications
  // Stationary server
  ndn::AppHelper serverHelper("ns3::ndn::KiteUploadServer");
  serverHelper.SetPrefix(mobilePrefix);
  serverHelper.SetAttribute("ServerPrefix", StringValue(serverPrefix));
  serverHelper.Install(grid.GetNode(0, 0));                        // first node

  // Mobile node
  ndn::AppHelper mobileNodeHelper("ns3::ndn::KiteUploadMobile");
  mobileNodeHelper.SetPrefix(mobilePrefix);
  mobileNodeHelper.SetAttribute("ServerPrefix", StringValue(serverPrefix));
  mobileNodeHelper.SetAttribute("MobilePrefix", StringValue(mobilePrefix));
  mobileNodeHelper.SetAttribute("PayloadSize", StringValue("1024"));
  mobileNodeHelper.Install(mobileNodes.Get(0)); // last node

  L2RateTracer::InstallAll("drop-trace.txt", Seconds(5));
  ndn::L3RateTracer::InstallAll("rate-trace.txt", Seconds(5));
  ndn::AppDelayTracer::InstallAll("app-delays-trace.txt");

  Simulator::Stop(Seconds(100.0));

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
