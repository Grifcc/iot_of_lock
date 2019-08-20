#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <aliyun_mqtt.h>


/* 连接您的WIFI SSID和密码 */
#define WIFI_SSID     ""
#define WIFI_PASSWD   ""


/* 设备证书信息*/
#define PRODUCT_KEY          ""
#define DEVICE_NAME          ""
#define DEVICE_SECRET        ""

#define ALINK_BODY_FORMAT        "{\"id\":\"%u\",\"version\":\"1.0\",\"method\":\"%s\",\"params\":%s}"   //ALINKJSON 格式
#define ALINK_TOPIC_PROP_POST    "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/event/property/post"   //发布消息 topic
#define ALINK_TOPIC_PROP_SET     "/sys/" PRODUCT_KEY "/" DEVICE_NAME "/thing/service/property/set"  //订阅消息 topic
#define ALINK_METHOD_PROP_POST   "thing.event.property.post"

int debug=0; //调试模式

int LockState = 1;      // the current state of the output pin
int previous = 1;       // the previous reading from the input pin

// the follow variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0; // the last time the output pin was toggled
unsigned long debounce = 500; // the debounce time, increase if the output flickers

unsigned long lastMqttConnectMs = 0;

unsigned int postMsgId = 0;

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void initWifi(const char *ssid, const char *password)
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    if(debug)  Serial.println("WiFi does not connect, try again ...");
    delay(3000);
  }
  if(debug)
  {
    Serial.println("Wifi is connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
  
}

void mqttCheckConnect()
{
  bool connected = connectAliyunMQTT(mqttClient, PRODUCT_KEY, DEVICE_NAME, DEVICE_SECRET);
  if (connected) 
  {
    if(debug) Serial.println("MQTT connect succeed!");
    if (mqttClient.subscribe(ALINK_TOPIC_PROP_SET)) 
    {
       if(debug)  Serial.println("subscribe done.");
    } 
    else 
    {
       if(debug)  Serial.println("subscribe failed!");
    }
  }
}

void mqttPublish()
{
  char param[32];
  char jsonBuf[128];

  sprintf(param, "{\"LockState\":%d,\"LockSwitch\":%d}", LockState,LockState);
  postMsgId += 1;
  sprintf(jsonBuf, ALINK_BODY_FORMAT, postMsgId, ALINK_METHOD_PROP_POST, param);

  if (mqttClient.publish(ALINK_TOPIC_PROP_POST, jsonBuf)) 
  {
    if(debug)
    {
     Serial.print("Post message to cloud: ");
     Serial.println(jsonBuf);
    }
    
  } 
  else
  {
     if(debug) Serial.println("Publish message to cloud failed!");
  }
}

// https://pubsubclient.knolleary.net/api.html#callback

void callback(char* topic, byte* payload, unsigned int length)
{ 
  if (strstr(topic, ALINK_TOPIC_PROP_SET))       //strstr(str1,str2) 函数用于判断字符串str2是否是str1的子串。
  {
     if(debug) Serial.print("Set message arrived [");
     if(debug) Serial.print(topic);
     if(debug) Serial.print("] ");
     payload[length] = '\0';
     if(debug) Serial.println((char *)payload);

    // Deserialization break change from 5.x to 6.x of ArduinoJson
    DynamicJsonDocument doc(100);   //创建一个 100字节的doc 缓存区(JSON文档形式)
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
       if(debug) Serial.println("parse json failed");
      return;
    }

    // {"method":"thing.service.property.set","id":"282860794","params":{"LightSwitch":1},"version":"1.0.0"}
    JsonObject setAlinkMsgObj = doc.as<JsonObject>();
    // LockSwitch
    int desiredLockState = setAlinkMsgObj["params"]["LockSwitch"];
    if( previous!= desiredLockState)
    {
      if (desiredLockState == HIGH || desiredLockState == LOW) 
      {
        LockState = desiredLockState;
        const char* cmdStr = desiredLockState == HIGH ? "on" : "off";
         if(debug) Serial.print("Cloud command: Turn ");
         if(debug) Serial.print(cmdStr);
         if(debug) Serial.println(" the door.");
      }
    }
  }
}

// the setup routine runs once when you press reset:
void setup() {
  Serial.begin(115200);
  if(debug) Serial.println("Iot of 120‘s lock starts.");

  initWifi(WIFI_SSID, WIFI_PASSWD);
  mqttClient.setCallback(callback);

  lastMqttConnectMs = millis();
  mqttCheckConnect();
  mqttPublish();
}

// the loop routine runs over and over again forever:
void loop() 
{
  int flag=0;
  int reading = 0;   //接受消息  from arduino
  while (Serial.available() > 0) 
    {
        reading=Serial.read();
        reading-=48;  
        delay(2);  //延迟保证传入正常
        flag=1;
        if(debug) Serial.print("reading:");
        if(debug) Serial.println(reading);
    }
  
  if (millis() - lastMqttConnectMs >= 5000)
  {
    lastMqttConnectMs = millis();
    mqttCheckConnect();
  }

  // https://pubsubclient.knolleary.net/api.html#loop
  if (!mqttClient.loop()) 
  {
     if(debug) Serial.println("The MQTT client is disconnected!");
  }
  if (flag&millis() - lastDebounceTime > debounce) 
  {
    
      if (reading == 0) 
      {
        LockState = LOW;
         if(debug) Serial.println("Turn off door locally.");
      } 
      else 
      {
        LockState = HIGH;
        if(debug) Serial.println("Turn on door locally.");
      }  
    lastDebounceTime = millis();
    mqttPublish();
     if(debug)
     {
      Serial.print("Lockstate: ");
      Serial.print(LockState);
      Serial.print(", Time: ");
      Serial.println(lastDebounceTime);
     }
    
    flag=0;             //清除标志位
  }
  if(LockState!=previous)
  {
    mqttPublish();
    if(debug) Serial.print("cmd");
    Serial.print(LockState);  //发送数据 to arduino
    previous=LockState;       //改变原有状态
  }
}
