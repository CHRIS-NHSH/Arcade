/*
  M8 聖誕樹互動遊戲機（五合一雙人對戰）
  Arduino Game Lab — christmas-tree.ino

  硬體：17 顆 WS2812 可定址 LED（1 顆樹頂星星 + 16 顆裝飾）、
        雙人各 3 顆按鈕（紅／綠／藍）、蜂鳴器（選配）

  依需求函式庫：
    - Adafruit_NeoPixel
    - Bounce2

  詳細接線與玩法說明請見網站上的 M8 教學頁。
*/

#include <Adafruit_NeoPixel.h>
#include <Bounce2.h>

// ================= 硬體設定 =================
#define LED_PIN       6      // 燈條數據腳位
#define NUM_LEDS      17     // 總燈數 (1顆星星 + 16顆裝飾)
#define STAR_LED      0      // 星星是第0顆
#define BUZZER_PIN    12     // 蜂鳴器腳位 (選配)
#define BRIGHTNESS    50     // 亮度 (0-255)

// 按鈕腳位定義
#define P1_BTN_R_PIN  2
#define P1_BTN_G_PIN  3
#define P1_BTN_B_PIN  4
#define P2_BTN_R_PIN  8
#define P2_BTN_G_PIN  9
#define P2_BTN_B_PIN  10

// ================= 物件宣告 =================
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// 按鈕物件 (使用 Bounce2 處理防彈跳)
Bounce2::Button P1_R = Bounce2::Button();
Bounce2::Button P1_G = Bounce2::Button();
Bounce2::Button P1_B = Bounce2::Button();
Bounce2::Button P2_R = Bounce2::Button();
Bounce2::Button P2_G = Bounce2::Button();
Bounce2::Button P2_B = Bounce2::Button();

// ================= 全域變數與邏輯映射 =================

// 左右分區映射 (假設物理排列是隨機或之字形，這裡定義哪些ID屬於哪一邊)
// 請根據實際黏貼順序修改這裡的數字
const int P1_LEDS[] = {1, 3, 5, 7, 9, 11, 13, 15}; // 左側 8 顆
const int P2_LEDS[] = {2, 4, 6, 8, 10, 12, 14, 16}; // 右側 8 顆
const int LEDS_PER_SIDE = 8;

// 遊戲狀態機
enum GameState {
  MENU,
  GAME_1_RED,     // 打地鼠
  GAME_2_ORANGE,  // 拔河
  GAME_3_YELLOW,  // 記憶
  GAME_4_GREEN,   // 時機
  GAME_5_BLUE     // 混色
};

GameState currentState = MENU;
int selectedGameIndex = 0; // 0=Red, 1=Orange, 2=Yellow, 3=Green, 4=Blue
const int TOTAL_GAMES = 5;

// 顏色定義
uint32_t COL_RED, COL_GREEN, COL_BLUE, COL_ORANGE, COL_YELLOW, COL_PURPLE, COL_CYAN, COL_WHITE, COL_OFF;
uint32_t MENU_COLORS[5];

// 遊戲控制變數
bool gameInit = true; // 進入新遊戲時的初始化標記
unsigned long gameTimer = 0;
int p1Score = 0;
int p2Score = 0;
int targetVal = 0;    // 通用目標變數

// ================= 初始化 Setup =================
void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(0));

  // 初始化燈條
  strip.begin();
  strip.setBrightness(BRIGHTNESS);

  // 定義顏色
  COL_OFF = strip.Color(0, 0, 0);
  COL_RED = strip.Color(255, 0, 0);
  COL_GREEN = strip.Color(0, 255, 0);
  COL_BLUE = strip.Color(0, 0, 255);
  COL_ORANGE = strip.Color(255, 100, 0);
  COL_YELLOW = strip.Color(255, 255, 0);
  COL_PURPLE = strip.Color(255, 0, 255);
  COL_CYAN = strip.Color(0, 255, 255);
  COL_WHITE = strip.Color(255, 255, 255);

  MENU_COLORS[0] = COL_RED;
  MENU_COLORS[1] = COL_ORANGE;
  MENU_COLORS[2] = COL_YELLOW;
  MENU_COLORS[3] = COL_GREEN;
  MENU_COLORS[4] = COL_BLUE;

  // 初始化按鈕
  P1_R.attach(P1_BTN_R_PIN, INPUT_PULLUP); P1_R.interval(5);
  P1_G.attach(P1_BTN_G_PIN, INPUT_PULLUP); P1_G.interval(5);
  P1_B.attach(P1_BTN_B_PIN, INPUT_PULLUP); P1_B.interval(5);

  P2_R.attach(P2_BTN_R_PIN, INPUT_PULLUP); P2_R.interval(5);
  P2_G.attach(P2_BTN_G_PIN, INPUT_PULLUP); P2_G.interval(5);
  P2_B.attach(P2_BTN_B_PIN, INPUT_PULLUP); P2_B.interval(5);

  if(BUZZER_PIN > 0) pinMode(BUZZER_PIN, OUTPUT);

  introEffect(); // 開機彩虹特效
}

// ================= 主迴圈 Loop =================
void loop() {
  // 更新按鈕狀態
  P1_R.update(); P1_G.update(); P1_B.update();
  P2_R.update(); P2_G.update(); P2_B.update();

  switch (currentState) {
    case MENU:          runMenu(); break;
    case GAME_1_RED:    runGame1_WhackAMole(); break;
    case GAME_2_ORANGE: runGame2_TugOfWar(); break;
    case GAME_3_YELLOW: runGame3_Memory(); break;
    case GAME_4_GREEN:  runGame4_Timing(); break;
    case GAME_5_BLUE:   runGame5_ColorMix(); break;
  }
}

// ================= 選單邏輯 =================
void runMenu() {
  // P1 紅色 (UP/Next in menu context, User said "Previous" logic but mapped to button names)
  // 這裡依據直覺：紅=上一個顏色，綠=下一個顏色
  if (P1_R.pressed()) {
    selectedGameIndex--;
    if (selectedGameIndex < 0) selectedGameIndex = TOTAL_GAMES - 1;
    playTone(400, 50);
  }

  if (P1_G.pressed()) {
    selectedGameIndex++;
    if (selectedGameIndex >= TOTAL_GAMES) selectedGameIndex = 0;
    playTone(500, 50);
  }

  // 顯示選單顏色在星星
  strip.setPixelColor(STAR_LED, MENU_COLORS[selectedGameIndex]);

  // 讓樹身有微弱的呼吸燈效果
  int breath = (millis() / 20) % 100; // 簡單呼吸
  if (breath > 50) breath = 100 - breath;
  uint32_t dimColor = strip.Color(breath, breath, breath);
  for(int i=1; i<NUM_LEDS; i++) strip.setPixelColor(i, dimColor);
  strip.show();

  // P1 藍色 (確認)
  if (P1_B.pressed()) {
    playTone(800, 200);
    startEffect(MENU_COLORS[selectedGameIndex]);
    gameInit = true;

    // 切換狀態
    switch(selectedGameIndex) {
      case 0: currentState = GAME_1_RED; break;
      case 1: currentState = GAME_2_ORANGE; break;
      case 2: currentState = GAME_3_YELLOW; break;
      case 3: currentState = GAME_4_GREEN; break;
      case 4: currentState = GAME_5_BLUE; break;
    }
  }
}

// ================= Game 1: 搶救聖誕裝飾 (紅) =================
// 簡單版：星星隨機亮色，玩家搶按
int g1_targetColor = 0; // 0=Red, 1=Green
unsigned long g1_waitStart = 0;
bool g1_waiting = true;

void runGame1_WhackAMole() {
  if (gameInit) {
    p1Score = 0; p2Score = 0;
    strip.clear();
    gameInit = false;
    g1_waiting = true;
    g1_waitStart = millis();
  }

  // 等待隨機時間後出題
  if (g1_waiting) {
    if (millis() - g1_waitStart > random(1000, 3000)) {
      g1_targetColor = random(2); // 0或1
      // 亮起題目燈 (星星顯示題目顏色)
      uint32_t c = (g1_targetColor == 0) ? COL_RED : COL_GREEN;
      strip.setPixelColor(STAR_LED, c);
      strip.show();
      g1_waiting = false;
      playTone(1000, 50);
    }
  } else {
    // 監聽按鍵
    bool p1_hit = (g1_targetColor == 0 && P1_R.pressed()) || (g1_targetColor == 1 && P1_G.pressed());
    bool p2_hit = (g1_targetColor == 0 && P2_R.pressed()) || (g1_targetColor == 1 && P2_G.pressed());

    // 懲罰按錯 (暫略，先做搶答)

    if (p1_hit || p2_hit) {
      if (p1_hit) { p1Score++; winAnim(1); }
      else { p2Score++; winAnim(2); }

      // 更新分數顯示 (亮燈數)
      updateScoreboard();

      // 檢查勝利 (誰先滿8分)
      if (p1Score >= LEDS_PER_SIDE || p2Score >= LEDS_PER_SIDE) {
        gameOver(p1Score > p2Score ? 1 : 2);
      } else {
        // 下一回合
        strip.setPixelColor(STAR_LED, 0); strip.show();
        g1_waiting = true;
        g1_waitStart = millis();
      }
    }
  }
}

// ================= Game 2: 爬煙囪拔河 (橙) =================
// 連打按鈕，燈光往上爬
void runGame2_TugOfWar() {
  if (gameInit) {
    p1Score = 0; p2Score = 0;
    gameInit = false;
    strip.clear();
    strip.setPixelColor(STAR_LED, COL_ORANGE);
    strip.show();
  }

  // P1 連打 (任意鍵)
  if (P1_R.pressed() || P1_G.pressed() || P1_B.pressed()) {
    p1Score++;
    // 限制分數上限
    if (p1Score > LEDS_PER_SIDE) p1Score = LEDS_PER_SIDE;
  }

  // P2 連打
  if (P2_R.pressed() || P2_G.pressed() || P2_B.pressed()) {
    p2Score++;
    if (p2Score > LEDS_PER_SIDE) p2Score = LEDS_PER_SIDE;
  }

  // 即時顯示進度
  strip.clear();
  strip.setPixelColor(STAR_LED, COL_ORANGE);
  // 顯示 P1 進度 (假設 P1_LEDS 陣列是從下往上排)
  for(int i=0; i<p1Score; i++) strip.setPixelColor(P1_LEDS[i], COL_RED);
  // 顯示 P2 進度
  for(int i=0; i<p2Score; i++) strip.setPixelColor(P2_LEDS[i], COL_BLUE);
  strip.show();

  // 檢查勝利
  if (p1Score >= LEDS_PER_SIDE) gameOver(1);
  else if (p2Score >= LEDS_PER_SIDE) gameOver(2);
}

// ================= Game 3: 聖誕記憶 (黃) =================
// 簡化版：星星閃爍序列，玩家輸入。這裡實作 "單次輸入比拼"
int seq[5]; // 題目序列
int seqLen = 1;
int inputIdx = 0;
bool isShowing = true;
bool p1_turn = true; // 輪流還是同時？提示說同時，這裡簡化為 "同時看，各自輸入"

void runGame3_Memory() {
  static int currentStep = 0; // 0:出題中, 1:等待玩家輸入
  static bool p1_active = true; // 是否還存活
  static bool p2_active = true;
  static int p1_input_ptr = 0;
  static int p2_input_ptr = 0;

  if (gameInit) {
    p1Score = 0; p2Score = 0;
    seqLen = 1;
    currentStep = 0;
    gameInit = false;
    generateSequence();
  }

  if (currentStep == 0) {
    // 播放題目
    playSequence();
    p1_input_ptr = 0;
    p2_input_ptr = 0;
    p1_active = true;
    p2_active = true;
    currentStep = 1; // 轉為輸入模式
  }
  else if (currentStep == 1) {
    // 讀取 P1 輸入
    int p1_val = -1;
    if (P1_R.pressed()) p1_val = 0;
    if (P1_B.pressed()) p1_val = 1; // 題目只出紅(0)跟藍(1)

    if (p1_active && p1_val != -1) {
      if (p1_val == seq[p1_input_ptr]) {
        p1_input_ptr++;
        playTone(600, 50);
        if (p1_input_ptr >= seqLen) { /* P1 完成 */ }
      } else {
        p1_active = false; // P1 錯誤
        playTone(100, 200);
      }
    }

    // 讀取 P2 輸入 (同上)
    int p2_val = -1;
    if (P2_R.pressed()) p2_val = 0;
    if (P2_B.pressed()) p2_val = 1;

    if (p2_active && p2_val != -1) {
      if (p2_val == seq[p2_input_ptr]) {
        p2_input_ptr++;
        playTone(600, 50);
      } else {
        p2_active = false;
        playTone(100, 200);
      }
    }

    // 回合結算條件
    bool p1_done = (p1_input_ptr >= seqLen) || !p1_active;
    bool p2_done = (p2_input_ptr >= seqLen) || !p2_active;

    if (p1_done && p2_done) {
      if (p1_active) p1Score++;
      if (p2_active) p2Score++;
      updateScoreboard();
      delay(1000);

      if (p1Score >= LEDS_PER_SIDE || p2Score >= LEDS_PER_SIDE) {
        gameOver(p1Score > p2Score ? 1 : 2);
      } else {
        seqLen++; // 加難度
        if(seqLen > 5) seqLen = 5; // 上限
        generateSequence();
        currentStep = 0;
      }
    }
  }
}

void generateSequence() {
  for(int i=0; i<5; i++) seq[i] = random(2); // 0 or 1
}

void playSequence() {
  strip.clear(); updateScoreboard(); // 保持分數亮著
  delay(500);
  for(int i=0; i<seqLen; i++) {
    uint32_t c = (seq[i] == 0) ? COL_RED : COL_BLUE;
    strip.setPixelColor(STAR_LED, c);
    strip.show();
    playTone(seq[i]==0?400:800, 200);
    delay(300);
    strip.setPixelColor(STAR_LED, 0);
    strip.show();
    delay(200);
  }
}

// ================= Game 4: 點亮星星 (綠) =================
// 跑馬燈，抓時機
int runnerPos = 0;
int runnerDir = 1; // 1上, -1下
unsigned long lastMove = 0;
int speed = 100;
bool p1_turn_flag = true; // 輪流制

void runGame4_Timing() {
  if (gameInit) {
    p1Score = 0; p2Score = 0;
    runnerPos = 0;
    gameInit = false;
    p1_turn_flag = true;
  }

  // 跑馬燈移動
  if (millis() - lastMove > speed) {
    lastMove = millis();
    runnerPos += runnerDir;
    // 簡單模擬：只在燈條上跑 (1-16) + 星星(0)
    // 為了邏輯簡單，我們把星星當作 Index 17 (虛擬)，燈條 1-16
    // 實際上：
    // Path: 底部 -> 星星 -> 底部
    if (runnerPos > 10) runnerPos = 0; // 簡化：0-10循環跑，0是星星

    // 繪製
    strip.clear();
    updateScoreboard(); // 保留分數
    // 顯示跑馬燈游標 (用白色)
    if (runnerPos == 0) strip.setPixelColor(STAR_LED, COL_WHITE);
    else strip.setPixelColor(runnerPos, COL_WHITE); // 這裡需要一個更精確的路徑映射，暫時簡化
    strip.show();

    // 變速邏輯
    speed = random(50, 200);
  }

  // 偵測按鈕 (P1 Turn)
  if (p1_turn_flag && P1_R.pressed()) {
    if (runnerPos == 0) { // 命中星星
       winAnim(1); p1Score++;
    } else {
       playTone(100, 200); // 失敗音
    }
    p1_turn_flag = false; //換人
    updateScoreboard();
  }
  else if (!p1_turn_flag && P2_R.pressed()) {
    if (runnerPos == 0) {
       winAnim(2); p2Score++;
    } else {
       playTone(100, 200);
    }
    p1_turn_flag = true; //換人
    updateScoreboard();
  }

  if (p1Score >= LEDS_PER_SIDE || p2Score >= LEDS_PER_SIDE) gameOver(p1Score > p2Score ? 1 : 2);
}

// ================= Game 5: 聖誕色彩大師 (藍) =================
// 混色題
int targetMix = 0; // 0:Yellow(R+G), 1:Purple(R+B), 2:Cyan(G+B), 3:White(R+G+B)
unsigned long mixWaitStart = 0;

void runGame5_ColorMix() {
  if (gameInit) {
    p1Score = 0; p2Score = 0;
    gameInit = false;
    newMixQuestion();
  }

  // 檢查 P1 組合
  bool p1_r = !digitalRead(P1_BTN_R_PIN); // 直接讀電位，因為需要同時按
  bool p1_g = !digitalRead(P1_BTN_G_PIN);
  bool p1_b = !digitalRead(P1_BTN_B_PIN);
  int p1_mix = checkMix(p1_r, p1_g, p1_b);

  // 檢查 P2 組合
  bool p2_r = !digitalRead(P2_BTN_R_PIN);
  bool p2_g = !digitalRead(P2_BTN_G_PIN);
  bool p2_b = !digitalRead(P2_BTN_B_PIN);
  int p2_mix = checkMix(p2_r, p2_g, p2_b);

  if (p1_mix == targetMix) {
    p1Score++; winAnim(1); updateScoreboard(); newMixQuestion();
  }
  if (p2_mix == targetMix) {
    p2Score++; winAnim(2); updateScoreboard(); newMixQuestion();
  }

  if (p1Score >= LEDS_PER_SIDE || p2Score >= LEDS_PER_SIDE) gameOver(p1Score > p2Score ? 1 : 2);
}

void newMixQuestion() {
  targetMix = random(4);
  uint32_t c = COL_WHITE;
  if (targetMix == 0) c = COL_YELLOW;
  if (targetMix == 1) c = COL_PURPLE;
  if (targetMix == 2) c = COL_CYAN;
  if (targetMix == 3) c = COL_WHITE;

  strip.setPixelColor(STAR_LED, c);
  strip.show();
  delay(500); // 防連刷
}

int checkMix(bool r, bool g, bool b) {
  if (r && g && b) return 3; // White
  if (r && g) return 0; // Yellow
  if (r && b) return 1; // Purple
  if (g && b) return 2; // Cyan
  return -1;
}

// ================= 通用輔助函式 =================

// 更新計分板 (把兩側的燈亮起代表分數)
void updateScoreboard() {
  for(int i=0; i<LEDS_PER_SIDE; i++) {
    // P1 分數
    if (i < p1Score) strip.setPixelColor(P1_LEDS[i], COL_RED);
    else strip.setPixelColor(P1_LEDS[i], 0);

    // P2 分數
    if (i < p2Score) strip.setPixelColor(P2_LEDS[i], COL_BLUE);
    else strip.setPixelColor(P2_LEDS[i], 0);
  }
  strip.show();
}

void winAnim(int player) {
  playTone(1000, 100);
  uint32_t c = (player == 1) ? COL_RED : COL_BLUE;
  for(int k=0; k<3; k++) {
    strip.setPixelColor(STAR_LED, c);
    strip.show(); delay(100);
    strip.setPixelColor(STAR_LED, 0);
    strip.show(); delay(100);
  }
}

void gameOver(int winner) {
  strip.clear();
  uint32_t c = (winner == 1) ? COL_RED : COL_BLUE;

  for(int j=0; j<3; j++) {
    for(int i=0; i<NUM_LEDS; i++) {
      strip.setPixelColor(i, c);
      strip.show();
      delay(30);
    }
    delay(500);
    strip.clear(); strip.show();
  }

  currentState = MENU;
  gameInit = true;
}

void introEffect() {
  for(long firstPixelHue = 0; firstPixelHue < 5*65536; firstPixelHue += 256) {
    for(int i=0; i<strip.numPixels(); i++) {
      int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show();
    delay(2);
  }
  strip.clear(); strip.show();
}

void startEffect(uint32_t color) {
  for(int i=0; i<NUM_LEDS; i++) {
    strip.setPixelColor(i, color);
    strip.show(); delay(20);
  }
  delay(200);
  strip.clear(); strip.show();
}

void playTone(int freq, int duration) {
  if (BUZZER_PIN > 0) {
    tone(BUZZER_PIN, freq, duration);
  }
}
