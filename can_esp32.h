

#include "driver/gpio.h"
#include "driver/can.h"

#define TX_GPIO_NUM GPIO_NUM_21
#define RX_GPIO_NUM GPIO_NUM_22

bool esp32_can_open()
{
  can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(TX_GPIO_NUM, RX_GPIO_NUM, CAN_MODE_NORMAL);
  can_timing_config_t t_config = CAN_TIMING_CONFIG_500KBITS();
  can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();
  bool flag = (can_driver_install(&g_config, &t_config, &f_config) == ESP_OK);
  if (flag) {
    flag = (can_start() == ESP_OK);
    if (!flag) {
      can_driver_uninstall();
    }
  }
  return flag;
}

bool esp32_can_close()
{
  bool flag1 = (can_stop() == ESP_OK);
  bool flag2 = (can_driver_uninstall() == ESP_OK);
  return (flag1 && flag2);
}

bool esp32_can_send_msg(struct CAN_DATA *pCan)
{
  can_message_t message;
  message.identifier = pCan->id;
  message.flags = CAN_MSG_FLAG_NONE;
  //message.flags = CAN_MSG_FLAG_EXTD;
  message.data_length_code = pCan->len;
  for (int i = 0; i < pCan->len; i++) {
    message.data[i] = pCan->buf[i];
  }
  return (can_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK);
}

bool esp32_can_read_msg(struct CAN_DATA *pCan)
{
  can_message_t message;
  bool flag = (can_receive(&message, pdMS_TO_TICKS(10000)) == ESP_OK);
  if (flag) {
    //message.flags & CAN_MSG_FLAG_EXTD
    pCan->id = message.identifier;
    pCan->len = message.data_length_code;
    if (!(message.flags & CAN_MSG_FLAG_RTR)) {
      for (char i=0;i<pCan->len;i++) {
         pCan->buf[i] = message.data[i];
      }
    }
  }
  return flag;
}
