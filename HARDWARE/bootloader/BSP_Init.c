#include "BSP_Init.h"
#include "bootloader_config.h"
#include "stm32f10x.h"



static uint8_t uart2_dma_buf[2][UART2_RX_DMA_BUF_LEN];
static volatile uint8_t uart2_dma_active = 0;             //缓冲区索引


/* 应用层接收 FIFO (环形缓冲) */
static uint8_t  uart2_rx_fifo[UART2_RX_FIFO_LEN];
static volatile uint16_t uart2_fifo_w = 0;
static volatile uint16_t uart2_fifo_r = 0;

/*统计*/

static volatile uint32_t uart2_idle_cnt = 0;
static volatile uint32_t uart2_tc_cnt = 0;
static volatile uint32_t uart2_drop_cnt = 0;



static void uart2_fifo_push(uint8_t data)
{
	uint16_t next = (uart2_fifo_w + 1u) % UART2_RX_DMA_BUF_LEN;
	if(next != uart2_fifo_r)
	{
		uart2_rx_fifo[uart2_fifo_w] = data;
		uart2_fifo_w = next;
	}
	else 
	{
		uart2_drop_cnt ++;
	}
}






void BSP_Init(uint32_t baud)
{

	
	
	
}



static  void BSP_GPIO_Init()
{

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	
	GPIO_InitTypeDef   gpio;
	
	
	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin   = UART2_DMA_TX;
  gpio.GPIO_Mode  = GPIO_Mode_AF_PP;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &gpio);

  gpio.GPIO_Pin   = UART2_DMA_RX;
  gpio.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &gpio);
	
}


static void BSP_USART_DMA_Init(uint32_t baud)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	
	
	USART_InitTypeDef  usart;
	
	DMA_InitTypeDef dma;
	
	USART_StructInit(&usart);
  usart.USART_BaudRate            = baud;
  usart.USART_WordLength          = USART_WordLength_8b;
  usart.USART_StopBits            = USART_StopBits_1;
  usart.USART_Parity              = USART_Parity_No;
  usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  usart.USART_Mode                = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(USART2, &usart);
	

	
	DMA_DeInit(DMA1_Channel6);
	
	DMA_StructInit(&dma);
	DMA_DeInit(DMA1_Channel6);
  dma.DMA_PeripheralBaseAddr  = (uint32_t)&USART2->DR;
  dma.DMA_MemoryBaseAddr      = (uint32_t)uart2_dma_buf[0];
  dma.DMA_DIR                 = DMA_DIR_PeripheralSRC;          /* 外设作为源 */
  dma.DMA_BufferSize          = UART2_RX_DMA_BUF_LEN;           /*缓冲区长度*/
  dma.DMA_PeripheralInc       = DMA_PeripheralInc_Disable;
  dma.DMA_MemoryInc           = DMA_MemoryInc_Enable;
  dma.DMA_PeripheralDataSize  = DMA_PeripheralDataSize_Byte;
  dma.DMA_MemoryDataSize      = DMA_MemoryDataSize_Byte;
  dma.DMA_Mode                = DMA_Mode_Normal;                /* 非循环: 靠中断里手动重载 */
  dma.DMA_Priority            = DMA_Priority_High;
  dma.DMA_M2M                 = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel6, &dma);
	
	
	DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, ENABLE);   /* 缓冲区填满时触发 -> 乒乓切换 */
  DMA_Cmd(DMA1_Channel6, ENABLE);

	USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);
  USART_ITConfig(USART2, USART_IT_IDLE, ENABLE);
  USART_Cmd(USART2, ENABLE);

}

static void BSP_NVIC_Init()
{
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	
	NVIC_InitTypeDef   nvic;


  nvic.NVIC_IRQChannel                   = USART2_IRQn;
  nvic.NVIC_IRQChannelPreemptionPriority = 1;
  nvic.NVIC_IRQChannelSubPriority        = 0;
  nvic.NVIC_IRQChannelCmd                = ENABLE;
  NVIC_Init(&nvic);

  nvic.NVIC_IRQChannel                   = DMA1_Channel6_IRQn;
  nvic.NVIC_IRQChannelPreemptionPriority = 2;
  nvic.NVIC_IRQChannelSubPriority        = 0;
  nvic.NVIC_IRQChannelCmd                = ENABLE;
  NVIC_Init(&nvic);
	
}

