#ifndef __WSC_H__
#define __WSC_H__

#include "ieee1905_network.h"
#include "tlv_parser.h"

#define TLV_TYPE_WSC 0x11
#define WSC_MSG_TYPE_M1 0x04

#define DATA_ELEM_TYPE_VERSION 0x104a
#define DATA_ELEM_TYPE_M1 0x1022
#define DATA_ELEM_TYPE_UUID_E 0x1047
#define DATA_ELEM_TYPE_MAC_ADDRESS 0x1020
#define DATA_ELEM_TYPE_ENROLLEE_NONCE 0x101a
#define DATA_ELEM_TYPE_PUBLIC_KEY 0x1032
#define DATA_ELEM_TYPE_AUTHENTICATION_TYPE_FLAGS 0x1004
#define DATA_ELEM_TYPE_ENCRYPTION_TYPE_FLAGS 0x1010
#define DATA_ELEM_TYPE_CONNECTION_TYPE_FLAGS 0x100d
#define DATA_ELEM_TYPE_CONFIG_METHODS 0x1008
#define DATA_ELEM_TYPE_WPS_STATE 0x1044
#define DATA_ELEM_TYPE_MANUFACTURER 0x1021
#define DATA_ELEM_TYPE_MODEL_NAME 0x1023
#define DATA_ELEM_TYPE_MODEL_NUMBER 0x1024
#define DATA_ELEM_TYPE_SERIAL_NUMBER 0x1042
#define DATA_ELEM_TYPE_PRIMARY_DEVICE_TYPE 0x1054
#define DATA_ELEM_TYPE_DEVICE_NAME 0x1011
#define DATA_ELEM_TYPE_RF_BANDS 0x103c
#define DATA_ELEM_TYPE_ASSOCIATION_STATE 0x1002
#define DATA_ELEM_TYPE_DEVICE_PASSWORD_ID 0x1012
#define DATA_ELEM_TYPE_CONFIGURATION_ERROR 0x1009
#define DATA_ELEM_TYPE_OS_VERSION 0x102d
#define DATA_ELEM_TYPE_VENDOR_EXTENSION 0x1049


unsigned char * wsc_m1_msg_create(char *ifaddr, int *len, unsigned char rf_bands);

#endif