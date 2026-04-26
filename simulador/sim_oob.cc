//Projeto de Conclusão de Curso - Especialização Lato Sensu em Computação Aplicada a Industria 4.0 UFRR
//Discente: Jussara M Soares
//https://github.com/jumiso/ecai_arquitetura_oob_lorawan_cps
//Arquivo C++
// bibliotecas
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/forwarder-helper.h"
#include "ns3/error-model.h"
#include "ns3/propagation-module.h"
#include "ns3/lorawan-module.h"
#include "ns3/network-server.h"
#include "ns3/oob-metric-tag.h" // ---OobMetricTag é usado apenas para filtragem de metricas.
#include "ns3/csma-module.h"
#include "ns3/udp-echo-helper.h"
#include "ns3/netanim-module.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <vector>
#include <cstdint>
using namespace ns3;
using namespace ns3::lorawan;

NS_LOG_COMPONENT_DEFINE("OobQ1");
//---VARIÁVEIS GLOBAIS PARA A COLETA DE DADOS---
static std::map<uint64_t, Time> g_txTimeByUid;
static std::vector<double> g_latSec; //Latência geral
static std::vector<double> g_latSecWin;   //Latência apenas da janela de falha


static uint32_t g_oobTx = 0; //Pacotes enviados pelo canal out-of-band
static uint32_t g_oobRx = 0; // Pacotes recebidos pelo canal out-of-band

static uint32_t g_oobTxWin = 0; //Pacotes enviados apenas durante a janela de falha
static uint32_t g_oobRxWin = 0; //Pacotes recebidos apenas durante a janela de falha

static double g_failStartS = 0.0; //Para alocação do início da janela falha
static double g_failEndS = 0.0; //Para alocação do fim da janela de falha



//--Métricas do canal In-band---
static uint32_t g_inTxTotal     = 0; //Total de pacotes enviados no canal in-band
static uint32_t g_inTxWin       = 0; //Total de pacotes enviados pelo canal in-band durante a janela de falha, para confirmação de que a simulação está coma lógica correta.

static uint32_t g_inAckRxTotal  = 0; //Total de pacotes de confirmação (ACKs) recebidos do in-band durante toda a simulação.
static uint32_t g_inAckRxWin    = 0; //Total de pacotes de confirmação (ACKs) recebidos do canal in-band durante a janela de falha, para confirmação de que a simulação está com a lógica correta.

static std::vector<double> g_detTdSec;   // Td (que é o tempode resposta) por ED (em segundos), para P50/P95
// Estado por ED para detecção (N perdas consecutivas)
static std::vector<uint32_t> g_missCount; // Contabiliza os pacotes perdidos
static std::vector<bool> g_ackSinceLastTx; // Retorna verdadeiro ou falso para os recebimentos de ACK a cada vez.
static std::vector<bool> g_detected; // Retorna verdadeiro ou falso após o missCount atingir o limite de pacotes perdidos.

static uint32_t g_missThreshold = 10; // N tentativas sem ACK para declarar in-band down

//Verificação de o tempo está passando pelo periodo correto da falha
static bool
InFailureWindow(Time t)
{
  double s = t.GetSeconds();
  return (s >= g_failStartS && s < g_failEndS);
}

// Registra o timestamp do pacote de diagnóstico transmitido pelo ED, para o cálculo de latência e atualização dos contadores TX.
static void
OobStartSending(Ptr<const Packet> pkt, uint32_t)
{
  uint64_t uid = pkt->GetUid();
  g_txTimeByUid[uid] = Simulator::Now();
  g_oobTx++;

  if (InFailureWindow(Simulator::Now()))
  {
    g_oobTxWin++;
  }
}


//Filtros de pacotes que cehgam no NS e calculos de latência.
static void
OobReceivedAtNs(Ptr<const Packet> pkt)
{
  //---Contabiliza apenas pacotes marcados como métrica (EDs - dispositivos finais reais)
  OobMetricTag tag;
  if (!pkt->PeekPacketTag(tag))
  {
    return; //ignora interferidores (e qualquer tráfego sem tag)
  }

  uint64_t uid = pkt->GetUid();

  //---Contabiliza apenas o RX se esse UID foi rastreado no TX---
  // Motivo: para evitar o PDR acima de 100% e evita contar interferidores e/ou pacotes duplicados
  auto it = g_txTimeByUid.find(uid);
  if (it == g_txTimeByUid.end())
  {
    return; // não pertence aos EDs monitorados (duplicado/interferidor/etc)
  }

  //---Só aqui conta---
  g_oobRx++;

  //---Calcula o tempo da latência---
  Time d = Simulator::Now() - it->second;
  
  //---Guarda a latência de toda a simulação---
  g_latSec.push_back(d.GetSeconds());

  if (InFailureWindow(Simulator::Now()))
  {
    g_oobRxWin++;
    //---Guarda a latência APENAS para os pacotes da janela de falha---
    g_latSecWin.push_back(d.GetSeconds());
  }
}

//---In-band - COntadores de pacote e ACK ---
static void
InbandTx(uint32_t idx, Ptr<const Packet>)
{
  g_inTxTotal++;
  if (InFailureWindow(Simulator::Now()))
  {
    g_inTxWin++;
  }

  //---Se durante o período da falha não detectou, conta "miss/perdido" quando não houve ACK desde o último TX---
  if (InFailureWindow(Simulator::Now()) && !g_detected[idx])
  {
    if (!g_ackSinceLastTx[idx])
    {
      g_missCount[idx]++;

      if (g_missCount[idx] >= g_missThreshold)
      {
        g_detected[idx] = true;

        // Td medido desde o início da janela de falha
        double td = Simulator::Now().GetSeconds() - g_failStartS;
        g_detTdSec.push_back(td);
      }
    }

    // após enviar, ainda não temos ACK deste envio
    g_ackSinceLastTx[idx] = false;
  }
}

static void
InbandRx(uint32_t idx, Ptr<const Packet>)
{
  g_inAckRxTotal++;
  if (InFailureWindow(Simulator::Now()))
  {
    g_inAckRxWin++;
  }

  // recebeu ACK dá reset
  g_ackSinceLastTx[idx] = true;
  g_missCount[idx] = 0;
}


//Reseta contadores para casos de suspeita de falha do in-band
static double
Percentile(std::vector<double> v, double p)
{
  if (v.empty())
  {
    return NAN;
  }
  std::sort(v.begin(), v.end());
  size_t idx = (size_t)std::ceil(p * v.size()) - 1;
  if (idx >= v.size())
  {
    idx = v.size() - 1;
  }
  return v[idx];
}

//Organização de Data Rate de acordo com os SF utilizados
static uint8_t
SfToEuDataRate(uint32_t sf)
{
  // EU868 helper mapping: (limitação do próprio simulador - sem aplicação do padrão AU915 por enquanto)
  switch (sf)
  {
  case 7:
    return 5;
  case 12:
    return 0;
  default:
    NS_FATAL_ERROR("Unsupported SF. Use 7 or 12.");
    return 0;
  }
}

int
main(int argc, char* argv[])
{
  //---Parâmetros (CLI)---
  std::string env = "outdoor";  // indoor|outdoor interno|externo
  std::string scenario = "C0";  // Cenários de teste C0|C1|C2 consultar tabela no read me
  uint32_t nEd = 5;
  uint32_t sf = 7;

  double simTimeS = 1800; //tempo total da simulação em segundos
  // OOB em dois modos: normal (heartbeat c/baixa frequência) e falha: mensagens mínimas de diagnóstico (frequência maior)
  double oobPeriodNormalS = 900;  // 15 min
  double oobPeriodFailS   = 120;  // 2 min
  double oobTrigDelayS = 20.0; // delay para "confirmar" falha antes de ativar OOB em modo diagnóstico

  double failStartS = 600; //Tempo de início da janela defalha
  double failEndS   = 1200; //Tempo de fim da janela de falha

  // Parâmetros de controle de aleatoriedade (PRNG) do ns-3.
  uint32_t seed = 1; 
  uint32_t run  = 1;

  // Parâmetros para geração de tráfego de fundo (ruído/interferência) na rede LoRa.
  uint32_t nInterferers = 0;        //default 0 - Quantidade de dispositivos fora do canal lorawan, mas que competem pelo espectro.
  double interfererPeriodS = 300; //Intervalo em segundos que cada interferidor leva para transmitir.
  // Jitter simples
  double startJitterS = 5.0;

  // In-band (UDP Echo) - detecção por ACK
  double inEchoIntervalS = 2.0;     // intervalo do Echo 
  uint32_t missThreshold = 10;       // N perdas consecutivas para detectar falha

  //Alocação dos aquivos CSV.
  std::string outCsv = "results/all.csv";

  // MODIFICAÇÃO: CLI para distâncias
  // Defaults por ambiente (baseline)
  double baseDistArg = -1.0;  // -1 => usar default do env
  double stepArg = -1.0;      // -1 => usar default do env
//Tabela completa com descrição disponivel no respositorio do github.
  CommandLine cmd;
  cmd.AddValue("env", "indoor or outdoor", env);
  cmd.AddValue("scenario", "C0, C1, C2", scenario);
  cmd.AddValue("nEd", "Number of end devices", nEd);
  cmd.AddValue("sf", "Spreading Factor (7 or 12)", sf);
  cmd.AddValue("simTime", "Simulation time (s)", simTimeS);
  cmd.AddValue("oobPeriodNormal", "OOB period in normal mode (s)", oobPeriodNormalS);
  cmd.AddValue("oobPeriodFail",   "OOB period in failure mode (s)", oobPeriodFailS);
  cmd.AddValue("failStart", "Failure window start (s)", failStartS);
  cmd.AddValue("failEnd", "Failure window end (s)", failEndS);
  cmd.AddValue("seed", "RNG seed", seed);
  cmd.AddValue("run", "RNG run", run);
  cmd.AddValue("out", "Output CSV path", outCsv);
  cmd.AddValue("nInt", "Number of interfering LoRa EDs (outdoor only)", nInterferers);
  cmd.AddValue("intPeriod", "Interferer period (s)", interfererPeriodS);
  cmd.AddValue("jitter", "Start jitter for ED apps (s)", startJitterS);
  cmd.AddValue("inEchoInt", "In-band Echo interval (s)", inEchoIntervalS);
  cmd.AddValue("missTh", "Miss threshold (N) for in-band failure detection", missThreshold);
  cmd.AddValue("oobTrigDelay", "Delay (s) after failStart to activate OOB fail-mode (hard confirmation)", oobTrigDelayS);
  cmd.AddValue("baseDist", "Base distance (m). -1 uses env default", baseDistArg);
  cmd.AddValue("step", "Step distance (m). -1 uses env default", stepArg);

  cmd.Parse(argc, argv);

  // Validações 
  if (scenario != "C0" && scenario != "C1" && scenario != "C2")
  {
    NS_FATAL_ERROR("Cenario desconhecido. Use C0, C1, C2.");
  }
  if (env != "indoor" && env != "outdoor")
  {
    NS_FATAL_ERROR("Ambiente desconhecido. Use indoor para ambientes internos ou outdoor para áreas externas.");
  }

  g_failStartS = failStartS;
  g_failEndS = failEndS;
  g_missThreshold = missThreshold;

  RngSeedManager::SetSeed(seed);
  RngSeedManager::SetRun(run);

  //---1) Organização da arquitura OOB LoRaWAN (ED - End Device, GW - Gateway, NS - Network Server)
  NodeContainer endDevices;
  endDevices.Create(nEd); //Variável nED adicionada para possibilitar testes de escalabilidade no estresse 1.

  NodeContainer gateways;
  gateways.Create(1);

  NodeContainer networkServer;
  networkServer.Create(1);

  MobilityHelper mob; //Posição fix dos dispositivos
  mob.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mob.Install(endDevices);
  mob.Install(gateways);
  mob.Install(networkServer);

  //---Configuracao do Ambiente: distância + expoente (atenuação de sinal)
  // outdoor pathExp ajustado para 2.7 ( para baseline)
  double pathExp = 2.7;
  double baseDist = 300.0; //em metros
  double step = 20.0; //em metros

  if (env == "indoor")
  {
    pathExp = 3.5;
    baseDist = 30.0;
    step = 5.0;
  }

  //---Configuração das Coordenadas Físicas---
  if (baseDistArg >= 0.0) { baseDist = baseDistArg; }
  if (stepArg >= 0.0) { step = stepArg; }

  gateways.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(0, 0, 0));
  networkServer.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(0, 0, 0));

  for (uint32_t i = 0; i < nEd; i++)
  {
    endDevices.Get(i)->GetObject<MobilityModel>()->SetPosition(Vector(baseDist + step * i, 0, 0));
  }

  // --------- Interferidores (somente no externo) ----------
  NodeContainer interferers;
  if (env == "outdoor" && nInterferers > 0)
  {
    interferers.Create(nInterferers);
    mob.Install(interferers);

    // Coloquei perto do GW pra aumentar chance de colisão
    for (uint32_t i = 0; i < nInterferers; i++)
    {
      interferers.Get(i)->GetObject<MobilityModel>()->SetPosition(Vector(50.0 + 10.0 * i, 10.0, 0));
    }
  }

  // ---Modelos de atenuação do sinal no ambiente---
  //LogDistance - Enfraquecimento pelo aumento da distância
  Ptr<LogDistancePropagationLossModel> distLoss = CreateObject<LogDistancePropagationLossModel>();
  distLoss->SetPathLossExponent(pathExp);

  //Shadowing - nativo do módulo do Magrin e não do NS-3.
  Ptr<CorrelatedShadowingPropagationLossModel> shadow = CreateObject<CorrelatedShadowingPropagationLossModel>();

  //Fading  - Desvanecimento multipercurso
  Ptr<NakagamiPropagationLossModel> fading = CreateObject<NakagamiPropagationLossModel>();

  //Encadeia os modelos
  distLoss->SetNext(shadow);
  shadow->SetNext(fading);

  //---Configuração do meio físico
  Ptr<PropagationDelayModel> delay = CreateObject<ConstantSpeedPropagationDelayModel>();
  Ptr<LoraChannel> channel = CreateObject<LoraChannel>(distLoss, delay);

  LoraPhyHelper phyHelper;
  phyHelper.SetChannel(channel);

  LorawanMacHelper macHelper;
  macHelper.SetRegion(LorawanMacHelper::EU);

  LoraHelper lora;

  // GW devices - dispositivos gateway
  phyHelper.SetDeviceType(LoraPhyHelper::GW);
  macHelper.SetDeviceType(LorawanMacHelper::GW);
  NetDeviceContainer gwDevices = lora.Install(phyHelper, macHelper, gateways);

  // ED devices (Classe A) - nós reais que entram na métrica
  phyHelper.SetDeviceType(LoraPhyHelper::ED);
  macHelper.SetDeviceType(LorawanMacHelper::ED_A);
  NetDeviceContainer edDevices = lora.Install(phyHelper, macHelper, endDevices);

  // Interferidores (também ED_A), mas NÃO entram em métricas e NÃO são registrados no NS
  NetDeviceContainer intDevices;
  if (interferers.GetN() > 0)
  {
    intDevices = lora.Install(phyHelper, macHelper, interferers);
  }

  // Rastreamwento TX (somente endDevices reais)
  for (uint32_t i = 0; i < edDevices.GetN(); ++i)
  {
    Ptr<LoraNetDevice> dev = DynamicCast<LoraNetDevice>(edDevices.Get(i));
    NS_ASSERT(dev);

    Ptr<LoraPhy> phy = dev->GetPhy();
    NS_ASSERT(phy);

    phy->TraceConnectWithoutContext("StartSending", MakeCallback(&OobStartSending));
  }

  // Força o SF via DataRate nos endDevices / Utilizado nesse recorte para analise dos SF fixos nesses ambientes.
  const uint8_t dr = SfToEuDataRate(sf);
  for (uint32_t i = 0; i < edDevices.GetN(); ++i)
  {
    Ptr<LoraNetDevice> dev = DynamicCast<LoraNetDevice>(edDevices.Get(i));
    NS_ASSERT(dev);

    Ptr<LorawanMac> mac = DynamicCast<LorawanMac>(dev->GetMac());
    NS_ASSERT(mac);

    Ptr<ClassAEndDeviceLorawanMac> edMac = DynamicCast<ClassAEndDeviceLorawanMac>(mac);
    NS_ASSERT(edMac);

    edMac->SetDataRate(dr);
  }

  // Mesma taxa nos interferidores (pior caso co-canal)
  for (uint32_t i = 0; i < intDevices.GetN(); ++i)
  {
    Ptr<LoraNetDevice> dev = DynamicCast<LoraNetDevice>(intDevices.Get(i));
    NS_ASSERT(dev);

    Ptr<LorawanMac> mac = DynamicCast<LorawanMac>(dev->GetMac());
    NS_ASSERT(mac);

    Ptr<ClassAEndDeviceLorawanMac> edMac = DynamicCast<ClassAEndDeviceLorawanMac>(mac);
    NS_ASSERT(edMac);

    edMac->SetDataRate(dr);
  }

  // Backbone GW <-> NS (P2P) + helpers
  PointToPointHelper p2pBackhaul;
  p2pBackhaul.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
  p2pBackhaul.SetChannelAttribute("Delay", StringValue("5ms"));

  P2PGwRegistration_t registration;

  Ptr<PointToPointNetDevice> nsP2pDev = nullptr;
  Ptr<PointToPointNetDevice> gwP2pDev = nullptr;

  {
    Ptr<Node> gwNode = gateways.Get(0);
    Ptr<Node> nsNode = networkServer.Get(0);

    NetDeviceContainer backhaul = p2pBackhaul.Install(gwNode, nsNode);

    for (uint32_t k = 0; k < backhaul.GetN(); ++k)
    {
      Ptr<PointToPointNetDevice> p2pDev = DynamicCast<PointToPointNetDevice>(backhaul.Get(k));
      if (!p2pDev) continue;

      if (p2pDev->GetNode() == nsNode)
      {
        nsP2pDev = p2pDev;
      }
      else if (p2pDev->GetNode() == gwNode)
      {
        gwP2pDev = p2pDev;
      }
    }

    NS_ASSERT_MSG(nsP2pDev, "Nao foi possivel encontrar P2P NetDevice no lado Network Server.");
    NS_ASSERT_MSG(gwP2pDev, "Nao foi possivel encontrar P2P NetDevice no lado do Gateway.");

    registration.push_back(std::make_pair(nsP2pDev, gwNode));
  }

  NetworkServerHelper nsHelper;
  nsHelper.SetGatewaysP2P(registration);

  //---Ponto de atenção: só os endDevices reais entram no NS---
  nsHelper.SetEndDevices(endDevices);

  ForwarderHelper forwarderHelper;

  ApplicationContainer nsApps = nsHelper.Install(networkServer.Get(0));
  nsApps.Start(Seconds(0));
  nsApps.Stop(Seconds(simTimeS));

  ApplicationContainer fwdApps = forwarderHelper.Install(gateways);
  fwdApps.Start(Seconds(0));
  fwdApps.Stop(Seconds(simTimeS));

//---Out-Of-Band em dois modos:
// -> Normal (C0): heartbeat de prontidão com baixa frequência, enviando a cada 900 segundos.
// -> Falha  (C1/C2): mensagens mínimas de diagnóstico com maior frequência, enviando a cada 120 segundos.

// App OOB NORMAL
PeriodicSenderHelper oobNormal;
oobNormal.SetPeriod(Seconds(oobPeriodNormalS));
oobNormal.SetPacketSize(12);
oobNormal.SetAttribute("TagMetrics", BooleanValue(true));
oobNormal.SetAttribute("MetricTagValue", UintegerValue(1));
ApplicationContainer edAppsNormal = oobNormal.Install(endDevices);

//---App OOB FALHA (diagnóstico mais frequente)---
PeriodicSenderHelper oobFail;
oobFail.SetPeriod(Seconds(oobPeriodFailS));
oobFail.SetPacketSize(12);
oobFail.SetAttribute("TagMetrics", BooleanValue(true));
oobFail.SetAttribute("MetricTagValue", UintegerValue(1));
ApplicationContainer edAppsFail = oobFail.Install(endDevices);

//---Jitter por nó (aplicado ao modo normal para evitar alinhamento perfeito)---
Ptr<UniformRandomVariable> uv = CreateObject<UniformRandomVariable>();
uv->SetAttribute("Min", DoubleValue(0.0));
uv->SetAttribute("Max", DoubleValue(std::max(0.0, startJitterS)));

for (uint32_t i = 0; i < endDevices.GetN(); ++i)
{
  double off = uv->GetValue();

  if (scenario == "C0")
  {
    // C0: somente heartbeat (modo normal) durante toda a simulação
    edAppsNormal.Get(i)->SetStartTime(Seconds(1.0 + off));
    edAppsNormal.Get(i)->SetStopTime(Seconds(simTimeS));

    // App de falha não roda
    edAppsFail.Get(i)->SetStartTime(Seconds(simTimeS + 1.0));
    edAppsFail.Get(i)->SetStopTime(Seconds(simTimeS + 1.0));
  }
  else
  {
    // C1/C2: dois estágios
    const double trigS = std::min(failStartS + oobTrigDelayS, failEndS);
    // Normal roda até o gatilho (failStart + delay)
    edAppsNormal.Get(i)->SetStartTime(Seconds(1.0 + off));
    edAppsNormal.Get(i)->SetStopTime(Seconds(trigS));
    // Fail-mode roda só após confirmação e somente até failEnd
    edAppsFail.Get(i)->SetStartTime(Seconds(trigS));
    edAppsFail.Get(i)->SetStopTime(Seconds(failEndS));
  }
}
 
  // App interferidor (somente no externo). Não conta métricas.
  if (interferers.GetN() > 0)
  {
    PeriodicSenderHelper intApp;
    intApp.SetPeriod(Seconds(interfererPeriodS));
    intApp.SetPacketSize(12);

    ApplicationContainer intApps = intApp.Install(interferers);
    intApps.Start(Seconds(2.0));
    intApps.Stop(Seconds(simTimeS));
  }

  // Trace RX no NetworkServer
  for (uint32_t i = 0; i < nsApps.GetN(); ++i)
  {
    Ptr<NetworkServer> nsApp = DynamicCast<NetworkServer>(nsApps.Get(i));
    NS_ASSERT(nsApp);
    nsApp->TraceConnectWithoutContext("ReceivedPacket", MakeCallback(&OobReceivedAtNs));
  }
  // ---C2 (Estresse4): derruba o caminho GW->NS durante a janela----
  Ptr<RateErrorModel> emOn = CreateObject<RateErrorModel>();
  emOn->SetAttribute("ErrorRate", DoubleValue(1.0)); // 100% drop

  Ptr<RateErrorModel> emOff = CreateObject<RateErrorModel>();
  emOff->SetAttribute("ErrorRate", DoubleValue(0.0)); // 0% drop

  auto ApplyBackhaulError = [nsP2pDev, emOn, emOff](bool dropAll) {
    if (!nsP2pDev) return;
    if (dropAll)
    {
      nsP2pDev->SetReceiveErrorModel(emOn);
    }
    else
    {
      nsP2pDev->SetReceiveErrorModel(emOff);
    }
  };

  if (scenario == "C2")
  {
    Simulator::Schedule(Seconds(failStartS), [ApplyBackhaulError]() { ApplyBackhaulError(true); });
    Simulator::Schedule(Seconds(failEndS),   [ApplyBackhaulError]() { ApplyBackhaulError(false); });
  }


//---2) In-band: IP (CSMA LAN) + UDP Echo (ACK)---
//---Nó SCADA/servidor do canal principal---
NodeContainer scadaNode;
scadaNode.Create(1);
mob.Install(scadaNode);
scadaNode.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(0, 0, 0));

// Stack IP nos sensores e no SCADA
InternetStackHelper internet;
internet.Install(endDevices);
internet.Install(scadaNode);

// Rede LAN principal (cabeada- ethernet) via CSMA
CsmaHelper csma;
csma.SetChannelAttribute("DataRate", StringValue("100Mbps"));
csma.SetChannelAttribute("Delay", StringValue("1ms"));

NodeContainer inbandAll;
inbandAll.Add(endDevices);
inbandAll.Add(scadaNode);

NetDeviceContainer csmaDevs = csma.Install(inbandAll);

// Endereçamento IP
Ipv4AddressHelper ipv4;
ipv4.SetBase("10.1.1.0", "255.255.255.0");
Ipv4InterfaceContainer ifs = ipv4.Assign(csmaDevs);

// IP do SCADA/usuario final que é o último nó do container
Ipv4Address scadaIp = ifs.GetAddress(inbandAll.GetN() - 1);

// UDP Echo Server no SCADA/usuariofinal
uint16_t port = 9000;
UdpEchoServerHelper echoServer(port);
ApplicationContainer serverApps = echoServer.Install(scadaNode.Get(0));
serverApps.Start(Seconds(0));
serverApps.Stop(Seconds(simTimeS));

// UDP Echo Client em cada sensor
UdpEchoClientHelper echoClient(scadaIp, port);
echoClient.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
echoClient.SetAttribute("Interval", TimeValue(Seconds(inEchoIntervalS)));
echoClient.SetAttribute("PacketSize", UintegerValue(20));

ApplicationContainer clientApps;
for (uint32_t i = 0; i < endDevices.GetN(); ++i)
{
  auto apps = echoClient.Install(endDevices.Get(i));
  apps.Start(Seconds(1.0));
  apps.Stop(Seconds(simTimeS));
  clientApps.Add(apps);
}

// Estado para detecção por ED
g_missCount.assign(nEd, 0);
g_ackSinceLastTx.assign(nEd, true); // começa true para não contar miss antes de iniciar
g_detected.assign(nEd, false);

// Conecta traces para contar TX/RX e detectar falha (Td)
for (uint32_t i = 0; i < nEd; ++i)
{
  Ptr<UdpEchoClient> c = DynamicCast<UdpEchoClient>(clientApps.Get(i));
  NS_ASSERT(c);

  c->TraceConnectWithoutContext("Tx", MakeBoundCallback(&InbandTx, i));
  c->TraceConnectWithoutContext("Rx", MakeBoundCallback(&InbandRx, i));
}

// Falha no canal principal (C1 e C2): derruba recepção no SCADA durante a janela
Ptr<RateErrorModel> inEmOn = CreateObject<RateErrorModel>();
inEmOn->SetAttribute("ErrorRate", DoubleValue(1.0)); // 100% drop do link

Ptr<RateErrorModel> inEmOff = CreateObject<RateErrorModel>();
inEmOff->SetAttribute("ErrorRate", DoubleValue(0.0)); // 0% drop do link

Ptr<NetDevice> scadaDev = csmaDevs.Get(csmaDevs.GetN() - 1);

auto ApplyInbandError = [scadaDev, inEmOn, inEmOff](bool dropAll) {
  Ptr<CsmaNetDevice> nd = DynamicCast<CsmaNetDevice>(scadaDev);
  if (!nd) return;
  nd->SetReceiveErrorModel(dropAll ? inEmOn : inEmOff);
};

if (scenario == "C1" || scenario == "C2")
{
  Simulator::Schedule(Seconds(failStartS), [ApplyInbandError]() { ApplyInbandError(true); });
  Simulator::Schedule(Seconds(failEndS),   [ApplyInbandError]() { ApplyInbandError(false); });
}

  //---3) Executar a simulação---
  Simulator::Stop(Seconds(simTimeS));
  Simulator::Run();

 //---4) Métricas---
  // O que vamos usar para análise e organização do dataset
  double oobPdr = (g_oobTx > 0) ? (double)g_oobRx / (double)g_oobTx : NAN;

  //--Calcula os percetils da latência durante toda a simulação {Vamos ver se fica}
  double latP50 = Percentile(g_latSec, 0.50);

  //---Calcula os percentis -> P50 apenas da janela de falha
  double latWinP50 = Percentile(g_latSecWin, 0.50);

  double winPdr = (g_oobTxWin > 0) ? (double)g_oobRxWin / (double)g_oobTxWin : NAN;

  double inAckPdr = (g_inTxTotal > 0) ? (double)g_inAckRxTotal / (double)g_inTxTotal : NAN;
  double inAckWinPdr = (g_inTxWin > 0) ? (double)g_inAckRxWin / (double)g_inTxWin : NAN;

  double detTdP50 = Percentile(g_detTdSec, 0.50); //Calculo em relação ao período de detecção de falha.

  bool writeHeader = false;
  {
    std::ifstream in(outCsv);
    writeHeader = !in.good() || in.peek() == std::ifstream::traits_type::eof();
  }

 std::ofstream f(outCsv, std::ios::app);
  if (writeHeader)
  {
    f << "env,scenario,nEd,sf,oobPeriodNormal,oobPeriodFail,failStart,failEnd,seed,run,"
     "oobTx,oobRx,oobPdr,latP50,latWinP50,oobTxWin,oobRxWin,winPdr,"
     "inTxTotal,inAckRxTotal,inAckPdr,inTxWin,inAckRxWin,inAckWinPdr,detTdP50\n";
    }
  f << env << "," << scenario << "," << nEd << "," << sf << ","
    << oobPeriodNormalS << "," << oobPeriodFailS << ","
    << failStartS << "," << failEndS << ","
    << seed << "," << run << ","
    << g_oobTx << "," << g_oobRx << "," << oobPdr << ","
    << latP50 << ","
    << latWinP50 << ","
    << g_oobTxWin << "," << g_oobRxWin << "," << winPdr << ","
    << g_inTxTotal << "," << g_inAckRxTotal << "," << inAckPdr << ","
    << g_inTxWin << "," << g_inAckRxWin << "," << inAckWinPdr << ","
    << detTdP50 << ","
    << "\n";
  f.close();
  Simulator::Destroy();
  return 0;

}