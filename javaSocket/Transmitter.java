package javaSocket;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.*;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.lang.*;

public class Transmitter {

    private static final int MAX_PACKET_SIZE = 60 * 1024; // 64 KB
    private static final String FILE_NAME = "/C://Users//Startklar//Downloads//Kapitel II.pdf/";
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


        //erstes Paket
        byte[] transIDBytes = intToBytes(transmissionID); // wandle transmission id in byte-array
        byte[] seqNumberBytes = intToBytes(sequenceNumber); // wandle sequence number in byte-array
        byte[] maxSeqNumber = intToBytes(Integer.MAX_VALUE);
        byte[] firstPaket = new byte[266];  // größe zum übertragen des ersten pakets sind 266 byte maximal
        byte[] fileNameBytes = FILE_NAME.getBytes("UTF-8");
        System.arraycopy(transIDBytes, 0, firstPaket, 0, 2); // kopiere transmission id in data[]
        System.arraycopy(seqNumberBytes, 0, firstPaket, 2, 4); // kopiere sequence number in data[]
        System.arraycopy(maxSeqNumber, 0, firstPaket, 6, 4); // kopiere max sequence number in data[]
        System.arraycopy(fileNameBytes, 0, firstPaket, 10, fileNameBytes.length); // kopiere file name in data[]

        DatagramPacket packet = new DatagramPacket(firstPaket, firstPaket.length, InetAddress.getByName(DESTINATION_IP), DESTINATION_PORT);
        socket.send(packet);  // sende paket

        sequenceNumber++; // Inkrementieren der Sequenznummer für das nächste Paket
        transmissionID++;


        // zweites bis n-1tes Paket
        while ((bytesRead = fileInputStream.read(buffer)) != -1) {  // daten in buffer[] lesen
            totalBytesRead += bytesRead;
            transIDBytes = intToBytes(transmissionID); // wandle transmission id in byte-array
            seqNumberBytes = intToBytes(sequenceNumber); // wandle sequence number in byte-array
            byte[] data = new byte[bytesRead + 1024];  // größe zum übertragen + 6 bytes für transmission id & sequence number
            System.arraycopy(transIDBytes, 0, data, 0, transIDBytes.length); // kopiere transmission id in data[]
            System.arraycopy(seqNumberBytes, 0, data, transIDBytes.length, seqNumberBytes.length); // kopiere sequence number in data[]
            System.arraycopy(buffer, 0, data, transIDBytes.length + seqNumberBytes.length, bytesRead); // kopiere buffer in data[]

            packet = new DatagramPacket(data, data.length, InetAddress.getByName(DESTINATION_IP), DESTINATION_PORT);
            socket.send(packet);  // sende paket

            // Update MD5 checksum
            md.update(buffer, 0, bytesRead);

            sequenceNumber++; // Inkrementieren der Sequenznummer für das nächste Paket
            transmissionID++;
            /*
            if (totalBytesRead >= fileSize - MAX_PACKET_SIZE) {
                break; // Stop sending packets if the last 64 KB of the file are reached
            } */
        }


        byte[] lastPacket = new byte[22]; // letztes Paket ist 22 Byte lang
        System.arraycopy(transIDBytes, 0, lastPacket, 0, 2); // kopiere transmission id in data[]
        System.arraycopy(seqNumberBytes, 0, lastPacket, 2, 4); // kopiere sequence number in data[]
        //System.arraycopy(md, 0, lastPacket, 6, 16);

        DatagramPacket eofPacket = new DatagramPacket(lastPacket, lastPacket.length, InetAddress.getByName(DESTINATION_IP), DESTINATION_PORT);
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
