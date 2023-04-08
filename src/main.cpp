/*
接线说明:无

程序说明:实现了客户端请求和风天气的JSON数据并实现解析
        程序功能:
        1. 向服务器端请求json数据信息
        2. 解析服务器端响应的json信息内容。
        3. 将解析后的数据信息显示于串口监视器
        4. 可以实现对不同的地区的天气进行请求

注意事项:1):这是心知天气的API调用文档: https://docs.seniverse.com/api/weather/now.html
        2):JSON数据解析网站:https://arduinojson.org/
        3):这是需要解析的JSON数据(示例):
          {
            "results": [
               {
                "location": {
                              "id": "C23NB62W20TF",
                              "timezone": "America/Los_Angeles",
                               "timezone_offset": "-07:00"
                            },
                "now":      {
                               "text": "多云",
                              "code": "4",
                              "temperature": "14"
                            },
                "last_update": "2015-09-25T22:45:00-07:00"
          }
                      ]
        }



函数示例:无

作者:灵首

时间:2023_4_6

*/
#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WebServer.h>

WiFiMulti wifi_multi; // 建立WiFiMulti 的对象,对象名称是 wifi_multi

#define LED_A 10
#define LED_B 11

static int ledState;
int BOOT_int;

// 定义的函数
void wifi_multi_con(void);
void wifi_multi_init(void);
void wifiClientRequest(const char *host, const int httpPort);
void parseData(WiFiClient client);

// 心知天气的相关设置
String reqUserkey = "SSD8O45y_m1XeM1SK"; // 设置自己的私钥
String reqUnit = "c";                    // 设置请求的单位
String reqLanguage = "en";               // 设置请求返回的语言

// 需要获取的数据,设置为全局变量
static String ID;
static String weather;
static int weatherCode;
static String updateDate;


/*
# brief 连接WiFi的函数
# param 无
# retval 无
*/
void wifi_multi_con(void)
{
  int i = 0;
  while (wifi_multi.run() != WL_CONNECTED)
  {
    delay(1000);
    i++;
    Serial.print(i);
  }
}

/*
# brief 写入自己要连接的WiFi名称及密码,之后会自动连接信号最强的WiFi
# param 无
# retval  无
*/
void wifi_multi_init(void)
{
  wifi_multi.addAP("LINGSOU1029", "12345678");
  wifi_multi.addAP("haoze1029", "12345678"); // 通过 wifi_multi.addAP() 添加了多个WiFi的信息,当连接时会在这些WiFi中自动搜索最强信号的WiFi连接
}


/*
# brief   建立HTTP的请求地址
# param   String reqLocation :这里需要填写一个地名(有多种写法,参考心知文档)
# retval    返回一个字符串(HTTP请求的资源地址)
*/
String reqRes(String reqLocation)
{

  String resquestResource = "/v3/weather/now.json?key=" +
                            reqUserkey + "&location=" +
                            reqLocation + "&language=" +
                            reqLanguage + "&unit=" +
                            reqUnit;

  Serial.print("request location is :  ");
  Serial.print(reqLocation);
  Serial.print("\n");

  return resquestResource;
}



/*
# brief 通过WiFiClient库向指定网址建立连接并发出信息
# param   const char* host:需要建立连接的网站的网址
# param   const int httpPort:对应的端口号
# param   String resource :需要请求的资源的地址(由另一个函数生成)
# retval  无,但是会通过串口打印一些内容
*/
void wifiClientRequest(const char *host, const int httpPort, String resource)
{
  WiFiClient client;

  // 格式很重要 String("GET ") 这个中有一个空格,应该是不能省的,省略会导致HTTP请求发送不出去,很关键的
  String httpRequest = String("GET ") + resource + " HTTP/1.1\r\n" +
                       "Host: " + host + "\r\n" +
                       "Connection: close\r\n\r\n";

  // 输出连接的网址
  Serial.print("connecting to :");
  Serial.print(host);
  Serial.print("\n");

  // 连接网络服务器
  if (client.connect(host, httpPort))
  {
    // 成功后输出success
    Serial.print("success\n");

    //连接成功后亮灯
    digitalWrite(LED_A,1);
    digitalWrite(LED_B,1);

    // 向服务器发送HTTP请求
    client.print(httpRequest);

    // 串口输出HTTP请求信息
    Serial.print("sending request:");
    Serial.print(httpRequest);
    Serial.print("\n");

    // 获取并显示服务器响应状态行
    // 只能用单引号
    String status_response = client.readStringUntil('\n');
    Serial.print("status_response is :");
    Serial.print(status_response);
    Serial.print("\n");

    // 跳过响应头获取响应体,服务器响应的数据就在这
    if (client.find("\r\n\r\n"))
    {
      Serial.print("Found Header End. Start Parsing.\n");
    }

    // 解析JSON数据
    parseData(client);
  }
  else
  {
    Serial.print("connect failed!!!\n");
  }

  // 结束连接
  client.stop();

  //结束连接后灭灯
  digitalWrite(LED_A,0);
  digitalWrite(LED_B,0);
}



/*
# brief   解析http请求获取的JSON数据
# param   WiFiClient client,建立一个WiFiClien对象
# retval  无
*/
void parseData(WiFiClient client)
{
  // 建立动态内存实现解析数据
  // 对接受的JSON数据进行对应的反序列化,并将输出存放在doc中待使用
  const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(6) + 230;
  DynamicJsonDocument doc(capacity);

  deserializeJson(doc, client);
  
  // 解析JSON数据
  JsonObject results_0 = doc["results"][0];

  JsonObject results_0_location = results_0["location"];
  const char *results_0_location_id = results_0_location["id"];
  const char *results_0_location_timezone = results_0_location["timezone"];
  const char *results_0_location_timezone_offset = results_0_location["timezone_offset"];

  String ID = results_0_location["id"].as<String>();

  //串口输出
  Serial.print("\n");
  Serial.print("===============return information begin:=============\n");
  Serial.print("id is :");
  Serial.print(ID);
  Serial.print("\n");

  JsonObject results_0_now = results_0["now"];
  const char *results_0_now_text = results_0_now["text"];               // "多云"
  const char *results_0_now_code = results_0_now["code"];               // "4"
  const char *results_0_now_temperature = results_0_now["temperature"]; // "14"

  String weather = results_0_now["text"].as<String>();
  int weatherCode = results_0_now["code"].as<int>();

  //串口输出
  Serial.print("weather is :");
  Serial.print(weather);
  Serial.print("\n");  
  Serial.print("weatherCode is :");
  Serial.print(weatherCode);
  Serial.print("\n");


  const char *results_0_last_update = results_0["last_update"]; // "2015-09-25T22:45:00-07:00"

  String updateDate = results_0["last_update"].as<String>();

  //串口输出
  Serial.print("updateDate is :");
  Serial.print(updateDate);
  Serial.print("\n");
  Serial.print("===============return information over!!!=============\n\n");
}



void setup()
{
  // 连接串口
  Serial.begin(9600);
  Serial.print("serial is OK\n");

  // 设置按键引脚,这是输出模式
  pinMode(LED_A, OUTPUT);
  pinMode(LED_B, OUTPUT);
  digitalWrite(LED_A, 0);
  digitalWrite(LED_B, 0);

  // wifi 连接设置
  wifi_multi_init();
  wifi_multi_con();
  Serial.print("wifi connected!!!\n");

  // 输出连接信息(连接的WIFI名称及开发板的IP地址)
  Serial.print("\nconnect wifi:");
  Serial.print(WiFi.SSID());
  Serial.print("\n");
  Serial.print("\nIP address:");
  Serial.print(WiFi.localIP());
  Serial.print("\n");
}



void loop()
{
  //发送请求并解析数据,在通过串口显示输出
  String resource_0 = reqRes("chongqing");
  wifiClientRequest("api.seniverse.com", 80,resource_0);
  delay(2500);

  //可以通过修改 reqRes("chengdu") 函数的参数实现对不同地区的访问
  String resource_1 = reqRes("chengdu");
  wifiClientRequest("api.seniverse.com", 80,resource_1);
  delay(2500);
}