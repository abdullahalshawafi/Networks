#include "Node.h"
#include "Constants.h"
#include "Packet_m.h"
#include <string>
#include <cstring>
#include <cstddef>
#include <bitset>

Define_Module(Node);

// Initializations
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

    // Initialize a vector of pointers to packets to store the timeout messages
    timeoutMsgs = std::vector<Packet_Base *>(WS, nullptr);

    // Initialize the output file
    // If we are in Node 0, then the output file will be output0.txt
    if (strcmp(getName(), NODE0) == 0)
    {
        outputFileName = "output0.txt";
        nodeIndex = 0;
    }
    // Else if we are in Node 1, then the output file will be output1.txt
    else if (strcmp(getName(), NODE1) == 0)
    {
        outputFileName = "output1.txt";
        nodeIndex = 1;
    }

    // Delete the old output file
    std::remove((outputPath + outputFileName).c_str());

    // Create the output file to log the messages events into it
    outputFile.open((outputPath + outputFileName).c_str(), std::ios_base::app);
}
void Node::readFileMsg()
{
    std::string inputMessage;

    std::ifstream inputFile;

    // If we are in Node 0, then read the messages from input0.txt
    if (strcmp(getName(), NODE0) == 0)
        inputFile.open("../inputs/input0.txt");

    // Else if we are in Node 1, then read messages from input1.txt
    else if (strcmp(getName(), NODE1) == 0)
        inputFile.open("../inputs/input1.txt");

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

    // Initialize the read vector to false
    read = std::vector<bool>(data.size(), false);
}

/* Utility methods */

// System outputs methods
void Node::printPacketRead(omnetpp::simtime_t time)
{
    outputFile << "At [" << time << "], Node [" << nodeIndex << "], Introducing channel error with code =[" << data[this->index].first << "]" << endl;
    EV << "At [" << time << "], Node [" << nodeIndex << "], Introducing channel error with code =[" << data[this->index].first << "]" << endl;
}
void Node::printPacketSent(omnetpp::simtime_t time, Packet_Base *packet, std::bitset<4> trailer, int modificationIndex, bool isLost, int duplicationType, bool isDelayed)
{
    outputFile << "At time [" << time << "], Node[" << nodeIndex << "] [sent] frame with seq_num=[" << packet->getHeader() << "] ";
    outputFile << "and payload=[ " << packet->getPayload() << " ] ";
    outputFile << "and trailer=[ " << trailer << " ], ";
    outputFile << "Modified [" << modificationIndex << "], ";
    outputFile << "Lost " << (isLost ? "[Yes]" : "[No]") << ", ";
    outputFile << "Duplicate [" << duplicationType << "], ";
    outputFile << "Delay [" << (isDelayed ? ED : 0) << "]" << endl;

    EV << "At time [" << time << "], Node[" << nodeIndex << "] [sent] frame with seq_num=[" << packet->getHeader() << "] ";
    EV << "and payload=[ " << packet->getPayload() << " ] ";
    EV << "and trailer=[ " << trailer << " ], ";
    EV << "Modified [" << modificationIndex << "], ";
    EV << "Lost " << (isLost ? "[Yes]" : "[No]") << ", ";
    EV << "Duplicate [" << duplicationType << "], ";
    EV << "Delay [" << (isDelayed ? ED : 0) << "]" << endl;
}
void Node::printPacketTimedOut(omnetpp::simtime_t time, int seqNum)
{
    outputFile << "Time out event at time [" << time << "], at Node[" << nodeIndex << "] for frame with seq_num=[" << seqNum << "]" << endl;
    EV << "Time out event at time [" << time << "], at Node[" << nodeIndex << "] for frame with seq_num=[" << seqNum << "]" << endl;
}
void Node::printControlPacketSent(omnetpp::simtime_t time, Packet_Base *packet, bool isLost)
{
    outputFile << "At time[" << time << "], Node[" << nodeIndex;
    outputFile << "] Sending [" << (packet->getFrame_type() == ACK_SIGNAL ? "ACK" : "NACK") << "] ";
    outputFile << "with number [" << packet->getACK_nr() << "] , loss " << (isLost ? "[Yes]" : "[No]") << endl;

    EV << "At time[" << time << "], Node[" << nodeIndex;
    EV << "] Sending [" << (packet->getFrame_type() == ACK_SIGNAL ? "ACK" : "NACK") << "] ";
    EV << "with number [" << packet->getACK_nr() << "] , loss " << (isLost ? "[Yes]" : "[No]") << endl;
}

// Packet methods
std::string Node::framing(std::string payload)
{
    std::string frame = "";

    // Start the frame be the flag byte
    frame += FLAG_BYTE;

    // Go through the message payload character by character
    for (auto character : payload)
    {
        // If the current character is either a flag or escape character
        if (character == FLAG_BYTE || character == ESCAPE_CHAR)
            // Stuff the escape character before it
            frame += ESCAPE_CHAR;

        // Append the character to the frame
        frame += character;
    }

    // End the frame by the flag byte
    frame += FLAG_BYTE;

    return frame;
}
char Node::getParity(std::string frame)
{
    char parity = 0;

    // Loop through each character in the frame
    for (auto character : frame)
        // XOR the parity with the character to calculate the even parity
        parity ^= character;

    return parity;
}
bool Node::checkParity(std::string frame, char expectedParity)
{
    // If the calculated parity is equal to the expected parity, then the frame is valid
    return getParity(frame) == expectedParity;
}
Packet_Base *Node::createPacket(Packet_Base *packet, std::string newPayload)
{
    // 1. Apply byte stuffing framing to the message payload
    std::string frame = Node::framing(newPayload);
    // 2. Set the trailer to the parity byte
    packet->setTrailer(Node::getParity(frame));
    // 3. Set the payload to the frame
    packet->setPayload(frame.c_str());
    // 4. Set the header to the sequence number
    packet->setHeader(index % WS);
    // 5. Set the frame type to data
    packet->setFrame_type(DATA_SIGNAL);

    return packet;
}

/* Transmission methods */

// Timing methods
void Node::notifyAfter(std::string event, double delay, int ack = -1)
{
    // Create a new packet with the specified event
    Packet_Base *packet = new Packet_Base(event.c_str());

    if (event == PROCESS_PACKET)
    {
        processedMsgs.emplace_back(packet);
    }
    else if (event == SEND_ACK || event == SEND_NACK)
    {
        controlMsgs.emplace_back(packet);
    }

    // If the event is an ACK or NACK, set the ACK number
    if (ack != -1)
        packet->setACK_nr(ack);

    // Schedule the packet to be sent after the specified delay
    scheduleAt(simTime() + delay, packet);
}

// Data packets methods
void Node::sendPacket(Packet_Base *packet, double delay = 0.0, bool isLost = false)
{
    // Check if the window is full
    if (index < start)
    {
        // If so, move the index to the start of the window
        index = start;
        return;
    }

    // Check if the packet is lost
    if (!isLost)
        // If not, send the packet to the receiver node with the specified delay
        sendDelayed(packet, delay, NODE_OUTPUT);

    // If the packet is lost,
    // Set the timeout packet for the current index
    std::string event = TIMEOUT + std::to_string(this->index);
    timeoutMsgs[this->index % WS] = new Packet_Base(event.c_str());
    scheduleAt(simTime() + TO, timeoutMsgs[this->index % WS]);

    return;
}
int Node::receivePacket(Packet_Base *packet)
{
    int seqNum = packet->getHeader();
    char parity = packet->getTrailer();
    std::string frame = packet->getPayload();

    // Check if the parity byte is error free
    if (Node::checkParity(frame, parity))
        // If so, return the sequence number
        return seqNum;

    // If not, return -1 to indicate an error
    return -1;
}

// ACK/NACK packets methods
void Node::sendControlPacket(Packet_Base *packet, short signal)
{
    // Set the frame type to the specified signal
    packet->setFrame_type(signal);

    // The contorl packet could be lost with probability LP
    bool isLost = uniform(0, 1) < LP;

    printControlPacketSent(simTime(), packet, isLost);

    // If the control packet is not lost, send it to the sender node with the specified delay
    if (!isLost)
        sendDelayed(packet, TD, NODE_OUTPUT);
}
bool Node::receiveAck(Packet_Base *packet)
{
    // If we received all the packets, end the simulation
    if (receivedCount == data.size())
    {
        finish(); // Finish the simulation
        return false;
    }

    int receiverSeqNum = packet->getACK_nr();
    if (receiverSeqNum == 0)
        receiverSeqNum = WS;

    // Increase the start of the window but not more than the index
    // Only if the received ack is the expected one
    if (receiverSeqNum - 1 == start % WS)
    {
        cancelAndDelete(timeoutMsgs[receiverSeqNum - 1]); // Cancel the timeout clock
        timeoutMsgs[receiverSeqNum - 1] = nullptr;        // Set the timeout clock to null
        receivedCount++;
        start++;
    }

    // Check if we was blocked on the ACK and we can send the next packet
    if (index < data.size() && index == start + WS - 1)
    {
        if (!read[this->index])
        {
            read[this->index] = true;
            printPacketRead(simTime());
        }

        notifyAfter(PROCESS_PACKET, PT);
    }

    return true;
}

// Sending methods
void Node::handleSending(Packet_Base *packet)
{
    std::string err = data[this->index].first;
    bool isDelay = err[3] == '1';
    bool isDuplication = err[2] == '1';
    bool isLoss = err[1] == '1';
    bool isModification = err[0] == '1';

    double delay = TD;
    int modificationIndex = isModification ? 1 : -1;
    int duplicationType = 0;

    packet = createPacket(packet, data[this->index].second); // Construct the packet to be sent
    std::bitset<4> trailer(packet->getTrailer());            // Get the trailer of the packet

    if (isDelay)
    {
        delay += ED; // Add the error delay to the delay variable
    }

    if (isModification)
    {
        std::string payload = packet->getPayload(); // Get the payload of the packet
        payload[modificationIndex] ^= 1;            // Flip the bit at the specified index
        packet->setPayload(payload.c_str());        // Reset the payload of the packet with the modified payload
    }

    if (isDuplication)
    {
        duplicationType = 1;                                           // Set the duplication type to 1
        std::string event = DUPLICATION + std::to_string(this->index); // Create the duplication event
        Packet_Base *duplicatePacket = new Packet_Base(event.c_str()); // Create a new packet with the duplication event

        // Set the header, payload, trailer and frame type of the duplicate packet to the same values as the original packet
        duplicatePacket->setHeader(packet->getHeader());
        duplicatePacket->setPayload(packet->getPayload());
        duplicatePacket->setTrailer(packet->getTrailer());
        duplicatePacket->setFrame_type(packet->getFrame_type());

        scheduleAt(simTime() + DD, duplicatePacket); // Schedule the duplication event with the specified delay

        duplicatedMsgs.emplace_back(duplicatePacket); // Add the duplicate packet to the duplicated messages vector
    }

    printPacketSent(simTime(), packet, trailer, modificationIndex, isLoss, duplicationType, isDelay); // Log the event
    sendPacket(packet, delay, isLoss);                                                                // Send the packet with the specified delay

    this->index++; // Increment the index
}

/* Message handling method */
void Node::handleMessage(cMessage *msg)
{
    // Cast the message to Packet_Base
    Packet_Base *packet = check_and_cast<Packet_Base *>(msg);

    // If the message is a self message (scheduled message)
    if (packet->isSelfMessage())
    {
        // Check if the message indicates the end of the processing
        if (strcmp(packet->getName(), PROCESS_PACKET) == 0)
        {
            // If so, send the packet
            handleSending(packet);

            // if the window is full, then return
            // will be blocked until the next ack is received
            if (this->index >= data.size() || this->index >= start + WS)
                return;

            if (!read[this->index])
            {
                read[this->index] = true;
                printPacketRead(simTime());
            }

            notifyAfter(PROCESS_PACKET, PT);
        }
        else if (strcmp(packet->getName(), SEND_ACK) == 0)
        {
            sendControlPacket(packet, ACK_SIGNAL);
        }
        else if (strcmp(packet->getName(), SEND_NACK) == 0)
        {
            sendControlPacket(packet, NACK_SIGNAL);
        }
        else
        {
            std::string packetStr = packet->getName();
            char event = packetStr[0];
            int currentIndex = std::stoi(packetStr.substr(1, packetStr.size() - 1));
            if (event == TIMEOUT)
            {
                // Resend all packets in the window again
                this->index = currentIndex;
                data[this->index].first = "0000"; // Reset the error flags
                printPacketTimedOut(simTime(), currentIndex % WS);

                // Remove all the timeout messages
                for (int i = 0; i < WS; i++)
                {
                    cancelAndDelete(timeoutMsgs[i]);
                }

                notifyAfter(PROCESS_PACKET, PT);
            }
            else if (event == DUPLICATION)
            {
                std::string err = data[currentIndex].first;
                bool isDelay = err[3] == '1';
                bool isLoss = err[1] == '1';
                bool isModification = err[0] == '1';

                double delay = TD;
                int modificationIndex = isModification ? 1 : -1;
                int duplicationType = 2;

                packet->setHeader(currentIndex % WS);
                std::bitset<4> trailer(packet->getTrailer());

                if (isDelay)
                    delay += ED; // Add the error delay to the delay variable

                printPacketSent(simTime(), packet, trailer, modificationIndex, isLoss, duplicationType, isDelay);

                if (!isLoss)
                    sendDelayed(packet, delay, NODE_OUTPUT);
            }
        }

        return;
    }

    // If the message is the start signal
    if (strcmp(packet->getPayload(), START_SIGNAL) == 0)
    {
        readFileMsg(); // Read the messages from the input file
        read[this->index] = true;
        printPacketRead(simTime());
        notifyAfter(PROCESS_PACKET, PT);
        cancelAndDelete(packet);
    }
    // Logic for the receiving node
    else if (packet->getFrame_type() == DATA_SIGNAL)
    {
        int seqNum = packet->getHeader();

        // Ignore the packet if it is greater than the expected one (out of order)
        if (seqNum > this->expectedSeqNum)
            return;

        int parity = receivePacket(packet);

        // If the parity is -1, then the packet is modified
        if (parity == -1)
        {
            // Send NACK and return
            notifyAfter(SEND_NACK, PT, this->expectedSeqNum);
            return;
        }

        // If the sequence number is the expected one
        if (seqNum == this->expectedSeqNum)
        {
            // Increment the expected sequence number
            this->expectedSeqNum = (this->expectedSeqNum + 1) % WS;

            notifyAfter(SEND_ACK, PT, this->expectedSeqNum);
        }
        else // If the sequence number is less than the expected one
        {
            // Send ACK for the next expected sequence number
            notifyAfter(SEND_ACK, PT, (seqNum + 1) % WS);
        }
    }
    // Logic for sending node when receiving ACK from receiver
    else if (packet->getFrame_type() == ACK_SIGNAL)
    {
        // If the ACK is not valid, return
        if (!receiveAck(packet))
            return;
    }
}

/* Session termination method */
void Node::finish()
{
    EV << "End of simulation" << endl;

    for (auto packet : processedMsgs)
    {
        cancelAndDelete(packet);
    }

    for (auto packet : duplicatedMsgs)
    {
        cancelAndDelete(packet);
    }

    for (auto packet : timeoutMsgs)
    {
        cancelAndDelete(packet);
    }

    for (auto packet : controlMsgs)
    {
        cancelAndDelete(packet);
    }

    outputFile.close();
}
