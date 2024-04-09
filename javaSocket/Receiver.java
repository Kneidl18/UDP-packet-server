package javaSocket;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.*;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

public class Receiver{
    private static final int MAX_PACKET_SIZE = 9 * 1024; // 60 KB max. Paketgröße
    private static final int MAX_PACKET_HEADER_SIZE = 10;
    private static String fileName = null;
    private static final int DESTINATION_PORT = 3000;
    private static FileOutputStream fileOutputStream = null;
    private static int firstPacketCount = 0;

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
        MessageDigest md = MessageDigest.getInstance("MD5");
        byte[] buffer = new byte[MAX_PACKET_SIZE + MAX_PACKET_HEADER_SIZE];
        DatagramPacket packet;

        while (true) {
            packet = new DatagramPacket(buffer, buffer.length);
            socket.receive(packet);

            byte[] receivedData = packet.getData();
            int length = packet.getLength();
            int sequenceNumber = bytesToInt(receivedData, 2);
            System.out.println(sequenceNumber);

            // TODO: the first sequence number doesn't have to be 0. it is a random number in the range
            // of possible numbers
            firstPacketCount++;
            if (length <= 266 && firstPacketCount < 2) {
                byte[] fileNameBytes = new byte[length - 10];
                System.arraycopy(receivedData, 10, fileNameBytes, 0, length - 10);
                fileName = new String(fileNameBytes, StandardCharsets.UTF_8);
                fileName = new File(fileName).getName(); // dateiname aus pfad extrahieren
                fileOutputStream = new FileOutputStream(fileName);
            }

            // TODO: max sequence number is the sequence number of the end-packet
            if (length == 22) {  //letztes Paket übertragen = springe aus der schleife
                System.out.println("DIE LETZTE SEQUENZ!!!");
                break;
            }

            fileOutputStream.write(receivedData, 6, length - 6);
            if (sequenceNumber != 0) {
                md.update(receivedData, 6, length - 6);
            }
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
