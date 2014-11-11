#ifndef DEFINES_H
#define DEFINES_H

#include "mbdev_unix.h"
#include "listfunctions.h"

#include <libxml/tree.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <stdint.h>
#include <inttypes.h>

// prints contents of received network and pipe messages
//#define DEBUG
// prints byte counts sent/received from network/pipes
//#define DEBUG_IO
// prints parsed xml message parts recevied from the network
//#define DEBUG_NETWORK
// prints from prime range related functions
//#define DEBUG_RANGES
// prings confirmed primes related functions
//#define DEBUG_CONFIRMATION
// application actually sends content over the network 2/3 of the time
//#define TEST_RETRY

// buffers used for inner processing
#define BUFSIZE_MAX PIPE_BUF //10240

// max length of string with primes
// (without taking other xml tags into account)
// customize it to adjust to different MTU settings
#define PRIMESPACKETSIZE_MAX 1000 // currently not used

#define PRIMEMAXCOUNT 50

#define XMLCHARS BAD_CAST
#define CHARS (const char*)

#define MSG_NULL 1
#define MSG_INVALID 2
#define MSG_VALID 3

#define MSGPART_NULL MSG_NULL
#define MSGPART_INVALID MSG_INVALID
#define MSGPART_PROCESSDATA 11
#define MSGPART_WORKERDATA 12
#define MSGPART_SERVERDATA 13
//#define MSGPART_PRIMES 14
//#define MSGPART_PING 14

#define STATUS_IDLE 1
#define STATUS_GENERATING 2
#define STATUS_CONFIRMING 3

// process is looking for primes
#define PROCSTATUS_COMPUTING 4
// computation finished, and at least some discovered primes
// are not yet confirmed by the server
#define PROCSTATUS_UNCONFIRMED 5
// all done
#define PROCSTATUS_CONFIRMED 6
// unique status reserved for situations when server requested less processes
#define PROCSTATUS_IDLE 7

#define HASH_MIN 1000000ULL
#define HASH_MAX 9999999ULL

#endif // DEFINES_H
