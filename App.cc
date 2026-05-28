#ifndef APP
#define APP

#include <string.h>
#include <omnetpp.h>
#include <packet_m.h>

using namespace omnetpp;

class App: public cSimpleModule {
private:
    cMessage *sendMsgEvent;

    // Metricas de delay
    cStdDev delayStats;
    cOutVector delayVector;

    // Metricas de hopCount
    cStdDev hopCountStats;
    cOutVector hopCountVector;
public:
    App();
    virtual ~App();
protected:
    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
};

Define_Module(App);

#endif /* APP */

App::App() {
}

App::~App() {
}

void App::initialize() {
    // If interArrivalTime for this node is higher than 0
    // initialize packet generator by scheduling sendMsgEvent
    if (par("interArrivalTime").doubleValue() != 0) {
        sendMsgEvent = new cMessage("sendEvent");
        scheduleAt(par("interArrivalTime"), sendMsgEvent);
    }

    // Estadisticas de delay
    delayStats.setName("TotalDelay");
    delayVector.setName("Delay");

    // Estadisticas de saltos por paquete
    hopCountStats.setName("HopCountStats");
    hopCountVector.setName("HopCount");
}

void App::finish() {
    // Record statistics
    recordScalar("Average delay", delayStats.getMean());
    recordScalar("Number of packets", delayStats.getCount());

    recordScalar("Average hop count", hopCountStats.getMean());
    recordScalar("Max hop count", hopCountStats.getMax());
    recordScalar("Hop count standard deviation", hopCountStats.getStddev());
}

void App::handleMessage(cMessage *msg) {

    // if msg is a sendMsgEvent, create and send new packet
    if (msg == sendMsgEvent) {
        // create new packet
        Packet *pkt = new Packet("packet",this->getParentModule()->getIndex());
        pkt->setByteLength(par("packetByteSize"));
        pkt->setSource(this->getParentModule()->getIndex());
        pkt->setDestination(par("destination"));
        // Paquete comienza con 0 saltos
        pkt->setHopCount(0);

        // send to net layer
        send(pkt, "toNet$o");

        // compute the new departure time and schedule next sendMsgEvent
        simtime_t departureTime = simTime() + par("interArrivalTime");
        scheduleAt(departureTime, sendMsgEvent);

    }
    // else, msg is a packet from net layer
    else {
        // Convertimos msg a Packet*
        Packet *pkt = (Packet *) msg;

        // compute delay and record statistics
        simtime_t delay = simTime() - msg->getCreationTime();
        delayStats.collect(delay);
        delayVector.record(delay);

        // Registrar hopCount
        int hops = pkt->getHopCount();
        hopCountStats.collect(hops);
        hopCountVector.record(hops);

        // delete msg
        delete (msg);
    }

}
