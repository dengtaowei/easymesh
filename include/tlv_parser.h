#ifndef __TLV_PARSER_H__
#define __TLV_PARSER_H__
#include <stddef.h>


typedef unsigned short tlv_type;
typedef unsigned short tlv_len;


typedef struct tagKamiTlv
{
    struct tagKamiTlv *next;
    struct tagKamiTlv *prev;
    struct tagKamiTlv *child;
    struct tagKamiTlv *parent;

    tlv_type type;
    char type_len;
    tlv_len length;
    int obj_type;
    char *value;
} KamiTlv_S;

typedef struct
{
    unsigned char *buffer;
    size_t length;
    size_t offset;
} KamiTlvPrintBuffer;

#define KamiTlv_Invalid (0)
#define KamiTlv_Tlv (1 << 0)
#define KamiTlv_Object (1 << 1)
#define KamiTlv_Array (1 << 2)


KamiTlv_S *Kami_Tlv_CreateObject(int len);
KamiTlv_S *Kami_Tlv_CreateArray(tlv_type usType, int len);
int Kami_Tlv_AddTlvToObject(KamiTlv_S *pstTlv, tlv_type usType, tlv_len usLength, void *pValue, int len);
int Kami_Tlv_AddItemToObject(KamiTlv_S *pstTlv, KamiTlv_S *pstItem);
unsigned char *Kami_Tlv_Print(KamiTlv_S *pstTlv);
int Kami_Tlv_ObjectLength(KamiTlv_S *pstObject);
void Kami_Tlv_Delete(KamiTlv_S *pstTlv);
KamiTlv_S *Kami_Tlv_ParseObject(void *pTlvData, int iLength, int len);
KamiTlv_S *Kami_Tlv_GetObjectItem(KamiTlv_S *pstObject, tlv_type usType);


#endif