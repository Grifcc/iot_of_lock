/**
 * 引脚定义
 * ----------------------------------
 *             MFRC522      Arduino
 * Signal      Reader       Uno
 * ----------------------------------
 * RST/Reset   RST          9
 * SPI SS      SDA(SS)      10
 * SPI MOSI    MOSI         11
 * SPI MISO    MISO         12
 * SPI SCK     SCK          13
 * beep        *            7-(+)&  GND
 * OFF_button  *            6 &  GND
 * Servo       *            8
 */
/**
*   180度舵机只须函数中传递相应角度
 *   360度舵机 参数为90时停（视不同情况而定），参数为0时最大速度顺时针旋转，参数为180时最大速度逆时针旋转
 *            当然可以自己写pwm输出
 *   此处舵机为MG966R 360度舵机
 */

#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>


#define RST_PIN         9
#define SS_PIN          10

Servo myservo;   //创建一个伺服电机对象
MFRC522 mfrc522(SS_PIN, RST_PIN);   // 创建一个mfrc522实例.

int debug=0;  //调试模式置1，实测置0
int flag=1;  //状态指示，1为开，0为关
int buttonState = 1; //存储按键状态值

//卡号
long unsigned CGC=0x00;
long unsigned XSR=0x00;
long unsigned ZZW=0x00;
long unsigned GTR=0x00;
long unsigned LWZ=0x00;
long unsigned RJY=0x00;
long unsigned YH =0X00;
long unsigned DZC=0x00;
long unsigned LYH=0x00;
long unsigned HZW=0x00;

void setup()
{
	myservo.attach(8);           //伺服电机信号输出口
	myservo.write(90);
	pinMode(7,OUTPUT);           //蜂鸣器接口
	pinMode(6,INPUT_PULLUP);     //关门开关接口(上拉模式）
	Serial.begin(115200);         // 初始化串口，波特率115200
	if(debug)
		{
			while (!Serial);     // 检测串口是否打开，否则死循环（测试使用，一般注释）
			Serial.println("120's sys start");
		}
	SPI.begin();                 // 初始化SPI
	mfrc522.PCD_Init();          // 初始化RC522阅读器
}
long unsigned  RFID_work()
{
	long unsigned UID;
	//  寻找卡
	if ( ! mfrc522.PICC_IsNewCardPresent())
		{
			UID=0X00;
			return UID ;
		}
	// 选择一张卡
	if (! mfrc522.PICC_ReadCardSerial())
		{

			UID=0X00;
			return  UID;
		}

	UID=dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);  //读出标签识别号
	if (debug)
		{
			Serial.print(F("Card UID:"));
			Serial.print(UID,HEX);
			Serial.print("\n");  //串口发送ID号
		}
	mfrc522.PICC_HaltA();  //RC522休眠
	return UID;
}
void loop()
{
	int reading=5;
/*******************************RFID功能实现区*************************/
	ID=RFID_work();
/**************************ESP8266功能实现区（串口）*************************/
	while (Serial.available() > 0)
		{
			reading=Serial.read();
			reading-=48;
			delay(2);  //延迟保证传入正常
			if(debug) Serial.print("reading:");
			if(debug) Serial.println(reading);
		}

	if((flag==0  &&  reading==1)||flag==0)
		{
			if(ID==CGC||ID==XSR||ID==GTR||ID==RJY||ID==LWZ||ID==ZZW||ID==YH||ID==LYH||ID==DZC||ID==HZW||reading==1)
				{
					flag=1;
					Serial.print(flag);
					Clockwise_rotation();
					ID=0x00;
				}

		}
	if((flag==1&&reading==0)||flag==1)
		{
			buttonState=digitalRead(6);
			if (buttonState ==LOW||reading==0)          
				{
					delay(80); //消抖
					if (buttonState ==LOW||reading==0)        
						{
							flag=0; 
							Serial.print(flag);
							Counterclockwise_rotation();
						}
				}
		}

}
//蜂鸣器提示
void Beep()
{
	tone(7,2000);      //蜂鸣器2000Hz
	delay(150);          //延时150ms 
	noTone(7);           //停止
}
//读卡号函数
long unsigned dump_byte_array(byte *buffer, byte bufferSize)
{
	long unsigned UID;
	for (byte i = 0; i < bufferSize; i++)
		{
			UID=UID*0x100+buffer[i];    //转化为一个数，由于是有8位的16进制数，所以使用无符号长整型
		}
	return  UID;
}
//锁
void Counterclockwise_rotation()      //逆时针旋转270度
{
	Beep();
	myservo.write(100);
	delay(1250);
	myservo.write(92);
}
//开
void Clockwise_rotation()          //顺时针旋转270度
{
	Beep();
	myservo.write(80);
	delay(1250);
	myservo.write(92);
}
