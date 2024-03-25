package javaSocket;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.*;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.lang.*;

public class Transmitter {

    private static final int MAX_PACKET_SIZE = 64 * 1024; // 64 KB
    private static final String FILE_NAME = "example.txt";
    private static final String DESTINATION_IP = "127.0.0.1";
    private static final int DESTINATION_PORT = 3000;

    public static void main(String[] args) {
        try {
            DatagramSocket socket = new DatagramSocket();

            sendFile(socket);

            socket.close();
        } catch (IOException | NoSuchAlgorithmException e) {
            e.printStackTrace();
        }
    }

    private static void sendFile(DatagramSocket socket) throws IOException, NoSuchAlgorithmException {
        FileInputStream fileInputStream = new FileInputStream(new File(FILE_NAME));
        byte[] buffer = new byte[MAX_PACKET_SIZE];
        int bytesRead = 0;
        int sequenceNumber = 0;
        int transmissionID = 1;

        MessageDigest md = MessageDigest.getInstance("MD5");

        int totalBytesRead = 0;
        long fileSize = new File(FILE_NAME).length();

        while ((bytesRead = fileInputStream.read(buffer)) != -1) {  // daten in buffer[] lesen
            totalBytesRead += bytesRead;
            byte[] transIDBytes = intToBytes(transmissionID); // wandle transmission id in byte-array
            byte[] seqNumberBytes = intToBytes(sequenceNumber); // wandle sequence number in byte-array
            byte[] data = new byte[bytesRead + 8];  // größe zum übertragen + 8 bytes für transmission id & sequence number
            System.arraycopy(transIDBytes, 0, data, 0, 4); // kopiere transmission id in data[]
            System.arraycopy(seqNumberBytes, 0, data, 4, 4); // kopiere sequence number in data[]
            System.arraycopy(buffer, 0, data, 8, bytesRead); // kopiere buffer in data[]

            DatagramPacket packet = new DatagramPacket(data, data.length, InetAddress.getByName(DESTINATION_IP), DESTINATION_PORT);
            socket.send(packet);  // sende paket

            // Update MD5 checksum
            //md.update(buffer, 0, bytesRead);

            sequenceNumber++; // Inkrementieren der Sequenznummer für das nächste Paket
            transmissionID++;
            if (totalBytesRead >= fileSize - MAX_PACKET_SIZE) {
                break; // Stop sending packets if the last 64 KB of the file are reached
            }
        }

        // Send packet with sequence number set to -1 to indicate end of file
        byte[] endOfFilePacket = intToBytes(Integer.MAX_VALUE);
        DatagramPacket eofPacket = new DatagramPacket(endOfFilePacket, endOfFilePacket.length, InetAddress.getByName(DESTINATION_IP), DESTINATION_PORT);
        socket.send(eofPacket);

        fileInputStream.close();

        // Calculate MD5 checksum and print
        byte[] md5Bytes = md.digest();
        System.out.println("MD5 checksum: " + bytesToHex(md5Bytes));
    }

    private static byte[] intToBytes(int value) {
        return new byte[] {
                (byte) (value >> 24),
                (byte) (value >> 16),
                (byte) (value >> 8),
                (byte) value
        };
    }

    private static String bytesToHex(byte[] bytes) {
        StringBuilder sb = new StringBuilder();
        for (byte b : bytes) {
            sb.append(String.format("%02x", b));
        }
        return sb.toString();
    }




















    /*
    public static void send(String message, String ipAddress, int port) throws IOException {
        DatagramSocket socket = new DatagramSocket();
        InetAddress address = InetAddress.getByName(ipAddress);

        DatagramPacket packet = new DatagramPacket(message.getBytes(), message.length(), address, port);
        socket.send(packet);
        socket.close();
    } */


}
