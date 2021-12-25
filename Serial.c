#include <stdint.h>

#include "Serial.h"

serial_ring_t sri={0,0,0,{0}};
serial_ring_t sro={0,0,0,{0}};

/*
Packet format
byte 0 == packet start, SP_START
byte 1 == length 0 to 15 bytes to follow duplicated in upper and lower 4 bits
byte n == data
byte -2 = checksum 
byte -1 = packet end, SP_END
*/
#define SP_START            'C'
#define SP_END              'A'
#define SP_MAX_DATA_SIZE    (8)
#define SP_MIN_SIZE         (4)
#define SP_MAX_SIZE         (14)
#define SP_LEN_INVERT       (0xf0)

uint8_t packet_length=0;
uint8_t packet[SP_MAX_SIZE];
uint8_t reply_packet_length=0;
uint8_t reply_packet[SP_MAX_SIZE];

uint8_t SerialRingUsed(serial_ring_t *rp)
{
    return rp->count;
}

uint8_t SerialRingFree(serial_ring_t *rp)
{
    uint8_t f=(SERIAL_RING_SIZE-1)-(rp->count);
    return f;
}

void SerialRingAdd(serial_ring_t *rp, uint8_t b)
{
    uint8_t f=SerialRingFree(rp);
    if( 0 == f )
    {
        return; // Throw b away.
    }
    
    if( 1 == f )
    {
        // indicate buffer full and force an error
        b='X';
    }

    rp->ring[rp->head]=b;
    rp->head=(rp->head+1)%SERIAL_RING_SIZE;
    rp->count++;
}

void SerialRingAddString(serial_ring_t *rp, const char *s)
{
    if( ! s )
    {
        return;
    }

    const char *walk=s;
    while( *walk )
    {
        SerialRingAdd(rp,*walk);
        walk++;
    }
}

uint8_t SerialRingPop(serial_ring_t *rp)
{
    uint8_t u=SerialRingUsed(rp);
    if( 0 == u )
    {
        return 0;
    }
    uint8_t returnme=rp->ring[rp->tail];
    rp->tail=(rp->tail+1)%SERIAL_RING_SIZE;
    rp->count--;
    return returnme;
}

uint8_t SerialRingPeek(serial_ring_t *rp, uint8_t offset)
{
    uint8_t u=SerialRingUsed(&sri);
    if( offset >= u )
    {
        // can't peek past the end of data
        return 0;
    }

    uint8_t index=(rp->tail+offset)%SERIAL_RING_SIZE;
    uint8_t returnme=rp->ring[index];
    return returnme;
}