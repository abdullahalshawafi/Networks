#include "Node.h"
#include "Constants.h"
#include "Packet_m.h"

Define_Module(Node);

void Node::initialize()
{
    std::remove("../output/output.txt");
}

void Node::handleMessage(cMessage *msg)
{
    // Cast the message to Packet_Base
    Packet_Base *packet = check_and_cast<Packet_Base *>(msg);
    
    // Create output.txt file to log the messages events into it
    std::ofstream outputFile("../output/output.txt", std::ios_base::app);

    // If the message is the start signal
    if (strcmp(packet->getPayload(), START_SIGNAL) == 0)
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
        while(std::getline(inputFile, inputMessage))
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

        //// Construct the packet
        // 1.Apply byte stuffing framing to the message payload
        std::string frame = Node::framing(data[index++].second);
        std::cout << "Send Frame: " << frame << endl;
        // 2.Set the trailer to the parity byte
        packet->setTrailer(Node::getParity(frame));
        std::cout << "Send Parity: " << packet->getTrailer() << endl;
        // 3.Set the payload to the frame
        packet->setPayload(frame.c_str());
        // 4.Set the header to the sequence number
        packet->setHeader(sequenceNumber++);
        
        // Send the first frame to the other node
        send(packet, NODE_OUTPUT);

        // Write the sent message to the output file
        outputFile << "Sent: " << packet->getPayload() << endl;
    }
    // If the message is an ACK signal, then send the next message
    else if (packet->getFrame_type() == ACK_SIGNAL)
    {
        // If we didn't reach the end of the vector yet
        if (index < data.size())
        {

            //// Construct the packet
            // 1.Apply byte stuffing framing to the message payload
            std::string  frame = Node::framing(data[index++].second);
            // 2.Set the trailer to the parity byte
            packet->setTrailer(Node::getParity(frame));
            // 3.Set the payload to the frame
            packet->setPayload(frame.c_str());
            // 4.Set the header to the sequence number
            packet->setHeader(sequenceNumber++);

            // Send the next frame to the other node
            send(packet, NODE_OUTPUT);

            // Write the sent message to the output file
            outputFile << "Sent: " << packet->getPayload() << endl;
        }
        // Else, finish the simulation
        else
        {
            // Delete the message object
            cancelAndDelete(packet);

            // Finish the simulation
            finish();
        }
    }
    // Logic for the receiving node
    else
    {
        std::string frame = packet->getPayload();
        char parity = packet->getTrailer();

        std::cout << "Received Frame: " << frame << endl;
        std::cout << "Received Parity: " << parity << endl;

        // Check if the parity byte is error free
        if (Node::checkParity(frame, parity))
        {
            outputFile << "Received: " << frame << endl;
        }
        else
        {
            outputFile << "Error detected!" << endl;
        }

        // Send an ACK signal to the sender node to send the next message
        packet->setFrame_type(ACK_SIGNAL);
        send(packet, NODE_OUTPUT);
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
