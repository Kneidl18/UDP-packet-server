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

### command line tools:

- **listen** <br>
general listen on default port 8080:
`./PP --listen` <br>
listen on specific port and ip (both optional): `./PP [-p <port>] [--ip <ip_addr>] --listen`<br>
write received files to specific output directory (new dir is created if necessary): `./PP [-o <pathToDir>] --listen` <br>
listen on port and ip and sotre to directory: `./PP [-p <port>] [--ip <ip_addr>] [-o <pathToDir>] --listen`
- **send** <br>
general send to default ip and port (localhost, 8080): `./PP --send <pathToFile>`<br>
send with ip and port (both optional): `./PP [-p <port>] [--ip <ip_addr>] --send <pathToFile>` <br>
- **verbose** <br>
for verbose output, add `-v` to the call: ```./PP -v --listen```