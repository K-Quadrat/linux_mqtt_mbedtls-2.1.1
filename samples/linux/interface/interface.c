#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <linux/limits.h>
#include <string.h>

#include "aws_iot_config.h"

#include "aws_iot_error.h"
#include "aws_iot_mqtt_client.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_shadow_interface.h"




#define ROOMTEMPERATURE_UPPERLIMIT 32.0f
#define ROOMTEMPERATURE_LOWERLIMIT 25.0f
#define STARTING_ROOMTEMPERATURE ROOMTEMPERATURE_LOWERLIMIT

#define MAX_LENGTH_OF_UPDATE_JSON_BUFFER 200
uint8_t numPubs = 5;

static void simulateRoomTemperature(float *pRoomTemperature) {
    static float deltaChange;

    if(*pRoomTemperature >= ROOMTEMPERATURE_UPPERLIMIT) {
        deltaChange = -0.5f;
    } else if(*pRoomTemperature <= ROOMTEMPERATURE_LOWERLIMIT) {
        deltaChange = 0.5f;
    }

    *pRoomTemperature += deltaChange;
}

void ShadowUpdateStatusCallback(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
                                const char *pReceivedJsonDocument, void *pContextData) {
    IOT_UNUSED(pThingName);
    IOT_UNUSED(action);
    IOT_UNUSED(pReceivedJsonDocument);
    IOT_UNUSED(pContextData);

    if(SHADOW_ACK_TIMEOUT == status) {
        IOT_INFO("Update Timeout--");
    } else if(SHADOW_ACK_REJECTED == status) {
        IOT_INFO("Update RejectedXX");
    } else if(SHADOW_ACK_ACCEPTED == status) {
        IOT_INFO("Update Accepted !!");
    }
}

void windowActuate_Callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
    IOT_UNUSED(pJsonString);
    IOT_UNUSED(JsonStringDataLen);

    if(pContext != NULL) {
        IOT_INFO("Delta - Window state changed to %d", *(bool *) (pContext->pData));
    }
}



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

/**
 * @brief This parameter will avoid infinite loop of publish and exit the program after certain number of publishes
 */
uint32_t publishCount = 0; // publishCount wird nicht gebraucht


/**
 * @brief CallbackHandler for Subscribe to a topic
 */
/**
void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
									IoT_Publish_Message_Params *params, void *pData) {
	IOT_UNUSED(pData);
	IOT_UNUSED(pClient);
	IOT_INFO("Subscribe callback");
	IOT_INFO("%.*s\t%.*s", topicNameLen, topicName, (int) params->payloadLen, params->payload);
}*/

void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data) {
	IOT_WARN("MQTT Disconnect");
	IoT_Error_t rc = FAILURE;

	if(NULL == pClient) {
		return;
	}

	IOT_UNUSED(data);

	if(aws_iot_is_autoreconnect_enabled(pClient)) {
		IOT_INFO("Auto Reconnect is enabled, Reconnecting attempt will start now");
	} else {
		IOT_WARN("Auto Reconnect not enabled. Starting manual reconnect...");
		rc = aws_iot_mqtt_attempt_reconnect(pClient);
		if(NETWORK_RECONNECTED == rc) {
			IOT_WARN("Manual Reconnect Successful");
		} else {
			IOT_WARN("Manual Reconnect Failed - %d", rc);
		}
	}
}

void parseInputArgsForConnectParams(int argc, char **argv) {
	int opt;

	while(-1 != (opt = getopt(argc, argv, "h:p:c:x:n"))) {
		switch(opt) {
			case 'h':
				strcpy(HostAddress, optarg);
				IOT_DEBUG("Host %s", optarg);
				break;
			case 'p':
				port = atoi(optarg);
				IOT_DEBUG("arg %s", optarg);
				break;
			case 'c':
				strcpy(certDirectory, optarg);
				IOT_DEBUG("cert root directory %s", optarg);
				break;
			case 'x':
				publishCount = atoi(optarg);
				IOT_DEBUG("publish %s times\n", optarg);
				break;
            case 'n':
                numPubs = atoi(optarg);
                IOT_DEBUG("num pubs %s", optarg);
                break;
			case '?':
				if(optopt == 'c') {
					IOT_ERROR("Option -%c requires an argument.", optopt);
				} else if(isprint(optopt)) {
					IOT_WARN("Unknown option `-%c'.", optopt);
				} else {
					IOT_WARN("Unknown option character `\\x%x'.", optopt);
				}
				break;
			default:
				IOT_ERROR("Error in command line argument parsing");
				break;
		}
	}

}

int main(int argc, char **argv) {
	bool infinitePublishFlag = true;

	char rootCA[PATH_MAX + 1];
	char clientCRT[PATH_MAX + 1];
	char clientKey[PATH_MAX + 1];
	char CurrentWD[PATH_MAX + 1];
	char cPayload[131000];

	int32_t i = 0;
    char macAddress[50];
    char memInfo[5000];

    char topicHardwareInfo[100]; // Defines a string for the first topic for the hardware information
    char topicMemInfo[100]; // Defines a string for the second topic for the memory information

    int topicHardwareInfoLen;
    int topicMemInfoLen;

	IoT_Error_t rc = FAILURE;

	AWS_IoT_Client mqttClient;
    AWS_IoT_Client shadowClient;
	IoT_Client_Init_Params clientInitParams = iotClientInitParamsDefault; // Defining a type for MQTT initialization parameters
	IoT_Client_Connect_Params clientConnectParams = iotClientConnectParamsDefault; // Defining a type for MQTT connection parameters
    ShadowInitParameters_t shadowInitParams = ShadowInitParametersDefault; // Defining a type for shadow initialization parameters
    ShadowConnectParameters_t shadowConnectParams = ShadowConnectParametersDefault; // Defining a type for shadow connection parameters


/**
 * @brief Defines a type for MQTT Publish messages. Used for both incoming and out going messages
 */
    /**
	IoT_Publish_Message_Params paramsQOS0;
	IoT_Publish_Message_Params paramsQOS1;
*/
    IoT_Publish_Message_Params msg;


	parseInputArgsForConnectParams(argc, argv);

	IOT_INFO("\nAWS IoT SDK Version %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

	getcwd(CurrentWD, sizeof(CurrentWD));
	snprintf(rootCA, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_ROOT_CA_FILENAME);
	snprintf(clientCRT, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_CERTIFICATE_FILENAME);
	snprintf(clientKey, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_PRIVATE_KEY_FILENAME);

	IOT_DEBUG("rootCA %s", rootCA);
	IOT_DEBUG("clientCRT %s", clientCRT);
	IOT_DEBUG("clientKey %s", clientKey);


/**
 * @brief Shadow stuff
 */
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


/**
 * @brief Defining shadow initialization parameters
 */
    shadowInitParams.pHost = AWS_IOT_MQTT_HOST;
    shadowInitParams.port = AWS_IOT_MQTT_PORT;
    shadowInitParams.pClientCRT = clientCRT;
    shadowInitParams.pClientKey = clientKey;
    shadowInitParams.pRootCA = rootCA;
    shadowInitParams.enableAutoReconnect = false;
    shadowInitParams.disconnectHandler = disconnectCallbackHandler;


/**
 * @brief Called to initialize the shadow
 * @return IoT_Error_t Type defining successful/failed API call
 */
    IOT_INFO("Shadow Init");
    rc = aws_iot_shadow_init(&shadowClient, &shadowInitParams);
    if(SUCCESS != rc) {
        IOT_ERROR("Shadow Connection Error");
        return rc;
    }


/**
* @brief Defining shadow connection parameters.
*/
    shadowConnectParams.pMyThingName = AWS_IOT_MY_THING_NAME;
    shadowConnectParams.pMqttClientId = AWS_IOT_THING_CLIENT_ID;
    shadowConnectParams.mqttClientIdLen = (uint16_t) strlen(AWS_IOT_THING_CLIENT_ID);


/**
 * @brief Called to establish an shadow connection with the AWS IoT Service
 * @return An IoT Error Type defining successful/failed connection
 */
    IOT_INFO("Shadow Connect");
    rc = aws_iot_shadow_connect(&shadowClient, &shadowConnectParams);
    if(SUCCESS != rc) {
        IOT_ERROR("Shadow Connection Error");
        return rc;
    }


/**
 * @brief Enable Auto Reconnect functionality.
 * #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
 * #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
 * @return IoT_Error_t Type defining successful/failed API call
 */
    rc = aws_iot_shadow_set_autoreconnect_status(&shadowClient, true);
    if(SUCCESS != rc) {
        IOT_ERROR("Unable to set Auto Reconnect to true - %d", rc);
        return rc;
    }


/**
 * @brief Defining MQTT initialization parameters. Passed into client when to initialize the client
 */
    clientInitParams.enableAutoReconnect = false; // We enable this later below
    clientInitParams.pHostURL = HostAddress;
    clientInitParams.port = port;
    clientInitParams.pRootCALocation = rootCA;
    clientInitParams.pDeviceCertLocation = clientCRT;
    clientInitParams.pDevicePrivateKeyLocation = clientKey;
    clientInitParams.mqttCommandTimeout_ms = 20000;
    clientInitParams.tlsHandshakeTimeout_ms = 5000;
    clientInitParams.isSSLHostnameVerify = true;
    clientInitParams.disconnectHandler = disconnectCallbackHandler;
    clientInitParams.disconnectHandlerData = &mqttClient;

/**
 * @brief Called to initialize the MQTT Client
 * @return IoT_Error_t Type defining successful/failed API call
 */
	rc = aws_iot_mqtt_init(&mqttClient, &clientInitParams);
	if(SUCCESS != rc) {
		IOT_ERROR("aws_iot_mqtt_init returned error : %d ", rc);
		return rc;
	}

/**
 * @brief Defining MQTT connection parameters. Passed into client when establishing a connection.
 */
	clientConnectParams.keepAliveIntervalInSec = 10;
    clientConnectParams.isCleanSession = true;
    clientConnectParams.MQTTVersion = MQTT_3_1_1;
    clientConnectParams.pClientID = AWS_IOT_MQTT_CLIENT_ID; // Or HostAddress
    clientConnectParams.clientIDLen = (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID);
    clientConnectParams.isWillMsgPresent = false;

/**
 * @brief Called to establish an MQTT connection with the AWS IoT Service This is the outer function
 * which does the validations and calls the internal connect above to perform the actual operation.
 * It is also responsible for client state changes
 * @return An IoT Error Type defining successful/failed connection
 */
	IOT_INFO("Connecting...");
	rc = aws_iot_mqtt_connect(&mqttClient, &clientConnectParams);

    if(SUCCESS != rc) {
		IOT_ERROR("Error(%d) connecting to %s:%d", rc, clientInitParams.pHostURL, clientInitParams.port);
		return rc;
	}

/**
 * @brief Enable Auto Reconnect functionality. Minimum and Maximum time of Exponential backoff are set in aws_iot_config.h
 * #AWS_IOT_MQTT_MIN_RECONNECT_WAIT_INTERVAL
 * #AWS_IOT_MQTT_MAX_RECONNECT_WAIT_INTERVAL
 * @return IoT_Error_t Type defining successful/failed API call
 */
	rc = aws_iot_mqtt_autoreconnect_set_status(&mqttClient, true);
	if(SUCCESS != rc) {
		IOT_ERROR("Unable to set Auto Reconnect to true - %d", rc);
		return rc;
	}

/**
 * @brief Called to send a subscribe message to the broker requesting a subscription to an MQTT topic.
 * This is the outer function which does the validations and calls the internal subscribe above to perform the actual operation.
 * It is also responsible for client state changes
 * @return An IoT Error Type defining successful/failed subscription
 */
    /**
	IOT_INFO("Subscribing...");
	rc = aws_iot_mqtt_subscribe(&client, "sdkTest/sub", 11, QOS0, iot_subscribe_callback_handler, NULL);
	if(SUCCESS != rc) {
		IOT_ERROR("Error subscribing : %d ", rc);
		return rc;
	}*/

	//sprintf(cPayload, "%s : %d ", "hello from SDK", i);

/**
 * @brief Defines the topics with MAC address
 */
    sprintf(topicHardwareInfo, "%s/%s/%s","sensorgruppe21",getMacAddress(macAddress),"hardwareInfo");
    sprintf(topicMemInfo, "%s/%s/%s","sensorgruppe21",getMacAddress(macAddress),"memInfo");

    topicHardwareInfoLen = strlen(topicHardwareInfo); // Length of the topic for aws_iot_mqtt_publish function
    topicMemInfoLen = strlen(topicMemInfo);


    IOT_DEBUG("MAC address: %s", getMacAddress(macAddress));
    IOT_DEBUG("Topic1: %s", topicHardwareInfo);
    IOT_DEBUG("Topic2: %s", topicMemInfo);
/**
	paramsQOS0.qos = QOS0;
	paramsQOS0.payload = (void *) cPayload;
	paramsQOS0.isRetained = 0;

	paramsQOS1.qos = QOS1;
	paramsQOS1.payload = (void *) cPayload;
	paramsQOS1.isRetained = 0;
*/
    msg.qos = QOS1;
    msg.payload = (void *) cPayload;
    msg.isRetained = 0;

	if(publishCount != 0) {
		infinitePublishFlag = false;
	}

/**
 * @brief Any time a delta is published the Json document will be delivered to the pStruct->cb.
 * @return An IoT Error Type defining successful/failed delta registering
 */
    rc = aws_iot_shadow_register_delta(&shadowClient, &windowActuator);

    if(SUCCESS != rc) {
        IOT_ERROR("Shadow Register Delta Error");
    }


    temperature = STARTING_ROOMTEMPERATURE;


    // loop and publish
	while((NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc)
		  && (publishCount > 0 || infinitePublishFlag)) {

		//Max time the yield function will wait for read messages
		rc = aws_iot_mqtt_yield(&mqttClient, 100);
		if(NETWORK_ATTEMPTING_RECONNECT == rc) {
			// If the client is attempting to reconnect we will skip the rest of the loop.
			continue;
		}

		sleep(1);

        sprintf(cPayload,"%s%d%s", "{\"Firma\": \"Hardkernel\", \"Sensor\": \"Odroid-C2\", \"Messwert\": \"", i++, "\"}");
		msg.payloadLen = strlen(cPayload);
        printf("%s %s%s %zu %s\n", "Payload length topic", topicHardwareInfo, ":", strlen(cPayload), "Byte");
		rc = aws_iot_mqtt_publish(&mqttClient, topicHardwareInfo, topicHardwareInfoLen, &msg);


        sprintf(cPayload,"%s", getMemInfo(memInfo));
		msg.payloadLen = strlen(cPayload);
        printf("%s %s%s %zu %s\n", "Payload length topic", topicMemInfo, ":", strlen(cPayload), "Byte");
        rc = aws_iot_mqtt_publish(&mqttClient, topicMemInfo, topicMemInfoLen, &msg);

        printf("%s %zu %s\n\n", "Max payload length each topic:", sizeof(cPayload), "Byte");


        /*
        if (rc == MQTT_REQUEST_TIMEOUT_ERROR) {
			IOT_WARN("QOS1 publish ack not received.\n");
			rc = SUCCESS;
		}*/
        /**
		if(publishCount > 0) {
			publishCount--;
		}*/


        rc = aws_iot_shadow_yield(&shadowClient, 200);
        if(NETWORK_ATTEMPTING_RECONNECT == rc) {
            sleep(1);
            // If the client is attempting to reconnect we will skip the rest of the loop.
            continue;
        }
        IOT_INFO("\n=======================================================================================\n");
        IOT_INFO("On Device: window state %s", windowOpen ? "true" : "false");
        simulateRoomTemperature(&temperature);

        rc = aws_iot_shadow_init_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
        if(SUCCESS == rc) {
            rc = aws_iot_shadow_add_reported(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 2, &temperatureHandler,
                                             &windowActuator);
            if(SUCCESS == rc) {
                rc = aws_iot_finalize_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
                if(SUCCESS == rc) {
                    IOT_INFO("Update Shadow: %s", JsonDocumentBuffer);
                    rc = aws_iot_shadow_update(&shadowClient, AWS_IOT_MY_THING_NAME, JsonDocumentBuffer,
                                               ShadowUpdateStatusCallback, NULL, 4, true);

                }
            }
        }
        IOT_INFO("*****************************************************************************************\n");
        sleep(1);






	}

	if(SUCCESS != rc) {
		IOT_ERROR("An error occurred in the loop.\n");
	} else {
		IOT_INFO("Publish done\n");
	}

	return rc;
}
