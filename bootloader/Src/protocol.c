#include "uart.h"
#include "crc.h"
#include "protocol.h"

int protocol_recv_frame(uint8_t *out, uint32_t max_len)
{
    if (uart_recv_byte() != FRAME_START) return -1;

    uint8_t len_h = uart_recv_byte();
    uint8_t len_l = uart_recv_byte();
    uint16_t len = ((uint16_t)len_h << 8) | len_l;
    if (len == 0 || len > max_len) return -1;

    uart_recv_buf(out, len);

    uint8_t crc_bytes[4];
    uart_recv_buf(crc_bytes, 4);
    uint32_t recv_crc = ((uint32_t)crc_bytes[0] << 24) | ((uint32_t)crc_bytes[1] << 16) |
                         ((uint32_t)crc_bytes[2] << 8)  |  (uint32_t)crc_bytes[3];

    if (crc32_calc(out, len) != recv_crc) return -1;
    return (int)len;
}
