#include <Adafruit_GFX.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <Fonts/FreeMonoBold9pt7b.h>

U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

// 选择正确的墨水屏型号
GxEPD2_BW<GxEPD2_290, GxEPD2_290::HEIGHT> display(GxEPD2_290(/*CS=D8*/ 5, /*DC=D3*/ 6, /*RST=D4*/ 7, /*BUSY=D2*/ 21));  // 2.9黑白   GDEH029A1   128x296, SSD1608 (IL3820)
void setup() {

  SPI.end();                // release standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  SPI.begin(4, -1, 10, 5);  // map and init SPI pins SCK(13), MISO(12), MOSI(14), SS(15)

  pinMode(20, OUTPUT);
  digitalWrite(20, HIGH);

  Serial.begin(115200);
  delay(10);
  display.init(115200, true, 2, false);
  display.setRotation(1);  // 0 是横向
  u8g2Fonts.begin(display);
  u8g2Fonts.setFontDirection(0);
  u8g2Fonts.setForegroundColor(GxEPD_BLACK);  // 设置前景色
  u8g2Fonts.setBackgroundColor(GxEPD_WHITE);  // 设置背景色
  u8g2Fonts.setFontMode(1);
  u8g2Fonts.setFont(u8g2_font_wqy12_t_gb2312);
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
  display.fillScreen(GxEPD_WHITE);  // 清空屏幕
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  int temperatures[] = { 5, 15, 20, 35, 10, 35 };
  int numDays = sizeof(temperatures) / sizeof(temperatures[0]);

  // 确定坐标轴范围
  int minTemp = 0;
  int maxTemp = 40;
  int minY = 20;                     // 留出顶部空间显示标题
  int maxY = display.height() - 10;  // 留出底部空间显示横坐标刻度

  // 绘制纵坐标刻度和显示数值
  for (int i = 0; i <= 8; i++) {
    int y = map(i * 5, minTemp, maxTemp, maxY, minY);
    display.drawFastHLine(0, y, 5, GxEPD_BLACK);
    display.setCursor(5, y - 6);
    //display.print(i * 5);
  }

  // 绘制横坐标刻度
  for (int i = 0; i < numDays; i++) {
    int x = map(i, 0, numDays - 1, 0, display.width() - 1);
    display.drawFastVLine(x, maxY, 5, GxEPD_BLACK);
    if (x < display.width() - 10) {  // 只在屏幕内部显示横坐标数值
      display.setCursor(x - 6, maxY + 15);
      // display.print(i + 1);
    }
  }

  /// 绘制折线和模拟贝塞尔曲线的过渡段
  for (int i = 0; i < numDays - 1; i++) {
    int x1 = map(i, 0, numDays - 1, 0, display.width() - 1);
    int x2 = map(i + 1, 0, numDays - 1, 0, display.width() - 1);
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
      //display.drawPixel(bx, by, GxEPD_BLACK);
      if (j > 0) {
        display.drawLine(prevX, prevY, bx, by, GxEPD_BLACK);
      }
      prevX = bx;
      prevY = by;
    }
   
  }


  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(10, 15);
  display.println("Temperature Chart");

  display.nextPage();  // 显示内容
}