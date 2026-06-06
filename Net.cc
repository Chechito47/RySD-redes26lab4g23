#ifndef NET
#define NET

#include <string.h>
#include <omnetpp.h>
#include "packet_m.h"
#include <cstdio>

using namespace omnetpp;

class Net: public cSimpleModule {
private:
    int totalNodes;         // Tamano del anillo (N)
    bool haveTotalNodes;    // Indica si ya conocemos N
    int nodeId;             // Indice del nodo

public:
    Net();
    virtual ~Net();
protected:
    virtual void initialize();
    virtual void finish();
    virtual void handleMessage(cMessage *msg);
};

Define_Module(Net);

#endif /* NET */

Net::Net() {
}

Net::~Net() {
}

void Net::initialize() {
    totalNodes = 0;
    haveTotalNodes = false;
 
    // Solo node[0] inicia el discovery enviando un hello por lnk[0]
    if (getParentModule()->getIndex() == 0) {
        Packet *hello = new Packet("hello");
        hello->setPacketType(1);    // tipo 1 = hello (discovery)
        hello->setOrigin(0);        // nodo que inicio el discovery
        hello->setHopCount(0);      // se incrementa en cada salto
        hello->setByteLength(64);   // paquete de control pequeño
        send(hello, "toLnk$o", 0);  // siempre por lnk[0]
        //fprintf(stderr, "Nodo 0: Enviando hello por lnk[0]\n");
    }
    nodeId = getParentModule()->getIndex();
}

void Net::finish() {
}

void Net::handleMessage(cMessage *msg) {
    // All msg (events) on net are packets
    Packet *pkt = (Packet *) msg;

    // ------------------------------------------------------------------
    // HELLO (packetType = 1): cuenta saltos usando lnk[0] hasta volver al origen
    // ------------------------------------------------------------------
    if (pkt->getPacketType() == 1) {
        // Si el paquete hello completo la vuelta N = hopCount
        if (pkt->getOrigin() == nodeId) {
            totalNodes = pkt->getHopCount() + 1;
            fprintf(stderr, "Nodo %d: Hello completó vuelta, N = %d\n", nodeId, totalNodes);
            delete pkt;

            // Inundo los demas nodos con el valor de N
            for (int i = 0; i < gateSize("toLnk$o"); i++) {
                if (gate("toLnk$o", i)->isConnected()) {
                    Packet *echo = new Packet("echo");
                    echo->setPacketType(2);          // tipo 2 = echo (announce)
                    echo->setTotalNodes(totalNodes); // propagar N
                    echo->setByteLength(64);
                    send(echo, "toLnk$o", i);
                    fprintf(stderr, "Nodo 0: Enviando echo por interfaz %d\n", i);
                }
            }
            // Node[0] ya conoce N entonces puede empezar a rutear
            haveTotalNodes = true;
        // Si el pquete todavia no completo la vuelta
        } else {
            // Node intermedio, incremento hopCount y reenvio por lnk[0]
            pkt->setHopCount(pkt->getHopCount() + 1);
            send(pkt, "toLnk$o", 0);
            fprintf(stderr, "Nodo %d: reenvía hello (hopCount=%d) por lnk[0]\n", nodeId, pkt->getHopCount());
        }
        return;
    }

    // ------------------------------------------------------------------
    // ECHO (packetType = 2): propagar N
    // ------------------------------------------------------------------
    if (pkt->getPacketType() == 2) {
        // Si el node todavia no tiene el valor de N entonces lo acepto y reenvio
        if (!haveTotalNodes) {
            totalNodes = pkt->getTotalNodes();
            fprintf(stderr, "Nodo %d: recibió echo, N = %d\n", nodeId, totalNodes);
            // Reenvio por todas las interfaces conectadas excepto la de entrada
            int inGate = pkt->getArrivalGate()->getIndex();
            for (int i = 0; i < gateSize("toLnk$o"); i++) {
                if (i != inGate && gate("toLnk$o", i)->isConnected()) {
                    Packet *fwd = pkt->dup();
                    send(fwd, "toLnk$o", i);
                    fprintf(stderr, "Nodo %d: reenvía echo por interfaz %d\n", nodeId, i);
                }
            }
            delete pkt;
            haveTotalNodes = true;
        } else {
            // Si ya tenia el valor de N descarto el paquete
            delete pkt;
        }
        return;
    }
    // ------------------------------------------------------------------
    // DATA (packetType = 0): ruteo normal hacia el destino
    // ------------------------------------------------------------------
    // Si este nodo es el destino, entrego
    if (pkt->getDestination() == nodeId) {
        fprintf(stderr, "Nodo %d: entrega paquete a App (destino)\n", nodeId);
        send(pkt, "toApp$o");
        return;
    }

    // Si no es lo nodo destino:
    // Incremento hopCount
    pkt->setHopCount(pkt->getHopCount() + 1);

    // Si no conocemos N, fallback al baseline (lnk[0])
    if (!haveTotalNodes) {
        fprintf(stderr, "Nodo %d: aún sin N, fallback a lnk[0]\n", nodeId);
        send(pkt, "toLnk$o", 0);
        return;
    }

    // Calculo camino mas corto
    int dest = pkt->getDestination();
    int horario = (dest - nodeId + totalNodes) % totalNodes;
    int antihorario = (nodeId - dest + totalNodes) % totalNodes;

    int outGate = (horario <= antihorario) ? 1 : 0;

    //fprintf(stderr, "Nodo %d: reenvía DATA a destino %d | horario=%d antihorario=%d -> outGate=%d\n", nodeId, dest, horario, antihorario, outGate);

    send(pkt, "toLnk$o", outGate);
}
