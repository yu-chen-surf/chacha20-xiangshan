struct uartlite_regs {
    volatile unsigned int rx_fifo;
    volatile unsigned int tx_fifo;
    volatile unsigned int status;
    volatile unsigned int control;
};

#define SR_TX_FIFO_FULL         (1<<3) /* transmit FIFO full */
#define SR_TX_FIFO_EMPTY        (1<<2) /* transmit FIFO empty */
#define SR_RX_FIFO_VALID_DATA   (1<<0) /* data in receive FIFO */
#define SR_RX_FIFO_FULL         (1<<1) /* receive FIFO full */

#define ULITE_CONTROL_RST_TX	0x01
#define ULITE_CONTROL_RST_RX	0x02

struct uartlite_regs *const ttyUL0 = (struct uartlite_regs *)0x40600000;

void uart_put_c(char c) {
    ttyUL0->tx_fifo = c;
}
