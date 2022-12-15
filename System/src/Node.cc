#include "Node.h"
#include "Constants.h"
#include "Packet_m.h"
#include <string>
#include <cstring>
#include <cstddef>
#include <bitset>

Define_Module(Node);
// initializations
void Node::initialize()
{
    // Initialize the constants
    WS = getParentModule()->par("WS").intValue();
    TO = getParentModule()->par("TO").intValue();
    PT = getParentModule()->par("PT").doubleValue();
    TD = getParentModule()->par("TD").doubleValue();
    ED = getParentModule()->par("ED").doubleValue();
    DD = getParentModule()->par("DD").doubleValue();
    LP = getParentModule()->par("LP").doubleValue();

    timeoutMsgs = std::vector<Packet_Base *>(WS, nullptr);

    if (strcmp(getName(), NODE0) == 0)
    {
        outputFileName = "output0.txt";
    }
    else if (strcmp(getName(), NODE1) == 0)
    {
        outputFileName = "output1.txt";
    }

    std::remove((outputPath + outputFileName).c_str());

    // Create output.txt file to log the messages events into it
    outputFile.open((outputPath + outputFileName).c_str(), std::ios_base::app);
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

// Utility functions
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
Packet_Base *Node::createPacket(Packet_Base *oldPacket, std::string newPayload)
{
    // 1.Apply byte stuffing framing to the message payload
    std::string frame = Node::framing(newPayload);
    // 2.Set the trailer to the parity byte
    oldPacket->setTrailer(Node::getParity(frame));
    // 3.Set the payload to the frame
    oldPacket->setPayload(frame.c_str());
    // 4.Set the header to the sequence number
    oldPacket->setHeader(index % WS);
    // 5.Set the frame type to data
    oldPacket->setFrame_type(DATA_SIGNAL);

    return oldPacket;
}

/// Transmission functions
// Timing functions
void Node::delayPacket(std::string event, double delay, int ack = -1)
{
    Packet_Base *packet = new Packet_Base(event.c_str());

    if (ack != -1)
    {
        packet->setACK_nr(ack);
    }
    scheduleAt(simTime() + delay, packet);
}
void Node::checkTimeout(int msgIndex)
{
    if (msgIndex < this->start || this->index <= msgIndex) // If the message index is less than the start of the window i.e acked, then return
        return;
    // if not acked, then reprocess the message and reset the start of the window to the message index
    this->start = msgIndex;
    this->index = msgIndex;
    delayPacket("processing finished", PT); // to avoid blocking
}
// Data packets functions
void Node::sendPacket(Packet_Base *packet, double delay = 0.0, bool loss = false)
{
    if (index < start)
    {
        index = start;
        return;
    }

    if (!loss)
    {
        // Write the sent message to the output file
        sendDelayed(packet, delay, NODE_OUTPUT);
    }

    // Set the time out clock
    timeoutMsgs[index % WS] = new Packet_Base(("T" + std::to_string(index)).c_str());
    scheduleAt(simTime() + TO, timeoutMsgs[index % WS]);

    return;
}
int Node::receivePacket(Packet_Base *packet)
{
    int seqNum = packet->getHeader();
    std::string frame = packet->getPayload();
    char parity = packet->getTrailer();

    // Check if the parity byte is error free
    if (Node::checkParity(frame, parity))
    {
        return seqNum;
    }

    return -1;
}
// Ack packets functions
void Node::sendAck(Packet_Base *packet)
{

    // Send an ACK signal to the sender node to send the next message
    packet->setFrame_type(ACK_SIGNAL);

    outputFile << "At time [" << simTime() << "], " << getName() << " sending [ACK] ";
    outputFile << "with number [" << packet->getACK_nr() << "]" << endl;
    sendDelayed(packet, TD, NODE_OUTPUT);
}
bool Node::receiveAck(Packet_Base *packet)
{
    int receiverSeqNum = packet->getACK_nr();
    if (receiverSeqNum == 0)
        receiverSeqNum = WS;

    // TODO: check if the received ACK is the expected one
    EV << (receiverSeqNum - 1 == start % WS ? "right ack" : "wrong ack") << endl;

    cancelAndDelete(timeoutMsgs[receiverSeqNum - 1]); // Cancel the timeout clock
    timeoutMsgs[receiverSeqNum - 1] = nullptr;        // Set the timeout clock to null

    // If we reached the end of the vector, then finish the simulation
    // TODO: check if the received ACK is the last one
    if (receiverSeqNum == data.size())
    {
        EV << "end of simulation" << endl;
        cancelAndDelete(packet); // Delete the packet object
        finish();                // Finish the simulation
        return false;
    }

    // increase the start of the window but not more than the index
    start++;

    EV << "start " << start << " index " << index << endl;

    // check if we was blocked on the ack resume sending
    if (index < data.size() && start + WS == index + 1)
    {
        outputFile << "At [" << simTime() << "], " << getName() << " Introducing channel error with code " << data[this->index].first << endl;
        delayPacket("processing finished", PT);
    }

    return true;
}
void Node::handleSending(Packet_Base *packet)
{
    std::string err = data[this->index].first;
    bool isDelay = err[3] == '1';
    bool isDuplication = err[2] == '1';
    bool isLoss = err[1] == '1';
    bool isModification = err[0] == '1';

    double delay = TD;
    // change errors to 0 to send it send it correctly in the next time

    packet = createPacket(packet, data[this->index].second); // Construct the first packet
    std::bitset<4> trailer(packet->getTrailer());            // Get the trailer of the packet

    if (!isLoss)
    {
        if (isDelay)
        {
            delay += ED;
        }
        if (isModification)
        {
            std::string m_message = data[this->index].second;

            EV << "message before modification " << m_message << endl;

            // change one bit in the message
            m_message[0] = m_message[0] ^ 1;

            EV << "message after modification " << m_message << endl;

            packet->setPayload((framing(m_message)).c_str()); // Construct the first packet
        }
        if (isDuplication)
        {
            std::string event = "D" + std::to_string(this->index);
            Packet_Base *packet = new Packet_Base(event.c_str());
            scheduleAt(simTime() + DD, packet->dup());
            // Send the first packet to the other node
            // sendDelayed(packet, delay + , NODE_OUTPUT);
        }
    }

    outputFile << "At time [" << simTime() << "], " << getName();
    outputFile << " [sent] frame with seq_num=[" << packet->getHeader();
    outputFile << "], and payload=[" << packet->getPayload();
    outputFile << "], and trailer=[" << trailer << "] ";
    outputFile << "Modified " << (isModification ? "[1] " : "[-1] ");
    outputFile << "Lost " << (isLoss ? "[Yes] " : "[No] ");
    outputFile << "Duplicated " << (isDuplication ? "[1] " : "[-1] ");
    outputFile << "Delayed [" << (isDelay ? std::to_string(ED) : "0");
    outputFile << "]" << endl;

    sendPacket(packet, delay, isLoss); // Send the first packet to the other node

    this->index++; // Increment the index
}
/// Message handling functions
void Node::handleMessage(cMessage *msg)
{
    // Cast the message to Packet_Base
    Packet_Base *packet = check_and_cast<Packet_Base *>(msg->dup());

    if (packet->isSelfMessage())
    {

        // receiver ready to send a packet
        // and prepare for the next packet
        if (strcmp(packet->getName(), "processing finished") == 0)
        {
            handleSending(packet);

            // if the window is full, then return
            // will be blocked until the next ack is received
            if (this->index >= data.size() || this->index >= start + WS)
            {
                EV << "window is full" << endl;
                return;
            }

            delayPacket("processing finished", PT);
            outputFile << "At [" << simTime() << "], " << getName() << " Introducing channel error with code " << data[this->index].first << endl;
        }
        else if (strcmp(packet->getName(), "send ack") == 0)
        {
            sendAck(packet);
        }
        // timeout
        else
        {
            std::string packetStr = packet->getName();
            char event = packetStr[0];
            int currIndex = std::stoi(packetStr.substr(1, packetStr.size() - 1));
            if (event == 'T')
            {
                // resend all packets in the window again
                this->index = currIndex;
                data[this->index].first = "0000";
                outputFile << "Time out event at time [" << simTime() << "], at " << getName() << " for frame with seq_num=[" << currIndex % WS << "]" << endl;

                // remove all the timeout messages
                for (int i = 0; i < WS; i++)
                {
                    cancelAndDelete(timeoutMsgs[i]);
                }

                delayPacket("processing finished", PT);
            }
            else if (event == 'D')
            {
                std::string err = data[currIndex].first;
                bool isDelay = err[3] == '1';
                bool isDuplication = err[2] == '1';
                bool isLoss = err[1] == '1';
                bool isModification = err[0] == '1';
                std::bitset<4> trailer(packet->getTrailer());
                outputFile << "At time [" << simTime() << "], " << getName();
                outputFile << " [sent] frame with seq_num=[" << packet->getHeader();
                outputFile << "], and payload=[" << packet->getPayload();
                outputFile << "], and trailer=[" << trailer << "] ";
                outputFile << "Modified " << (isModification ? "[1] " : "[-1] ");
                outputFile << "Lost " << (isLoss ? "[Yes] " : "[No] ");
                outputFile << "Duplicated [2] ";
                outputFile << "Delayed [" << (isDelay ? std::to_string(ED) : "0");
                outputFile << "]" << endl;

                double delay = TD;
                if (isDelay)
                {
                    delay += ED;
                }
                if (isModification)
                {
                    std::string m_message = data[this->index].second;

                    EV << "message before modification " << m_message << endl;

                    // change one bit in the message
                    m_message[0] = m_message[0] ^ 1;

                    EV << "message after modification " << m_message << endl;

                    packet->setPayload((framing(m_message)).c_str()); // Construct the first packet
                }

                packet = createPacket(packet, data[currIndex].second); // Construct the first packet
                sendDelayed(packet, TD, NODE_OUTPUT);
            }
        }

        return;
    }

    // If the message is the start signal
    if (strcmp(packet->getPayload(), START_SIGNAL) == 0)
    {

        readFileMsg(); // Read the messages from the input file
        outputFile << "At time [" << simTime() << "], " << getName() << " Introducing channel error with code " << data[0].first << endl;
        delayPacket("processing finished", PT);
    }
    // Logic for the receiving node in the receiver from sender
    else if (packet->getFrame_type() == DATA_SIGNAL)
    {
        int seqNum = receivePacket(packet);
        EV << "seqNum: " << seqNum << " expectedSeqNum: " << expectedSeqNum << endl;
        EV << "correct parity: " << (checkParity(packet->getPayload(), packet->getTrailer()) ? "Yes" : "No") << endl;
        if (seqNum != expectedSeqNum)
            return;
        // Increment the expected sequence number
        expectedSeqNum++;
        expectedSeqNum %= WS;
        delayPacket("send ack", PT, expectedSeqNum);
    }
    // Logic for sending receiving ACK from receiver to sender
    else if (packet->getFrame_type() == ACK_SIGNAL)
    {
        if (!receiveAck(packet)) // Receive the ACK signal
            return;
    }
}

void Node::finish()
{
    outputFile.close();
}