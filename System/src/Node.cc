#include "Node.h"
#include "Constants.h"
#include "Packet_m.h"

Define_Module(Node);

void Node::initialize()
{
    std::remove("../output/output.txt");
}

void Node::sendPacket(Packet_Base *packet)
{
    // Send the next packet to the other node
    send(packet, NODE_OUTPUT);

    int TO = getParentModule()->par("TO").intValue();
    Packet_Base *timeout = new Packet_Base(std::to_string(index).c_str());
    scheduleAt(simTime() + exponential(TO), timeout);

    // Write the sent message to the output file
    outputFile << "Sent: " << packet->getPayload() << endl;
}

void Node::checkTimeout(int msgIndex)
{
    EV << "Checking timeout\n";
    if (msgIndex < this->start)
        return;
    else
    {
        EV << "Timed out\n";
        this->start = msgIndex;
        this->index = msgIndex;
    }
}

Packet_Base *Node::createPacket(Packet_Base *oldPacket, std::string newPayload)
{
    // 1.Apply byte stuffing framing to the message payload
    std::string frame = Node::framing(newPayload);
    // 2.Set the trailer to the parity byte
    oldPacket->setTrailer(Node::getParity(frame));
    // 3.Set the payload to the frame
    oldPacket->setPayload(frame.c_str());
    // 4.Set the header to the sequence number
    oldPacket->setHeader(index);
    // 5.Set the frame type to data
    oldPacket->setFrame_type(DATA_SIGNAL);

    return oldPacket;
}

void Node::readFileMsg()
{
    std::string inputMessage;

    std::ifstream inputFile;

    // If we are in Node 0, then read the messages from input0.txt
    if (strcmp(getName(), NODE0) == 0)
    {
        inputFile.open("../inputs/input0.txt");
    }
    // Else if we are in Node 1, then read messages from input1.txt
    else if (strcmp(getName(), NODE1) == 0)
    {
        inputFile.open("../inputs/input1.txt");
    }

    // Read messages from the input file
    while (std::getline(inputFile, inputMessage))
    {
        // The first part will be errors
        std::string errors = strtok((char *)inputMessage.c_str(), " ");

        // The rest of the data will be the message to be sent
        std::string message = inputMessage.substr(errors.size() + 1);

        // Store the errors-message pair in the data vector
        data.emplace_back(std::make_pair(errors, message));
    }

    // Close the input file
    inputFile.close();
}

int Node::receivePacket(Packet_Base *packet)
{
    int seqNum = packet->getHeader();
    std::string frame = packet->getPayload();
    char parity = packet->getTrailer();

    // Check if the parity byte is error free
    if (Node::checkParity(frame, parity))
    {
        outputFile << "Received: " << frame << " with sequence number " << seqNum << endl;
        return seqNum;
    }

    outputFile << "Error detected!" << endl;
    return -1;
}

void Node::sendAck(Packet_Base *packet, int seqNum)
{
    // If the received message is corrupted, then don't send an ACK
    if (seqNum != expectedSeqNum)
        return;

    // Increment the expected sequence number
    expectedSeqNum++;

    // Send an ACK signal to the sender node to send the next message
    packet->setFrame_type(ACK_SIGNAL);
    packet->setACK_nr(expectedSeqNum);
    send(packet, NODE_OUTPUT);

    // Write the sent ACK to the output file
    outputFile << "Sent: ACK " << expectedSeqNum << endl;
}

void Node::handleMessage(cMessage *msg)
{
    // Cast the message to Packet_Base
    Packet_Base *packet = check_and_cast<Packet_Base *>(msg);

    if (msg->isSelfMessage())
        this->checkTimeout(std::atoi(msg->getName()));

    // Create output.txt file to log the messages events into it
    outputFile.open("../output/output.txt", std::ios_base::app);

    // If the message is the start signal
    if (strcmp(packet->getPayload(), START_SIGNAL) == 0)
    {
        readFileMsg();                                     // Read the messages from the input file
        packet = createPacket(packet, data[index].second); // Construct the packet
        sendPacket(packet);                                // Send the first packet to the other node
        index++;                                           // Increment the index
    }
    // If the message is an ACK signal, then send the next message
    else if (packet->getFrame_type() == ACK_SIGNAL)
    {
        // If we reached the end of the vector, then finish the simulation
        if (index == data.size())
        {
            cancelAndDelete(packet); // Delete the packet object
            return finish();         // Finish the simulation
        }

        packet = createPacket(packet, data[index].second); // Construct the packet
        sendPacket(packet);                                // Send the next packet to the other node
        index++;                                           // Increment the index
    }
    // Logic for the receiving node
    else
    {
        int seqNum = receivePacket(packet);
        sendAck(packet, seqNum);
    }

    // Close the output file
    outputFile.close();
}

std::string Node::framing(std::string payload)
{
    std::string frame = "";

    // Start the frame be the flag byte
    frame += FLAG_BYTE;

    // Go through the message payload character by character
    for (auto c : payload)
    {
        // If the current character is either a flag or escape character
        if (c == FLAG_BYTE || c == ESCAPE_CHAR)
        {
            // Stuff the escape character before it
            frame += ESCAPE_CHAR;
        }

        // Add the character to the frame
        frame += c;
    }

    // End the frame by the flag byte
    frame += FLAG_BYTE;

    return frame;
}

char Node::getParity(std::string frame)
{
    // Initialize the parity byte by 8 0s
    std::bitset<8> parityByte(0);

    // Loop through each character in the frame
    for (auto c : frame)
    {
        // Convert the character to bits
        std::bitset<8> characterByte(c);

        // XOR the parity with the character to calculate the even parity
        parityByte ^= characterByte;
    }

    // Convert the parity byte to ASCII character and return it
    return (char)parityByte.to_ulong();
}

bool Node::checkParity(std::string frame, char expectedParity)
{
    // Initialize the parity byte by 8 0s
    std::bitset<8> parityByte(0);

    // Loop through each character in the frame
    for (int i = 0; i < frame.size(); i++)
    {
        // Convert the character to bits
        std::bitset<8> characterByte(frame[i]);

        // XOR the parity with the character to calculate the even parity
        parityByte ^= characterByte;
    }

    // Check if the calculated parity is equal to the expected parity
    return (char)parityByte.to_ulong() == expectedParity;
}
