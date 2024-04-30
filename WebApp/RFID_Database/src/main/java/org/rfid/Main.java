package org.rfid;

import org.rfid.rfid_client.ServerCommunicator;

public class Main {

    public static void main(String[] args) {
        ServerCommunicator serverCommunicator = new ServerCommunicator();
        serverCommunicator.serverLoop();
    }
}