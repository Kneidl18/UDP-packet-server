package javaSocket;

import java.io.FileOutputStream;
import java.io.IOException;
import java.net.*;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class Receiver{
    private static final int MAX_PACKET_SIZE = 60 * 1024; // 60 KB max. Paketgröße
    private static final String DESTINATION_IP = "127.0.0.1";
    private static final int DESTINATION_PORT = 3000;

    public static void main(String[] args) {
        try {
            DatagramSocket socket = new DatagramSocket(DESTINATION_PORT);
            receiveFile(socket);
            socket.close();
        } catch (IOException | NoSuchAlgorithmException e) {
            e.printStackTrace();
        }
    }

    private static void receiveFile(DatagramSocket socket) throws IOException, NoSuchAlgorithmException {
        FileOutputStream fileOutputStream = new FileOutputStream("received_file4.pdf");
        MessageDigest md = MessageDigest.getInstance("MD5");

        byte[] buffer = new byte[MAX_PACKET_SIZE];
        DatagramPacket packet;

        while (true) {
            packet = new DatagramPacket(buffer, buffer.length);
            socket.receive(packet);

            byte[] receivedData = packet.getData();
            int length = packet.getLength();
            int sequenceNumber = bytesToInt(receivedData, 2); // Extrahiere die Sequenznummer aus den empfangenen Daten
            System.out.println(sequenceNumber);

            if (length == 22) {  //letztes Paket übertragen = springe aus der schleife
                break;
            }

            fileOutputStream.write(receivedData, 6, length - 6);
            md.update(receivedData, 6, length - 6);
        }

        fileOutputStream.close();

        byte[] mdBytes = md.digest();
        System.out.println("Received MD5 checksum: " + bytesToHex(mdBytes));
    }

    private static String bytesToHex(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            sb.append(String.format("%02x", b));
        }
        return sb.toString();
    }

    private static int bytesToInt(byte[] bytes, int offset) {
        return ((bytes[offset] & 0xFF) << 24) |
                ((bytes[offset + 1] & 0xFF) << 16) |
                ((bytes[offset + 2] & 0xFF) << 8) |
                (bytes[offset + 3] & 0xFF);
    }

}
