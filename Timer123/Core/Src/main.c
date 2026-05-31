#include <stdint.h>
#include <system_stm32f4xx.h>
#include <stm32f401re_rcc.h>
#include <stm32f401re_usart.h>
#include <stm32f401re_gpio.h>
#include <stm32f401re_tim.h>
#include <misc.h>
#include "utilities.h"

volatile uint32_t Number_Press = 0;
volatile uint32_t Tim_Rising   = 0;
volatile uint32_t Tim_Update   = 0;
volatile uint8_t  Status       = 0;

#define TimLimit_SendData 2000  // 1s tại tần số đếm 2KHz

/*===========================================================*/
void RCC_Configuration(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,   ENABLE); // Chỉ cấp 1 lần
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,  ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,  ENABLE);
}

/*===========================================================*/
void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // PA2 - USART2_TX
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_UP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);

    // PB3 - TIM2_CH2 Input Capture
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_NOPULL;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
    GPIO_PinAFConfig(GPIOB, GPIO_PinSource3, GPIO_AF_TIM2);
}

/*===========================================================*/
void USART2_Configuration(void)
{
    USART_InitTypeDef USART_InitStructure;

    USART_InitStructure.USART_BaudRate            = 9600;
    USART_InitStructure.USART_WordLength          = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits            = USART_StopBits_1;
    USART_InitStructure.USART_Parity              = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode                = USART_Mode_Tx; // Chỉ truyền
    USART_Init(USART2, &USART_InitStructure);
    USART_Cmd(USART2, ENABLE);
}

/*===========================================================*/
void OutString(char *s)
{
    while(*s) {
        while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
        USART_SendData(USART2, *s++);
    }
}

/*===========================================================*/
void TIM2_Configuration(void)
{
    TIM_TimeBaseInitTypeDef TIM_BaseStruct;
    TIM_ICInitTypeDef       TIM_ICStruct;
    NVIC_InitTypeDef        NVIC_InitStructure;

    // Timer Base: 84MHz / (41999+1) = 2KHz
    TIM_BaseStruct.TIM_Prescaler   = 41999;
    TIM_BaseStruct.TIM_Period      = 0xFFFF;
    TIM_BaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_BaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseInit(TIM2, &TIM_BaseStruct);

    // Input Capture kênh 2, bắt cả 2 sườn
    TIM_ICStruct.TIM_Channel     = TIM_Channel_2;
    TIM_ICStruct.TIM_ICPolarity  = TIM_ICPolarity_BothEdge;
    TIM_ICStruct.TIM_ICSelection = TIM_ICSelection_DirectTI;
    TIM_ICStruct.TIM_ICPrescaler = TIM_ICPSC_DIV1;
    TIM_ICStruct.TIM_ICFilter    = 0x0;
    TIM_ICInit(TIM2, &TIM_ICStruct);

    // NVIC
    TIM_ITConfig(TIM2, TIM_IT_CC2, ENABLE);
    NVIC_InitStructure.NVIC_IRQChannel                   = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority        = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd                = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    TIM_Cmd(TIM2, ENABLE);
}

/*===========================================================*/
static void Check_Tim_Press(void)
{
    static uint8_t Status1 = 0;
    Status1 = !Status1;

    if(Status1 == 1)           // Sườn xuống → nhấn nút
    {
        Number_Press++;
    }
    else                       // Sườn lên → nhả nút
    {
        Tim_Rising = TIM_GetCapture2(TIM2);
        Status = 1;            // Bắt đầu tính 1s
    }
}

void TIM2_IRQHandler(void)
{
    if(TIM_GetITStatus(TIM2, TIM_IT_CC2) != RESET)
    {
        Check_Tim_Press();
        TIM_ClearITPendingBit(TIM2, TIM_IT_CC2);
    }
}

/*===========================================================*/
void Send_NumberPress(void)
{
    uint32_t Tim_SendData = 0;

    if(Status == 1)
    {
        Tim_Update = TIM_GetCounter(TIM2);

        if(Tim_Update < Tim_Rising)
            Tim_SendData = (0xFFFF + Tim_Update) - Tim_Rising;
        else
            Tim_SendData = Tim_Update - Tim_Rising;

        if(Tim_SendData > TimLimit_SendData)
        {
            // Gửi số lần ấn dưới dạng ký tự ASCII
            while(USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
            USART_SendData(USART2, (uint8_t)(Number_Press + '0'));

            Status       = 0;
            Number_Press = 0;
        }
    }
}

/*===========================================================*/
int main(void)
{
    SystemCoreClockUpdate();
    RCC_Configuration();
    GPIO_Configuration();
    USART2_Configuration();
    TIM2_Configuration();

    OutString("Welcome to Nucleo F401RE\r\n");

    while(1)
    {
        Send_NumberPress();
    }
}
