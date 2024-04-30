package org.rfid.rfid_client;

import org.rfid.requests.CheckTagRequest;
import org.rfid.requests.Request;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketTimeoutException;

public class ServerCommunicator {
    private static final int RX_FRAME_LENGTH = 16;
    private static final int TX_FRAME_LENGTH = 100;
    private Socket clientSocket = null;
    private DataOutputStream dos;
    private DataInputStream dis;
    private byte[] rxBuffer = new byte[RX_FRAME_LENGTH];
    private byte[] txBuffer = new byte[TX_FRAME_LENGTH];
    private final Request requestManager;
    private static final int PORT = 8080;
    private static final String HOST = "192.168.1.6";

    public ServerCommunicator() {
        requestManager = new CheckTagRequest();

        while (clientSocket == null || !clientSocket.isConnected()) {
            try {
                clientSocket = new Socket();
                InetSocketAddress adress = new InetSocketAddress(HOST, PORT);
                clientSocket.connect(adress, 1000);
            } catch (IOException e) {
                System.err.println("Could not connect to RFID server. Retrying...");
                try {
                    Thread.sleep(1000); // Așteaptă 1 secundă înainte de a încerca din nou
                } catch (InterruptedException ex) {
                    System.err.println("Sleep interrupted.");

                    /* Clean up whatever needs to be handled before interrupting  */
                    Thread.currentThread().interrupt();
                }
            }
        }
        try {
            clientSocket.setKeepAlive(true);
            clientSocket.setSoTimeout(1000);
            dos = new DataOutputStream(clientSocket.getOutputStream());
            dis = new DataInputStream(clientSocket.getInputStream());
        }catch (IOException ex) {
            System.err.println("Could not get Streams or set Keep Alive flag.");
        }
    }

    private boolean readESPdata(){
        boolean result = false;
        int readStatus;
        boolean writeStatus;
        byte[] temporaryResponse = "RX:Done\n".getBytes();

        try {
            if(clientSocket.isConnected()) {
                readStatus = dis.read(rxBuffer, 0, RX_FRAME_LENGTH);
                if (readStatus != -1) {
                    System.out.print("Bytes have been received: "+ new String(rxBuffer, 0, 4));
                    for(int i = 4; i < rxBuffer.length; i++) {
                        System.out.printf("%02X ", rxBuffer[i]);
                    }
                    System.out.print("\n");
                    if (readStatus != RX_FRAME_LENGTH) {
                        System.err.println("Incomplete response has been received: "+ new String(rxBuffer, 0, 4));
                        result = false;
                    } else {
                        writeStatus = writeESPdata(temporaryResponse);
                        if(writeStatus) {
                            result = true;
                        }
                    }
                } else {
                    // Close the socket if the loop exits due to end of stream
                    result = false;
                    clientSocket.close();
                    System.err.println("Client disconnected");
                }
            }
        }catch (SocketTimeoutException e)
        {
            /*Do nothing if timeout is over*/
        }
        catch (IOException e) {
            try {
                clientSocket.close();
            } catch (IOException ex) {
                System.err.println("Could not close socket.");
            }
            System.err.println("Cannot read socket data: " + e.getMessage());
        }

        return result;
    }
    private boolean writeESPdata(byte[] data) {
        boolean result = false;

        try {
            if(clientSocket.isConnected()) {
                result = true;
                dos.write(data);
            }
            else
            {
                clientSocket.close();
                System.err.println("Client disconnected");
            }
        }catch (IOException e)
        {
            System.err.println("Cannot write socket data: " + e.getMessage());
            try {
                clientSocket.close();
            } catch (IOException ex) {
                System.err.println("Cannot close socket: " + ex.getMessage());
            }
        }

        return result;
    }

    private void decodeRequest()
    {
        StringBuilder textualEPC = new StringBuilder();
        String rxTextualBuffer;

        for (int i = 4; i < rxBuffer.length; i++) {
            textualEPC.append(String.format("%02X", rxBuffer[i]));
        }

        rxTextualBuffer = new String(rxBuffer, 0, 4) + textualEPC;

        if(rxBuffer.length <= RX_FRAME_LENGTH) {
            if(rxTextualBuffer.contains("tag:")) {
                String[] request = rxTextualBuffer.split(":");
                if(request.length == 2) {
                    txBuffer = requestManager.processRequest(request[1]).getBytes();
                }
                else {
                    txBuffer = "TX:NOK\n".getBytes();
                    System.err.println("Incomplete request: " + rxTextualBuffer);
                }
            }
        }
        else
        {
            System.err.println("Invalid RFID data received : Length of frame do not match: "+rxTextualBuffer);
        }
    }

    public void serverLoop()
    {
        while(true)
        {
            if(clientSocket == null || !clientSocket.isConnected() || clientSocket.isClosed()) {
                try {
                    clientSocket = new Socket();
                    InetSocketAddress adress = new InetSocketAddress(HOST, PORT);
                    clientSocket.connect(adress, 1000);

                    clientSocket.setSoTimeout(1000);
                    clientSocket.setKeepAlive(true);
                    dos = new DataOutputStream(clientSocket.getOutputStream());
                    dis = new DataInputStream(clientSocket.getInputStream());
                } catch (IOException e) {
                    System.err.println("Could not connect to RFID server. Retrying...");
                }
            }
            else
            {
                if (readESPdata()) {
                    decodeRequest();
                    if (!writeESPdata(txBuffer)) {
                        System.err.println("Cannot write RFID data");
                    }
                }
            }
        }
    }
}
