#ifndef PROTOCOL_H
#define PROTOCOL_H

//Command codes 
#define ACK         0x79
#define NACK        0x1F
#define CMD_WRITE   0x31
#define CMD_ERASE   0x43
#define CMD_GO      0x21

void bootloader_run(void);

#endif