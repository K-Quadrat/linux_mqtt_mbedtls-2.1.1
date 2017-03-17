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
#include <vector>


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

#include <mysql.cpp>
#include <tool.cpp>


#define MAX_LENGTH_OF_UPDATE_JSON_BUFFER 500
#define _ENABLE_THREAD_SUPPORT_

using namespace std;
using namespace boost::property_tree;


char probe_id[50];    // AWS_IOT_MY_THING_NAME
char sub_pub_id[50];  // AWS_IOT_MQTT_CLIENT_ID
char shadow_id[50];   // AWS_IOT_THING_CLIENT_ID

AWS_IoT_Client mqttClient;
IoT_Publish_Message_Params msg;

AWS_IoT_Client shadowClient;

char data[100]; // Defines a string for the data topic to publish
int dataLen;

char topicExitStatus[100]; // Defines a string for the exit status topic to publish
int topicExitStatusLen;

char resultJSONcharGlob[AWS_IOT_MQTT_TX_BUF_LEN];


/**
 * @brief Removes all double points from a string
 * @param dest Destination string
 * @param source Source string
 * @return String without double points
 */
char* stringCopyExceptionDoublePoint(char* dest, const char* source){

    int i, idx=0;
    for(i=0; i<=strlen(source); i++, idx++){
        if(source[i] != ':'){
            dest[idx]=source[i];
        }
        else{
            idx--;
        }
    }

    return dest;
}


/**
 * @brief Read the mac address from the system
 * @param out Destination string
 * @return Mac address
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
char certDirectory[PATH_MAX + 1] = "certs";

/**
 * @brief Default MQTT HOST URL is pulled from the aws_iot_config.h
 */
char HostAddress[255] = AWS_IOT_MQTT_HOST;

/**
 * @brief Default MQTT port is pulled from the aws_iot_config.h
 */
uint32_t port = AWS_IOT_MQTT_PORT;


/**
 * @brief Callback Handler for the case of a mqtt disconnect
 * @param mqttClient MQTT client
 * @param data pointer to the data (JSON value)
 */
void disconnectCallbackHandlerMqtt(AWS_IoT_Client *mqttClient, void *data) {
    IOT_WARN("MQTT Client Disconnect");
    IoT_Error_t rc = FAILURE;

    if(NULL == mqttClient) {
        return;
    }

    IOT_UNUSED(data);

    if(aws_iot_is_autoreconnect_enabled(mqttClient)) {
        IOT_INFO("Auto Reconnect is enabled, Reconnecting attempt will start now");
    } else {
        IOT_WARN("Auto Reconnect not enabled. Starting manual reconnect...");
        rc = aws_iot_mqtt_attempt_reconnect(mqttClient);
        if(NETWORK_RECONNECTED == rc) {
            IOT_WARN("Manual Reconnect Successful");
        } else {
            IOT_WARN("Manual Reconnect Failed - %d", rc);
        }
    }
}

/**
 * @brief Callback Handler for the case of a device shadow disconnect
 * @param shadowClient Shadow Client
 * @param data pointer to the data (JSON value)
 */
void disconnectCallbackHandlerShadow(AWS_IoT_Client *shadowClient, void *data) {
    IOT_WARN("MQTT Disconnect");
    IoT_Error_t rc = FAILURE;

    if(NULL == shadowClient) {
        return;
    }

    IOT_UNUSED(data);

    if(aws_iot_is_autoreconnect_enabled(shadowClient)) {
        IOT_INFO("Auto Reconnect is enabled, Reconnecting attempt will start now");
    } else {
        IOT_WARN("Auto Reconnect not enabled. Starting manual reconnect...");
        rc = aws_iot_mqtt_attempt_reconnect(shadowClient);
        if(NETWORK_RECONNECTED == rc) {
            IOT_WARN("Manual Reconnect Successful");
        } else {
            IOT_WARN("Manual Reconnect Failed - %d", rc);
        }
    }
}


/**
 * @brief Replace Occurrences of oldStr with newStr
 * @param str Helper string
 * @param oldStr Old string
 * @param newStr New string
 */
void stringReplace(std::string& str, const std::string& oldStr, const std::string& newStr) {

    size_t pos = 0;
    while((pos = str.find(oldStr, pos)) != std::string::npos){
        str.replace(pos, oldStr.length(), newStr);
        pos += newStr.length();
    }
}

/**
 * @brief Is called to process the measurement data from the mysql database in json
 * @param myMap vector map
 * @return JSON string
 */
string mapToJSON(map<string, string> myMap) {

    ostringstream json;

    json.str("");
    json << "{";

    bool first = true;

    for (auto itmap=myMap.begin();itmap!=myMap.end();++itmap) {

        //filter out table specific ids
        if (!itmap->first.compare("binaryRun_id")) continue;
        if (!itmap->first.compare("jobRun_id")) continue;
        if (!itmap->first.compare("result_id")) continue;

        if (!itmap->first.compare("BinaryRun_timestamp_id")) continue;
        if (!itmap->first.compare("JobRun_timestamp_id")) continue;
        if (!itmap->first.compare("binary_id")) continue;
        if (!itmap->first.compare("groupJob_id")) continue;
        if (!itmap->first.compare("job_id")) continue;
        if (!itmap->first.compare("unit")) continue;



        if (!first) json << ",";
        first = false;

        json << "\"" << itmap->first << "\":";

        CTool::stringReplace(itmap->second, "\n", "\\n");
        CTool::stringReplace(itmap->second, "\r", "\\r");
        CTool::stringReplace(itmap->second, "\b", "\\b");
        CTool::stringReplace(itmap->second, "\t", "\\t");
        CTool::stringReplace(itmap->second, "\f", "\\f");

        json << "\"" << itmap->second << "\"";
    }

    json << "}";

    return json.str();
}


/**
 * @brief Convert JSON to property tree
 * @param inputstring input JSON
 * @return property tree
 */
ptree readJSON(string inputstring) {
    ptree pt;

    try {
        std::istringstream is(inputstring);
        boost::property_tree::read_json(is, pt);
    }
    catch(boost::property_tree::json_parser::json_parser_error &je) {
        cout << "ERROR readJSON" << "\n";
    }

    return pt;
}


/**
 * @brief Callback Handler is called when the shadow is updated
 * In the event of errors, the error message is output
 * @param status Status of the update process
 */
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
  * @brief Execute the measuring routine, get exit status and publish to exitstatus topic.
  * Select measurement results from the MySQL database an publish to data topic.
  * @param in Measuring routine
  * @return 1
  */
int runCommand(char* in) {

    char jsonToRead [10000];
    char command [10000];

    char buffer [10000];
    sprintf(jsonToRead, "%s", in);
    char out [10000];

    ptree pt;
    pt = readJSON(jsonToRead);

    char cPayload[131000];

    IoT_Error_t rc = FAILURE;

    ::msg.qos = QOS1;
    ::msg.payload = (void *) cPayload;
    ::msg.isRetained = 0;


    const char *commandChar = pt.get<string>("command").c_str();
    const char *optionsChar = pt.get<string>("options").c_str();
    const char *testidChar = pt.get<string>("testid").c_str();


    strcpy(command, commandChar);
    strcat(command, " ");
    strcat(command, optionsChar);
    strcat(command, " -testid ");
    strcat(command, testidChar);


    printf("\n***command: %s\n", command);


    int exitStatus;
    printf ("Executing command...\n");
    exitStatus = system(command);

    char exitStatusPub[100];
    sprintf(exitStatusPub,"%s %d", "The value returned was:", exitStatus);


    sprintf(cPayload,"%s", exitStatusPub);

    msg.payloadLen = strlen(cPayload);

    IOT_INFO("Publishing exit status %s", cPayload);

    rc = aws_iot_mqtt_publish(&mqttClient, topicExitStatus, topicExitStatusLen, &msg);
    sleep(1);

    while(SUCCESS != rc) {
        IOT_ERROR("error publishing exit status, repeating... : %d ", rc);
        rc = aws_iot_mqtt_publish(&mqttClient, topicExitStatus, topicExitStatusLen, &msg);
        sleep(1);
    }


    //instantiate Mysql dbhandler
    CMysql::getInstance()->setDBParameters(DBHOST, DBNAME, DBUSER, DBPASSWORD);


    bool firstResult = true;

    //get associated Results
    string resultJSON = "[";
    string querystring = "SELECT * FROM `Result` WHERE test_id = "+ string(testidChar);

    cout << "\n querystring: " << querystring << "\n";

    vector<map<string, string>> resultVector = CMysql::getInstance()->select(querystring);

    //iterate over result rows
    for (auto itrs=resultVector.begin(); itrs!=resultVector.end(); ++itrs) {


        if (!firstResult) resultJSON += ",";
        firstResult = false;


        resultJSON += mapToJSON(itrs[0]);
    }

    resultJSON += "]";

    const char * resultJSONchar = resultJSON.c_str();


    sprintf(cPayload,"%s", resultJSONchar);

    msg.payloadLen = strlen(cPayload);

    IOT_INFO("\nPublishing payload:\n%s", cPayload);
    printf("\n%s %zu %s\n", "Payload length:", strlen(cPayload), "Byte");
    printf("%s %zu %s\n", "Max payload length:", sizeof(cPayload), "Byte");

    rc = aws_iot_mqtt_publish(&mqttClient, data, dataLen, &msg);
    sleep(1);

    while(SUCCESS != rc) {
        IOT_ERROR("error publishing payload, repeating... : %d ", rc);
        rc = aws_iot_mqtt_publish(&mqttClient, data, dataLen, &msg);
        sleep(1);
    }

    return 1;
}


/**
 * @brief CallbackHandler for Subscribe to a topic
 * @param topicName Topic name
 * @param topicNameLen Topic name length
 * @param params Message Parameters
 */
void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
                                    IoT_Publish_Message_Params *params, void *pData) {
    IOT_UNUSED(pData);
    IOT_UNUSED(pClient);
    char thisTopicName[100];
    char thisPayload[params->payloadLen];

    IOT_INFO("Subscribe callback, i'm here");
    IOT_INFO("%.*s\t%.*s", topicNameLen, topicName, (int) params->payloadLen, (char*)params->payload);

    char receivedSettings[10000];
    sprintf(receivedSettings, "%.*s", (int) params->payloadLen, (char*)params->payload);

    std::thread threadRunCommand (runCommand,receivedSettings);
    threadRunCommand.detach();
}

/**
 * @brief Removes all \\ character from a string
 * @param dest Destination string
 * @param source Source string
 * @return String without \\ character
 */
char* stringCopyException(char* dest, const char* source){

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

/**
 * @brief Is called when the device shadow update contains channel configuration
 * Check the channel configuration for contains channel names and if it set to true or false
 * Subscribe channel if is set to true, unsubscribe channel is is set to false
 * @param pContext Payload context
 */
void channels_Callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
    IOT_UNUSED(pJsonString);
    IOT_UNUSED(JsonStringDataLen);
    IoT_Error_t rc = FAILURE;

    char pData [10000];
    sprintf(pData, "%s", (char*)pContext->pData);
    char dest [10000];

    stringCopyException(dest, pData);

    char jsonToRead [10000];
    sprintf(jsonToRead, "%s", "{\"channels\":");

    strcat(jsonToRead, dest);
    strcat(jsonToRead, "}");

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
                }

}
/**
 * @brief Is called when the device shadow update contains groups configuration
 * @param pContext Payload context
 */
void groups_Callback(const char *pJsonString, uint32_t JsonStringDataLen, jsonStruct_t *pContext) {
    IOT_UNUSED(pJsonString);
    IOT_UNUSED(JsonStringDataLen);
    IoT_Error_t rc = FAILURE;

    char pData [10000];
    sprintf(pData, "%s", (char*)pContext->pData);
    char dest [10000];

    stringCopyException(dest, pData);

    char jsonToRead [10000];
    sprintf(jsonToRead, "%s", "{\"groups\":");

    strcat(jsonToRead, dest);
    strcat(jsonToRead, "}");

    ptree pt;
    pt = readJSON(jsonToRead);

    printf("\n%s%s\n", "***** Output groups pContext->pData ***** ",(char*)pContext->pData);
    printf("\n%s%s\n", "***** Output groups jsonToRead ***** ", jsonToRead);
}



void parseInputArgsForConnectParams(int argc, char **argv) {
	int opt;

	while(-1 != (opt = getopt(argc, argv, "h:p:c"))) {
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

/**
 * Is executed as pthread
 * Includes the entire shadow synchronization
 * @param threadid thread id
 */
void *shadowRun(void *threadid) {
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


    char JsonDocumentBuffer[MAX_LENGTH_OF_UPDATE_JSON_BUFFER];
    size_t sizeOfJsonDocumentBuffer = sizeof(JsonDocumentBuffer) / sizeof(JsonDocumentBuffer[0]);
    char *pJsonStringToUpdate;
    float temperature = 0.0;

    char customer[100] = "unknown";
    jsonStruct_t customerStruct;
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


/**
 * @brief Defining shadow initialization parameters
 */
    shadowInitParams.pHost = AWS_IOT_MQTT_HOST;
    shadowInitParams.port = AWS_IOT_MQTT_PORT;
    shadowInitParams.pClientCRT = clientCRT;
    shadowInitParams.pClientKey = clientKey;
    shadowInitParams.pRootCA = rootCA;
    shadowInitParams.enableAutoReconnect = false;
    shadowInitParams.disconnectHandler = disconnectCallbackHandlerShadow;


/**
 * @brief Called to initialize the shadow
 * @return IoT_Error_t Type defining successful/failed API call
 */
    IOT_INFO("Shadow Init");
    rc = aws_iot_shadow_init(&shadowClient, &shadowInitParams);
    if(SUCCESS != rc) {
        IOT_ERROR("Shadow Connection Error");
    }


/**
* @brief Defining shadow connection parameters.
*/
    shadowConnectParams.pMyThingName = probe_id;
    shadowConnectParams.pMqttClientId = shadow_id;
    shadowConnectParams.mqttClientIdLen = (uint16_t) strlen(shadow_id);


/**
 * @brief Called to establish an shadow connection with the AWS IoT Service
 * @return An IoT Error Type defining successful/failed connection
 */
    IOT_INFO("Shadow Connect");
    rc = aws_iot_shadow_connect(&shadowClient, &shadowConnectParams);
    if(SUCCESS != rc) {
        IOT_ERROR("Shadow Connection Error");
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
    }


/**
 * @brief Any time a delta is published the Json document will be delivered to the pStruct->cb.
 * @return An IoT Error Type defining successful/failed delta registering
 */
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


/**
 * @brief Loop for device shadow synchronization
*/
    while(NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc) {

        rc = aws_iot_shadow_yield(&shadowClient, 200);
        if(NETWORK_ATTEMPTING_RECONNECT == rc) {
            sleep(1);
            continue;
        }

        rc = aws_iot_shadow_init_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer); // Initialize the JSON document with Shadow
        if(SUCCESS == rc) {
            rc = aws_iot_shadow_add_reported(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 6, // Add the reported section of the JSON document of jsonStruct_t.
                                             &customerStruct, &locationStruct, &idStruct, &connectionStruct, &groupsStruct, &channelsStruct);
            if(SUCCESS == rc) {
                rc = aws_iot_finalize_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer); // This function will automatically increment the client token every time this function is called.
                if(SUCCESS == rc) {
                    IOT_INFO("Update Shadow: %s", JsonDocumentBuffer);
                    rc = aws_iot_shadow_update(&shadowClient, probe_id, JsonDocumentBuffer, // This function is the one used to perform an Update action to a Thing Name's Shadow.
                                               ShadowUpdateStatusCallback, NULL, 30, true);

                }
            }
        }
        if(SUCCESS != rc) {
            IOT_ERROR("An error occurred in the loop %d", rc);
        }

        sleep(3);
    }


    IOT_INFO("Disconnecting shadow");
    rc = aws_iot_shadow_disconnect(&shadowClient); // This will close the underlying TCP connection, MQTT connection will also be closed

    if(SUCCESS != rc) {
        IOT_ERROR("Disconnect error %d", rc);
    }
}


int main(int argc, char **argv) {

    char macAddress[50];
    /**
     * @brief Create probe_id, sub_pub_id, shadow_id
     */
    stringCopyExceptionDoublePoint(::probe_id, getMacAddress(macAddress));

    stringCopyExceptionDoublePoint(::sub_pub_id, getMacAddress(macAddress));
    sprintf(::sub_pub_id,"%s%s", ::sub_pub_id, "subpub");

    stringCopyExceptionDoublePoint(::shadow_id, getMacAddress(macAddress));
    sprintf(::shadow_id,"%s%s", ::shadow_id, "shadow");

    bool infinitePublishFlag = true;

	char rootCA[PATH_MAX + 1];
	char clientCRT[PATH_MAX + 1];
	char clientKey[PATH_MAX + 1];
	char CurrentWD[PATH_MAX + 1];
	char cPayload[131000];

	int32_t i = 0;
    char memInfo[5000];
    char runCommandOut[10000];

	IoT_Error_t rc = FAILURE;
    pthread_t pThreadShadow;

    IoT_Client_Init_Params clientInitParams = iotClientInitParamsDefault; // Defining a type for MQTT initialization parameters
	IoT_Client_Connect_Params clientConnectParams = iotClientConnectParamsDefault; // Defining a type for MQTT connection parameters


/**
 * @brief Defines a type for MQTT Publish messages. Used for both incoming and out going messages
 */
	parseInputArgsForConnectParams(argc, argv);

	IOT_INFO("\nAWS IoT SDK Version %d.%d.%d-%s\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);

	getcwd(CurrentWD, sizeof(CurrentWD));
	snprintf(rootCA, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_ROOT_CA_FILENAME);
	snprintf(clientCRT, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_CERTIFICATE_FILENAME);
	snprintf(clientKey, PATH_MAX + 1, "%s/%s/%s", CurrentWD, certDirectory, AWS_IOT_PRIVATE_KEY_FILENAME);


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
    clientInitParams.disconnectHandler = disconnectCallbackHandlerMqtt;
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
    clientConnectParams.pClientID = sub_pub_id; // Or HostAddress
    clientConnectParams.clientIDLen = (uint16_t) strlen(sub_pub_id);
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
 * @brief Create topics to publish the measurement data and the exit status
 */
    sprintf(data, "%s/%s/%s","probes",probe_id,"data");
    sprintf(topicExitStatus, "%s/%s/%s","probes",probe_id,"exitstatus");

    dataLen = strlen(data);
    topicExitStatusLen = strlen(topicExitStatus); // Length of the topic for aws_iot_mqtt_publish function


    IOT_DEBUG("MAC address: %s", getMacAddress(macAddress));
    IOT_DEBUG("Topic1: %s", data);
    IOT_DEBUG("Topic2: %s", topicExitStatus);

    pthread_create (&pThreadShadow, NULL, shadowRun, (void *)1);

    /**
     * @brief Loop for mqtt client
     * yield will give time slot to read messages
     */
    while(1) {

        //Max time the yield function will wait for read messages
        rc = aws_iot_mqtt_yield(&mqttClient, 200);

        sleep(1);
    }


    IOT_INFO("Disconnecting");

	return rc;
}
