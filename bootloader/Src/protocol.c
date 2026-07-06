#include "protocol.h"
#include "uart.h"
#include "flash.h"
#include "stm32f411_regs.h"

static void send_ack(void) {
    uart_send(ACK);
}

static void send_nack(void) {
    uart_send(NACK);
}

// XOR checksum 
static uint8_t checksum(uint8_t *data, uint32_t len) {
    uint8_t cs = 0;
    uint32_t i;
    for (i = 0; i < len; i++) {
        cs ^= data[i];
    }
    return cs;
}

// Write data to flash 
static void handle_write(void) {
    uint8_t addr_bytes[4];
    uint8_t addr_cs;
    uint8_t N;
    uint8_t data[256];
    uint8_t data_cs;
    uint32_t addr;
    uint32_t i;

    send_ack();

    //Receive address + checksum 
    addr_bytes[0] = uart_recv();
    addr_bytes[1] = uart_recv();
    addr_bytes[2] = uart_recv();
    addr_bytes[3] = uart_recv();
    addr_cs = uart_recv();

    if (addr_cs != (addr_bytes[0] ^ addr_bytes[1] ^
                    addr_bytes[2] ^ addr_bytes[3])) {
        send_nack();
        return;
    }

    addr = ((uint32_t)addr_bytes[0] << 24) |
           ((uint32_t)addr_bytes[1] << 16) |
           ((uint32_t)addr_bytes[2] << 8)  |
           (uint32_t)addr_bytes[3];

    // Only allow writing to app region 
    if (addr < APP_START) {
        send_nack();
        return;
    }

    send_ack();

    //Receive data 
    N = uart_recv();
    for (i = 0; i <= N; i++) {
        data[i] = uart_recv();
    }
    data_cs = uart_recv();

    if (data_cs != checksum(data, N + 1)) {
        send_nack();
        return;
    }

    flash_unlock();
    flash_write(addr, data, N + 1);
    flash_lock();

    send_ack();
}

// Erase flash sectors 
static void handle_erase(void) {
    uint8_t N;
    uint8_t sectors[16];
    uint8_t cs;
    uint32_t i;

    send_ack();

    N = uart_recv();
    for (i = 0; i <= N; i++) {
        sectors[i] = uart_recv();
    }
    cs = uart_recv();

    if (cs != checksum(sectors, N + 1)) {
        send_nack();
        return;
    }

    flash_unlock();

    if (N == 0xFF) {
        /* Global erase: sectors 4,5,6,7 */
        for (i = 4; i <= 7; i++) {
            flash_erase_sector(i);
        }
    } else {
        for (i = 0; i <= N; i++) {
            if (sectors[i] >= 4 && sectors[i] <= 7) {
                flash_erase_sector(sectors[i]);
            }
        }
    }

    flash_lock();
    send_ack();
}

//Jump to application 
static void handle_go(void) {
    uint8_t addr_bytes[4];
    uint8_t addr_cs;
    uint32_t addr;
    uint32_t sp;
    uint32_t pc;

    send_ack();

    addr_bytes[0] = uart_recv();
    addr_bytes[1] = uart_recv();
    addr_bytes[2] = uart_recv();
    addr_bytes[3] = uart_recv();
    addr_cs = uart_recv();

    if (addr_cs != (addr_bytes[0] ^ addr_bytes[1] ^
                    addr_bytes[2] ^ addr_bytes[3])) {
        send_nack();
        return;
    }

    addr = ((uint32_t)addr_bytes[0] << 24) |
           ((uint32_t)addr_bytes[1] << 16) |
           ((uint32_t)addr_bytes[2] << 8)  |
           (uint32_t)addr_bytes[3];

    send_ack();

    // Small delay for ACK to transmit 
    volatile uint32_t d = 100000;
    while (d--);

    // Read app's vector table 
    sp = *(volatile uint32_t *)addr;
    pc = *(volatile uint32_t *)(addr + 4);

    // Validate stack pointer 
    if ((sp & 0x2FFE0000) != 0x20000000) {
        return;
    }

    // Disable interrupts and jump 
    __asm volatile ("cpsid i");

    SCB_VTOR = addr;
    __asm volatile ("msr msp, %0" :: "r"(sp));

    void (*app)(void) = (void (*)(void))pc;
    app();
}

// Try to jump to existing app 
static void jump_to_app(void) {
    uint32_t sp = *(volatile uint32_t *)APP_START;

    if ((sp & 0x2FFE0000) == 0x20000000) {
        uint32_t pc = *(volatile uint32_t *)(APP_START + 4);

        SCB_VTOR = APP_START;
        __asm volatile ("msr msp, %0" :: "r"(sp));

        void (*app)(void) = (void (*)(void))pc;
        app();
    }
}

// Main bootloader loop 
void bootloader_run(void) {
    int cmd;

    //Send ready signal 
    uart_send('B');
    uart_send('O');
    uart_send('O');
    uart_send('T');
    uart_send('\r');
    uart_send('\n');

    // Wait 3 seconds for command 
    cmd = uart_recv_timeout(3000);

    if (cmd < 0) {
        // No command - jump to app if valid 
        jump_to_app();

        //No valid app, stay in bootloader 
        while (1) {
            uart_send('?');
            cmd = uart_recv_timeout(1000);
            if (cmd >= 0) break;
        }
    }

    //Command loop 
    while (1) {
        uint8_t c = (uint8_t)cmd;
        uint8_t comp = uart_recv();

        if ((c ^ comp) != 0xFF) {
            send_nack();
            cmd = uart_recv();
            continue;
        }

        switch (c) {
            case CMD_WRITE:
                handle_write();
                break;
            case CMD_ERASE:
                handle_erase();
                break;
            case CMD_GO:
                handle_go();
                break;
            default:
                send_nack();
                break;
        }

        cmd = uart_recv();
    }
}