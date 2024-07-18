//
// Created by stardust on 7/17/24.
//
#include "stdlib.h"
#include "string.h"
#include "usart.h"
#include "stdio.h"
#include "stdbool.h"
#include "time.h"
#include "stdarg.h"
#include "ec20_device.h"

////need change according to which uart use to connect ec20
#define EC20_TRANSMIT_UART huart6

uint8_t rx_buffer_;
uint8_t receive_buffer_[RX_MAX_LENGTH];
uint32_t rx_count_ = 0;

uint8_t initEC20Done   = 0;

char ec20_sprint_buf[1000];
int ec20_printf(char *fmt, ...) {
  memset(ec20_sprint_buf, 0, 1000);
  va_list args;
  int n;
  va_start(args, fmt);
  n = vsprintf(ec20_sprint_buf, fmt, args);
  va_end(args);
  return HAL_UART_Transmit(&EC20_TRANSMIT_UART, (uint8_t *) ec20_sprint_buf, n, 1000);
}
#define EC20_UART_SEND ec20_printf

void UartHandle() {
  if (rx_count_ >= RX_MAX_LENGTH) {
    uart_printf("rx buffer is full!\r\n");
    ClearRxBuffer();
  }

  receive_buffer_[rx_count_++] = rx_buffer_;
//  uart_printf("receive data = %x\r\n", receive_buffer_[rx_count_ - 1]);

  if (initEC20Done == 1) {
    if (receive_buffer_[rx_count_ - 1] == 0x0A) {
      if (rx_count_ - 1 > 0 && receive_buffer_[rx_count_ - 2] == 0x0D) {
        ////a whole msg from ec20 begin and end all at : OD OA
        if (rx_count_ == 2) {
          ////begin msg, continue receive
        } else {
          ////end of msg
          if (rx_count_ >= RX_MAX_LENGTH)
          {
            uart_printf("DEVICE-EC20, %d-msg from ec20 out of rang\n", __LINE__);
            ClearRxBuffer();
            HAL_UART_Receive_IT(&EC20_TRANSMIT_UART, (uint8_t *)&rx_buffer_, 1);
            return;
          }

          if (NULL == strstr((const char *)receive_buffer_, (const char *)"QMTRECV"))
          {
            if (NULL == strstr((const char *)receive_buffer_, (const char *)"QMTSTAT")) {
              ClearRxBuffer();
              HAL_UART_Receive_IT(&EC20_TRANSMIT_UART, (uint8_t *)&rx_buffer_, 1);
              return;
            }
          }

          //TODO:Add code, do action for this msg, usually can send to queue
        }
      }
    }
  }

  HAL_UART_Receive_IT(&EC20_TRANSMIT_UART, (uint8_t *)&rx_buffer_, 1);
}

void ClearRxBuffer()//清空接收缓存
{
  uint32_t i;
  for(i = 0; i < rx_count_; i++)
  {
    receive_buffer_[i] = 0;
  }
  rx_count_ = 0;
}

/**
  * @brief  This function Execute the EC20's AT Command.
  * @note   This function is called in the place of sending AT command.
  * @param  cmd: the command need to execute.
  * @param  successReply: if executing the cmd succeed, ec20 will reply this str.
  * @param  retry: if executing failed, how many times will retry.
  * @retval DEVICE_STATE status
  */
DEVICE_STATE ec20DoCmd(char *cmd, char *successReply, int8_t retry)
{
  char *ret;

  if (NULL == cmd || NULL == successReply)
  {
//    elog_e("DEVICE_EC20", "%d-parameter err when execute ec20cmd", __LINE__);
    return DEV_PARA_ERR;
  }

  EC20_UART_SEND("%s\r\n", cmd);
  HAL_Delay(500);

  do {
    ret = strstr((const char*)receive_buffer_, (const char*)successReply);
    retry--;
    EC20_UART_SEND("%s\r\n", cmd);

    HAL_Delay(500);

  }while(NULL == ret && retry >= 0);



  if (NULL == ret)
  {
//    elog_e("DEVICE_EC20", "%d-execute cmd(%s) err, err msg: %s", __LINE__, cmd, receive_buffer_);
    return DEV_NORMAL_ERR;
  }
//  elog_i("DEVICE_EC20", "%d-execute cmd(%s) success", __LINE__, cmd);

  return DEV_OK;
}

/**
  * @brief  This function connecting to the network through SIM card.
  * @note   This function is called at the beginning of ec20Init.
  * @param  ispInfo(IN): ISP APN name, userName, password.
  * @param  ec20StateInfo(OUT): signal strength, version, cops(isp), IP
  * @retval DEVICE_STATE status
  */
DEVICE_STATE ec20ConnectInternet(ISP_INFO *ispInfo, EC20_STATE_INFO *ec20StateInfo)
{
  char connInterCmd[256];
  char successReply[30] = {0};
  char *reply;
  int retry;
  char *cmdReply;
  int csqValue1, csqValue2;
  int i = 0;
  DEVICE_STATE ret;

//  elog_i("DEVICE_EC20", "%d-Connect Internet......", __LINE__);

  ////1.waiting ec20 up
  ////create success reply
  snprintf(successReply, sizeof(successReply), "OK");

  ////create cmd to connect MQTT server, and then send cmd.
  snprintf(connInterCmd, sizeof(connInterCmd), "AT");
  if (DEV_OK != (ret = ec20DoCmd(connInterCmd, successReply, 50)))
  {
    return ret;
  }

#ifdef INCLUDE_GPS
  ////2.enable GPS
    EC20_UART_SEND("AT+QGPS?\r\n");//查询当前状态
    HAL_Delay(500);
    cmdReply = strstr((const char*)receive_buffer_, (const char*)"+QGPS: 1");//返回已经上电
    if(cmdReply == NULL)//如果没上，电就上电；如果上电了，不能重复上电
        EC20_UART_SEND("AT+QGPS=1\r\n");//对GNSS（GPS模块）上电
    HAL_Delay(500);

#endif


  ////2.check signal strength
  while(i++ < 10)
  {
    ////wait 5s, then check signal strength
    HAL_Delay(500);


  }
  EC20_UART_SEND("AT+CSQ\r\n"); //检查CSQ，返回应该是0~31,99
  HAL_Delay(500);
  if (-1 == sscanf((const char*)receive_buffer_, (const char*)"AT+CSQ\n+CSQ: %d,%d", &csqValue1, &csqValue2))
  {
//    elog_e("DEVICE_EC20", "%d-check CSQ(Signal strength) err", __LINE__);
    return DEV_NORMAL_ERR;
  }
  ec20StateInfo->csqValue1 = csqValue1;
  ec20StateInfo->csqValue2 = csqValue2;
  if (csqValue1 < 0 || csqValue1 > 31)// || csqValue2 != 99)
  {
//    elog_e("DEVICE_EC20", "%d-Signal strength(%d,%d); need to be (0~31,99) err", __LINE__, csqValue1, csqValue2);
    return DEV_NORMAL_ERR;
  }
//  elog_i("DEVICE_EC20", "%d-Signal strength(%d,%d)", __LINE__, csqValue1, csqValue2);


  ////3.check EC20 version
  EC20_UART_SEND("ATI\r\n"); //检查模块版本号
  HAL_Delay(500);
  if (NULL == (cmdReply = strstr((const char*)receive_buffer_,(const char*)"Revision:")))
  {
//    elog_e("DEVICE", "%d-check EC20 version, err msg:%s", __LINE__, receive_buffer_);
    return DEV_NORMAL_ERR;
  }
  if (-1 == sscanf((const char*)cmdReply, (const char*)"Revision: %s", ec20StateInfo->version))
  {
//    elog_e("DEVICE", "%d-can not ansys version err, buffer: %s", __LINE__, cmdReply);
    return DEV_NORMAL_ERR;
  }
//  elog_i("DEVICE_EC20", "%d-ec20 module version:%s", __LINE__, ec20StateInfo->version);


  ////4.check if the SIM is in place
  ////create success reply
  memset(successReply, 0, sizeof(successReply));
  snprintf(successReply, sizeof(successReply), "+CPIN: READY");

  ////create cmd to check SIM.
  memset(connInterCmd, 0, sizeof(connInterCmd));
  snprintf(connInterCmd, sizeof(connInterCmd), "AT+CPIN?");
  if (DEV_OK != (ret = ec20DoCmd(connInterCmd, successReply, 3)))
  {
    return ret;
  }
//  elog_i("DEVICE_EC20", "%d-check SIM success", __LINE__);

  ////5.check GSM net
  ////create success reply
  memset(successReply, 0, sizeof(successReply));
  snprintf(successReply, sizeof(successReply), "+CREG: 0,1");//////返回正常

  ////create cmd to check GSM.
  memset(connInterCmd, 0, sizeof(connInterCmd));
  snprintf(connInterCmd, sizeof(connInterCmd), "AT+CREG?");////查看是否注册GSM网络（2g 用于打电话）
  if (DEV_OK != (ret = ec20DoCmd(connInterCmd, successReply, 3)))
  {
    ////check roam
    memset(successReply, 0, sizeof(successReply));
    snprintf(successReply, sizeof(successReply), "+CREG: 0,5");//////返回正常，漫游
    if (DEV_OK != (ret = ec20DoCmd(connInterCmd, successReply, 3)))
    {
      return ret;
    }
  }
//  elog_i("DEVICE_EC20", "%d-check GSM net success", __LINE__);

  ////6.check GPRS net
  ////create success reply
  memset(successReply, 0, sizeof(successReply));
  snprintf(successReply, sizeof(successReply), "+CGREG: 0,1");//////返回正常，只有注册成功，才可以进行GPRS数据传输

  ////create cmd to check GPRS.
  memset(connInterCmd, 0, sizeof(connInterCmd));
  snprintf(connInterCmd, sizeof(connInterCmd), "AT+CGREG?");////查看是否注册GPRS网络
  if (DEV_OK != (ret = ec20DoCmd(connInterCmd, successReply, 3)))
  {
    ////check roam
    memset(successReply, 0, sizeof(successReply));
    snprintf(successReply, sizeof(successReply), "+CGREG: 0,5");//////返回正常，漫游
    if (DEV_OK != (ret = ec20DoCmd(connInterCmd, successReply, 3)))
    {
      return ret;
    }
  }
//  elog_i("DEVICE_EC20", "%d-check GPRS net success", __LINE__);

  ////7.check ISP
  EC20_UART_SEND("AT+COPS?\r\n"); ////查看注册到那个运营商，支持移动、联通、电信
  HAL_Delay(500);
  if (-1 == sscanf((const char*)receive_buffer_, (const char*)"AT+COPS?\n+COPS: %d,%d,\"%s\",%d",
                   &ec20StateInfo->copsID1, &ec20StateInfo->copsID2, ec20StateInfo->cops, &ec20StateInfo->copsID3))
  {
//    elog_e("DEVICE", "%d-can not ansys ISP, buffer: %s", __LINE__, receive_buffer_);
    return DEV_NORMAL_ERR;
  }
//  elog_i("DEVICE_EC20", "%d-ISP of this SIM is :%s", __LINE__, ec20StateInfo->cops);


  ////8.close socket, unknown setting
  EC20_UART_SEND("AT+QICLOSE=0\r\n");////关闭socket链接
  HAL_Delay(500);


  ////9.connect to APN net
  ////create success reply
//    memset(successReply, 0, sizeof(successReply));
//    snprintf(successReply, sizeof(successReply), "OK");//////开启成功
//
//    ////create cmd to connect to APN net.
//    memset(connInterCmd, 0, sizeof(connInterCmd));
//    snprintf(connInterCmd, sizeof(connInterCmd), "AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",0",
//             ispInfo->ispAPNName, ispInfo->ispUserName, ispInfo->ispPassword);////接入APN
//    if (DEV_OK != (ret = ec20DoCmd(connInterCmd, successReply, 3)))
//    {
//        return ret;
//    }
//    elog_i("DEVICE", "%d-connect to APN net success", __LINE__);

  ////10.inactive
  ////create success reply
  memset(successReply, 0, sizeof(successReply));
  snprintf(successReply, sizeof(successReply), "OK");//////开启成功

  ////create cmd to inactive.
  memset(connInterCmd, 0, sizeof(connInterCmd));
  snprintf(connInterCmd, sizeof(connInterCmd), "AT+QIDEACT=1");////去激活
  if (DEV_OK != (ret = ec20DoCmd(connInterCmd, successReply, 3)))
  {
    return ret;
  }
//  elog_i("DEVICE_EC20", "%d-inactive success", __LINE__);

  ////11.active
  ////create success reply
  memset(successReply, 0, sizeof(successReply));
  snprintf(successReply, sizeof(successReply), "OK");//////开启成功

  ////create cmd to active.
  memset(connInterCmd, 0, sizeof(connInterCmd));
  snprintf(connInterCmd, sizeof(connInterCmd), "AT+QIACT=1");////激活
  if (DEV_OK != (ret = ec20DoCmd(connInterCmd, successReply, 3)))
  {
    return ret;
  }
//  elog_i("DEVICE_EC20", "%d-active success", __LINE__);

  ////12.check IP
  EC20_UART_SEND("AT+QIACT?\r\n"); ////获取当前卡的IP地址 +QIACT: 1,1,1,"10.230.233.248"
  HAL_Delay(500);
  if (-1 == sscanf((const char*)receive_buffer_, (const char*)"AT+QIACT?\n+QIACT: %d,%d,%d,\"%s\"",
                   &ec20StateInfo->ipID1, &ec20StateInfo->ipID2, &ec20StateInfo->ipID3, ec20StateInfo->ec20IpAddr))
  {
//    elog_e("DEVICE", "%d-can not ansys IP, buffer: %s", __LINE__, receive_buffer_);
    return DEV_NORMAL_ERR;
  }
//  elog_i("DEVICE_EC20", "%d-IP of this station is :%s", __LINE__, ec20StateInfo->ec20IpAddr);

//  elog_i("DEVICE_EC20", "%d-connect to internet success, ip: %s", __LINE__, ec20StateInfo->ec20IpAddr);
  return DEV_OK;
}

/**
  * @brief  This function has 2 functions.
  *         1.Connect the MQTT server by TCP connection.
  *         2.Login to MQTT server.
  * @note   This function is a public func, can use in other modules.
  * @param  mqttRegisterInfo: client_name of station, user_name and password for logging to MQTT server.
  * @retval DEVICE_STATE status
  */
DEVICE_STATE ec20RegisterMQTT(MQTT_REGISTER_INFO * mqttRegisterInfo)
{
  char registerMQTTCmd[MQTT_URL_LENGTH + 20];
  char successReply[30] = {0};
  DEVICE_STATE ret;

//  elog_i("DEVICE_EC20", "%d-Register to MQTT server......", __LINE__);

  ////1.parameter check
  if (0 == strlen(mqttRegisterInfo->mqttUrl) || 0 == strlen(mqttRegisterInfo->cliName)
      || 0 == strlen(mqttRegisterInfo->username) || 0 == strlen(mqttRegisterInfo->password))
  {
//    elog_e("DEVICE_EC20", "%d-parameter err when logging to MQTT server", __LINE__);
    return DEV_PARA_ERR;
  }

  ////connecting MQTT server
  ////create success reply for connecting MQTT server
  snprintf(successReply, sizeof(successReply), "+QMTOPEN: %d,0", MQTT_CLIENT_ID);

  ////create cmd to connect MQTT server, and then send cmd.
  snprintf(registerMQTTCmd, sizeof(registerMQTTCmd), "AT+QMTOPEN=%d,\"%s\",1883", MQTT_CLIENT_ID, mqttRegisterInfo->mqttUrl);
  if (DEV_OK != (ret = ec20DoCmd(registerMQTTCmd, successReply, 3)))
  {
    return ret;
  }

  ////2.Login MQTT server
  ////create success reply for logging MQTT server
  memset(successReply, 0, sizeof(successReply));
  snprintf(successReply, sizeof(successReply), "+QMTCONN: %d,0,0", MQTT_CLIENT_ID);

  ////create cmd to connect to MQTT server, and then send cmd.
  memset(registerMQTTCmd, 0, sizeof(registerMQTTCmd));
  snprintf(registerMQTTCmd, sizeof(registerMQTTCmd), "AT+QMTCONN=%d,\"%s\",\"%s\",\"%s\"", MQTT_CLIENT_ID,
           mqttRegisterInfo->cliName, mqttRegisterInfo->username, mqttRegisterInfo->password);
  if (DEV_OK != (ret = ec20DoCmd(registerMQTTCmd, successReply, 3)))
  {
    return ret;
  }

//  elog_i("DEVICE_EC20", "%d-Register to MQTT server(%s) success", __LINE__, mqttRegisterInfo->mqttUrl);
  return DEV_OK;
}

/**
  * @brief  This function subscribes topics in MQTT Server.
  * @note   This function is a public func, can use in other modules.
  * @param  mqttSubTopic: topics needed to subscribe.
  * @retval DEVICE_STATE status
  */
DEVICE_STATE ec20SubscribeTopic(MQTT_SUB_TOPIC *mqttSubTopic)
{
  uint8_t i;
  char subTopicCmd[MQTT_TOPIC_LENGTH + 20];
  char successReply[30] = {0};
  DEVICE_STATE ret;

  ////create success reply for subscribing topic
  snprintf(successReply, sizeof(successReply), "+QMTSUB: %d,1,0,0", MQTT_CLIENT_ID);

  for (i = 0; i < mqttSubTopic->topicNum; i++)
  {
//    elog_i("DEVICE_EC20", "%d-Subscribe topic(%s)......", __LINE__, mqttSubTopic->subTopic[i]);

    ////check parameters
    if (i >= MQTT_SUB_TOPIC_MAX_NUM)
    {
//      elog_e("DEVICE_EC20", "%d-Number(%d) of subscribed topics exceed max num", __LINE__, i);
      return DEV_PARA_ERR;
    }
    if (strlen(mqttSubTopic->subTopic[i]) == 0)
    {
//      elog_e("DEVICE_EC20", "%d-Topic(%d) to be subscribed is null", __LINE__, i);
      return DEV_PARA_ERR;
    }

    ////create cmd to subscribe topic, and then send cmd.
    memset(subTopicCmd, 0, MQTT_TOPIC_LENGTH + 20);
    snprintf(subTopicCmd, sizeof(subTopicCmd), "AT+QMTSUB=%d,1,\"%s\",0", MQTT_CLIENT_ID, mqttSubTopic->subTopic[i]);
    if (DEV_OK != (ret = ec20DoCmd(subTopicCmd, successReply, 3)))
    {
      return ret;
    }
//    elog_i("DEVICE_EC20", "%d-Subscribe topic(%s) success", __LINE__, mqttSubTopic->subTopic[i]);
  }

//  elog_i("DEVICE_EC20", "%d-All topic have subscribe success", __LINE__);
  return DEV_OK;
}

/**
  * @brief  This function has 2 functions.
  *         1.Init the EC20 model to connect the Internet.
  *         2.Register to MQTT server and subscribe the topic.
  * @note   This function is called at the beginning of mainTask.
  * @param  ec20ConfigInfo: ISP info, MQTT server info, and ec20 state info(out).
  * @retval DEVICE_STATE status
  */
DEVICE_STATE ec20SendMsg(char *topic, char *msg, uint32_t byteLength)
{
  int ret;
  ret = EC20_UART_SEND("AT+QMTPUBEX=%d,0,0,0,\"%s\",%d\r\n", MQTT_CLIENT_ID, topic, byteLength);//发布消息
  if (ret != HAL_OK)
  {
//    elog_e("DEVICE-EC20", "Send QMTPUBEX cmd err : %d\r", ret);
  }

  HAL_Delay(100);
  ret = EC20_UART_SEND("%s\r\n", msg);//发布消息
  if (ret != HAL_OK)
  {
//    elog_e("DEVICE-EC20", "Send msg err : %d", ret);
  }

  if (msg[byteLength - 1] != '}')
  {
    uart_printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~fail msg = %s\n", msg);
    HAL_Delay(200);
    EC20_UART_SEND("AT+QMTPUBEX=%d,0,0,0,\"clean_robot_station/Err_msg\",%d\r\n", MQTT_CLIENT_ID, byteLength);//发布消息
    HAL_Delay(100);
    EC20_UART_SEND("%s\r\n", msg);//发布消息
    return DEV_NORMAL_ERR;
  } else{
//        uart_printf("~~~~~~~~~~~~~~~~~~~~~~success msg = %s\r\n", msg);
  }
}

#ifdef NTP_ENABLE
char ntpServerArray[5][64] = {NTP_SERVER_ADDR1, NTP_SERVER_ADDR2, NTP_SERVER_ADDR3,
                              NTP_SERVER_ADDR4, NTP_SERVER_ADDR5};
DEVICE_STATE ntpByEC20()
{
    char *reply;
    char *replyOk;
    char currentTime[64] = {0};
    int ntpID;

    int timeZone;
    time_t timestamp;
    struct tm timeinfo;



    bool doNtp = true;
    uint8_t ntpCount = 0;
    uint8_t waitCount = 0;
//    elog_i("DEVICE_EC20", "begin ntp");
    do {

        EC20_UART_SEND("AT+QNTP=1,\"%s\",123,1\r\n", ntpServerArray[ntpCount]); ////同步网络时间
        HAL_Delay(1000);


        while (NULL == strstr((const char*)receive_buffer_, (const char*)"ERROR") &&
                NULL == strstr((const char*)receive_buffer_, (const char*)"OK") &&
                waitCount++ < 125)
        {
            HAL_Delay(1000);

//            elog_i("DEVICE_EC20", "wait ec20 response for ntp, time %ds", waitCount);
        }
        if (waitCount > 125)
        {
            ////ec20 has not reply, response over time

//            elog_e("DEVICE_EC20", "%d-EC20 ntp over time err!", __LINE__);
            reply = NULL;
            return DEV_NORMAL_ERR;
        }

        if (NULL != strstr((const char*)receive_buffer_, (const char*)"ERROR") ||
            NULL == (replyOk = strstr((const char*)receive_buffer_, (const char*)"OK")))
        {
            ////ec20 ntp fail, tyr next ntp server after 5s
//            elog_i("DEVICE_EC20", "ec20 ntp fail(%s), tyr next ntp server(%s)",
//                   ntpServerArray[ntpCount], (ntpCount + 1 < 5) ? ntpServerArray[ntpCount+1] : ntpServerArray[0]);

            for (int i = 0; i < 5; i++)
            {
                HAL_Delay(1000);

            }

            replyOk = NULL;

            if (++ntpCount > 4)
            {
                ntpCount = 0;
            }
            continue;
        }
        else
        {
            for (int k = 0; k < 125; k++)
            {
                reply = strstr((const char*)replyOk, (const char*)"+QNTP:");
                if (reply != NULL &&
                    -1 != sscanf((const char *)reply, (const char *)"+QNTP: %d,\"%s\"", &ntpID, currentTime))
                {
                    ////ntp success
                    if(currentTime[0] != '\0')
                    {
                        doNtp = false;
                    }
                    break;
                }

                HAL_Delay(1000);

//                elog_d("DEVICE_EC20", "wait ntp time, count %d s", k);
            }

        }


        if (++ntpCount > 4)
        {
            ntpCount = 0;
        }
    } while (doNtp);

    //// 将日期时间字符串转换为时间戳
    if (-1 == sscanf((const char *)currentTime, (const char *)"%d/%d/%d,%d:%d:%d\uF0B1%d",
           &timeinfo.tm_year, &timeinfo.tm_mon, &timeinfo.tm_mday,
           &timeinfo.tm_hour, &timeinfo.tm_min, &timeinfo.tm_sec,
           &timeZone))
    {
//        elog_i("DEVICE_EC20", "%d-Ansys current time err!", __LINE__);
        reply = NULL;
        return DEV_NORMAL_ERR;
    }
    timeinfo.tm_year -= 1900;
    timeinfo.tm_mon -= 1;
    timestamp = mktime(&timeinfo);
//    elog_i("DEVICE_EC20", "data time %s change to unix time %lld", currentTime, timestamp);

    reply = NULL;
    return DEV_OK;
}
#endif

/**
  * @brief  This function reconnect mqtt server when mqtt connection closed
  * @note   This function is called when receiving QMTSTAT msg.
  * @param  ec20ConfigInfo: ISP info, MQTT server info, and ec20 state info(out).
  * @retval DEVICE_STATE status
  */
//DEVICE_STATE ReconnectMqtt() {
////  elog_w("DEVICE_EC20", "%d-Re connect mqtt server......", __LINE__);
//  DEVICE_STATE ret;
//  initEC20Done = 0;
//
//  ////register to MQTT Server
//  ret = ec20RegisterMQTT(g_ec20_config_info.MQTTRegisterInfo);
//  if (DEV_OK != ret)
//  {
////    elog_e("DEVICE_EC20", "%d-ec20RegisterMQTT err", __LINE__);
//    return ret;
//  }
//
//  ////subscribe topic
//  ret = ec20SubscribeTopic(g_ec20_config_info.MQTTTopic);
//  if (DEV_OK != ret)
//  {
////    elog_e("DEVICE_EC20", "%d-ec20SubscribeTopic err", __LINE__);
//    return ret;
//  }
//
//  initEC20Done = 1;
////  elog_i("DEVICE_EC20", "%d-Init ec20 module success", __LINE__);
//
//  return DEV_OK;
//}

/**
  * @brief  This function has 2 functions.
  *         1.Init the EC20 model to connect the Internet.
  *         2.Register to MQTT server and subscribe the topic.
  * @note   This function is called at the beginning of mainTask.
  * @param  ec20ConfigInfo: ISP info, MQTT server info, and ec20 state info(out).
  * @retval DEVICE_STATE status
  */
DEVICE_STATE ec20Init(EC20_CONFIG_INFO *ec20ConfigInfo)
{
  DEVICE_STATE ret;
  HAL_StatusTypeDef halRet;

//  elog_i("DEVICE_EC20", "%d-Init ec20 module......", __LINE__);
//    HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);

//    memset(&ec20Data1, 0, sizeof(ec20Data1));
//    memset(&ec20Data2, 0, sizeof(ec20Data2));

  ////parameter check
  if (NULL == ec20ConfigInfo
      || NULL == ec20ConfigInfo->ispInfo
      || NULL == ec20ConfigInfo->MQTTRegisterInfo
      || NULL == ec20ConfigInfo->MQTTTopic)
  {
//    elog_e("DEVICE_EC20", "[%s]:parameter err", __LINE__);
    return DEV_PARA_ERR;
  }

  ////start ec20 receive interrupt
  halRet = HAL_UART_Receive_IT(&EC20_TRANSMIT_UART, (uint8_t *)&rx_buffer_, 1);
  if (HAL_OK != halRet)
  {
//    elog_e("DEVICE_EC20", "[%s]:HAL_UART_Receive_IT err(%d)", __LINE__, halRet);
    return DEV_NORMAL_ERR;
  }

  ////connect to internet
  ret = ec20ConnectInternet(ec20ConfigInfo->ispInfo, ec20ConfigInfo->ec20StateInfo);
  if (DEV_OK != ret)
  {
//    elog_e("DEVICE_EC20", "%d-ec20ConnectInternet err", __LINE__);
    return ret;
  }

  ////register to MQTT Server
  ret = ec20RegisterMQTT(ec20ConfigInfo->MQTTRegisterInfo);
  if (DEV_OK != ret)
  {
//    elog_e("DEVICE_EC20", "%d-ec20RegisterMQTT err", __LINE__);
    return ret;
  }

  ////subscribe topic
  ret = ec20SubscribeTopic(ec20ConfigInfo->MQTTTopic);
  if (DEV_OK != ret)
  {
//    elog_e("DEVICE_EC20", "%d-ec20SubscribeTopic err", __LINE__);
    return ret;
  }
#ifdef NTP_ENABLE
  ret = ntpByEC20();
    if (DEV_OK != ret)
    {
//        elog_e("DEVICE_EC20", "%d-ntpByEC20 err", __LINE__);
        return ret;
    }
#endif
  initEC20Done = 1;
//  g_ec20_state_info.ec20InitDone = initEC20Done;
//    HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET);
//  elog_i("DEVICE_EC20", "%d-Init ec20 module success", __LINE__);

  return DEV_OK;
}