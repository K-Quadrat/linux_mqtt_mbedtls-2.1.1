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
#include "aws_iot_shadow_interface.h"
#include "aws_iot_shadow_json_data.h"

#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_config.h"

#include <string.h>

/**
 * @brief Filereader to read memory info from the System
 */

char* getMemInfo(char* out) {
    FILE *fp;
    char line[1000];
    int i = 0;
    int j = 0;
    int k = 0;
    char names[1000][1000];
    char names3[1000][1000];
    fp = fopen("/proc/meminfo", "r");
    int c = 0;

    while((fscanf(fp,"%s",line)) != EOF ) {

        sprintf(names[i], "%s", line);
        i++;
    }

    int l;

    for(l=0; l<150; l=l+3) {
        size_t len = strlen(names[l]);
        names[l][len-1] = 0;
    }

    for(l=0; l<38; l++) {
        sprintf(names3[l], "%s%s%s%s %s%s","\"", names[k],"\": \"", names[k+1], names[k+2], "\",");
        k=k+3;
    }

    for(l=0; l<38; l++) {
        strcat(names3[0], names3[l+1]);
        l++;
    }
    sprintf(names3[1], "%s%s%s", "{", names3[0], "\n}");

    size_t len = strlen(names3[1]);
    if(len>0)
        names3[1][len-3] = 0;

    sprintf(names3[2], "%s%s", names3[1], "}");
    sprintf(out, "%s", names3[2]);
    fclose(fp);

    return out;
}



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
/**
int MQTTcallbackHandler(MQTTCallbackParams params) {

	INFO("Subscribe callback");
	INFO("%.*s\t%.*s",
			(int)params.TopicNameLen, params.pTopicName,
			(int)params.MessageParams.PayloadLen, (char*)params.MessageParams.pPayload);

	return 0;
}
*/

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

#define ROOMTEMPERATURE_UPPERLIMIT 32.0f
#define ROOMTEMPERATURE_LOWERLIMIT 25.0f
#define STARTING_ROOMTEMPERATURE ROOMTEMPERATURE_LOWERLIMIT

static void simulateRoomTemperature(float *pRoomTemperature) {
	static float deltaChange;

	if (*pRoomTemperature >= ROOMTEMPERATURE_UPPERLIMIT) {
		deltaChange = -0.5f;
	} else if (*pRoomTemperature <= ROOMTEMPERATURE_LOWERLIMIT) {
		deltaChange = 0.5f;
	}

	*pRoomTemperature += deltaChange;
}

void ShadowUpdateStatusCallback(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
								const char *pReceivedJsonDocument, void *pContextData) {

	if (status == SHADOW_ACK_TIMEOUT) {
		INFO("Update Timeout--");
	} else if (status == SHADOW_ACK_REJECTED) {
		INFO("Update RejectedXX");
	} else if (status == SHADOW_ACK_ACCEPTED) {
		INFO("Update Accepted !!");
	}
}

void windowActuate_Callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
	if (pContext != NULL) {
		INFO("Delta - Window state changed to %d", *(bool *)(pContext->pData));
	}
}

/**
 * @brief Default cert location
 */
char certDirectory[PATH_MAX + 1] = "../../../certs";

/**
 * @brief Default MQTT HOST URL is pulled from the aws_iot_config.h
 */
char HostAddress[255] = AWS_IOT_MQTT_HOST;

/**
 * @brief Default MQTT port is pulled from the aws_iot_config.h
 */
uint32_t port = AWS_IOT_MQTT_PORT;

uint8_t numPubs = 5;


void parseInputArgsForConnectParams(int argc, char** argv) {
	int opt;

	while (-1 != (opt = getopt(argc, argv, "h:p:c:n:"))) {
		switch (opt) {
			case 'h':
				strcpy(HostAddress, optarg);
				DEBUG("Host %s", optarg);
				break;
			case 'p':
				port = atoi(optarg);
				DEBUG("arg %s", optarg);
				break;
			case 'c':
				strcpy(certDirectory, optarg);
				DEBUG("cert root directory %s", optarg);
				break;
			case 'n':
				numPubs = atoi(optarg);
				DEBUG("num pubs %s", optarg);
				break;
			case '?':
				if (optopt == 'c') {
					ERROR("Option -%c requires an argument.", optopt);
				} else if (isprint(optopt)) {
					WARN("Unknown option `-%c'.", optopt);
				} else {
					WARN("Unknown option character `\\x%x'.", optopt);
				}
				break;
			default:
				ERROR("ERROR in command line argument parsing");
				break;
		}
	}

}

#define MAX_LENGTH_OF_UPDATE_JSON_BUFFER 200


int main(int argc, char** argv) {
	IoT_Error_t rcp = NONE_ERROR;
    IoT_Error_t rc = NONE_ERROR;
	int32_t i = 0;
	char macAddress[50];
	char memInfo[5000];

/**
 * @brief shadow
 */
	MQTTClient_t mqttClient;
	aws_iot_mqtt_init(&mqttClient);

	char JsonDocumentBuffer[MAX_LENGTH_OF_UPDATE_JSON_BUFFER];
	size_t sizeOfJsonDocumentBuffer = sizeof(JsonDocumentBuffer) / sizeof(JsonDocumentBuffer[0]);
	char *pJsonStringToUpdate;
	float temperature = 0.0;

	bool windowOpen = false;
	jsonStruct_t windowActuator;
	windowActuator.cb = windowActuate_Callback;
	windowActuator.pData = &windowOpen;
	windowActuator.pKey = "windowOpen";
	windowActuator.type = SHADOW_JSON_BOOL;

	jsonStruct_t temperatureHandler;
	temperatureHandler.cb = NULL;
	temperatureHandler.pKey = "temperature";
	temperatureHandler.pData = &temperature;
	temperatureHandler.type = SHADOW_JSON_FLOAT;


	char rootCA[PATH_MAX + 1];
	char clientCRT[PATH_MAX + 1];
	char clientKey[PATH_MAX + 1];
	char CurrentWD[PATH_MAX + 1];
	char cafileName[] = AWS_IOT_ROOT_CA_FILENAME;
	char clientCRTName[] = AWS_IOT_CERTIFICATE_FILENAME;
	char clientKeyName[] = AWS_IOT_PRIVATE_KEY_FILENAME;

	parseInputArgsForConnectParams(argc, argv);

	INFO("\nAWS IoT SDK Version %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

	getcwd(CurrentWD, sizeof(CurrentWD));
	sprintf(rootCA, "%s/%s/%s", CurrentWD, certDirectory, cafileName);
	sprintf(clientCRT, "%s/%s/%s", CurrentWD, certDirectory, clientCRTName);
	sprintf(clientKey, "%s/%s/%s", CurrentWD, certDirectory, clientKeyName);


/**
 * @brief Define first topic with MAC address
 */
	char topic1[100];
	sprintf(topic1, "%s/%s/%s", "sensorgruppe21", getMacAddress(macAddress), "topic_1");

	char topic2[100];
	sprintf(topic2, "%s/%s/%s", "sensorgruppe21", getMacAddress(macAddress), "topic_2");


	DEBUG("rootCA %s", rootCA);
	DEBUG("clientCRT %s", clientCRT);
	DEBUG("clientKey %s", clientKey);
	DEBUG("MAC address: %s", getMacAddress(macAddress));
	DEBUG("Topic1: %s", topic1);
	DEBUG("Topic2: %s", topic2);

/**
 * @brief shadow
 */
	ShadowParameters_t sp = ShadowParametersDefault;
	sp.pMyThingName = AWS_IOT_MY_THING_NAME;
	sp.pMqttClientId = AWS_IOT_MQTT_CLIENT_ID;
	sp.pHost = HostAddress;
	sp.port = port;
	sp.pClientCRT = clientCRT;
	sp.pClientKey = clientKey;
	sp.pRootCA = rootCA;


    INFO("Shadow Init");
	rc = aws_iot_shadow_init(&mqttClient);

	INFO("Shadow Connect");
	rc = aws_iot_shadow_connect(&mqttClient, &sp);

	if (NONE_ERROR != rc) {
		ERROR("Shadow Connection Error %d", rc);
	}


    /*
     * Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
     *  #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
     *  #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
     */
	rc = mqttClient.setAutoReconnectStatus(true);
	if (NONE_ERROR != rc) {
		ERROR("Unable to set Auto Reconnect to true - %d", rc);
		return rc;
	}

	rc = aws_iot_shadow_register_delta(&mqttClient, &windowActuator);

	if (NONE_ERROR != rc) {
		ERROR("Shadow Register Delta Error");
	}
	temperature = STARTING_ROOMTEMPERATURE;




/**
 * @brief Connection
 */
	MQTTConnectParams connectParams = MQTTConnectParamsDefault;

	connectParams.KeepAliveInterval_sec = 10;
	connectParams.isCleansession = true;
	connectParams.MQTTVersion = MQTT_3_1_1;
	connectParams.pClientID = HostAddress; //"CSDK-test-device"
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
	/**
    //Ab hier Auskommentieren
    MQTTSubscribeParams subParams = MQTTSubscribeParamsDefault;
    subParams.mHandler = MQTTcallbackHandler;
    subParams.pTopic = topic;
    subParams.qos = QOS_0;

    if (NONE_ERROR == rc) {
        INFO("Subscribing...");
        rc = aws_iot_mqtt_subscribe(&subParams);
        if (NONE_ERROR != rc) {
            ERROR("Error subscribing");
        }
    }
    //Bis hier Auskommentieren
     */


/**
 * @brief Publish to a topic
 */

	MQTTMessageParams Msg = MQTTMessageParamsDefault;
	Msg.qos = QOS_0;
	char cPayload[131000];
	Msg.pPayload = (void *) cPayload;

	MQTTPublishParams Params = MQTTPublishParamsDefault;
	Params.pTopic = topic1;


	MQTTMessageParams Msg2 = MQTTMessageParamsDefault;
	Msg2.qos = QOS_0;
	char cPayload2[131000];
	Msg2.pPayload = (void *) cPayload2;

	MQTTPublishParams Params2 = MQTTPublishParamsDefault;
	Params2.pTopic = topic2;




		// loop and publish a change in temperature
		while (NETWORK_ATTEMPTING_RECONNECT == rc || RECONNECT_SUCCESSFUL == rc || NONE_ERROR == rc) {
			rc = aws_iot_shadow_yield(&mqttClient, 200);
			if (NETWORK_ATTEMPTING_RECONNECT == rc) {
				sleep(1);
				// If the client is attempting to reconnect we will skip the rest of the loop.
				continue;
			}
			INFO("\n=======================================================================================\n");
			INFO("On Device: window state %s", windowOpen ? "true" : "false");
			simulateRoomTemperature(&temperature);
			//printf("Return Variable windowOpen: %s\n", windowOpen ? "true" : "false");
			//sprintf(cPayload,"%s", getMemInfo(memInfo));

			rc = aws_iot_shadow_init_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
			if (rc == NONE_ERROR) {
				rc = aws_iot_shadow_add_reported(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 2, &temperatureHandler,
												 &windowActuator);
				if (rc == NONE_ERROR) {
					rc = aws_iot_finalize_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
					if (rc == NONE_ERROR) {
						INFO("Update Shadow: %s", JsonDocumentBuffer);
						rc = aws_iot_shadow_update(&mqttClient, AWS_IOT_MY_THING_NAME, JsonDocumentBuffer,
												   ShadowUpdateStatusCallback, NULL, 4, true);
					}
				}
			}
			INFO("*****************************************************************************************\n");
			sleep(1);
		}

		if (NONE_ERROR != rc) {
			ERROR("An error occurred in the loop %d", rc);
		}

		INFO("Disconnecting");
		rc = aws_iot_shadow_disconnect(&mqttClient);

		if (NONE_ERROR != rc) {
			ERROR("Disconnect error %d", rc);


	}

/**
	//while ((NETWORK_ATTEMPTING_RECONNECT == rc || RECONNECT_SUCCESSFUL == rc || NONE_ERROR == rc) && (windowOpen == true)) {
    while ((NETWORK_ATTEMPTING_RECONNECT == rc || RECONNECT_SUCCESSFUL == rc || NONE_ERROR == rc)) {

		//Max time the yield function will wait for read messages
		rc = aws_iot_mqtt_yield(100);
		if (NETWORK_ATTEMPTING_RECONNECT == rc) {
			INFO("-->sleep");
			sleep(1);
			// If the client is attempting to reconnect we will skip the rest of the loop.
			continue;
		}
		//INFO("-->sleep");
		sleep(1);
		//sprintf(cPayload, "%s : %d ", "hello from SDK xxxxxxxxxx", i++);
		//sprintf(cPayload,"%s%d%s", "{\"Firma\": \"Hardkernel\", \"Sensor\": \"Odroid-C2\", \"Messwert\": \"", i++, "\"}");

		//printf("%s", getMemInfo(memInfo));
		sprintf(cPayload, "%s", getMemInfo(memInfo));
		printf("Payload length 1st topic: %d Byte\n", strlen(cPayload));
		Msg.PayloadLen = strlen(cPayload) + 1;
		Params.MessageParams = Msg;
		rc = aws_iot_mqtt_publish(&Params);

		sprintf(cPayload2, "%s%d%s", "{\"Firma\": \"Hardkernel\", \"Sensor\": \"Odroid-C2\", \"Messwert\": \"", i++,
				"\"}");
		printf("Payload length 2nd topic: %d Byte\n", strlen(cPayload2));
		printf("Max payload length each topic: %d Byte\n\n", sizeof(cPayload2));
		Msg2.PayloadLen = strlen(cPayload2) + 1;
		Params2.MessageParams = Msg2;
		rc = aws_iot_mqtt_publish(&Params2);
	}


	if (NONE_ERROR != rc) {
		ERROR("An error occurred in the loop.\n");
	} else {
		INFO("Publish done\n");
	}*/



	return rc;
}

