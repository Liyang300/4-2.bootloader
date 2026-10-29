#ifndef _BOOTLOADER_CONFIG
#define _BOOTLOADER_CONFIG


#define UART2_RX_DMA_BUF_LEN  512u    /* 每个 DMA 双缓冲区的长度(字节)，建议 2 的幂 */
#define UART2_RX_FIFO_LEN     1024u    /* 应用层接收 FIFO 长度(字节)，建议 2 的幂 */


#define UART2_DMA_TX  GPIO_Pin_2
#define UART2_DMA_RX  GPIO_Pin_3




#endif