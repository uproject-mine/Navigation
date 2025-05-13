#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <ui.h>
#include <sdkconfig.h>
#include <ChronosESP32.h>

#define SW_WIDTH 320
#define SW_HEIGHT 172
#define DEBOUNCE_DELAY_MS 50    // Debounce delay in milliseconds
#define LONG_PRESS_TIME_MS 1000 // Long press threshold in milliseconds
// A structure to store key state, including timers and press duration
typedef struct
{
  uint32_t last_pressed_time;
  uint32_t press_duration;
  bool is_debounced;
  bool was_pressed;
} key_state_t;

enum
{
  SCREENBUFFER_SIZE_PIXELS = TFT_WIDTH * TFT_HEIGHT / 5
};

TFT_eSPI glob_hw_disp = TFT_eSPI(TFT_WIDTH, TFT_HEIGHT); /* TFT instance HW Screen*/
static lv_disp_t *glob_sw_disp;

// put function declarations here:
void task_mainGUI(void *par);
/* Display flushing */
void sup_frontend_my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *pixelmap);
/*Set tick routine needed for LVGL internal timings*/

// Input device
static uint32_t sup_frontend_my_tick_get_cb(void);
void sup_frontend_my_button_cb(lv_indev_t *indev_driver, lv_indev_data_t *data);
lv_key_t sup_backend_my_button_read();

// Chronos;
ChronosESP32 watch("uProject Tiny Nav");

bool change = false;

void connectionCallback(bool state)
{
  Serial.print("Connection state: ");
  Serial.println(state ? "Connected" : "Disconnected");
}

void notificationCallback(Notification notification)
{
  Serial.print("Notification received at ");
  Serial.println(notification.time);
  Serial.print("From: ");
  Serial.print(notification.app);
  Serial.print("\tIcon: ");
  Serial.println(notification.icon);
  Serial.println(notification.title);
  Serial.println(notification.message);
}

void configCallback(Config config, uint32_t a, uint32_t b)
{
  switch (config)
  {
  case CF_NAV_DATA:
    Serial.print("Navigation state: ");
    Serial.println(a ? "Active" : "Inactive");
    change = true;
    break;
  case CF_NAV_ICON:
    Serial.print("Navigation Icon data, position: ");
    Serial.println(a);
    Serial.print("Icon CRC: ");
    Serial.printf("0x%04X\n", b);
    Serial.printf("So icon: %d\n", b);
    break;
  }
}
String directions = "";
String distance = "";
String eta="";
String duration="";
String title="";
boolean map_act = false;

void update_map(lv_timer_t *timer){
  if (map_act)
  { 
    if(!change){ //Change = false when map_acting = true mean it updated
      lv_label_set_text(ui_Screen2LabelLabel10, directions.c_str());
      lv_label_set_text(ui_Screen2LabelLabel9, title.c_str());
      lv_label_set_text(ui_Screen2LabelLabel12, distance.c_str());
    }
    if (lv_screen_active() != ui_ScreenScreen2)
    {
      _ui_screen_change(&ui_ScreenScreen2, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, &ui_ScreenScreen2_screen_init);
    }
  }
  else
  {
    if (lv_screen_active() == ui_ScreenScreen2)
    {

      _ui_screen_change(&ui_ScreenScreen1, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, &ui_ScreenScreen2_screen_init);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  lv_i18n_init(lv_i18n_language_pack);
  lv_i18n_set_locale("en-GB");

  pinMode(HW_KEYSET, INPUT_PULLUP);
  pinMode(HW_KEYLEFT, INPUT_PULLUP);
  pinMode(HW_KEYMID, INPUT_PULLUP);
  pinMode(HW_KEYRIGHT, INPUT_PULLUP);
  pinMode(HW_KEYUP, INPUT_PULLUP);
  pinMode(HW_KEYDWN, INPUT_PULLUP);

  lv_init();
  glob_hw_disp.begin();                                  /* TFT init */
  glob_hw_disp.setRotation(1);                           /* Landscape orientation, flipped */
  glob_sw_disp = lv_display_create(SW_WIDTH, SW_HEIGHT); /* SW Display init */
  lv_color_t *buf1 = (lv_color_t *)heap_caps_malloc(SCREENBUFFER_SIZE_PIXELS * sizeof(lv_color_t), MALLOC_CAP_DMA);
  // lv_color_t *buf2 = (lv_color_t *)heap_caps_malloc(SCREENBUFFER_SIZE_PIXELS * sizeof(lv_color_t), MALLOC_CAP_DMA);
  lv_display_set_buffers(glob_sw_disp, buf1, NULL, SCREENBUFFER_SIZE_PIXELS * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(glob_sw_disp, sup_frontend_my_disp_flush);
  lv_tick_set_cb(sup_frontend_my_tick_get_cb);
  static lv_indev_t *indev;
  indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
  lv_indev_set_read_cb(indev, sup_frontend_my_button_cb);
  master_group = lv_group_create();
  lv_indev_set_group(indev, master_group);
  ui_init();
  static StackType_t guiTaskStack[1024 * 8];
  static StaticTask_t guiTaskBuffer;
  lv_timer_t *timer = lv_timer_create(update_map,500,NULL);
  xTaskCreateStaticPinnedToCore(
      task_mainGUI,
      "GUI Task",
      1024 * 8,
      NULL,
      1,
      guiTaskStack,
      &guiTaskBuffer,
      1);

  // Chronos setup
  //  set the callbacks before calling begin funtion
  watch.setConnectionCallback(connectionCallback);
  watch.setNotificationCallback(notificationCallback);
  watch.setConfigurationCallback(configCallback);

  watch.begin(); // initializes the BLE
  // make sure the ESP32 is not paired with your phone in the bluetooth settings
  // go to Chronos app > Watches tab > Watches button > Pair New Devices > Search > Select your board
  // you only need to do it once. To disconnect, click on the rotating icon (Top Right)

  Serial.println(watch.getAddress()); // mac address, call after begin()

  watch.setBattery(80); // set the battery level, will be synced to the app
}

void loop()
{
  watch.loop();
  if (change)
  {
    change = false;
    Navigation nav = watch.getNavigation();
    map_act = nav.active;
    if (map_act)
    {
      directions = nav.directions;
      distance = nav.distance;
      title = nav.title;
      //          lv_label_set_text();
      //          lv_label_set_text();
      //          lv_label_set_text();
      //Serial.println(nav.directions);
      //Serial.println(nav.eta);
      //Serial.println(nav.duration);
      //Serial.println(nav.distance);
      //Serial.println(nav.title);
    }
  }
  // put your main code here, to run repeatedly:
}

void task_mainGUI(void *par)
{
  while (true)
  {
    lv_timer_handler();
    vTaskDelay(5 / portTICK_PERIOD_MS);
    // createTask(&staticTempTask, NULL);
  }
}

void createTask(void (*taskFunc)(void *), TaskHandle_t *handler)
{
  xTaskCreatePinnedToCore(taskFunc, "HeaterTask", 1024, &handler, 1, NULL, 1);
}

void staticTempTask(void *par)
{
  while (true)
  {
    /* code */
    yield();
  }
}

void fixedScenariosTempTask(void *par)
{
  while (true)
  {
    /* code */
    yield();
  }
}

void customizeScenariosTempTask(void *par)
{
  while (true)
  {
    /* code */
    yield();
  }
}

/* Display flushing */
void sup_frontend_my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *pixelmap)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  if (LV_COLOR_16_SWAP)
  {
    size_t len = lv_area_get_size(area);
    lv_draw_sw_rgb565_swap(pixelmap, len);
  }
  glob_hw_disp.startWrite();
  glob_hw_disp.setAddrWindow(area->x1, area->y1, w, h);
  glob_hw_disp.pushColors((uint16_t *)pixelmap, w * h, true);
  glob_hw_disp.endWrite();
  lv_display_flush_ready(disp);
}

/*Set tick routine needed for LVGL internal timings*/
static uint32_t sup_frontend_my_tick_get_cb(void)
{
  return millis();
  // lv_tick_inc(1);
}

lv_key_t sup_backend_my_button_read()
{
  if (digitalRead(HW_KEYMID) == LOW)
  {
    return LV_KEY_ENTER; // Pressed
  }
  else if (digitalRead(HW_KEYSET) == LOW)
  {
    return LV_KEY_ESC;
  }
  else if (digitalRead(HW_KEYLEFT) == LOW)
  {
    return LV_KEY_LEFT;
  }
  else if (digitalRead(HW_KEYRIGHT) == LOW)
  {
    return LV_KEY_RIGHT;
  }
  else if (digitalRead(HW_KEYUP) == LOW)
  {
    return LV_KEY_UP;
  }
  else if (digitalRead(HW_KEYDWN) == LOW)
  {
    return LV_KEY_DOWN;
  }
  else
  {
    return LV_KEY_NONE;
  }
}

key_state_t key_states;

void sup_frontend_my_button_cb(lv_indev_t *indev_driver, lv_indev_data_t *data)
{
  static uint32_t last_time = 0;         // Track last update time
  uint32_t current_time = lv_tick_get(); // Get current time (in milliseconds)
  lv_key_t btn_pressed = sup_backend_my_button_read();
  key_state_t *key = &key_states;
  if (btn_pressed)
  {
    // If the key is pressed, start the debounce timer if it's not already
    if (!key->was_pressed)
    {
      key->last_pressed_time = current_time;
    }
    key->was_pressed = true;
  }
  else
  {
    // If the key is released, clear debounce state
    key->was_pressed = false;
  }
  // Debouncing logic: only allow the key press if the debounce delay has passed
  if (key->was_pressed && (current_time - key->last_pressed_time) > DEBOUNCE_DELAY_MS)
  {
    key->is_debounced = true;
  }
  else
  {
    key->is_debounced = false;
  }

  // Long press logic: track if the key is held for long enough
  if (key->was_pressed && key->is_debounced)
  {
    key->press_duration = current_time - key->last_pressed_time;
    if (key->press_duration > LONG_PRESS_TIME_MS)
    {
      // Long press detected
      data->key = btn_pressed;              // You can assign an actual key here
      data->state = LV_INDEV_STATE_PRESSED; // Or set it to RELEASED when released
      return;                               // We don't need to process more keys if we found a long press
    }
  }

  // If the key is debounced and pressed, register the key press
  if (key->is_debounced)
  {
    data->key = btn_pressed; // Map your keypad key to LVGL key
    data->state = LV_INDEV_STATE_PRESSED;
    return; // Stop after registering the first key press
  }

  // If no key is pressed, mark as released
  data->state = LV_INDEV_STATE_RELEASED;
  return;
  // if (btn_pressed != LV_KEY_NONE)
  // {
  //   data->key = btn_pressed;
  //   data->state = LV_INDEV_STATE_PRESSED;
  // }
  // else
  // {
  //   data->state = LV_INDEV_STATE_RELEASED;
  // }
}