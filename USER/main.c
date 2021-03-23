#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "lcd.h"
#include "24l01.h"
/************************************************
 ALIENTEK Mini STM32F103开发板实验24
 NRF24L01无线通信实验-HAL库函数版
 技术支持：www.openedv.com
 淘宝店铺： http://eboard.taobao.com 
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/
u8 MESSAGE[41] = {"CONNECT--SUCCESSFUL\0"}; //判断通讯的消息
u8 RX_buf[41];								//接受的缓存
char uart_buf[41];							//串口的缓存
char TX_buf[41];							//发送的缓存

u8 status = 0;								//状态：1是开启连接，0是关闭连接
char screen_msg[10][40];
uint16_t line_color[10];

int print_idx, write_idx;
int is_full;

void left_show(char *msg);
void screen_write(char *msg, uint16_t color);
void screen_show();
void show_status(char *msg);

void build_ui()
{
	LCD_Clear(BLACK);
	POINT_COLOR = WHITE;

	//LCD_ShowString(0,220,120,20,20,(uint8_t *)"Group 5");
	LCD_DrawRectangle(29, 55, 212, 256);
	LCD_Fill(30, 56, 211, 255, GRAY);
	LCD_Fill(32, 58, 210, 254, WHITE);
	LCD_DrawRectangle(31, 26, 101, 46);
	LCD_Fill(32, 27, 100, 45, GRAY);
	LCD_Fill(33, 28, 99, 44, WHITE);
	LCD_DrawRectangle(141, 26, 211, 46);
	LCD_Fill(142, 27, 210, 45, GRAY);
	LCD_Fill(143, 28, 209, 44, WHITE);
	POINT_COLOR = BLACK;
	BACK_COLOR = WHITE;
	LCD_ShowString(35, 28, 50, 16, 16, (uint8_t *)"Client");
	LCD_ShowString(145, 28, 50, 16, 16, (uint8_t *)"Server");
}

void show_status(char *msg)
{
	BACK_COLOR = BLACK;
	POINT_COLOR = RED;
	LCD_Fill(40, 280, 300, 20, BLACK);
	LCD_ShowString(30, 280, 260, 16, 16, (uint8_t *)msg);
}

void screen_show()
{
	int i, idx;

	uint16_t point_save_color;
	point_save_color = POINT_COLOR;
	BACK_COLOR = WHITE;
	for (i = 0, idx = print_idx; i < 10; ++i)
	{
		POINT_COLOR = line_color[idx];
		LCD_ShowString(40, 55 + i * 20, 300, 6, 16, (uint8_t *)(screen_msg[idx]));
		if (++idx == 10)
		{
			idx = 0;
		}
	}
	POINT_COLOR = point_save_color;
}

void screen_write(char *msg, uint16_t color)
{
	char *p = msg;
	unsigned len = strlen(msg);
	static char tmp[65];

	if (color == RED)
	{
		show_status(msg);
		return;
	}

	do
	{
		strncpy(screen_msg[write_idx], p, 20);
		line_color[write_idx++] = color;
		if (write_idx == 10)
		{
			write_idx = 0;
			is_full = 1;
		}
		p += 20;
	} while (p < msg + len);
	if (is_full)
	{
		print_idx = write_idx;
		if (print_idx == 10)
		{
			print_idx = 0;
		}
	}
	build_ui();
	screen_show();
}

void left_show(char *msg)
{
	static char tmp[35];
	char *p;
	unsigned int len = strlen(msg);
	p = msg;

	msg += len;
	while (p + 20 < msg)
	{
		p += 20;
	}
	msg -= len;
	strcpy(tmp, p);
	sprintf(p, "%20s", tmp);
	screen_write(msg, BLACK);
	strcpy(p, tmp);
}

int main(void)
{
	u8 key; //key是按键信息
	u16 t, i;

	HAL_Init();						//初始化HAL库
	Stm32_Clock_Init(RCC_PLL_MUL9); //设置时钟,72M
	delay_init(72);					//初始化延时函数
	uart_init(115200);				//初始化串口，波特率115200
	LED_Init();						//初始化LED:
									//(红灯闪烁：2.4G模组连接失败；绿灯闪烁：通讯连接正常；红灯常亮：通讯连接不正常）
	KEY_Init();						//初始化按键:
									//key0: 关闭连接；key1: 开启连接；
	LCD_Init();						//初始化LCD
	NRF24L01_Init();				//初始化NRF24L01

	POINT_COLOR = RED;
	BACK_COLOR = GRAY;

	build_ui();

	while (NRF24L01_Check()) //检查NRF24L01通讯模块是否连接正常
	{
		//若连接不正常：
		//在LCD上显示error
		screen_write("NRF24L01 Error", RED);

		delay_ms(200);
		//闪烁红灯(LED0)
		LED0 = !LED0;
	}
	//若通讯模块连接正常：
	//打印操作信息: KEY0:CONNECT; KEY1:DISCONNECT
	screen_write("KEY0:CONNECT KEY1:CANCEL", RED);
	//关掉红灯(LED0)
	LED1 = 1;

	//通讯部分:
	while (1)
	{
		//死循环，每一次循环时间为1ms (delay_ms(1))
		t++;
		delay_ms(1);

		//获取KEY输入，key等于按键状态，按下key0则key置1，按下key1则key置2
		key = KEY_Scan(0);

		if (key == KEY0_PRES)
		{
			if (status == 0)
			{
				// 开启状态
				status = 1;
				// 打印LCD成功
				screen_write("Connect Successful", RED);
			}
		}
		// 若key置2
		else if (key == KEY1_PRES)
		{
			if (status == 1)
			{
				// 关闭状态
				status = 0;

				// 设置红灯(LED0)长亮，绿灯(LED1)熄灭
				LED1 = 1;
				LED0 = 0;

				screen_write("Connect Has Closed", RED);
			}
		}

		if (status) //状态开启，则进入接受模组;状态关闭则跳过
		{
			//初始化RX模块接收设置
			NRF24L01_RX_Mode();
			if (NRF24L01_RxPacket(RX_buf) == 0) //一旦接收到信息,则进入:
			{

				RX_buf[40] = 0; //加入字符串结束符

				//判断是否是建立连接的通讯，若是则不发送到串口和LCD
				for (i = 0; i < 20; i++)
					if (RX_buf[i] != MESSAGE[i])
						break;
				if (i != 20)
				{
					printf("Receive: %s\n", RX_buf);
					screen_write(RX_buf, BLUE);
				}
			}
		}

		if (t % 100 == 0) //每0.1s(100个循环)进入一次串口模块
		{
			//接受串口消息，每0.1s接受一个字符
			HAL_UART_Receive_IT(&UART1_Handler, (uint8_t *)uart_buf, 1);

			if (status) //状态开启，则进入发送模组; 状态关闭则跳过
			{

				//初始化TX模块接收设置
				NRF24L01_TX_Mode();

				if (NRF24L01_TxPacket((uint8_t *)TX_buf) == TX_OK) //如果发送信息成功，则为建立通讯成功，则进入:
				{
					//设置红灯(LED0)熄灭，绿灯(LED1)闪烁
					LED0 = 1;
					LED1 = !LED1;

					//判断是否是建立连接的通讯，若是则不发送到串口和LCD
					for (i = 0; i < 20; i++)
						if (TX_buf[i] != MESSAGE[i])
							break;
					if (i == 20)
					{
						// 发送默认的message，显示在状态栏
						screen_write(TX_buf, RED);
					}
					else
					{
						//打印发送的消息在lcd
						left_show(TX_buf);
					}
				}
				else //如果发送信息失败，则为连接失败，则进入:
				{
					//设置红灯(LED0)长亮，绿灯(LED1)熄灭
					LED1 = 1;
					LED0 = 0;

					screen_write("404 NOT FOUND", RED);
				}

				if (strcmp(TX_buf, MESSAGE) != 0)
				{
					strcpy(TX_buf, MESSAGE);
				}
			}
		}
	}
}

void response(char *msg)
{
	static char tmp[35];
	char *p;
	unsigned int len = strlen(msg);
	p = msg;
	msg += len;
	while (p + 20 < msg)
	{
		p += 20;
	}
	msg -= len;
	if (strcmp(msg, "open") == 0)
	{
		//设置红灯(LED0)熄灭，绿灯(LED1)闪烁
		LED0 = 1;

		status = 1;
	}
	else if (strcmp(msg, "close") == 0)
	{
		status = 0;

		// 设置红灯(LED0)长亮，绿灯(LED1)熄灭
		LED1 = 1;
		LED0 = 0;

		screen_write("Cancel Connection", RED);
	}
	else
	{
		strcpy(TX_buf, msg);
	}
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART1)
	{
		static int recv_idx = 0;
		static char tem_buf[4096];
		if (uart_buf[0] == '\n')
		{
			tem_buf[recv_idx] = '\0';
			recv_idx = 0;
			printf("Send: %s\n", tem_buf);

			response(tem_buf);
		}
		else if (uart_buf[0] != '\r')
		{
			tem_buf[recv_idx++] = uart_buf[0];
		}
	}
}

void USART1_IRQHandler(void)
{
	/* USER CODE BEGIN USART1_IRQn 0 */

	/* USER CODE END USART1_IRQn 0 */
	HAL_UART_IRQHandler(&UART1_Handler);
	/* USER CODE BEGIN USART1_IRQn 1 */
	HAL_UART_Receive_IT(&UART1_Handler, (uint8_t *)uart_buf, 1);
}