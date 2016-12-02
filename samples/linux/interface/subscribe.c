#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#include <signal.h>
#include <memory.h>
#include <sys/time.h>
#include <limits.h>

#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_interface.h"
#include "aws_iot_config.h"

#include <string.h>


/**
 * @brief Filereader to read MAC address from the System
 */

char* getMacAddress(char* out) {
    FILE *fp;
    fp = fopen("/sys/class/net/eth3/address", "r");
    if (fp == NULL) {
        sprintf(out, "%s", "ERROR! Could not read the Mac address!");
    } else {
        sprintf(out, "%s", fgets(out, 50, fp));

        size_t len = strlen(out);
        if(len>0)
            out[len-1] = 0;

        fclose(fp);
    }

    return out;
}




/**
 * @brief CallbackHandler for Subscribe to a topic
 */
int MQTTcallbackHandler(MQTTCallbackParams params) {

    //INFO("Subscribe callback");
    INFO("%.*s\t%.*s\n",
         (int)params.TopicNameLen, params.pTopicName,
         (int)params.MessageParams.PayloadLen, (char*)params.MessageParams.pPayload);

    return 0;
}

void disconnectCallbackHandler(void) {
    WARN("MQTT Disconnect");
    IoT_Error_t rc = NONE_ERROR;
    if(aws_iot_is_autoreconnect_enabled()){
        INFO("Auto Reconnect is enabled, Reconnecting attempt will start now");
    }else{
        WARN("Auto Reconnect not enabled. Starting manual reconnect...");
        rc = aws_iot_mqtt_attempt_reconnect();
        if(RECONNECT_SUCCESSFUL == rc){
            WARN("Manual Reconnect Successful");
        }else{
            WARN("Manual Reconnect Failed - %d", rc);
        }
    }
}

/**
 * @brief Default cert location
 */
char certDirectory[PATH_MAX + 1] = "../../certs";

/**
 * @brief Default MQTT HOST URL is pulled from the aws_iot_config.h
 */
char HostAddress[255] = AWS_IOT_MQTT_HOST;

/**
 * @brief Default MQTT port is pulled from the aws_iot_config.h
 */
uint32_t port = AWS_IOT_MQTT_PORT;


int main(int argc, char** argv) {
    IoT_Error_t rc = NONE_ERROR;
    int32_t i = 0;
    char macAddress[50];
    char memInfo[500];

    char rootCA[PATH_MAX + 1];
    char clientCRT[PATH_MAX + 1];
    char clientKey[PATH_MAX + 1];
    char CurrentWD[PATH_MAX + 1];
    char cafileName[] = AWS_IOT_ROOT_CA_FILENAME;
    char clientCRTName[] = AWS_IOT_CERTIFICATE_FILENAME;
    char clientKeyName[] = AWS_IOT_PRIVATE_KEY_FILENAME;

    INFO("\nAWS IoT SDK Version %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

    getcwd(CurrentWD, sizeof(CurrentWD));
    sprintf(rootCA, "%s/%s/%s", CurrentWD, certDirectory, cafileName);
    sprintf(clientCRT, "%s/%s/%s", CurrentWD, certDirectory, clientCRTName);
    sprintf(clientKey, "%s/%s/%s", CurrentWD, certDirectory, clientKeyName);


/**
 * @brief Define first topic with MAC address
 */
    char topic1[100];
    sprintf(topic1, "%s/%s/%s","sensorgruppe21",getMacAddress(macAddress),"topic_1");

    char topic2[100];
    sprintf(topic2, "%s/%s/%s","sensorgruppe21",getMacAddress(macAddress),"topic_2");



    DEBUG("rootCA %s", rootCA);
    DEBUG("clientCRT %s", clientCRT);
    DEBUG("clientKey %s", clientKey);
    DEBUG("MAC address: %s", getMacAddress(macAddress));
    DEBUG("Topic1: %s", topic1);
    DEBUG("Topic2: %s", topic2);


/**
 * @brief Connection
 */
    MQTTConnectParams connectParams = MQTTConnectParamsDefault;

    connectParams.KeepAliveInterval_sec = 10;
    connectParams.isCleansession = true;
    connectParams.MQTTVersion = MQTT_3_1_1;
    connectParams.pClientID = "subscribe1"; //"CSDK-test-device"
    connectParams.pHostURL = HostAddress;
    connectParams.port = port;
    connectParams.isWillMsgPresent = false;
    connectParams.pRootCALocation = rootCA;
    connectParams.pDeviceCertLocation = clientCRT;
    connectParams.pDevicePrivateKeyLocation = clientKey;
    connectParams.mqttCommandTimeout_ms = 2000;
    connectParams.tlsHandshakeTimeout_ms = 5000;
    connectParams.isSSLHostnameVerify = true; // ensure this is set to true for production
    connectParams.disconnectHandler = disconnectCallbackHandler;

    INFO("Connecting...");
    rc = aws_iot_mqtt_connect(&connectParams);
    if (NONE_ERROR != rc) {
        ERROR("Error(%d) connecting to %s:%d", rc, connectParams.pHostURL, connectParams.port);
    }
    /*
     * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
     *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
     *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
     */
    rc = aws_iot_mqtt_autoreconnect_set_status(true);
    if (NONE_ERROR != rc) {
        ERROR("Unable to set Auto Reconnect to true - %d", rc);
        return rc;
    }


/**
 * @brief Subscribe to a topic
 */
    MQTTSubscribeParams subParams = MQTTSubscribeParamsDefault;
    subParams.mHandler = MQTTcallbackHandler;
    subParams.pTopic = topic1;
    subParams.qos = QOS_0;

    if (NONE_ERROR == rc) {
        INFO("Subscribing to the first topic...");
        rc = aws_iot_mqtt_subscribe(&subParams);
        if (NONE_ERROR != rc) {
            ERROR("Error subscribing to the first topic");
        }
    }

    MQTTSubscribeParams subParams2 = MQTTSubscribeParamsDefault;
    subParams2.mHandler = MQTTcallbackHandler;
    subParams2.pTopic = topic2;
    subParams2.qos = QOS_0;


    if (NONE_ERROR == rc) {
        INFO("Subscribing to the second topic...");
        rc = aws_iot_mqtt_subscribe(&subParams2);
        if (NONE_ERROR != rc) {
            ERROR("Error subscribing to the second topic");
        }
    }


/**
 * @brief Publish to a topic
 */


    while ((NETWORK_ATTEMPTING_RECONNECT == rc || RECONNECT_SUCCESSFUL == rc || NONE_ERROR == rc)) {


        //Max time the yield function will wait for read messages
        rc = aws_iot_mqtt_yield(100);
        if(NETWORK_ATTEMPTING_RECONNECT == rc){
            INFO("-->sleep");
            sleep(1);
            // If the client is attempting to reconnect we will skip the rest of the loop.
            continue;
        }
        //INFO("-->sleep");
        sleep(1);

    }

    if (NONE_ERROR != rc) {
        ERROR("An error occurred in the loop.\n");
    } else {
        INFO("Publish done\n");
    }

    return rc;
}

