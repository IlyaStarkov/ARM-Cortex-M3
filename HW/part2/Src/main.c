#include "main.h"


char RxBuffer[RX_BUFF_SIZE];
char TxBuffer[TX_BUFF_SIZE];

float voltage_values[64];

int increment = 0;

bool ComReceived = false;

int main(void)
{
	memset(voltage_values,0,sizeof(voltage_values));
	initTIM2();
	initUSART1();
	initDAC();
	while(1)
	{
		if (ComReceived)
			ExecuteCommand();
	}


}

void initDAC(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;				//Включить тактирование альт. функций
	RCC->APB1ENR |= RCC_APB1ENR_DACEN;             	//Включить тактирование DAC
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;				//включить тактирование GPIOA

	GPIOA->CRL &=~(GPIO_CRL_CNF4 | GPIO_CRL_MODE4);

	DAC->CR |= DAC_CR_MAMP1;						//выбираем максимальную амплитуду
	DAC->CR |= DAC_CR_EN1;							//Включить ЦАП1 (PA4)
}

void initUSART1(void)
{
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
	RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;						//включить тактирование альтернативных ф-ций портов
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;					//включить тактирование UART2


	GPIOA->CRH &= ~(GPIO_CRH_MODE9 | GPIO_CRH_CNF9);		//PA9 на выход
	GPIOA->CRH |= (GPIO_CRH_MODE9_1 | GPIO_CRH_CNF9_1);

	GPIOA->CRH &= ~(GPIO_CRH_MODE10 | GPIO_CRH_CNF10);		//PA10 - вход
	GPIOA->CRH |= GPIO_CRH_CNF10_0;

	/*****************************************
	Скорость передачи данных - 19200
	Частота шины APB1 - 8МГц
	1. USARTDIV = 8'000'000/(16*19200) = 26.04
	2. 26 = 0x1A
	3. 16*0.04 = 0
	4. Итого 0x1A0
	*****************************************/
	USART1->BRR = 0x1A0; //19200 бод

	USART1->CR1 |= USART_CR1_RE | USART_CR1_TE | USART_CR1_UE;
	USART1->CR1 |= USART_CR1_RXNEIE;

	NVIC_EnableIRQ(37);
}

void USART1_IRQHandler(void)
{
	if ((USART1->SR & USART_SR_RXNE)!=0)
	{

		uint16_t pos = strlen(RxBuffer);


		RxBuffer[pos] = USART1->DR;
		if (RxBuffer[pos]== 0x0D)
		{

			ComReceived = true;
			return;
		}
	}
}

void initTIM2(void)
{
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;		//Включить тактирование TIM2

	//Частота APB1 для таймеров = APB1Clk * 2 = 32МГц * 2 = 64МГц
	TIM2->PSC = 8-1;						//Предделитель частоты (8МГц/8 = 1МГц)
	TIM2->ARR = 1000-1;						//Модуль счёта таймера (1MГц/1000 = 1 кГц)
	TIM2->DIER |= TIM_DIER_UIE;				//Разрешить прерывание по переполнению таймера

	NVIC_EnableIRQ(TIM2_IRQn);				//Рарзрешить прерывание от TIM2
	NVIC_SetPriority(TIM2_IRQn, 1);			//Выставляем приоритет
}

void TIM2_IRQHandler(void)
{
	TIM2->SR &= ~TIM_SR_UIF; //Сброс флага переполнения
	DAC->DHR12R1 = (voltage_values[increment%64]/3)*4095;
	increment++;
}


void txStr(char *str, bool crlf)
{
	uint16_t j;

	if (crlf)												//если просят,
		strcat(str,"\n");									//добавляем символ конца строки

	for (j = 0; j < strlen(str); j++)
	{
		USART1->DR = str[j];								//передаём байт данных
		while ((USART1->SR & USART_SR_TC)==0) {};			//ждём окончания передачи
	}
}

void ExecuteCommand(void)
{
	memset(TxBuffer,0,sizeof(TxBuffer));

	int acc = 0;
	if(checkString(RxBuffer))
	{
		voltValues(RxBuffer, voltage_values);
		if(max(voltage_values) <= 3.0)
		{
			strcpy(TxBuffer, "OK");
			txStr(TxBuffer, false);
			memset(RxBuffer, 0, RX_BUFF_SIZE);
			TIM2->CR1 |= TIM_CR1_CEN;		//Включить таймер
			ComReceived = false;
		}

		else
		{
			strcpy(TxBuffer, "One of the values exceeds 3V");
			txStr(TxBuffer, false);
			memset(RxBuffer, 0, RX_BUFF_SIZE);
			ComReceived = false;
		}
	}
	else
	{
		acc++;
		strcpy(TxBuffer, "The string does not match the pattern");
		txStr(TxBuffer, false);
		memset(RxBuffer, 0, RX_BUFF_SIZE);
		ComReceived = false;
	}

}

void voltValues(char* str, float* res)
{
	char clear_string[320];
	memset(clear_string,0,sizeof(clear_string));
	int j = 0;
	int volt = 0;
	for(int i = 0; i < 320; i++)
	{
		if(isdigit(str[i]))
		{
			clear_string[j] = str[i]-'0';
			j++;
		}
	}


	int c = 0;
	for(int i = 0; i < 64; i++)
	{
		for(int j = 0; j < 3; j++)
		{
			volt += clear_string[c+j]*pow(10, 2-j);
		}
		res[i] = (float)volt/100;
		volt = 0;
		c += 3;
	}

	return;
}

float max(float* array)
{
	int tmp;
	bool noSwap;

	for (int i = 64 - 1; i >= 0; i--)
	{
    	noSwap = 1;
    	for (int j = 0; j < i; j++)
    	{
        	if (array[j] > array[j + 1])
        	{
            	tmp = array[j];
            	array[j] = array[j + 1];
            	array[j + 1] = tmp;
            	noSwap = 0;
        	}
    	}
    	if (noSwap == 1)
       	    break;
	}

	return array[63];
}

bool checkString(char* str)
{
	if((strlen(str) == 320) && (findSemicolon(str) == 63) && (checkDigit(str) == 192))
	{
		return true;
	}
	else
	{
		return false;
	}



}

int findSemicolon(char* str)
{
	int acc = 0;
	int border = strlen(str)/5;
	for(int i = 0; i<border; i++)
	{
		if(str[4+i*0] == ';')
		{
			acc++;
		}
	}
	return acc-1;
}

int checkDigit(char* str)
{
	char clear_string[320];
	memset(clear_string,0,sizeof(clear_string));
	int j = 0;
	for(int i = 0; i < 320; i++)
	{
		if(isdigit(str[i]))
		{
			clear_string[j] = str[i];
			j++;
		}
	}
	return strlen(clear_string);
}


