package javaSocket;

import java.net.*;
public class Receiver{
    public static void main(String[] args) throws Exception {
        DatagramSocket ds = new DatagramSocket(3000);
        byte[] buf = new byte[1024];
        DatagramPacket dp = new DatagramPacket(buf, 1024);
        ds.receive(dp);
        String receivedMessage = new String(dp.getData(), 0, dp.getLength());
        System.out.println(receivedMessage);
        ds.close();
    }
}
