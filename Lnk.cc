#ifndef LNK
#define LNK

#include <string.h>
#include <omnetpp.h>
#include <packet_m.h>

using namespace omnetpp;

class Lnk: public cSimpleModule {
private:
    cQueue buffer;
    cMessage *endServiceEvent;
    // Tiempo
    simtime_t serviceTime;
    simtime_t totalServiceTime;
    // Buffer
    cOutVector bufferSizeVector;
    cStdDev bufferLengthStats;
public:
    Lnk();
    virtual ~Lnk();
protected:
    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
};

Define_Module(Lnk);

#endif /* LNK */

Lnk::Lnk() {
    endServiceEvent = NULL;
}

Lnk::~Lnk() {
    cancelAndDelete(endServiceEvent);
}

void Lnk::initialize() {
    endServiceEvent = new cMessage("endService");
    bufferSizeVector.setName("Buffer Size");
    bufferLengthStats.setName("BufferLengthStats");
    totalServiceTime = 0;
}

void Lnk::finish() {
    recordScalar("Average buffer length", bufferLengthStats.getMean());
    recordScalar("Max buffer length", bufferLengthStats.getMax());

    double utilization = (totalServiceTime / simTime()) * 100.0;
    recordScalar("Link utilization (%)", utilization);
}

void Lnk::handleMessage(cMessage *msg) {

    if (msg == endServiceEvent) {
        if (!buffer.isEmpty()) {
            // dequeue
            Packet* pkt = (Packet*) buffer.pop();
            // Buffer
            bufferSizeVector.record(buffer.getLength());
            bufferLengthStats.collect(buffer.getLength());
            // send
            send(pkt, "toOut$o");
            serviceTime = pkt->getDuration();
            // Tiempo
            totalServiceTime += serviceTime;
            scheduleAt(simTime() + serviceTime, endServiceEvent);
        }
    } else { // msg is a packet
        if (msg->arrivedOn("toNet$i")) {
            // enqueue
            buffer.insert(msg);
            bufferSizeVector.record(buffer.getLength());
            bufferLengthStats.collect(buffer.getLength());
            // if the server is idle
            if (!endServiceEvent->isScheduled()) {
                // start the service now
                scheduleAt(simTime() + 0, endServiceEvent);
            }
        } else {
            //msg is from out, send to net
            send(msg, "toNet$o");
        }
    }
}
