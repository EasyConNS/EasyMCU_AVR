#pragma once

#define SERIAL_RING_SIZE    (32)
typedef struct serial_ring_t {
    uint8_t head; // incremented as bytes added
    uint8_t tail; // incremented as bytes removed
    uint8_t count; // number of bytes of data present
    uint8_t ring[SERIAL_RING_SIZE];
} serial_ring_t;

extern serial_ring_t sri;
extern serial_ring_t sro;

uint8_t SerialRingUsed(serial_ring_t *rp);
uint8_t SerialRingFree(serial_ring_t *rp);
void SerialRingAdd(serial_ring_t *rp, uint8_t b);
void SerialRingAddString(serial_ring_t *rp, const char *s);
uint8_t SerialRingPop(serial_ring_t *rp);
uint8_t SerialRingPeek(serial_ring_t *rp, uint8_t offset);
