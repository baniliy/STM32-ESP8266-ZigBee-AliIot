#include "mqtt.h"
#include "common.h"
#include "MQTTPacket.h"
#include "key.h"
#include "text.h"
#include "transport.h"
#include "cjson.h"


/*****************************************************************************
 *	用户配置  */

// 网络配置
//const char *WIFI_SSID = "Baniliy-AP";
//const char *WIFI_Password = "L3!8715c.";
const char *WIFI_SSID = "Baniliy- iPhone";
const char *WIFI_Password = "yu12345678..";
// 订阅和发布Topic 
#define Set_Topic "/sys/h3cyOqbsuEM/Test/thing/service/property/set"
#define Post_Topic "/sys/h3cyOqbsuEM/Test/thing/event/property/post"
// Payload配置
u16 TopicId = 0;		
char payload[100];
/*****************************************************************************
 *	串口3和串口2接收的数据
 */
char receive_data[100];
char receive_data1[100];
unsigned char buf[200];

/*****************************************************************************
 *	自定义结构体（终端信息）
 */
 /*
struct End
{
	u8 No;		// 终端编号
	u8 RAIN;	// 雨量信息
	u8 RELAY;	// 继电器状态
};*/
//struct End EndInfo[END_MAX_NUM];

u8 Auto_MODE = 0;	// 工作模式
u8 RAIN;			// 雨量信息
u8 RELAY1;			// 继电器状态
u8 RELAY2;			// 继电器状态

/*----------------------------------------------------------*/
/*函数名：连接目标MQTT服务器									*/
/*参  数：无                                                */
/*返回值：无                                                */
/*----------------------------------------------------------*/
void MQTT_Connect()
{
    int32_t len;
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;//配置部分可变头部的值
    
    int buflen = sizeof(buf);
	memset(buf,0,buflen); 
    data.clientID.cstring = "h3cyOqbsuEM.Test|securemode=2,signmethod=hmacsha256,timestamp=1664431040843|";//客户端标识，用于区分每个客户端xxx为自定义，后面为固定格式
    data.keepAliveInterval = 120;//保活计时器，定义了服务器收到客户端消息的最大时间间隔
    data.cleansession = 1;//该标志置1服务器必须丢弃之前保持的客户端的信息，将该连接视为“不存在”
    data.username.cstring = "Test&h3cyOqbsuEM";//用户名 DeviceName&ProductKey
    data.password.cstring = "774496b77794e18b66cd1902e4a2183ac4fc9b9b81e17dbd1814026b7953afe4";//密码
    len = MQTTSerialize_connect(buf, buflen, &data); /*1 构造连接报文*/
    transport_sendPacketBuffer(USART3,buf,len);
	printf("\r\nMQTT连接成功");
}

/*----------------------------------------------------------*/
/*函数名：SUBSCRIBE订阅topic报文								*/
/*参  数：temp_buff：保存报文数据的缓冲区					*/
/*参  数：QoS：订阅等级										*/
/*参  数：topic_name：订阅topic报文名称						*/
/*返回值：报文长度                                                                                                          */
/*----------------------------------------------------------*/
void MQTT_Subscribe(unsigned char *temp_buff, char *topic_name, int QoS)
{
	u8 Fixed_len = 2;                                 //SUBSCRIBE报文中，固定报头长度=2
	u8 Variable_len = 2;                              //SUBSCRIBE报文中，可变报头长度=2
	u8 Payload_len = 2 + strlen(topic_name) + 1;   //计算有效负荷长度 = 2字节(topic_name长度)+ topic_name字符串的长度 + 1字节服务等级

	temp_buff[0]=0x82;                                          //第1个字节 ：固定0x82
	temp_buff[1]=Variable_len + Payload_len;                    //第2个字节 ：可变报头+有效负荷的长度
	temp_buff[2]=0x00;                                          //第3个字节 ：报文标识符高字节，固定使用0x00
	temp_buff[3]=0x01;		                                    //第4个字节 ：报文标识符低字节，固定使用0x01
	temp_buff[4]=strlen(topic_name)/256;                     //第5个字节 ：topic_name长度高字节
	temp_buff[5]=strlen(topic_name)%256;		                //第6个字节 ：topic_name长度低字节
	memcpy(&temp_buff[6],topic_name,strlen(topic_name));  //第7个字节开始 ：复制过来topic_name字串
	temp_buff[6+strlen(topic_name)]=QoS;                     //最后1个字节：订阅等级
	
	transport_sendPacketBuffer(USART3,buf,Fixed_len + Variable_len + Payload_len);
	printf("\r\n订阅主题成功");
}

/*----------------------------------------------------------*/
/*函数名：发布消息                 			                */
/*参  数：pTopic：发布Topic									*/
/*参  数：pMessage：发布消息									*/
/*参  数：QoS：订阅等级			                            */
/*返回值：消息发布是否成功									*/
/*----------------------------------------------------------*/
//发布消息
int MQTT_Publish(char *pTopic,char *pMessage,int QoS)
{
    int32_t len;
    MQTTString topicString = MQTTString_initializer;
    int msglen = strlen(pMessage);//计算发布消息的长度
    int buflen = sizeof(buf);

    memset(buf,0,buflen);
    topicString.cstring = pTopic;//"设备topic列表中的第一条";
    len = MQTTSerialize_publish(buf, buflen, 0, QoS, 0, 0, topicString, (unsigned char*)pMessage, msglen); /*构造发布消息的报文*/

    transport_sendPacketBuffer(USART3,buf,len);//发送消息
	free(pMessage);
    return 0;
}

/*----------------------------------------------------------*/
/*函数名：PING报文，心跳包									*/
/*参  数：无												*/
/*返回值：无												*/
/*----------------------------------------------------------*/
void MQTT_PingREQ()
{
	while((USART3->SR&0x0080)==0) ;
	USART3->DR=0xC0;
	while((USART3->SR&0x0080)==0) ;
	USART3->DR=0x00;
}

/*----------------------------------------------------------*/
/*函数名：JSON创建		                					*/
/*参  数：无                                                */
/*返回值：无                                                */
/*----------------------------------------------------------*/
void Json_Create()
{   
	sprintf(payload,"{\"id\": %013d,\"params\":{\"RAIN\":%d,\"RELAY1\":%d,\"RELAY2\":%d,\"MODE\":%d},\"version\":\"1.0\",\"method\":\"thing.event.property.post\"}", 
		++TopicId,RAIN,RELAY1,RELAY2,Auto_MODE);		// 创建JSON字符串		
	printf("\r\n>>> BANILIY 发布 %s \r\n", payload);
	MQTT_Publish(Post_Topic,payload,1);					// 发布信息
}

/*----------------------------------------------------------*/
/*函数名：JSON解析，解析由协调器发送来数据					*/
/*参  数：无												*/
/*返回值：无												*/
/*----------------------------------------------------------*/
void Json_Parse_ZigBee(char *buffer)
{
	int i=0,j=0,k=0;
	sscanf( buffer, "{\"RAIN\":%d,\"RELAY1\":%d,\"RELAY2\":%d}", &i,&j,&k); // 解析JSON字符串
	RAIN = i;
	RELAY1 = j;
	RELAY2 = k;
	printf("\r\n>ZIGBEE  %s",buffer);
}

/*----------------------------------------------------------*/
/*函数名：JSON解析，解析由云端发送来数据					*/
/*参  数：无												*/
/*返回值：无												*/
/*----------------------------------------------------------*/
void Json_Parse_Aliyun()
{
	//判断接收到的数据有没有目标数据
	if(strstr((const char*)receive_data ,(const char*)"params")) 	
	{
		printf("\r\n >ALIYUN  %s",receive_data);
	}
	if(strstr((const char*)receive_data ,(const char*)"\"RELAY1\":1")) 	
	{
		u2_printf("21\n\r");	//发送命令
	}
	if(strstr((const char*)receive_data ,(const char*)"\"RELAY1\":0")) 	
	{
		u2_printf("20\n\r");	//发送命令
	}
	if(strstr((const char*)receive_data ,(const char*)"\"RELAY2\":1")) 	
	{
		u2_printf("31\n\r");	//发送命令
	}
	if(strstr((const char*)receive_data ,(const char*)"\"RELAY2\":0")) 	
	{
		u2_printf("30\n\r");	//发送命令
	}
	if(strstr((const char*)receive_data ,(const char*)"\"MODE\":1")) 	
	{
		Auto_MODE = 1;
	}
	if(strstr((const char*)receive_data ,(const char*)"\"MODE\":0")) 	
	{
		Auto_MODE = 0;
	}	
}

/*----------------------------------------------------------*/
/*函数名：初始化ESP8266 配置网络环境		                */
/*参  数：无												*/
/*返回值：无												*/
/*----------------------------------------------------------*/
void MQTT_Init()
{
	char Connect_CMD[50];
	while(atk_8266_send_cmd("ATE0","OK",20));				//关闭回显
	while(atk_8266_send_cmd("AT+CWMODE=1","OK",20));		//设为客户端模式
	printf("\r\n正在尝试连接WIFI  >>>%s<<< ",WIFI_SSID);
	sprintf(Connect_CMD,"AT+CWJAP=\"%s\",\"%s\"",WIFI_SSID,WIFI_Password);
	printf("\r\n %s <<< \r\n",Connect_CMD);
	while(atk_8266_send_cmd((u8 *)Connect_CMD,"OK",18));			//连接WIFI
	printf("\r\nWIFI连接成功");
	while(atk_8266_send_cmd("AT+CIPSTART=\"TCP\",\"iot-06z00i5jz65jb3w.mqtt.iothub.aliyuncs.com\",1883","OK",20));	// 连接服务器
	delay_ms (1000);
	while(atk_8266_send_cmd("AT+CIPMODE=1","OK",20));		//打开透传模式
	delay_ms (1000);
	while(atk_8266_send_cmd("AT+CIPSEND",">",20));			//开始传输数据
	printf("\r\n开始传输数据");
	delay_ms (1000);
	free(Connect_CMD);
}

/*----------------------------------------------------------*/
/*函数名：按键处理		                    				*/
/*参  数：无												*/
/*返回值：无												*/
/*----------------------------------------------------------*/
void MQTT_HandleKeys()
{
	u8 key;		// 响应的按键
	key=KEY_Scan(0); 
	if(key)
	{
		printf("\r\n检测到按键响应→%d						",key);
		switch(key)
		{
			case 1://KEY0
				u2_printf("21\n\r");	//发送命令
				printf("\r\n命令发送成功!\r\n");
				break;
			case 2://KEY1
				u2_printf("31\n\r");	//发送命令
				printf("\r\n命令发送成功!\r\n");
				break;
			case 4://WK_UP
				Json_Create();
				printf("\r\n 发布成功! ");
				break;
		}
	}
}

/*----------------------------------------------------------*/
/*函数名：当程序处于模式1时将自动根据雨量控制继电器状态		 */
/*参  数：无												*/
/*返回值：无												*/
/*----------------------------------------------------------*/
void RAIN_Auto_Process()
{
	if(Auto_MODE == 1)
	{
		if(RAIN == 1)
		{
			u2_printf("21\n");	//发送命令
			u2_printf("31\n");
			RELAY1 = 1;
			RELAY2 = 1;
		}
		else
		{
			u2_printf("20\n");	//发送命令
			u2_printf("30\n");
			RELAY1 = 0;
			RELAY2 = 0;
		}
	}
}

/*----------------------------------------------------------*/
/*函数名：服务器通信		                    			*/
/*参  数：无												*/
/*返回值：无												*/
/*----------------------------------------------------------*/
void MQTT_Process()
{
	u16 k = 0;
	MQTT_Init();						// 初始化MQTT
	MQTT_Connect();						// 连接目标MQTT服务器
	delay_ms(1000);
	MQTT_Subscribe(buf,Set_Topic,1);	// 订阅主题
	
	while(1)
	{
		LED0=0;
		LED1=0;
		delay_ms(10);							
		if(k%15){LED0 = 1;Json_Parse_Aliyun();}				// 解析云端数据
		if(USART2_RX_STA&0X8000)							// 串口2接收到数据
		{ 		
			USART2_RX_BUF[USART2_RX_STA&0X7FFF]=0;			// 添加结束符
			Json_Parse_ZigBee((char *)USART2_RX_BUF);		// 解析终端数据
			USART2_RX_STA=0;	
			if(k%10){LED1 = 1;}
		}
		RAIN_Auto_Process();						// 根据雨量自动决策
		MQTT_HandleKeys();							// 按键响应
		k++;  
		if(k%90==0){Json_Create();k=0;}				// 周期发布信息
	}
}

