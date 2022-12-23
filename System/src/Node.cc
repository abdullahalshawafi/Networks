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
        nodeIndex = 0;
    }
    else if (strcmp(getName(), NODE1) == 0)
    {
        outputFileName = "output1.txt";
        nodeIndex = 1;
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

    read = std::vector<bool>(data.size(), false);
}

// Utility functions
void Node::log(omnetpp::simtime_t time, std::bitset<4> trailer, Packet_Base *packet, bool isModification, bool isLoss, int isDuplication, bool isDelay)
{

    outputFile << "At time [" << time << "], Node[" << nodeIndex << "] [sent] frame with seq_num=[" << packet->getHeader() << "] ";
    outputFile << "and payload=[ " << packet->getPayload() << " ] ";
    outputFile << "and trailer=[ " << trailer << " ], ";
    outputFile << "Modified " << (isModification ? "[1]" : "[-1]") << ", ";
    outputFile << "Lost " << (isLoss ? "[Yes]" : "[No]") << ", ";
    outputFile << "Duplicate [" << (isDuplication != 0 ? std::to_string(isDuplication) : "0") << "], ";
    outputFile << "Delay [" << (isDelay ? std::to_string(ED) : "0") << "]" << endl;

    EV << "At time [" << time << "], Node[" << nodeIndex << "] [sent] frame with seq_num=[" << packet->getHeader() << "] ";
    EV << "and payload=[ " << packet->getPayload() << " ] ";
    EV << "and trailer=[ " << trailer << " ], ";
    EV << "Modified " << (isModification ? "[1]" : "[-1]") << ", ";
    EV << "Lost " << (isLoss ? "[Yes]" : "[No]") << ", ";
    EV << "Duplicate [" << (isDuplication != 0 ? std::to_string(isDuplication) : "0") << "], ";
    EV << "Delay [" << (isDelay ? std::to_string(ED) : "0") << "]" << endl;
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
    char parity = 0;

    // Loop through each character in the frame
    for (int i = 0; i < frame.size(); i++)
    {
        // XOR the parity with the character to calculate the even parity
        parity ^= frame[i];
    }

    return parity;
}
bool Node::checkParity(std::string frame, char expectedParity)
{
    char parity = 0;

    // Loop through each character in the frame
    for (int i = 0; i < frame.size(); i++)
    {
        // XOR the parity with the character to calculate the even parity
        parity ^= frame[i];
    }

    // If the calculated parity is equal to the expected parity, then the frame is valid
    return parity == expectedParity;
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

    // the ack can be lost with probability LP
    bool lost = uniform(0, 1) < LP;

    outputFile << "At time[" << simTime() << "], Node[" << nodeIndex << "] Sending [ACK] ";
    outputFile << "with number [" << packet->getACK_nr() << "], loss " << (lost ? "[Yes]" : "[No]") << endl;

    EV << "At time [" << simTime() << "], Node[" << nodeIndex << "] Sending [ACK] ";
    EV << "with number [" << packet->getACK_nr() << "], loss " << (lost ? "[Yes]" : "[No]") << endl;

    if (!lost)
        sendDelayed(packet, TD, NODE_OUTPUT);
}
void Node::sendNAck(Packet_Base *packet)
{

    // Send an ACK signal to the sender node to send the next message
    packet->setFrame_type(NACK_SIGNAL);

    bool lost = uniform(0, 1) < LP;

    outputFile << "At time[" << simTime() << "], Node[" << nodeIndex << "] Sending [NACK] ";
    outputFile << "with number [" << packet->getACK_nr() << "], loss " << (lost ? "[Yes]" : "[No]") << endl;

    EV << "At time [" << simTime() << "], Node[" << nodeIndex << "] Sending [NACK] ";
    EV << "with number [" << packet->getACK_nr() << "], loss " << (lost ? "[Yes]" : "[No]") << endl;

    if (!lost)
        sendDelayed(packet, TD, NODE_OUTPUT);
}
bool Node::receiveAck(Packet_Base *packet)
{
    int receiverSeqNum = packet->getACK_nr();
    if (receiverSeqNum == 0)
        receiverSeqNum = WS;

    // increase the start of the window but not more than the index
    // only if the received ack is the expected one
    if (receiverSeqNum - 1 == start % WS)
    {
        cancelAndDelete(timeoutMsgs[receiverSeqNum - 1]); // Cancel the timeout clock
        timeoutMsgs[receiverSeqNum - 1] = nullptr;        // Set the timeout clock to null
        receivedCount++;
        start++;
    }

    // If we reached the end of the vector, then finish the simulation
    if (receivedCount == data.size())
    {
        EV << "end of simulation" << endl;
        cancelAndDelete(packet); // Delete the packet object
        finish();                // Finish the simulation
        return false;
    }

    // check if we was blocked on the ack resume sending
    if (index < data.size() && start + WS == index + 1)
    {
        if (!read[this->index])
        {
            read[this->index] = true;
            outputFile << "At [" << simTime() << "], Node [" << nodeIndex << "], Introducing channel error with code =[" << data[this->index].first << "]" << endl;
            EV << "At [" << simTime() << "], Node [" << nodeIndex << "], Introducing channel error with code =[" << data[this->index].first << "]" << endl;
        }
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

            // change one bit in the message
            m_message[0] = m_message[0] ^ 1;

            packet->setPayload((framing(m_message)).c_str()); // Construct the first packet
        }
        if (isDuplication)
        {
            std::string event = "D" + std::to_string(this->index);
            Packet_Base *packet = new Packet_Base(event.c_str());
            scheduleAt(simTime() + DD, packet->dup());
        }
    }

    int dup = isDuplication ? 1 : 0;
    log(simTime(), trailer, packet, isModification, isLoss, dup, isDelay);
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
                // get the timeouts
                int t = 0;
                for (int i = 0; i < WS; i++)
                {
                    if (timeoutMsgs[i] != nullptr)
                        t++;
                }

                return;
            }

            delayPacket("processing finished", PT);

            if (!read[this->index])
            {
                outputFile << "At [" << simTime() << "], Node [" << nodeIndex << "], Introducing channel error with code =[" << data[this->index].first << "]" << endl;
                EV << "At [" << simTime() << "], Node [" << nodeIndex << "], Introducing channel error with code =[" << data[this->index].first << "]" << endl;
                read[this->index] = true;
            }
        }
        else if (strcmp(packet->getName(), "send ack") == 0)
        {
            sendAck(packet);
        }
        else if (strcmp(packet->getName(), "send nack") == 0)
        {
            sendNAck(packet);
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
                outputFile << "Time out event at time [" << simTime() << "], at Node[" << nodeIndex << "] for frame with seq_num=[" << currIndex % WS << "]" << endl;
                EV << "Time out event at time [" << simTime() << "], at Node[" << nodeIndex << "] for frame with seq_num=[" << currIndex % WS << "]" << endl;

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
                bool isLoss = err[1] == '1';
                bool isModification = err[0] == '1';
                std::bitset<4> trailer(packet->getTrailer());

                double delay = TD;
                if (isDelay)
                {
                    delay += ED;
                }
                if (isModification)
                {
                    std::string m_message = data[this->index].second;

                    // change one bit in the message
                    m_message[0] = m_message[0] ^ 1;

                    packet->setPayload((framing(m_message)).c_str());
                }

                packet = createPacket(packet, data[currIndex].second);
                packet->setHeader(currIndex % WS);
                log(simTime(), trailer, packet, isModification, isLoss, 2, isDelay);
                sendDelayed(packet, delay, NODE_OUTPUT);
            }
        }

        return;
    }

    // If the message is the start signal
    if (strcmp(packet->getPayload(), START_SIGNAL) == 0)
    {

        readFileMsg(); // Read the messages from the input file
        read[0] = true;
        outputFile << "At [" << simTime() << "], Node [" << nodeIndex << "], Introducing channel error with code =[" << data[0].first << "]" << endl;
        EV << "At [" << simTime() << "], Node [" << nodeIndex << "], Introducing channel error with code =[" << data[0].first << "]" << endl;
        delayPacket("processing finished", PT);
    }
    // Logic for the receiving node in the receiver from sender
    else if (packet->getFrame_type() == DATA_SIGNAL)
    {
        int seqNum = packet->getHeader();
        if (seqNum > expectedSeqNum)
            return;

        int parity = receivePacket(packet);

        // modified message
        if (parity == -1)
        {
            delayPacket("send nack", PT, expectedSeqNum);
            return;
        }
        // ack for new msg
        if (seqNum == expectedSeqNum)
        {
            // Increment the expected sequence number
            expectedSeqNum++;
            expectedSeqNum %= WS;
            delayPacket("send ack", PT, expectedSeqNum);
        }
        // send ack again for the same msg
        else
        {
            delayPacket("send ack", PT, (seqNum + 1) % WS);
        }
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