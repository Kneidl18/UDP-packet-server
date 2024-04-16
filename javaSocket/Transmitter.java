package javaSocket;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.*;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.lang.*;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Random;

public class Transmitter {

    private static final int MAX_PACKET_SIZE = 9000; // 9 KB max. Übertragungsgröße pro paket
    private static final String FILE_NAME = "/C://Users//Startklar//Downloads//IMG_6064.MOV/";
    private static final String DESTINATION_IP = "127.0.0.1";
    private static final int DESTINATION_PORT = 3004;
    private static IOException IllegalArgumentException;
    private static final Random rand = new Random();

    public static void main(String[] args) {
        try {
            DatagramSocket socket = new DatagramSocket();

            sendFile(socket);

            socket.close();
        } catch (IOException | NoSuchAlgorithmException e) {
            e.printStackTrace();
        }
    }

    private static void revertArray(byte[] a){
        for (int i = 0; i < a.length / 2; i++){
            byte tmp = a[i];
            a[i] = a[a.length - i - 1];
            a[a.length - i - 1] = tmp;
        }
    }


    private static void sendFile(DatagramSocket socket) throws IOException, NoSuchAlgorithmException {
        FileInputStream fileInputStream = new FileInputStream(new File(FILE_NAME));
        byte[] buffer = new byte[MAX_PACKET_SIZE];
        int bytesRead = 0;
        long fileSize = new File(FILE_NAME).length();
        int sequenceNumber = 0; //rand.nextInt(Integer.MAX_VALUE - (int) (fileSize/MAX_PACKET_SIZE));
        byte[] maxSeqNumber = intToBytes(sequenceNumber + (int) (fileSize/MAX_PACKET_SIZE) + 2);
        revertArray(maxSeqNumber);
        int transmissionID = rand.nextInt(Integer.MAX_VALUE);

        MessageDigest md = MessageDigest.getInstance("MD5");
        String splitFileName[] = FILE_NAME.split("/");
        md.update(splitFileName[splitFileName.length-1].getBytes());

        int totalBytesRead = 0;


        //erstes Paket
        byte[] transIDBytes = shortToBytes(transmissionID); // wandle transmission id in byte-array
        byte[] seqNumberBytes = intToBytes(sequenceNumber); // wandle sequence number in byte-array
        // because of little-endian/big-endian difference in c++ and java:
        // revert the numbers
        revertArray(transIDBytes);
        revertArray(seqNumberBytes);

        byte[] fileNameBytes = splitFileName[splitFileName.length-1].getBytes(StandardCharsets.UTF_8);
        byte[] firstPaket = null;
        if (fileNameBytes.length < 256) {
            firstPaket = new byte[fileNameBytes.length + 10];  // größe zum übertragen des ersten pakets ist variabelSystem.arraycopy(transIDBytes, 0, firstPaket, 0, 2); // kopiere transmission id in data[]
        } else {
            throw IllegalArgumentException;
        }
        System.arraycopy(transIDBytes, 0, firstPaket,0, 2);
        System.arraycopy(seqNumberBytes, 0, firstPaket, 2, 4); // kopiere sequence number in data[]
        System.arraycopy(maxSeqNumber, 0, firstPaket, 6, 4); // kopiere max sequence number in data[]
        System.arraycopy(fileNameBytes, 0, firstPaket, 10, fileNameBytes.length); // kopiere file name in data[]
        System.out.println("maximale Sequenznummer: " + byteArrayToInt(maxSeqNumber));

        DatagramPacket packet = new DatagramPacket(firstPaket, firstPaket.length, InetAddress.getByName(DESTINATION_IP), DESTINATION_PORT);
        socket.send(packet);  // sende paket
        System.out.println(sequenceNumber);

        sequenceNumber++;


        // zweites bis n-1tes Paket
        while ((bytesRead = fileInputStream.read(buffer)) != -1) {  // daten in buffer[] lesen
            totalBytesRead += bytesRead;
            transIDBytes = shortToBytes(transmissionID); // wandle transmission id in byte-array
            seqNumberBytes = intToBytes(sequenceNumber); // wandle sequence number in byte-array
            revertArray(transIDBytes);
            revertArray(seqNumberBytes);

            byte[] data = new byte[bytesRead + 6];  // 60 kb + 6 b
            System.arraycopy(transIDBytes, 0, data, 0, 2); // kopiere transmission id in data[]
            System.arraycopy(seqNumberBytes, 0, data, 2, 4); // kopiere sequence number in data[]
            System.arraycopy(buffer, 0, data, 6, bytesRead); // kopiere buffer in data[]

            packet = new DatagramPacket(data, data.length, InetAddress.getByName(DESTINATION_IP), DESTINATION_PORT);
            socket.send(packet);  // sende paket
            System.out.println(sequenceNumber /*+ (int) (fileSize/MAX_PACKET_SIZE)*/);

            // Update MD5 checksum
            md.update(buffer, 0, bytesRead);

            sequenceNumber++;
        }


        byte[] lastPacket = new byte[22]; // letztes Paket ist 22 Byte lang
        System.arraycopy(transIDBytes, 0, lastPacket, 0, 2); // kopiere transmission id in data[]
        System.arraycopy(maxSeqNumber, 0, lastPacket, 2, 4); // kopiere sequence number in data[]
        byte[] mdBytes = md.digest();
        System.arraycopy(mdBytes, 0, lastPacket, 6, 16);

        DatagramPacket eofPacket = new DatagramPacket(lastPacket, lastPacket.length, InetAddress.getByName(DESTINATION_IP), DESTINATION_PORT);
        socket.send(eofPacket);
        System.out.println("max Seq. Number sent: " + byteArrayToInt(maxSeqNumber));

        fileInputStream.close();

        System.out.println("MD5 checksum: " + bytesToHex(mdBytes));
    }

    private static byte[] intToBytes(int value) { // 4 Byte
        return new byte[] {
                (byte) (value >> 24),
                (byte) (value >> 16),
                (byte) (value >> 8),
                (byte) value
        };
    }

    private static byte[] shortToBytes(int value) {  // 2 Byte
        return new byte[] {
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

    public static int byteArrayToInt(byte[] bytes) {
        if (bytes.length != 4) {
            throw new IllegalArgumentException("Byte array length must be 4 to convert to int");
        }

        int value = 0;
        for (int i = 0; i < 4; i++) {
            value = (value << 8) | (bytes[i] & 0xFF);
        }
        return value;
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
