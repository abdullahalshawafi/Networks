#include "Node.h"
#include "Constants.h"

Define_Module(Node);

void Node::initialize()
{
    std::remove("../output/output.txt");
}

void Node::handleMessage(cMessage *msg)
{
    // Create output.txt file to log the messages events into it
    std::ofstream outputFile("../output/output.txt", std::ios_base::app);

    // If the message is the start signal
    if (strcmp(msg->getName(), START_SIGNAL) == 0)
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

        // Send the first byte stuffed message (frame) to the other node
        std::string frame = Node::framing(data[index++].second);
        msg->setName(frame.c_str());
        send(msg, NODE_OUTPUT);

        // Write the sent message to the output file
        outputFile << "Sent: " << msg->getName() << endl;
    }
    // If the message is an ACK signal, then send the next message
    else if (strcmp(msg->getName(), ACK_SIGNAL) == 0)
    {
        // If we didn't reach the end of the vector yet
        if (index < data.size())
        {
            // Send the next byte stuffed message (frame) to the other node
            std::string frame = Node::framing(data[index++].second);
            msg->setName(frame.c_str());
            send(msg, NODE_OUTPUT);

            // Write the sent message to the output file
            outputFile << "Sent: " << msg->getName() << endl;
        }
        // Else, finish the simulation
        else
        {
            // Delete the message object
            cancelAndDelete(msg);

            // Finish the simulation
            finish();
        }
    }
    else
    {
        // Convert the received frame back to the original message payload
        std::string payload = Node::convertFrameToPayload(std::string(msg->getName()));

        // Write the message payload to the output file
        outputFile << "Received: " << payload << endl;

        // Send an ACK signal to the sender node to send the next message
        msg->setName(ACK_SIGNAL);
        send(msg, NODE_OUTPUT);
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

std::string Node::convertFrameToPayload(std::string frame)
{
    std::string payload = "";

    // A flag to check if the character before was an escape character
    bool wasPreceededByEscape = false;

    // Loop through the frame characters
    for (int i = 0; i < frame.size(); i++)
    {
        // If the first character is a flag byte, then ignore it
        if (i == 0 && frame[i] == FLAG_BYTE)
        {
            continue;
        }

        // If the last character is a flag byte and it wasn't preceeded
        // by an escape character, then ignore it
        if (i == frame.size() - 1 && frame[i] == FLAG_BYTE && !wasPreceededByEscape)
        {
            continue;
        }

        // If the current character is an escape character and it wasn't
        // preceeded by another escape character, that means that the current
        // character is used to ignore the next character
        if (frame[i] == ESCAPE_CHAR && !wasPreceededByEscape)
        {
            wasPreceededByEscape = true;
            continue;
        }

        // If the current character is either a flag or an escape character
        // and it was preceeded by an escape character, then reset the wasPreceededByEscape
        // flag
        if ((frame[i] == FLAG_BYTE || frame[i] == ESCAPE_CHAR) && wasPreceededByEscape)
        {
            wasPreceededByEscape = false;
        }

        // Add the current character to the payload
        payload += frame[i];
    }

    return payload;
}
