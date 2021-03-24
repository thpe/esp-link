#include <esp8266.h>
#include <config.h>

#include <uart.h>

#include "mqtt.h"
#include "mqtt_client.h"
extern MQTT_Client mqttClient;

#define SCALE_STATUS_INTERVAL (20*60*1000)

static ETSTimer uarttimer;

char resp_buf[24];
short resp_idx = 0;
char resp_state = 0;

void ICACHE_FLASH_ATTR
mqttWeightMsg() {
  char buf[32];

  // compose MQTT message
  os_sprintf(buf, "{\"weight\":%s}", resp_buf);

  if (!flashConfig.mqtt_status_enable || os_strlen(flashConfig.mqtt_status_topic) == 0 ||
    mqttClient.connState != MQTT_CONNECTED)
    return;

  MQTT_Publish(&mqttClient, flashConfig.mqtt_status_topic, buf, os_strlen(buf), 1, 0);
}

static void ICACHE_FLASH_ATTR poll_task ()
{
  uart0_write_char(0x1b);
  uart0_write_char('p');
}


static void ICACHE_FLASH_ATTR read_cb (char* buf, short len)
{
   short i = 0;
   for (; i < len; i++) {
     char c = buf[i];
     switch (c) {
     case '-':
     case '0':
     case '1':
     case '2':
     case '3':
     case '4':
     case '5':
     case '6':
     case '7':
     case '8':
     case '9':
       resp_buf[resp_idx++] = c;
       break;
     case 'g':
       resp_buf[resp_idx++] = 0;
       mqttWeightMsg ();
       resp_idx = 0;
       break;
     case ' ':
     case '.':
     default:
       break;
     }
     if (resp_idx >= 24) {
       resp_idx = 0;
     }
   }
}

// initialize the custom stuff that goes beyond esp-link
void app_init() {
  uart_add_recv_cb (&read_cb);
  os_timer_disarm(&uarttimer);
  os_timer_setfn(&uarttimer, poll_task, NULL);
  os_timer_arm(&uarttimer, SCALE_STATUS_INTERVAL, 1); // recurring timer
}
