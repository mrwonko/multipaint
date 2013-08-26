#ifndef COMMON_CONST_HPP_INCLUDED
#define COMMON_CONST_HPP_INCLUDED

#define IMAGE_WIDTH 480
#define IMAGE_HEIGHT 320

#define NET_VERSION 1
#define NET_MESSAGE_HELLO "HELLO MRWONKO MULTIPAINT" // sent by client, followed by UInt32 version
#define NET_MESSAGE_REJECT_CLIENT "SORRY" // Server rejecting client
#define NET_MESSAGE_ACCEPT_CLIENT "WELCOME" // Server accepting client
#define NET_MESSAGE_QUEUE_INFO "QUEUE" // followed by UInt32 place in queue
#define NET_MESSAGE_SYNC "SYNC" // by server, followed by turn time left (float) and binary image data packed in a string
#define NET_MESSAGE_START_TURN "GO" // sent by server to current player
#define NET_MESSAGE_STARTING_TURN "GOING" // sent by client to confirm turn start
#define NET_MESSAGE_TURN_DONE "DONE" // sent by client once done, followed by binary image data packed in string
#define NET_MESSAGE_TURN_OVERTIME "SLACKER" // sent by server if client took to long to hand in turn
#define NET_MESSAGE_TURN_ACCEPTED "ACCEPTED" // sent by server to confirm turn
#define NET_MESSAGE_BYE "BYE" // ideally sent before closing the connection

#endif
