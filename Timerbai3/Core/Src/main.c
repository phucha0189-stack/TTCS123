#include <stdint.h>
#include <stm32f401re_rcc.h>
#include <stm32f401re_tim.h>
#include <stm32f401re_gpio.h>
#include <stm32f401re_usart.h>
#include <misc.h>

#define Tim_Period      			  		 8399

uint32_t Number_Press = 0;
uint32_t Tim_Rising = 0;
uint8_t Status = 0;
static void AppInitCommont(void);
void TimPwm_Init(void);
void LedControl_PWM(uint16_t Duty_Cycle);
void delay(uint32_t ms);
int main(void)
{
	AppInitCommont();
	while(1)
		{
			LedControl_PWM(60);
		}
}

static void AppInitCommont(void)
{
	SystemCoreClockUpdate();
	TimPwm_Init();
}


void delay(uint32_t ms)
{
	for(uint32_t i = 0; i<ms;i++)
	{
		for(uint32_t j = 0; j<500000; j++);
	}
}
void TimPwm_Init(void)
{
	GPIO_InitTypeDef 			GPIO_InitStruct;
	TIM_TimeBaseInitTypeDef 	TIM_TimeBaseInitStruct;
	TIM_OCInitTypeDef			TIM_OC_InitStruct;


	// GPIO Configure
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;

	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_11;
	GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource11, GPIO_AF_TIM1);

	//TimeBase Configure

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

	TIM_TimeBaseInitStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInitStruct.TIM_Prescaler = 0;
	TIM_TimeBaseInitStruct.TIM_Period = 8399;
	TIM_TimeBaseInitStruct.TIM_ClockDivision = 0;
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseInitStruct);


	//TimeOC Configure
	TIM_OC_InitStruct.TIM_OCMode = TIM_OCMode_PWM2;
	TIM_OC_InitStruct.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OC_InitStruct.TIM_Pulse = 0;
	TIM_OC_InitStruct.TIM_OCPolarity = TIM_OCPolarity_Low;

	TIM_OC4Init(TIM1, &TIM_OC_InitStruct);

	TIM_Cmd(TIM1, ENABLE);


	TIM_CtrlPWMOutputs(TIM1, ENABLE);

}

void LedControl_PWM(uint16_t Duty_Cycle)
{
	uint16_t pulse_length = 0;

	// caculation pulse_length

	pulse_length = ((Tim_Period *Duty_Cycle)/ 100);

	TIM_SetCompare4(TIM1, pulse_length);
}

