//
// Created by stardust on 7/17/24.
//

#ifndef UART_INTERRUPT_EC20_MQTT_USER_EC20_DEVICE_H_
#define UART_INTERRUPT_EC20_MQTT_USER_EC20_DEVICE_H_
#include "main.h"

#define RX_MAX_LENGTH (500)

#define ISP_NAME_LENGTH        (30)
#define ISP_USERNAME_LENGTH    (64)
#define ISP_PASSWORD_LENGTH    (64)

#define MQTT_URL_LENGTH        (256)
#define DEV_ID_LENGTH  (36)
#define MQTT_CLI_NAME_LENGTH   DEV_ID_LENGTH
#define MQTT_USERNAME_LENGTH   (64)
#define MQTT_PASSWORD_LENGTH   (64)
#define MQTT_CLIENT_ID (1)

#define MQTT_SUB_TOPIC_MAX_NUM (10)
#define MQTT_TOPIC_LENGTH      (256)

#define ISP_APN_NAME "3GNET"
#define MQTT_REGISTER_INFO_URL "sxf.kaylordut.com"
#define MQTT_REGISTER_INFO_USR_NAME "stardust"
#define MQTT_REGISTER_INFO_PASSWORD "12345678"

#define NTP_ENABLE
#ifdef NTP_ENABLE
////阿里云公共 NTP 服务器
#define NTP_SERVER_ADDR1 "time.pool.aliyun.com"

////国家授时中心 NTP 服务器
#define NTP_SERVER_ADDR2 "ntp.ntsc.ac.cn"

////中国 NTP 快速授时服务
#define NTP_SERVER_ADDR3 "cn.ntp.org.cn"

////教育网
#define NTP_SERVER_ADDR4 "edu.ntp.org.cn"

////腾讯云公共 NTP 服务器
#define NTP_SERVER_ADDR5 "ntp.tencent.com"
#endif

typedef enum {
  DEV_OK = 0,
  DEV_NORMAL_ERR,
  DEV_PARA_ERR,
  DEV_HAL_ERR,
}DEVICE_STATE;

typedef struct {
  char ispAPNName[ISP_NAME_LENGTH];
  char ispUserName[ISP_USERNAME_LENGTH];
  char ispPassword[ISP_PASSWORD_LENGTH];
}ISP_INFO;

typedef struct {
  int csqValue1;////Signal strength
  int csqValue2;////Signal strength

  char version[48];////version of ec20

  int copsID1;
  int copsID2;
  char cops[ISP_NAME_LENGTH];////isp
  int copsID3;

  int ipID1;
  int ipID2;
  int ipID3;
  char ec20IpAddr[48];////IP of the station

  char ec20NTC[64];

  int ec20InitDone;
}EC20_STATE_INFO;

typedef struct {
  char mqttUrl[MQTT_URL_LENGTH];
  char cliName[MQTT_CLI_NAME_LENGTH];
  char username[MQTT_USERNAME_LENGTH];
  char password[MQTT_PASSWORD_LENGTH];
}MQTT_REGISTER_INFO;

typedef struct {
  uint8_t topicNum;
  char subTopic[MQTT_SUB_TOPIC_MAX_NUM][MQTT_TOPIC_LENGTH];
}MQTT_SUB_TOPIC;

typedef struct {
  ISP_INFO *ispInfo;
  MQTT_REGISTER_INFO *MQTTRegisterInfo;
  MQTT_SUB_TOPIC *MQTTTopic;
  EC20_STATE_INFO *ec20StateInfo;
}EC20_CONFIG_INFO;

void UartHandle();
void ClearRxBuffer();
DEVICE_STATE ec20Init(EC20_CONFIG_INFO *ec20ConfigInfo);
DEVICE_STATE ec20SendMsg(char *topic, char *msg, uint32_t byteLength);
#endif //UART_INTERRUPT_EC20_MQTT_USER_EC20_DEVICE_H_
