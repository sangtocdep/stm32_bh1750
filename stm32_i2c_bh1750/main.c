#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_i2c.h"
#include "string.h"
#include "stdio.h"

#define BH1750_ADDRESS 0x23
#define BH1750_CMD_START 0x10 

void I2C_Config(void);
void USART_Config(void);
void I2C_Start(void);
void I2C_Write(uint8_t data);



volatile uint32_t sysTickCounter = 0;

void SysTick_Config_Init(void)
{
    SysTick_Config(SystemCoreClock / 1000);
}

void SysTick_Handler(void)
{
    if(sysTickCounter > 0)
    {
        sysTickCounter--;
    }
}

void delay_ms(uint32_t ms)
{
    sysTickCounter = ms;
    while(sysTickCounter != 0);
}

void GPIO_Config(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitTypeDef GPIO;
    
    // USART TX (PA9) và RX (PA10)
    GPIO.GPIO_Pin = GPIO_Pin_9;
    GPIO.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO);
    
    GPIO.GPIO_Pin = GPIO_Pin_10;
    GPIO.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO);
    
    // I2C SCL (PB6) và SDA (PB7)
    GPIO.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO);
}

void USART_Config(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);
    
    USART_InitTypeDef USART;
    USART.USART_BaudRate = 9600;
    USART.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART.USART_Parity = USART_Parity_No;
    USART.USART_StopBits = USART_StopBits_1;
    USART.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART1, &USART);
    USART_Cmd(USART1, ENABLE);
}

void I2C_Config(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    
    I2C_InitTypeDef I2C;
    I2C.I2C_Ack = I2C_Ack_Enable;
    I2C.I2C_ClockSpeed = 200000;
    I2C.I2C_Mode = I2C_Mode_I2C;
    I2C.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C.I2C_OwnAddress1 = 0x0B;
    I2C.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &I2C);
    
    I2C_Cmd(I2C1, ENABLE);
}

void I2C_Start(void)
{
    I2C_GenerateSTART(I2C1, ENABLE);
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
}

void I2C_Write(uint8_t data)
{
	  //I2C_GenerateSTART(I2C1, ENABLE);
    //while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
	
    I2C_Send7bitAddress(I2C1, BH1750_ADDRESS << 1, I2C_Direction_Transmitter);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));
    
    I2C_SendData(I2C1, data);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
}

uint16_t I2C_Read_Light(void)
{
    I2C_GenerateSTART(I2C1, ENABLE);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT));
    
    I2C_Send7bitAddress(I2C1, BH1750_ADDRESS << 1, I2C_Direction_Receiver);
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));
    
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
    uint8_t msb = I2C_ReceiveData(I2C1);
    
    while (!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED));
    uint8_t lsb = I2C_ReceiveData(I2C1);
    
    I2C_AcknowledgeConfig(I2C1, DISABLE);
    I2C_GenerateSTOP(I2C1, ENABLE);
    
    return (msb << 8) | lsb;
}

void usart_string_transmit(char data[])
{
    for(uint8_t i = 0; i < strlen(data); i++)
    {
        USART_SendData(USART1, data[i]);
        while(USART_GetFlagStatus(USART1, USART_FLAG_TXE)==RESET);
    }
}

int main(void)
{
    SysTick_Config_Init();
    GPIO_Config();
    I2C_Config();
    USART_Config();
    
    I2C_Start();
    I2C_Write(BH1750_CMD_START);
    
    while(1)
    {
        delay_ms(180);
        uint16_t lux = I2C_Read_Light();
        
        char buffer[20];
        sprintf(buffer, "Light Intensity: %u lux\r\n", lux);
        usart_string_transmit(buffer);
        delay_ms(1000);
    }
}

