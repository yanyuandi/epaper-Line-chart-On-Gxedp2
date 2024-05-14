#include <Adafruit_GFX.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "";
const char* password = "";
const char* url = "http://10.205.12.34";

U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

// 选择正确的墨水屏型号
GxEPD2_BW<GxEPD2_290, GxEPD2_290::HEIGHT> display(GxEPD2_290(/*CS=D8*/ 5, /*DC=D3*/ 6, /*RST=D4*/ 7, /*BUSY=D2*/ 21));  // 2.9黑白   GDEH029A1   128x296, SSD1608 (IL3820)
void setup() {
  Serial.begin(115200);
  SPI.end();                // 释放标准SPI引脚，例如 SCK(18), MISO(19), MOSI(23), SS(5)
  SPI.begin(4, -1, 10, 5);  // 映射并初始化SPI引脚 SCK(13), MISO(12), MOSI(14), SS(15)
  display.init(115200, true, 2, false);
  display.setRotation(1);  // 0 是横向
  u8g2Fonts.begin(display);
  u8g2Fonts.setFontDirection(0);
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);  // 设置前景色
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);  // 设置背景色
  u8g2Fonts.setFontMode(1);
  u8g2Fonts.setFont(u8g2_font_wqy12_t_gb2312);
  pinMode(20, OUTPUT);
  digitalWrite(20, HIGH);

  WiFi.setTxPower(WIFI_POWER_2dBm);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  drawChart();
}

void loop() {
}


// 计算贝塞尔曲线上的点的函数
int calculateBezierPoint(int p0, int p1, int p2, int p3, float t) {
  float u = 1 - t;
  float tt = t * t;
  float uu = u * u;
  float uuu = uu * u;
  float ttt = tt * t;

  int point = uuu * p0;
  point += 3 * uu * t * p1;
  point += 3 * u * tt * p2;
  point += ttt * p3;

  return round(point);
}


void drawChart() {

  HTTPClient http;
  http.begin(url);

  int httpCode = http.GET();



  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, http.getString());

  if (error) {
    Serial.print("解析JSON时发生错误: ");
    Serial.println(error.c_str());
  }

  JsonArray data = doc["data"];
  int dataSize = data.size();

  String Days[dataSize];
  int temperatures[dataSize];

  for (int i = 0; i < dataSize; i++) {
    Days[i] = data[i]["fxDate"].as<String>();
    temperatures[i] = data[i]["tempMax"].as<int>();
  }

  for (int i = 0; i < dataSize; i++) {
    Serial.print("日期: ");
    Serial.println(Days[i]);
    Serial.print("最高温度: ");
    Serial.println(temperatures[i]);
  }

  http.end();

  display.fillScreen(GxEPD_WHITE);  // 清空屏幕
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  //int temperatures[] = { 5, 10, 20, 35, 10, 35, 10 };
  int numDays = 7;
  //int Days[] = { 14, 15, 16, 17, 18, 19, 20 };

  // 确定坐标轴范围
  int minTemp = 0;
  int maxTemp = 40;
  int minY = 20;                     // 留出顶部空间显示标题
  int maxY = display.height() - 30;  // 留出底部空间显示横坐标刻度

  // 绘制纵坐标刻度和显示数值
  for (int i = 0; i <= 8; i++) {
    int y = map(i * 5, minTemp, maxTemp, maxY, minY);
    display.drawFastHLine(20, y, 5, GxEPD_BLACK);  // 左边空出20像素
    u8g2Fonts.setCursor(5, y + 5);
    u8g2Fonts.print(i * 5);
  }

  // 绘制横坐标刻度
  for (int i = 0; i < numDays; i++) {
    int x = map(i, 0, numDays - 1, 25, display.width() - 21);  // 左右各空出20像素
    display.drawFastVLine(x, maxY + 1, 4, GxEPD_BLACK);

    u8g2Fonts.setCursor(x - 6, maxY + 15);
    u8g2Fonts.print(Days[i]);
  }

  /// 绘制折线和模拟贝塞尔曲线的过渡段
  for (int i = 0; i < numDays - 1; i++) {
    int x1 = map(i, 0, numDays - 1, 25, display.width() - 21);      // 左右各空出20像素
    int x2 = map(i + 1, 0, numDays - 1, 20, display.width() - 21);  // 左右各空出20像素
    int y1 = map(temperatures[i], minTemp, maxTemp, maxY, minY);
    int y2 = map(temperatures[i + 1], minTemp, maxTemp, maxY, minY);

    // 添加贝塞尔曲线的控制点
    int cx1 = x1 + (x2 - x1) / 3;
    int cy1 = y1 + 5;
    int cx2 = x1 + 2 * (x2 - x1) / 3;
    int cy2 = y2 + 5;

    // 绘制模拟贝塞尔曲线的过渡段
    int prevX, prevY;
    for (int j = 0; j <= 10; j++) {
      float t = j / 10.0;
      int bx = calculateBezierPoint(x1, cx1, cx2, x2, t);
      int by = calculateBezierPoint(y1, cy1, cy2, y2, t);
      display.drawPixel(bx, by, GxEPD_BLACK);
      if (j > 0) {
        display.drawLine(prevX, prevY, bx, by, GxEPD_BLACK);
      }
      //display.drawFastVLine(bx, by, 128 - by, GxEPD_BLACK);//填充竖线


      //display.fillCircle(x1, y1, 3, GxEPD_WHITE);//转折点白色圆模拟虚线

      u8g2Fonts.setCursor(x1 - 8, y1 -7);//绘制温度数值
      u8g2Fonts.print(temperatures[i]);

      prevX = bx;
      prevY = by;
    }
    //Serial.print("x1: ");
    //Serial.println(x1);
    //Serial.print("y1: ");
    //Serial.println(y1);
    //Serial.print("x2: ");
    //Serial.println(x2);
    //Serial.print("y2: ");
    //Serial.println(y2);
    display.fillCircle(x1, y1, 3, GxEPD_BLACK);  //转折点画圆
  }
  // 在纵坐标上绘制一根直线
  display.drawFastVLine(20, minY, maxY - minY + 5, GxEPD_BLACK);

  // 在横坐标上绘制一根直线
  display.drawFastHLine(20, maxY + 5, display.width() - 41, GxEPD_BLACK);

  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(10, 15);
  display.println("Temperature Chart");

  display.nextPage();  // 显示内容
}
