#!/bin/bash
rm bin/aws_iot_mqtt_measurement
make -f Makefile
cd bin
./aws_iot_mqtt_measurement
