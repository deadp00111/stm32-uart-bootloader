#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stdint.h>

/* Frame: [0xAA][LEN_H][LEN_L][DATA...LEN][CRC32 4B big-endian] */
#define FRAME_START 0xAA
#define ACK         0x06
#define NACK        0x15
#define MAX_CHUNK   256

/* Reads one framed packet off UART into out[]. Returns length on success,
 * -1 on bad start byte / bad length / CRC mismatch. */
int protocol_recv_frame(uint8_t *out, uint32_t max_len);

#endif
