#ifndef _BOOTLOADER_CONFIG
#define _BOOTLOADER_CONFIG


#define UART2_RX_DMA_BUF_LEN  512u    /* dma缓冲区块 */
#define UART2_RX_FIFO_LEN     1024u    /*应用全局缓冲区 */



#define UART2_BAUD    115200u    /* USART2 波特率，务必与串口助手设置一致 */
#define UART2_DMA_TX  GPIO_Pin_2
#define UART2_DMA_RX  GPIO_Pin_3






#endif