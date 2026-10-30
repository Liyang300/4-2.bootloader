#include "BSP_Init.h"
#include "bootloader_config.h"




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

static volatile uint8_t  swapping = 0;

static Buffer_status rx_sta = {0};



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



/************************************************************************************/

void BSP_Init(uint32_t baud)
{

	BSP_GPIO_Init();
	BSP_USART_DMA_Init(baud);
	BSP_NVIC_Init();
	
}



static void uart2_fifo_push(uint8_t data)
{
	uint16_t next = (uart2_fifo_w + 1u) % UART2_RX_FIFO_LEN;
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

static void uart2_dma_swap(void)
{
	
	if(swapping)
	{
		swapping = 0;
		return;
	}
	swapping = 1;
	
  uint16_t remain;    //寄存器值
	
	DMA_Cmd(DMA1_Channel6, DISABLE);
	
	//切换缓冲区
	rx_sta.done   = uart2_dma_active;
	remain = DMA_GetCurrDataCounter(DMA1_Channel6);
	rx_sta.recv   = (remain < UART2_RX_DMA_BUF_LEN) ? (UART2_RX_DMA_BUF_LEN - remain) : 0u;
	
	
	
	
	if (rx_sta.recv == 0u) 
	{                                  /* 没有新数据，直接恢复 DMA(仍指向当前缓冲区)，避免接收被永久关闭 */
    DMA_Cmd(DMA1_Channel6, ENABLE);
    return;
  }
	
	

	uart2_dma_active ^= 1u;
	
	DMA1_Channel6->CMAR = (uint32_t)uart2_dma_buf[uart2_dma_active];
	
	DMA_SetCurrDataCounter(DMA1_Channel6, UART2_RX_DMA_BUF_LEN);
	
	rx_sta.BufferChangeFlag = 1;            /*通知主循环取数据*/
	
	DMA_Cmd(DMA1_Channel6, ENABLE);
	
	swapping = 0;
	
}	

Buffer_status DMA_Buffer_PUSH(void)
{
	
	
	Buffer_status snap = {0};
	if(rx_sta.BufferChangeFlag == 0)
	{
		return snap;
	}
	
	
	// 关中断，原子读出一整份快照
	__disable_irq();
	snap = rx_sta;
	rx_sta.BufferChangeFlag = 0;
	__enable_irq();
	
	
	for (uint16_t i = 0u; i < snap.recv; i++) 
	{
    uart2_fifo_push(uart2_dma_buf[snap.done][i]);
  }
	
	
	return snap;
	
}



void USART2_IRQHandler(void)
{

	if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)
	{
		volatile uint16_t sr = USART2->SR;
    volatile uint16_t dr = USART2->DR;
    (void)sr; (void)dr;
    uart2_idle_cnt++;
    uart2_dma_swap();
	}
}


void DMA1_Channel6_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_TC6) != RESET) {
        DMA_ClearITPendingBit(DMA1_IT_TC6);
        uart2_tc_cnt++;
        uart2_dma_swap();
    }
}
