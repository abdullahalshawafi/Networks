package system;

@license(LGPL);

network Network
{
    @display("bgb=609,286");
    submodules:
        Node0: Node {
            @display("p=99,80;i=device/device;is=l");
        }
        Node1: Node {
            @display("p=500,80;i=device/device;is=l");
        }
        coordinator: Coordinator {
            @display("p=301,175;i=block/cogwheel;is=l");
        }
    connections:
        Node0.coordinator_gate_out --> coordinator.nodes_gate_in++;
        Node1.coordinator_gate_out --> coordinator.nodes_gate_in++;
        coordinator.nodes_gate_out++ --> Node0.coordinator_gate_in;
        coordinator.nodes_gate_out++ --> Node1.coordinator_gate_in;
        Node0.node_gate_out --> Node1.node_gate_in;
        Node1.node_gate_out --> Node0.node_gate_in;
}