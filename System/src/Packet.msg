packet Packet {
    @customize(true);
    int header;
    string payload;
    char trailer;
    short frame_type;
    int ACK_nr;
}
