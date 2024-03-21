# This is a small and simple project to work with UDP

The goal is to create simple packages and send them over UDP. First step is
to just check correctness of the packets using a java.MD5 checksum at the end of
the packet transmission.

### Aufbau Pakete:
```
// normales Paket
Packet 
    Transmission id (16),
    Sequence Number (32),
    Data (..)
}

// wenn die Sequenz Nummber = null ist (also erste Paket der startenden
// Übertragung)
Packet {
    Transmission id (16),
    Sequence Number (32),
    Max Sequence Number (32),
    File Name (2..2048)
}

// das lezte Paket der Übertragung (wichtig: checksum)
Packet {
    Transmission id (16),
    Sequence Number (32),
    java.MD5 (128)
}
```