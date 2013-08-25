# MultiPaint Network Protocol

All done via TCP, port unspecified (use whatever).

## Current Version

1

## Messages

Always preceded with "MRW MULTIPAINT\n" followed by integer version number in ASCII and another \n. Then the actual message.

### HELLO

Used by client on connection to verify compatibility

### WELCOME

Used by server on connection to accept a client (correct version)

### SORRY

Used by server to reject a client (wrong version, incorrect protocol) before closing connection

### QUEUE\n<uint>

Server notifying client of queue length change (happens every turn and when somebody leaves)

### NEWTURN\n<binary image data>

Server notifying client of start of (someone's) turn to sync the turn countdown (happens every turn) and the image

### GO

Server notifying client of start of his turn, he has 5 seconds to respond

### GOING

Client notifying server of having started the turn. He now has 15 seconds to turn in the result.

### DONE\n<binary image data>

Client notifying server of being done editing

### ACCEPTED

Server notifying client that the image was accepted

### SLACKER

Server notifying client that he took to long and need not bother anymore

### BYE

Client/server signing off before closing the connection

## Binary Image Data

This is identical to the image data in the netpbm format - the image left to right, top to bottom, 1 bit per color, most significant bit being left. Resolution is 480*320, resulting in 19200 bytes.