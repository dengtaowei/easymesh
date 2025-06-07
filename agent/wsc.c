#include <arpa/inet.h>
#include "wsc.h"
#include "tlv_parser.h"

unsigned char * wsc_m1_msg_create(NetworkInterface *interface, int *len)
{
    KamiTlv_S *pstWsc = Kami_Tlv_CreateObject(2);

    unsigned char ver = 0x10;
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_VERSION, 1, &ver, 2);

    unsigned char msg_type = WSC_MSG_TYPE_M1;
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_M1, 1, &msg_type, 2);

    unsigned char uuid[16] = {0};
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_UUID_E, sizeof(uuid), uuid, 2);

    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_MAC_ADDRESS, sizeof(interface->addr), interface->addr, 2);

    unsigned char enrollee_nonce[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf};
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_ENROLLEE_NONCE, sizeof(enrollee_nonce), enrollee_nonce, 2);

    unsigned char public_key[192] = {0};
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_PUBLIC_KEY, sizeof(public_key), public_key, 2);

    unsigned short auth_type_flags = htons(0x0063);
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_AUTHENTICATION_TYPE_FLAGS, sizeof(auth_type_flags), &auth_type_flags, 2);

    unsigned short enc_type_flags = htons(0x000f);
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_ENCRYPTION_TYPE_FLAGS, sizeof(enc_type_flags), &enc_type_flags, 2);


    unsigned char conn_type_flags = 0x01;
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_CONNECTION_TYPE_FLAGS, sizeof(conn_type_flags), &conn_type_flags, 2);

    unsigned short config_methods = htons(0x0680);
    Kami_Tlv_AddTlvToObject(pstWsc, DATA_ELEM_TYPE_CONFIG_METHODS, sizeof(config_methods), &config_methods, 2);

    unsigned char wps_state = 0x00;
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