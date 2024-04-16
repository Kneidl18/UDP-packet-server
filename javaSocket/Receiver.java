package javaSocket;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.*;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;

import static java.lang.Integer.reverseBytes;

public class Receiver{
    private static final int MAX_PACKET_SIZE = 9000; // 60 KB max. Paketgröße
    private static final int MAX_PACKET_HEADER_SIZE = 10;
    private static String fileName = null;
    private static final int DESTINATION_PORT = 3004;
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
        int sequenceNumber = 0;
        int maxSeqNumber = Integer.MAX_VALUE;

        long startTime = 0;
        while (sequenceNumber < maxSeqNumber) {
            packet = new DatagramPacket(buffer, buffer.length);
            socket.receive(packet);

            byte[] receivedData = packet.getData();
            int length = packet.getLength();
            sequenceNumber = bytesToInt(receivedData, 2);
            sequenceNumber = reverseBytes(sequenceNumber); // revert because c++ code sends it reversed
            int totalReceivedBytes = 0;

            System.out.println("Seq. Number: " + sequenceNumber);

            firstPacketCount++;
            if (length <= 266 && firstPacketCount < 2) {
                byte[] fileNameBytes = new byte[length - 10];
                System.arraycopy(receivedData, 10, fileNameBytes, 0, length - 10);
                fileName = new String(fileNameBytes, StandardCharsets.UTF_8);
                fileName = new File(fileName).getName(); // dateiname aus pfad extrahieren
                fileOutputStream = new FileOutputStream(fileName);
                maxSeqNumber = bytesToInt(receivedData, 6);
                maxSeqNumber = reverseBytes(maxSeqNumber);
                startTime = System.currentTimeMillis();
                
                // add filename to md5 checksum
                md.update(fileName.getBytes());
                continue;
            }


            if (sequenceNumber != maxSeqNumber) {
                fileOutputStream.write(receivedData, 6, length - 6);
                md.update(receivedData, 6, length - 6);
            }
        }

        fileOutputStream.close();
        long endTime = System.currentTimeMillis();
        String filePath = fileName;
        File file = new File(filePath);
        long fileSize = file.length();

        byte[] mdBytes = md.digest();
        System.out.println("Received MD5 checksum: " + bytesToHex(mdBytes));
        System.out.println("Dateigröße: " + fileSize / 1000 + " Kb"); //Byte
        System.out.println("Gesamte Übertragungszeit: " + (double) (endTime - startTime)/1000.0 + " sek.");

        double dataRateMBps = (((double) fileSize / 1000000.0) / (double) (endTime - startTime)) * 1000.0;
        System.out.println("Datenrate: " + dataRateMBps + " MB/s");

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
