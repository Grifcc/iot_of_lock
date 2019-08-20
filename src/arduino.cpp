/**
 * ���Ŷ���
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
*   180�ȶ��ֻ�뺯���д�����Ӧ�Ƕ�
 *   360�ȶ�� ����Ϊ90ʱͣ���Ӳ�ͬ���������������Ϊ0ʱ����ٶ�˳ʱ����ת������Ϊ180ʱ����ٶ���ʱ����ת
 *            ��Ȼ�����Լ�дpwm���
 *   �˴����ΪMG966R 360�ȶ��
 */

#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>


#define RST_PIN         9
#define SS_PIN          10

Servo myservo;   //����һ���ŷ��������
MFRC522 mfrc522(SS_PIN, RST_PIN);   // ����һ��mfrc522ʵ��.

int debug=0;  //����ģʽ��1��ʵ����0
int flag=1;  //״ָ̬ʾ��1Ϊ����0Ϊ��
int buttonState = 1; //�洢����״ֵ̬

//����
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
	myservo.attach(8);           //�ŷ�����ź������
	myservo.write(90);
	pinMode(7,OUTPUT);           //�������ӿ�
	pinMode(6,INPUT_PULLUP);     //���ſ��ؽӿ�(����ģʽ��
	Serial.begin(115200);         // ��ʼ�����ڣ�������115200
	if(debug)
		{
			while (!Serial);     // ��⴮���Ƿ�򿪣�������ѭ��������ʹ�ã�һ��ע�ͣ�
			Serial.println("120's sys start");
		}
	SPI.begin();                 // ��ʼ��SPI
	mfrc522.PCD_Init();          // ��ʼ��RC522�Ķ���
}
long unsigned  RFID_work()
{
	long unsigned UID;
	//  Ѱ�ҿ�
	if ( ! mfrc522.PICC_IsNewCardPresent())
		{
			UID=0X00;
			return UID ;
		}
	// ѡ��һ�ſ�
	if (! mfrc522.PICC_ReadCardSerial())
		{

			UID=0X00;
			return  UID;
		}

	UID=dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);  //������ǩʶ���
	if (debug)
		{
			Serial.print(F("Card UID:"));
			Serial.print(UID,HEX);
			Serial.print("\n");  //���ڷ���ID��
		}
	mfrc522.PICC_HaltA();  //RC522����
	return UID;
}
void loop()
{
	int reading=5;
/*******************************RFID����ʵ����*************************/
	ID=RFID_work();
/**************************ESP8266����ʵ���������ڣ�*************************/
	while (Serial.available() > 0)
		{
			reading=Serial.read();
			reading-=48;
			delay(2);  //�ӳٱ�֤��������
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
					delay(80); //����
					if (buttonState ==LOW||reading==0)        
						{
							flag=0; 
							Serial.print(flag);
							Counterclockwise_rotation();
						}
				}
		}

}
//��������ʾ
void Beep()
{
	tone(7,2000);      //������2000Hz
	delay(150);          //��ʱ150ms 
	noTone(7);           //ֹͣ
}
//�����ź���
long unsigned dump_byte_array(byte *buffer, byte bufferSize)
{
	long unsigned UID;
	for (byte i = 0; i < bufferSize; i++)
		{
			UID=UID*0x100+buffer[i];    //ת��Ϊһ��������������8λ��16������������ʹ���޷��ų�����
		}
	return  UID;
}
//��
void Counterclockwise_rotation()      //��ʱ����ת270��
{
	Beep();
	myservo.write(100);
	delay(1250);
	myservo.write(92);
}
//��
void Clockwise_rotation()          //˳ʱ����ת270��
{
	Beep();
	myservo.write(80);
	delay(1250);
	myservo.write(92);
}
