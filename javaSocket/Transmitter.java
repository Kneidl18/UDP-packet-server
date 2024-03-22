package javaSocket;

import java.io.IOException;
import java.net.*;

public class Transmitter {

    public static void main(String[] args) throws IOException {
        send("heute ist ein schoener tag", "127.0.0.1", 3000);
    }

    public static void send(String message, String ipAddress, int port) throws IOException {
        DatagramSocket socket = new DatagramSocket();
        InetAddress address = InetAddress.getByName(ipAddress);

        DatagramPacket packet = new DatagramPacket(message.getBytes(), message.length(), address, port);
        socket.send(packet);
        socket.close();
    }


}
