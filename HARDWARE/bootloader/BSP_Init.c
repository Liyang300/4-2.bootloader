#include "BSP_Init.h"
#include "bootloader_config.h"




static uint8_t uart2_dma_buf[2][UART2_RX_DMA_BUF_LEN];
static volatile uint8_t uart2_dma_active = 0;             //ª∫≥Â«¯À˜“˝


/* ”¶”√≤„Ω” ’ FIFO (ª∑–Œª∫≥Â) */
static uint8_t  uart2_rx_fifo[UART2_RX_FIFO_LEN];
static volatile uint16_t uart2_fifo_w = 0;
static volatile uint16_t uart2_fifo_r = 0;

/*Õ≥º∆*/

static volatile uint32_t uart2_idle_cnt = 0;
static volatile uint32_t uart2_tc_cnt = 0;
static volatile uint32_t uart2_drop_cnt = 0;

static volatile uint8_t  swapping = 0;

static volatile Buffer_status rx_sta = {0};



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
  dma.DMA_DIR                 = DMA_DIR_PeripheralSRC;          /* Õ‚…Ë◊˜Œ™‘¥ */
  dma.DMA_BufferSize          = UART2_RX_DMA_BUF_LEN;           /*ª∫≥Â«¯≥§∂»*/
  dma.DMA_PeripheralInc       = DMA_PeripheralInc_Disable;
  dma.DMA_MemoryInc           = DMA_MemoryInc_Enable;
  dma.DMA_PeripheralDataSize  = DMA_PeripheralDataSize_Byte;
  dma.DMA_MemoryDataSize      = DMA_MemoryDataSize_Byte;
  dma.DMA_Mode                = DMA_Mode_Normal;                /* ∑«—≠ª∑: øø÷–∂œ¿Ô ÷∂Ø÷ÿ‘ÿ */
  dma.DMA_Priority            = DMA_Priority_High;
  dma.DMA_M2M                 = DMA_M2M_Disable;
  DMA_Init(DMA1_Channel6, &dma);
	
	
	DMA_ITConfig(DMA1_Channel6, DMA_IT_TC, ENABLE);   /* ª∫≥Â«¯ÃÓ¬˙ ±¥•∑¢ -> ∆π≈“«–ªª */
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
		
		return;
	}
	swapping = 1;
	
  uint16_t remain;    //dmaËÆ°Êï∞÷µ
	
	DMA_Cmd(DMA1_Channel6, DISABLE);
	
	//Áä∂ÊÄÅÊ†áËÆ∞
	rx_sta.done   = uart2_dma_active;
	remain = DMA_GetCurrDataCounter(DMA1_Channel6);
	rx_sta.recv   = (remain < UART2_RX_DMA_BUF_LEN) ? (UART2_RX_DMA_BUF_LEN - remain) : 0u;
	
	
	
	
	if (rx_sta.recv == 0u) 
	{                                  /* Ê≤°ÊúâÊï∞ÊçÆÁõ¥Êé•ËøîÂõû*/
    DMA_Cmd(DMA1_Channel6, ENABLE);
		
		swapping = 0;
    return;
  }
	
	

	uart2_dma_active ^= 1u;
	
	DMA1_Channel6->CMAR = (uint32_t)uart2_dma_buf[uart2_dma_active];
	
	DMA_SetCurrDataCounter(DMA1_Channel6, UART2_RX_DMA_BUF_LEN);
	
	rx_sta.BufferChangeFlag = 1;            /*ÈÄöÁü•‰∏ªÂæ™ÁéØÂèØ‰ª•Êõ¥Êç¢ÁºìÂÜ≤Âå∫Êã∑Ë¥ùÊï∞ÊçÆ*/
	
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
	
	
	// ÂÖ≥‰∏≠Êñ≠ËµãÂÄº
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

//Ëé∑Âèñ‰∏Ä‰∏™Â≠óËäÇ
int uart2_dma_fifo_getByte(uint8_t *ch)
{
	if(uart2_fifo_r == uart2_fifo_w)
	{
		return 0;
	}
	*ch = uart2_rx_fifo[uart2_fifo_r];
	uart2_fifo_r = (uart2_fifo_r + 1u) % UART2_RX_FIFO_LEN;
	
	return 1;
}


//Ëé∑ÂèñÊåáÂÆöÈïøÂ∫¶Êï∞ÊçÆ
int uart2_dma_fifo_getData(uint8_t *dst, uint16_t datalen)
{
	
	uint16_t n = 0;
	uint8_t  byte = 0;
	while((n < datalen) && (uart2_dma_fifo_getByte(&byte)))
	{
		dst[n++] = byte;
	}
	
	return n;
	
}

//ÂèëÈÄÅ
void UART2_SendByte(uint8_t b)
{
    USART_SendData(USART2, b);
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET) { }
}


//ÂèëÈÄÅ‰∏Ä‰∏™ÁºìÂÜ≤Âå∫
void UART2_SendBuf(const uint8_t *pbuf, uint16_t len)
{
    for(uint16_t i = 0; i < len; i++)
    {
        UART2_SendByte(pbuf[i]);
    }
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
