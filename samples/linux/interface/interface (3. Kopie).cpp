#define _GLIBCXX_USE_CXX11_ABI 0
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <linux/limits.h>
#include <string.h>
#include <pthread.h>
#include <iostream>
#include <map>


#include "aws_iot_config.h"

#include "aws_iot_error.h"
#include "aws_iot_mqtt_client.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_shadow_interface.h"

#include "boost/property_tree/ptree.hpp"
#include "boost/property_tree/json_parser.hpp"
#include <boost/foreach.hpp>
#include <thread>         // std::thread



#define SUBTOPIC1 "get/measurement/settings/channel1"
#define SUBTOPIC2 "get/measurement/settings/channel2"
#define SUBTOPIC3 "get/measurement/settings/channel3"
#define MAX_LENGTH_OF_UPDATE_JSON_BUFFER 500
#define _ENABLE_THREAD_SUPPORT_

using namespace std;
using namespace boost::property_tree;



AWS_IoT_Client mqttClient;
AWS_IoT_Client shadowClient;
//char receivedSettings[3][1000]; // 1. Number of topics, 3. Playload

uint8_t numPubs = 5;


string writeJSON(boost::property_tree::ptree const& pt) {
    //convert property tree to JSON encoded string
    stringstream outputstring;
    outputstring << "{" << endl;
    ptree::const_iterator end = pt.end();
    for (ptree::const_iterator it = pt.begin(); it != end; ++it) {
        outputstring << "\"" << it->first << "\":\"" << it->second.get_value<std::string>() << "\"";
        if (boost::next(it) != end) outputstring << "," << endl;
    }
    outputstring << endl << "}" << endl;
    return outputstring.str();
}


void printJSON(boost::property_tree::ptree const& pt) {

    //print property tree (JSON encoded Data)
    stringstream outputstring;
    ptree::const_iterator end = pt.end();
    for (ptree::const_iterator it = pt.begin(); it != end; ++it) {
        outputstring << "\"" << it->first << "\":\"" << it-> second.get_value<std::string>() << "\"" << endl;
    }
    cout << outputstring.str();
}


ptree readJSON(string inputstring) {
    //convert JSON encoded inputstring to ptree
    ptree pt;

    //check for error before processing
//    if (errorcheck() != 0) return pt;

    try {
        //convert JSON to property tree
        std::istringstream is(inputstring);
        boost::property_tree::read_json(is, pt);
    }
    catch(boost::property_tree::json_parser::json_parser_error &je) {
        //catch exceptions
//        CLOG(ERROR, "JSON") << "Failed: readJSON: " << je.message();
//        pthread_mutex_lock(&errorMutex);
//        ::readJSONerror = -1;
//        pthread_mutex_unlock(&errorMutex);
        cout << "ERROR readJSON" << "\n";
    }

    return pt;
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


/**
 * @brief Run command to run command with options
 */
int runCommand(char* in) {

    char thisIn [100];
    char buffer [10000];
    sprintf(thisIn, "%s", in);
    char out [10000];

    char delimiter[] = ";";
    char *ptr;

    int32_t i = 0;
    char commandAndOptions[100][10];

// initialisieren und ersten Abschnitt erstellen
    ptr = strtok(thisIn, delimiter);

    while(ptr != NULL) {
        sprintf(commandAndOptions[i], "%s", ptr);
        i++;

        // naechsten Abschnitt erstellen
        ptr = strtok(NULL, delimiter);
    }


    FILE *fp;

    char blank[] = " ";

    char commandRun[80];

    strcpy(commandRun, commandAndOptions[0]);
    strcat(commandRun, blank);
    strcat(commandRun, commandAndOptions[1]);


    /* Open the command for reading. */
    fp = popen(commandRun, "r");
    if (fp == NULL) {
        printf("Failed to run command\n" );
        exit(1);
    }


    /* Read the output a line at a time - output to out. */
    *out = 0; // reset output variable

    while (fgets(buffer, sizeof(buffer)-1, fp) != NULL) {

        if (strlen(out) == 0) {
            strcpy(out, buffer);
        } else
            strcat(out, buffer);
    }

    pclose(fp); // close

    printf("%s\n", commandRun);
//    printf("%s\n", out);


    return 1;
}


/**
 * @brief CallbackHandler for Subscribe to a topic
 */
void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                    IoT_Publish_Message_Params *params, void *pData) {
    IOT_UNUSED(pData);
    IOT_UNUSED(pClient);
    char thisTopicName[100];
    char thisPayload[params->payloadLen];

    IOT_INFO("Subscribe callback, i'm here");
    IOT_INFO("%.*s\t%.*s", topicNameLen, topicName, (int) params->payloadLen, (char*)params->payload);

/*

    printf("***%.*s***", topicNameLen, topicName);

    sprintf(thisTopicName, "%.*s", topicNameLen, topicName);

    sprintf(receivedSettings[0], "%.*s", (int) params->payloadLen, (char*)params->payload);
*/

    char receivedSettings[10000];
    sprintf(receivedSettings, "%.*s", (int) params->payloadLen, (char*)params->payload);

    std::thread threadRunCommand (runCommand,receivedSettings);
    threadRunCommand.detach();

/*
    //debug
    printf("%s\n", "1");
    printf("%s\n", receivedSettings[0]);


    else if(strcmp(SUBTOPIC2, thisTopicName) == 0){
        sprintf(receivedSettings[1], "%.*s", (int) params->payloadLen, (char*)params->payload);

        //debug
        printf("%s\n", "2");
        printf("%s\n", receivedSettings[1]);

    }
    else if(strcmp(SUBTOPIC3, thisTopicName) == 0){
        sprintf(receivedSettings[2], "%.*s", (int) params->payloadLen, (char*)params->payload);

        //debug
        printf("%s\n", "3");
        printf("%s\n", receivedSettings[2]);
    }*/
}


/**
 * @brief Callback handler for delta changes
 */
void onlineState_Callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
    IOT_UNUSED(pJsonString);
    IOT_UNUSED(JsonStringDataLen);
    IoT_Error_t rc = FAILURE;

    if(pContext != NULL) {
        IOT_INFO("Online state changed to %d", *(bool *) (pContext->pData));
    }
}


char* myStringCopyAusnahme(char* dest, const char* source){

    int i, idx=0;
    for(i=0; i<=strlen(source); i++, idx++){
        if(source[i] != '\\'){      // überprüfung ob zeichen aus bei source[i] ungleich der ausnahme
            dest[idx]=source[i];// wenn ja, dann kopiere
        }
        else{                // wenn nicht
            idx--;           // reduziere idx um lücke in dest zu schliessen
        }
    }

    return dest;
}


void channels_Callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
    IOT_UNUSED(pJsonString);
    IOT_UNUSED(JsonStringDataLen);
    IoT_Error_t rc = FAILURE;

    char pData [10000];
    sprintf(pData, "%s", (char*)pContext->pData);
    char dest [10000];


    myStringCopyAusnahme(dest, pData);


//
//    printf("\n%s%s\n", "***** Output channels pContext->pData ***** ",(char*)pContext->pData);
//
//    printf("\n%s%s\n", "***** Output dest ***** ",dest);


    char jsonToRead [10000];
    sprintf(jsonToRead, "%s", "{\"channels\":");

    strcat(jsonToRead, dest);
    strcat(jsonToRead, "}");

//    printf("\n%s%s\n", "***** Output jsonToRead ***** ",jsonToRead);


    ptree pt;
    pt = readJSON(jsonToRead);


    BOOST_FOREACH(const ptree::value_type &v, pt.get_child("channels")) {

                    if (v.second.get<string>("subscribed").compare("true") == 0){

                        const char * channelNameTrue = v.second.get<string>("name").c_str();

                        IOT_INFO("Subscribing %s", channelNameTrue);
                        aws_iot_mqtt_unsubscribe(&mqttClient, channelNameTrue, strlen(channelNameTrue));
                        sleep(1);
                        rc = aws_iot_mqtt_subscribe(&mqttClient, channelNameTrue, strlen(channelNameTrue), QOS0, iot_subscribe_callback_handler, NULL);
                        sleep(1);

                        while(SUCCESS != rc) {
                            IOT_ERROR("error subscribing, repeating... : %d ", rc);
                            aws_iot_mqtt_unsubscribe(&mqttClient, channelNameTrue, strlen(channelNameTrue));
                            sleep(1);
                            rc = aws_iot_mqtt_subscribe(&mqttClient, channelNameTrue, strlen(channelNameTrue), QOS0, iot_subscribe_callback_handler, NULL);
                            sleep(1);

                        }


/*                        if(SUCCESS != rc) {
                            IOT_ERROR("error subscribing, repeating... : %d ", rc);
//                            sleep(2);
//                            aws_iot_mqtt_unsubscribe(&mqttClient, channelNameTrue, strlen(channelNameTrue));
                            sleep(3);
                            rc = aws_iot_mqtt_subscribe(&mqttClient, channelNameTrue, strlen(channelNameTrue), QOS0, iot_subscribe_callback_handler, NULL);

                            if(SUCCESS != rc) {
                                IOT_ERROR("final error subscribing : %d ", rc);
                            }


                        }*/

                    }

                        else if (v.second.get<string>("subscribed").compare("false") == 0){

                            const char * channelNameFalse = v.second.get<string>("name").c_str();

                            IOT_INFO("Unsubscribing %s", channelNameFalse);
                            rc = aws_iot_mqtt_unsubscribe(&mqttClient, channelNameFalse, strlen(channelNameFalse));
                            sleep(1);

                        if(SUCCESS != rc) {
                                IOT_ERROR("Error unsubscribing : %d ", rc);
                            }
                    }


                    // v.first is the name of the child.
                    // v.second is the child tree.
                }

}

void groups_Callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
    IOT_UNUSED(pJsonString);
    IOT_UNUSED(JsonStringDataLen);
    IoT_Error_t rc = FAILURE;

    printf("\n%s%s\n", "***** Output groups pContext->pData ***** ",(char*)pContext->pData);

    ptree pt;
    pt = readJSON((char*)pContext->pData);


}

void thirdMeasurement_Callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
    IOT_UNUSED(pJsonString);
    IOT_UNUSED(JsonStringDataLen);
    IoT_Error_t rc = FAILURE;

    if(pContext != NULL && (*(bool *) (pContext->pData)) == true) {
        IOT_INFO("Activating the third measurement! State changed to %d", *(bool *) (pContext->pData));

        // obtain measurement settings on channel three
        IOT_INFO("Subscribing %s", SUBTOPIC3);
        rc = aws_iot_mqtt_subscribe(&mqttClient, SUBTOPIC3, 33, QOS0, iot_subscribe_callback_handler, NULL);

        if(SUCCESS != rc) {
            IOT_ERROR("Error subscribing : %d ", rc);
        }
    }
    else if(pContext != NULL && (*(bool *) (pContext->pData)) == false){
        IOT_INFO("Deactivating the third measurement! State changed to %d", *(bool *) (pContext->pData));

        // cancel the subscription on channel three
        IOT_INFO("Cancelling the subscription of %s", SUBTOPIC3);
        rc = aws_iot_mqtt_unsubscribe(&mqttClient, SUBTOPIC3, 33);

        if(SUCCESS != rc) {
            IOT_ERROR("Error unsubscribing : %d ", rc);
        }
    }
}


/**
 * @brief Filereader to read memory info from the System
 */

char* jsonParserMemInfo(char* out) {
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

/*
    while((fscanf(out,"%s",line)) != EOF ) {

    sprintf(names[i], "%s", line);
    i++;
}
*/


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
    fp = fopen("/sys/class/net/eth0/address", "r");
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

void *shadowRun(void *threadid) { //IoT_Error_t
    char rootCA[PATH_MAX + 1];
    char clientCRT[PATH_MAX + 1];
    char clientKey[PATH_MAX + 1];
    char CurrentWD[PATH_MAX + 1];
    int32_t i = 0;
    IoT_Error_t rc = FAILURE;

    ShadowInitParameters_t shadowInitParams = ShadowInitParametersDefault; // Defining a type for shadow initialization parameters
    ShadowConnectParameters_t shadowConnectParams = ShadowConnectParametersDefault; // Defining a type for shadow connection parameters

    getcwd(CurrentWD, sizeof(CurrentWD));
    snprintf(rootCA, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_ROOT_CA_FILENAME);
    snprintf(clientCRT, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_CERTIFICATE_FILENAME);
    snprintf(clientKey, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_PRIVATE_KEY_FILENAME);


/**
 * @brief Shadow stuff
 */
    char JsonDocumentBuffer[MAX_LENGTH_OF_UPDATE_JSON_BUFFER];
    size_t sizeOfJsonDocumentBuffer = sizeof(JsonDocumentBuffer) / sizeof(JsonDocumentBuffer[0]);
    char *pJsonStringToUpdate;
    float temperature = 0.0;

    char online = false;
    jsonStruct_t onlineState;
    onlineState.cb = NULL;
    onlineState.pData = &online;
    onlineState.pKey = "online";
    onlineState.type = SHADOW_JSON_BOOL;

    char customer[100] = "unknown";
    jsonStruct_t customerStruct;
//    firstMeasurement.cb = firstMeasurement_Callback;
    customerStruct.cb = NULL;
    customerStruct.pData = &customer;
    customerStruct.pKey = "customer";
    customerStruct.type = SHADOW_JSON_STRING;

    char location[100] = "unknown";
    jsonStruct_t locationStruct;
    locationStruct.cb = NULL;
    locationStruct.pData = &location;
    locationStruct.pKey = "location";
    locationStruct.type = SHADOW_JSON_STRING;

    char id[100] = "unknown";
    jsonStruct_t idStruct;
    idStruct.cb = NULL;
    idStruct.pData = &id;
    idStruct.pKey = "id";
    idStruct.type = SHADOW_JSON_STRING;

    char connection[100] = "unknown";
    jsonStruct_t connectionStruct;
    connectionStruct.cb = NULL;
    connectionStruct.pData = &connection;
    connectionStruct.pKey = "connection";
    connectionStruct.type = SHADOW_JSON_STRING;

    char groups[1000] = "unknown";
    jsonStruct_t groupsStruct;
    groupsStruct.cb = groups_Callback;
    groupsStruct.pData = &groups;
    groupsStruct.pKey = "groups";
    groupsStruct.type = SHADOW_JSON_STRING;

    char channels[1000] = "unknown";
    jsonStruct_t channelsStruct;
    channelsStruct.cb = channels_Callback;
    channelsStruct.pData = &channels;
    channelsStruct.pKey = "channels";
    channelsStruct.type = SHADOW_JSON_STRING;


/*
    bool thirdMeasurementActivated = false;
    jsonStruct_t thirdMeasurement;
    thirdMeasurement.cb = thirdMeasurement_Callback;
    thirdMeasurement.pData = &thirdMeasurementActivated;
    thirdMeasurement.pKey = "thirdMeasurementActivated";
    thirdMeasurement.type = SHADOW_JSON_BOOL;*/


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
//        return rc;
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
//        return rc;
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
//        return rc;
    }


/**
 * @brief Any time a delta is published the Json document will be delivered to the pStruct->cb.
 * @return An IoT Error Type defining successful/failed delta registering
 */
    rc = aws_iot_shadow_register_delta(&shadowClient, &onlineState);
    if(SUCCESS != rc) {
        IOT_ERROR("Shadow Register Delta Error");
    }

    rc = aws_iot_shadow_register_delta(&shadowClient, &customerStruct);
    if(SUCCESS != rc) {
        IOT_ERROR("Shadow Register Delta Error");
    }

    rc = aws_iot_shadow_register_delta(&shadowClient, &locationStruct);
    if(SUCCESS != rc) {
        IOT_ERROR("Shadow Register Delta Error");
    }

    rc = aws_iot_shadow_register_delta(&shadowClient, &idStruct);
    if(SUCCESS != rc) {
        IOT_ERROR("Shadow Register Delta Error");
    }

    rc = aws_iot_shadow_register_delta(&shadowClient, &connectionStruct);
    if(SUCCESS != rc) {
        IOT_ERROR("Shadow Register Delta Error");
    }
    rc = aws_iot_shadow_register_delta(&shadowClient, &groupsStruct);
    if(SUCCESS != rc) {
        IOT_ERROR("Shadow Register Delta Error");
    }
    rc = aws_iot_shadow_register_delta(&shadowClient, &channelsStruct);
    if(SUCCESS != rc) {
        IOT_ERROR("Shadow Register Delta Error");
    }


    // loop and publish
    while(NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc) {

        rc = aws_iot_shadow_yield(&shadowClient, 200);
        if(NETWORK_ATTEMPTING_RECONNECT == rc) {
            sleep(1);
            // If the client is attempting to reconnect we will skip the rest of the loop.
            continue;
        }
/*
        IOT_INFO("On Device: Online state %s", online ? "true" : "false");
        IOT_INFO("On Device: First measurement activation state -> %s", firstMeasurementActivated ? "true" : "false");
        IOT_INFO("On Device: Second measurement activation state -> %s", secondMeasurementActivated ? "true" : "false");
        IOT_INFO("On Device: Third measurement activation state -> %s", thirdMeasurementActivated ? "true" : "false");
*/

        rc = aws_iot_shadow_init_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer); // Initialize the JSON document with Shadow
        if(SUCCESS == rc) {
            rc = aws_iot_shadow_add_reported(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 7, // Add the reported section of the JSON document of jsonStruct_t.
                                             &onlineState, &customerStruct, &locationStruct, &idStruct, &connectionStruct, &groupsStruct, &channelsStruct);
            if(SUCCESS == rc) {
                rc = aws_iot_finalize_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer); // This function will automatically increment the client token every time this function is called.
                if(SUCCESS == rc) {
                    IOT_INFO("Update Shadow: %s", JsonDocumentBuffer);
                    rc = aws_iot_shadow_update(&shadowClient, AWS_IOT_MY_THING_NAME, JsonDocumentBuffer, // This function is the one used to perform an Update action to a Thing Name's Shadow.
                                               ShadowUpdateStatusCallback, NULL, 30, true);

                }
            }
        }
        if(SUCCESS != rc) {
            IOT_ERROR("An error occurred in the loop %d", rc);
        }


/*
        for(i=0; i<=sizeof(receivedSettings) / sizeof(receivedSettings[0]); i++) {

            if (strlen(receivedSettings[i]) != 0){
                printf("%s %zu\n", "Channel->", i+1);

            }
        }
*/

//      *receivedSettings[0] = 0;
        sleep(3);
    } // end of while



    IOT_INFO("Disconnecting shadow");
    rc = aws_iot_shadow_disconnect(&shadowClient); // This will close the underlying TCP connection, MQTT connection will also be closed

    if(SUCCESS != rc) {
        IOT_ERROR("Disconnect error %d", rc);
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
    char runCommandOut[10000];

    char pub1[100]; // Defines a string for the first topic to publish
    char topicHardwareInfo[100]; // Defines a string for the second topic to publish

    int pub1Len;
    int topicHardwareInfoLen;

	IoT_Error_t rc = FAILURE;
    pthread_t pThreadShadow;


/*
	AWS_IoT_Client mqttClient;
    AWS_IoT_Client shadowClient;
*/
	IoT_Client_Init_Params clientInitParams = iotClientInitParamsDefault; // Defining a type for MQTT initialization parameters
	IoT_Client_Connect_Params clientConnectParams = iotClientConnectParamsDefault; // Defining a type for MQTT connection parameters


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

/*
	IOT_DEBUG("rootCA %s", rootCA);
	IOT_DEBUG("clientCRT %s", clientCRT);
	IOT_DEBUG("clientKey %s", clientKey);
*/


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
 * @brief Defines the topics with MAC address
 */
    sprintf(pub1, "%s/%s/%s","sensorgruppe21",getMacAddress(macAddress),"pub1");
    sprintf(topicHardwareInfo, "%s/%s/%s","sensorgruppe21",getMacAddress(macAddress),"hardwareInfo");

    pub1Len = strlen(pub1);
    topicHardwareInfoLen = strlen(topicHardwareInfo); // Length of the topic for aws_iot_mqtt_publish function


    IOT_DEBUG("MAC address: %s", getMacAddress(macAddress));
    IOT_DEBUG("Topic1: %s", pub1);
    IOT_DEBUG("Topic2: %s", topicHardwareInfo);
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

    pthread_create (&pThreadShadow, NULL, shadowRun, (void *)1);

    // loop and publish
	while(1) {

        //Max time the yield function will wait for read messages
        rc = aws_iot_mqtt_yield(&mqttClient, 200);
        if(NETWORK_ATTEMPTING_RECONNECT == rc) {
            // If the client is attempting to reconnect we will skip the rest of the loop.
            continue;
        }


/*
        IOT_INFO("On Device: Online state %s", online ? "true" : "false");
        IOT_INFO("On Device: First measurement activation state -> %s", firstMeasurementActivated ? "true" : "false");
        IOT_INFO("On Device: Second measurement activation state -> %s", secondMeasurementActivated ? "true" : "false");
        IOT_INFO("On Device: Third measurement activation state -> %s", thirdMeasurementActivated ? "true" : "false");
*/


/*        for(i=0; i<sizeof(receivedSettings) / sizeof(receivedSettings[0]); i++) { //<=
//        for(i=0; i<=2; i++) {
//            printf("%s %zu\n", "i=", i);


                if (strlen(receivedSettings[i]) != 0){
                printf("%s %zu\n", "Channel", i+1);
                runCommand(receivedSettings[i], runCommandOut);
                printf("%s\n", runCommandOut);

                if (strcmp(receivedSettings[i], "cat;/proc/meminfo") == 0){
                    sprintf(cPayload,"%s", jsonParserMemInfo(runCommandOut));
                    msg.payloadLen = strlen(cPayload);
                    printf("%s %s%s %zu %s\n", "Payload length topic", pub1, ":", strlen(cPayload), "Byte");
                    rc = aws_iot_mqtt_publish(&mqttClient, pub1, pub1Len, &msg);

                    printf("%s %zu %s\n\n", "Max payload length each topic:", sizeof(cPayload), "Byte");
                }
                *receivedSettings[i] = 0;
                *runCommandOut = 0;
            }
        }*/

        sleep(1);
    } // end of while



    IOT_INFO("Disconnecting because of main");

	return rc;
}
