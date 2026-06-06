packet Packet {
    int source;
    int destination;
    int hopCount;
    int packetType;  //0=data, 1=hello (discovery), 2=echo (announce)
    int totalNodes;  //usado en el echo para propagar N
    int origin;      //usado en el hello para saber si volvio al origen
}
