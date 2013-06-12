#ifndef DEFINES_H
#define DEFINES_H

#include "mbdev_unix.h"
#include "listfunctions.h"

#include <libxml/tree.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

//#define DEBUG
//#define DEBUG_NETWORK
//#define DEBUG_RANGES
//#define DEBUG_CONFIRMATION

// buffers used for inner processing
#define BUFSIZE_MAX 10240

// max length of string with primes
// (without taking other xml tags into account)
// customize it to adjust to different MTU settings
#define PRIMESPACKETSIZE_MAX 1000

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
#define MSGPART_PING 14

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

#endif // DEFINES_H
