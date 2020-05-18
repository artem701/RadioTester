
// FROM RECEIVER

// tx buffer of receiver contents of one byte of status
// and then received payload 

// if no responce from receiver
#define NO_RESPONSE 0

// receiver is setting up
// (changing channel or filling spis buffer)
#define NOT_READY   1

// receiver hasn't got any packets
#define NO_PACKET   2

#define GOT_PACKET  3

// transmitter's command performed successfully
#define SUCCESS	    4

// FROM TRANSMITTER

// message from transmitter is always one byte command

// every commad should be followed by one status check!

// after START_LISTEN and one STATUS_CHECK

// do nothing, return current status
#define STATUS_CHECK 0

// start to loopback
#define START_LISTENING 1

#define STOP_LISTENING 2

// values 11-26 for channel switch