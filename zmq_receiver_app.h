#include "ns3/application.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/vector.h"
#include "ns3/mobility-model.h"
#include <zmq.hpp>
#include <string>
#include <iostream>
#include <sstream>
#include <thread>

using namespace ns3;

class ZmqReceiverApp : public Application
{
public:
    static TypeId GetTypeId();
    ZmqReceiverApp();
    virtual ~ZmqReceiverApp() { StopApplication(); };

    void StartApplication() override;
    void StopApplication() override;
    void Run();
    Vector PositionConverter(std::string message);
    void SetNodePosition(Ptr<Node> node, Vector position);

private:
    bool m_running;
    std::unique_ptr<std::thread> m_thread;
    std::string m_address;
    int m_port;
    zmq::context_t m_context;
    zmq::socket_t m_socket;
    std::string m_id;
    zmq::message_t m_message;

    std::string m_heartBeatTopic;
};