#!/bin/bash
rm bin/aws_iot_mqtt_measurement
make -f Makefile
./bin/aws_iot_mqtt_measurement
