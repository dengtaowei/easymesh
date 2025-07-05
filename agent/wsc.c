#include <arpa/inet.h>
#include "wsc.h"
#include "tlv_parser.h"

unsigned char *wsc_m1_msg_create(NetworkInterface *interface, int *len)
{
    KamiTlv_S *pstWsc = Kami_Tlv_CreateObject(2);

    unsigned char ver = 0x10;
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_VERSION, 1, &ver, 2);

    unsigned char msg_type = WSC_MSG_TYPE_M1;
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_M1, 1, &msg_type, 2);

    unsigned char uuid[16] = {0};
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_UUID_E, sizeof(uuid), uuid, 2);

    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_MAC_ADDRESS, sizeof(interface->addr), interface->addr, 2);

    unsigned char enrollee_nonce[16] = {0x53, 0x83, 0x78, 0x76, 0x1c, 0x9a, 0x4b, 0xee, 0x59, 0x3e, 0x6b, 0x53, 0x37, 0xd6, 0x25, 0x4b};
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_ENROLLEE_NONCE, sizeof(enrollee_nonce), enrollee_nonce, 2);

    unsigned char public_key[192] = {0x14, 0x03, 0x2d, 0x1d, 0x10, 0x87, 0x4d,
                                     0xc4, 0xe5, 0x19, 0x5b, 0x0b, 0xeb, 0x15, 0x02, 0x25, 0x3a, 0x29, 0xad, 0x43, 0x44, 0x85, 0x0f,
                                     0x6a, 0x77, 0xf0, 0x53, 0xf9, 0x7a, 0x0e, 0x1e, 0xc9, 0x38, 0x46, 0xef, 0x05, 0x94, 0x5c, 0x95,
                                     0x70, 0x5a, 0x49, 0x6b, 0xba, 0x3f, 0xcf, 0xd3, 0x23, 0x97, 0x7f, 0xd6, 0xa6, 0xec, 0x74, 0x51,
                                     0xa9, 0xa3, 0xe3, 0xda, 0xe5, 0x41, 0x68, 0x07, 0xa4, 0xcd, 0xb3, 0x28, 0x81, 0x76, 0xaa, 0x4f,
                                     0x2a, 0x5b, 0x60, 0x17, 0xee, 0x65, 0xf8, 0xd1, 0x8a, 0x8d, 0x9e, 0x47, 0x5a, 0x24, 0xd9, 0x81,
                                     0xc8, 0x41, 0xf1, 0xfc, 0xa2, 0x3d, 0x22, 0x81, 0xb6, 0x47, 0x1a, 0x6f, 0xc6, 0xcc, 0x0b, 0x8e,
                                     0xc6, 0x2f, 0x52, 0xb0, 0xa6, 0xf8, 0x5a, 0xd4, 0x93, 0x25, 0xff, 0xf7, 0x97, 0xba, 0x46, 0x9a,
                                     0x25, 0x18, 0x8d, 0xbc, 0x75, 0x55, 0x28, 0x87, 0xe8, 0x5d, 0x56, 0x4d, 0xa1, 0xd4, 0x09, 0x7f,
                                     0x1f, 0xbe, 0x12, 0x23, 0x6a, 0x51, 0xd4, 0x5a, 0x85, 0xa0, 0xaa, 0xda, 0x68, 0x29, 0x92, 0x5d,
                                     0x0e, 0xea, 0x31, 0xd8, 0x99, 0x52, 0x9a, 0x13, 0xa7, 0xd3, 0x56, 0x97, 0x04, 0x7c, 0x03, 0xaa,
                                     0xe7, 0x37, 0xe7, 0x0d, 0x06, 0x09, 0x9a, 0x7a, 0x8e, 0x62, 0xe8, 0x7c, 0x1e, 0xa0, 0x4e, 0x24,
                                     0xdd, 0x41, 0xcc, 0xa4, 0x9d, 0x56, 0x69, 0x40, 0x30};
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_PUBLIC_KEY, sizeof(public_key), public_key, 2);

    unsigned short auth_type_flags = htons(0x0063);
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_AUTHENTICATION_TYPE_FLAGS, sizeof(auth_type_flags), &auth_type_flags, 2);

    unsigned short enc_type_flags = htons(0x000f);
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_ENCRYPTION_TYPE_FLAGS, sizeof(enc_type_flags), &enc_type_flags, 2);

    unsigned char conn_type_flags = 0x01;
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_CONNECTION_TYPE_FLAGS, sizeof(conn_type_flags), &conn_type_flags, 2);

    unsigned short config_methods = htons(0x0680);
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_CONFIG_METHODS, sizeof(config_methods), &config_methods, 2);

    // unsigned char wps_state = 0x00;
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_WPS_STATE, sizeof(conn_type_flags), &conn_type_flags, 2);

    unsigned char manufac[8] = {'m', 'e', 'd', 'i', 'a', 't', 'e', 'k'};
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_MANUFACTURER, sizeof(manufac), manufac, 2);

    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_MODEL_NAME, strlen("MTK 0000"), (void *)"MTK 0000", 2);

    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_MODEL_NUMBER, strlen("v2.0.2"), (void *)"v2.0.2", 2);

    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_SERIAL_NUMBER, strlen("223333-768367373646"), (void *)"223333-768367373646", 2);

    unsigned char dev_type[8] = {0x00, 0x06, 0x00, 0x50, 0xf2, 0x04, 0x00, 0x01};
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_PRIMARY_DEVICE_TYPE, sizeof(dev_type), dev_type, 2);

    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_DEVICE_NAME, strlen("EROLEE"), (void *)"EROLEE", 2);

    unsigned char band = 1;
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_RF_BANDS, sizeof(band), &band, 2);

    unsigned short assoc_state = htons(0x0001);
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_ASSOCIATION_STATE, sizeof(assoc_state), &assoc_state, 2);

    unsigned short dev_pass = htons(0x0004);
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_DEVICE_PASSWORD_ID, sizeof(dev_pass), &dev_pass, 2);

    unsigned short err = htons(0x0000);
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_CONFIGURATION_ERROR, sizeof(err), &err, 2);

    unsigned int os_ver = htonl(0x80000000);
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_OS_VERSION, sizeof(os_ver), &os_ver, 2);

    unsigned char vend_ext[8] = {00, 0x37, 0x2a, 00, 01, 0x20};
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_VENDOR_EXTENSION, sizeof(vend_ext), vend_ext, 2);

    *len = Kami_Tlv_ObjectLength(pstWsc);

    return Kami_Tlv_Print(pstWsc);
}