# MultiPaint Network Protocol

All done via TCP, port 14792 by default.

## Current Version

1

## Messages

Strings consist of first their length in 32 bit, then the Characters without Null-termination.

### "HELLO MRWONKO MULTIPAINT"<32 bit version>

Used by client on connection to verify compatibility

### "WELCOME"

Used by server on connection to accept a client (correct version)

### "SORRY"

Used by server to reject a client (wrong version, incorrect protocol) before closing connection

### "QUEUE"<uint>

Server notifying client of queue length change (happens every turn and when somebody leaves)

### "SYNC"<turn time left (float)><binary image data>

Synchronization of Image and turn-time from server to client.

### "GO"

Server notifying client of start of his turn, he has 5 seconds to respond

### "GOING"

Client notifying server of having started the turn. He now has 15 seconds to turn in the result.

### "DONE"<binary image data>

Client notifying server of being done editing

### "ACCEPTED"

Server notifying client that the image was accepted

### "SLACKER"

Server notifying client that he took to long and need not bother anymore

### "BYE"

Client/server signing off before closing the connection

## Binary Image Data

This is identical to the image data in the netpbm format - the image left to right, top to bottom, 1 bit per color, most significant bit being left. Resolution is 480*320, resulting in 19200 bytes.

It's sent as a string and as such prefixed with its size, 19200, as a 32 bit integer.