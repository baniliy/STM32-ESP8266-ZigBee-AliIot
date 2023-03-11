#include "sys.h"
#include "delay.h"
#include "usart.h" 
#include "led.h" 		 	 
#include "lcd.h"  
#include "key.h"     
#include "usmart.h" 
#include "malloc.h"
#include "sdio_sdcard.h"  
#include "w25qxx.h"    
#include "ff.h"  
#include "exfuns.h"   
#include "text.h"	
#include "touch.h"	
#include "usart2.h" 
#include "usart3.h"
#include "common.h" 
#include "MQTTPacket.h"
#include "mqtt.h"
#include "cjson.h"


/************************************************

 物联网综合实训
 AUTHOR: BANILIY			CREATE DATE：2022-09-26
 
 开发板：					ALIENTEK STM32F103 
 WIFI：						ESP8266 
 终端协调器：				ZigBee 
 物联网云平台：				Ali-IOT
 开发文档：https://www.yuque.com/docs/share/3488aa6b-e126-4e8a-a9c6-a47a6ebee61b?# 《物联网综合项目实训》


************************************************/


 int main(void)
 {	 
	delay_init();	    	 //延时函数初始化	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	uart_init(115200);	 	//串口初始化为115200	
 	LED_Init();		  			//初始化与LED连接的硬件接口
	KEY_Init();					//初始化按键
	usart2_init(115200);		//初始化串口2
	usart3_init(115200);		//初始化串口3 
	
	printf("\r\n---------------------------------------------------\r\n");
	printf("\r\n>>>    BANILIY  物联网综合项目实训    <<<\r\n");
	printf("\r\n---------------------------------------------------\r\n");
	printf("\r\n连接模块...\r\n"); 
	 while( atk_8266_send_cmd("AT","OK",20))			//检查WIFI模块是否在线
	{
		atk_8266_send_cmd("AT+RST","OK",20);		//重启模块 
		delay_ms(1000);								//延时2S等待重启成功
		delay_ms(1000);
		delay_ms(1000);
		delay_ms(1000);
		delay_ms(1000);
		atk_8266_quit_trans();						//退出透传
		atk_8266_send_cmd("AT+CIPMODE=0","OK",200);	//关闭透传模式	
		printf("\r\n未检测到模块!!!\r\n"); 
		delay_ms(800);
		Show_Str(40,55,200,16,"尝试连接模块...",16,0);
		printf("\r\n尝试连接模块...\r\n"); 
	} 
	// 进入主进程
	while(1)
	{
		MQTT_Process();
	}
	
}


















