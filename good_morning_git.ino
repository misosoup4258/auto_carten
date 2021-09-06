#include "M5Atom.h"
#include <time.h>
#include <WiFi.h>

//wifi
const char* ssid       = "ssid";
const char* password   = "password";

//time
const char* ntpServer = "ntp.nict.jp";
const long  gmtOffset_sec = 3600 * 9;
const int   daylightOffset_sec = 0;

//hour,minute,second
int h, m, s;
int morning, evening, morning_min, evening_min;

//srv
const uint8_t srv = 33;
const uint8_t srv_CH1 = 1; //srv_channel
const double PWM_Hz = 50;   //srv_PWM_frequency
const uint8_t PWM_level = 16; //PWM 16bit(0～65535)

//sleep_count
RTC_DATA_ATTR int count;
RTC_DATA_ATTR int remaining_count;
RTC_DATA_ATTR int mode_switch;
//-----------------------getting time through the internet------------------------//

void GetTime()
{
  struct tm timeinfo;
  setCpuFrequencyMhz(80);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(50);
  }

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  getLocalTime(&timeinfo);
  h = timeinfo.tm_hour;
  m = timeinfo.tm_min;
  s = timeinfo.tm_sec;

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  setCpuFrequencyMhz(10);
}

//-----------------------time setting function------------------------//

void set_time() {
  int time_m, time_e, time_now;

  morning = 11;
  morning_min = 49;
  evening = 11;
  evening_min = 54;

  time_m = (morning * 60) + morning_min;
  time_e = (evening * 60) + evening_min;
  time_now = (h * 60) + m;

  if (time_m >= time_now) {
    if (time_e > time_m) {
      mode_switch = 1;
    } else if (time_e < time_m && time_e >= time_now ) {
      mode_switch = 2;
    } else if (time_e < time_m && time_e < time_now) {
      mode_switch = 1;
    } else {
      mode_switch = 0;
    }
  } else if (time_e >= time_now) {
    if (time_e != time_m) {
      mode_switch = 2;
    } else {
      mode_switch = 0;
    }
  } else {
    mode_switch = 0;
  }
}


//-----------------------sleep function for mobile batteries------------------------//

void set_sleep(int time) {

  if (time == 9999) {
    //time_count_mode
    if (count != 0) {
      setCpuFrequencyMhz(80);
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(50);
      }
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      setCpuFrequencyMhz(10);
      count--;
      esp_sleep_enable_timer_wakeup(50 * 1000000);
      esp_deep_sleep_start();
    } else if (remaining_count != 0) {
      esp_sleep_enable_timer_wakeup(remaining_count * 1000000);
      setCpuFrequencyMhz(80);
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(50);
      }
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      setCpuFrequencyMhz(10);
      remaining_count = 0;
      esp_deep_sleep_start();
    }

  } else {
    //time_set_mode
    count = time / 50;
    remaining_count = time - (50 * count);
  }
}

//----------------------- srv function ------------------------//

void goodmorning() {
  int i;
  for (i = 0; i <= 36; i++) {
    M5.update();
    if (M5.Btn.wasPressed()) {
      set_sleep(60);
    } else {
      ledcWrite(srv_CH1, 4600);
      delay(1000);
    }
  }
  ledcWrite(srv_CH1, 4800);//4800:stop
  delay(1000);
}

void goodevening() {
  int i;
  for (i = 0; i <= 26; i++) {
    M5.update();
    if (M5.Btn.wasPressed()) {
      set_sleep(60);
    } else {
      ledcWrite(srv_CH1, 4950);
      delay(1000);
    }
  }
  ledcWrite(srv_CH1, 4850);//4800:stop
  delay(1000);

}

//----------------------- sleep timer function ------------------------//

void sleep_timer() {

  int aft;

  //ーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーー good_morning_action
  if (h == morning && mode_switch == 1) {
    //ーーーーーーーーー wait_up_to_30_min
    if (m >= 0 && m < morning_min) {
      if (morning_min - m > 30) {
        aft = 1800;
      } else if (morning_min - m == 1) {
        aft = 60 - s;
      } else {
        aft = 60 * ( morning_min - m) - s;
      }
      set_sleep(aft);
    } else if (m == morning_min) {
      //ーーーーーーーーー good_morning
      goodmorning();
      set_sleep(60);
    } else {
      //ーーーーーーーーー after
      if (m < 30 && h != evening) {
        aft = 1800;
      } else if (h != evening) {
        aft = 3600 - ((60 * m) + s);
      } else {
        aft = 0;
      }
      set_sleep(aft);
    }
    //ーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーー good_evening_action
  } else if (h == evening && mode_switch == 2) {
    //ーーーーーーーーー wait_up_to_30_min
    if (m >= 0 && m < evening_min) {
      if (evening_min - m > 30) {
        aft = 1800;
      } else if (evening_min - m == 1) {
        aft = 60 - s;
      } else {
        aft = 60 * (evening_min - m) - s;
      }
      set_sleep(aft);
    } else if (m == evening_min) {
      //ーーーーーーーーー good_evening
      goodevening();
      set_sleep(40);
    } else {
      if (m < 30 && h != morning) {
        aft = 1800;
      } else if (h != morning) {
        aft = 3600 - ((60 * m) + s);
      } else {
        aft = 0;
      }
      //ーーーーーーーーー after
      set_sleep(aft);
    }
    //ーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーtime_correction (every_30_minutes)
  } else if (h != morning && h != evening && m == 0) {
    set_sleep(1800);

  } else if (h != morning && h != evening && m == 30) {
    set_sleep(1800);

    //ーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーーtime_adjustment
  } else {
    if (m < 30) {
      aft = 1800 - ((60 * m) + s);
    } else if (m >= 30) {
      aft = 3600 - ((60 * m) + s);
    }
    set_sleep(aft);
  }
}

void setup() {
  //M5atom_setup
  M5.begin(false, false, false);
  setCpuFrequencyMhz(10);
  //pin setting
  pinMode(srv, OUTPUT);
  //srv_setting
  ledcSetup(srv_CH1, PWM_Hz, PWM_level);
  //srv_pin_setting
  ledcAttachPin(srv, srv_CH1);
}

void loop() {
  set_sleep(9999);
  GetTime();
  set_time();
  sleep_timer();
}
