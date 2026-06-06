#ifndef APP
#define APP

#include <string.h>
#include <omnetpp.h>
#include "packet_m.h"

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

    // Metricas separadas para el Caso 1
    cStdDev delayStats_0;
    cStdDev delayStats_2;
    cStdDev hopCountStats_0;
    cStdDev hopCountStats_2;

    // Paquetes
    long packetsSent;
    long packetsReceived;
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

    delayStats_0.setName("Delay_Src0");
    delayStats_2.setName("Delay_Src2");
    hopCountStats_0.setName("HopCount_Src0");
    hopCountStats_2.setName("HopCount_Src2");

    // Paquetes
    packetsSent = 0;
    packetsReceived = 0;
}

void App::finish() {
    // Record statistics
    recordScalar("Average delay", delayStats.getMean());
    recordScalar("Number of packets", delayStats.getCount());

    recordScalar("Average hop count", hopCountStats.getMean());
    recordScalar("Max hop count", hopCountStats.getMax());
    recordScalar("Hop count standard deviation", hopCountStats.getStddev());

    recordScalar("Number of packets sent", packetsSent);
    recordScalar("Number of packets received", packetsReceived);

    if (delayStats_0.getCount() > 0) {
        recordScalar("Average delay Src 0", delayStats_0.getMean());
        recordScalar("Average hops Src 0", hopCountStats_0.getMean());
    }
    if (delayStats_2.getCount() > 0) {
        recordScalar("Average delay Src 2", delayStats_2.getMean());
        recordScalar("Average hops Src 2", hopCountStats_2.getMean());
    }
}

void App::handleMessage(cMessage *msg) {

    // if msg is a sendMsgEvent, create and send new packet
    if (msg == sendMsgEvent) {
        // create new packet
        Packet *pkt = new Packet("packet",this->getParentModule()->getIndex());
        pkt->setPacketType(0);
        pkt->setByteLength(par("packetByteSize"));
        pkt->setSource(this->getParentModule()->getIndex());
        pkt->setDestination(par("destination"));
        // Paquete comienza con 0 saltos
        pkt->setHopCount(0);

        // send to net layer
        send(pkt, "toNet$o");
        packetsSent++;

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

        int src = pkt->getSource();
        if (src == 0) {
            delayStats_0.collect(delay);
            hopCountStats_0.collect(hops);
        } else if (src == 2) {
            delayStats_2.collect(delay);
            hopCountStats_2.collect(hops);
        }

        // delete msg
        delete (msg);
    }

}
