#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "tlv_parser.h"

unsigned short little2bigs(unsigned short num)
{
    unsigned short swapped = (num >> 8) | (num << 8);
    return swapped;
}

//大端转小端
unsigned short big2littles(unsigned short be)
{
    unsigned short swapped = (be << 8) | (be >> 8);
    return swapped;
}

tlv_type little2bigs_type(tlv_type num, int len)
{
    tlv_type swapped;
    if (len == 2)
    {
        swapped = (num << 8) | (num >> 8);
    }
    else if(len == 1)
    {
        swapped = num;
    }
    
    return swapped;
}

//大端转小端
tlv_type big2littles_type(tlv_type be, int len)
{
    tlv_type swapped;
    if (len == 2)
    {
        swapped = (be << 8) | (be >> 8);
    }
    else if(len == 1)
    {
        swapped = be;
    }
    
    return swapped;
}

tlv_len little2bigs_len(tlv_len num)
{
    tlv_len swapped;
    if (sizeof(tlv_len) == 2)
    {
        swapped = (num << 8) | (num >> 8);
    }
    else if(sizeof(tlv_len) == 1)
    {
        swapped = num;
    }
    
    return swapped;
}

//大端转小端
tlv_len big2littles_len(tlv_len be)
{
    tlv_len swapped;
    if (sizeof(tlv_len) == 2)
    {
        swapped = (be << 8) | (be >> 8);
    }
    else if(sizeof(tlv_len) == 1)
    {
        swapped = be;
    }
    
    return swapped;
}

#define htons little2bigs
#define ntohs big2littles

#define htons_type(num, len) little2bigs_type(num, len)
#define ntohs_type(num, len) big2littles_type(num, len)

#define htons_len little2bigs_len
#define ntohs_len big2littles_len

void Kami_Tlv_Delete(KamiTlv_S *pstTlv);
int Kami_Tlv_PrintValue(KamiTlv_S *pstItem, KamiTlvPrintBuffer *output_buffer);

KamiTlv_S *Kami_Tlv_CreateObject(int type_len)
{
    KamiTlv_S *pstTlv = (KamiTlv_S *)malloc(sizeof(KamiTlv_S));
    if (!pstTlv)
    {
        return NULL;
    }
    memset(pstTlv, 0, sizeof(KamiTlv_S));
    pstTlv->obj_type = KamiTlv_Object;
    pstTlv->type_len = type_len;
    return pstTlv;
}

KamiTlv_S *Kami_Tlv_CreateArray(tlv_type usType, int len)
{
    KamiTlv_S *pstTlv = (KamiTlv_S *)malloc(sizeof(KamiTlv_S));
    if (!pstTlv)
    {
        return NULL;
    }
    memset(pstTlv, 0, sizeof(KamiTlv_S));
    pstTlv->obj_type = KamiTlv_Array;
    pstTlv->type = usType;
    pstTlv->type_len = len;
    return pstTlv;
}

KamiTlv_S *Kami_Tlv_CreateTlv(tlv_type usType, tlv_len usLength, void *pValue, int len)
{
    KamiTlv_S *pstTlv = Kami_Tlv_CreateObject(len);
    if (pstTlv)
    {
        pstTlv->type = usType;
        pstTlv->type_len = len;
        pstTlv->length = usLength;
        if (usLength)
        {
            pstTlv->value = (char *)malloc(usLength);
            if (!pstTlv->value)
            {
                Kami_Tlv_Delete(pstTlv);
                return NULL;
            }
            memcpy(pstTlv->value, pValue, usLength);
        }
        pstTlv->obj_type = KamiTlv_Tlv;
    }

    return pstTlv;
}

void Kami_Tlv_Delete(KamiTlv_S *pstTlv)
{
    KamiTlv_S *pstNext = NULL;
    while (pstTlv != NULL)
    {
        pstNext = pstTlv->next;
        if (pstTlv->child != NULL)
        {
            Kami_Tlv_Delete(pstTlv->child);
        }
        if (pstTlv->value != NULL)
        {
            printf("free %s\n", pstTlv->value);
            free(pstTlv->value);
        }
        free(pstTlv);
        pstTlv = pstNext;
    }
}

int Kami_Tlv_AddItemToObject(KamiTlv_S *pstTlv, KamiTlv_S *pstItem)
{
    KamiTlv_S *pstChild = pstTlv->child;

    pstItem->parent = pstTlv;

    if (pstChild == NULL)
    {
        pstTlv->child = pstItem;
        pstItem->prev = pstItem;
        pstItem->next = NULL;
    }
    else
    {
        if (pstChild->prev)
        {
            pstChild->prev->next = pstItem;
            pstItem->prev = pstChild->prev;
            pstTlv->child->prev = pstItem;
        }
    }

    tlv_len usIncLength = 0;

    if (pstItem->obj_type == KamiTlv_Tlv || pstItem->obj_type == KamiTlv_Array)
    {
        usIncLength = (pstTlv->type_len + sizeof(tlv_len) + pstItem->length);
    }
    else if (pstItem->obj_type == KamiTlv_Object)
    {
        usIncLength = pstItem->length;
    }

    

    while (pstTlv)
    {
        KamiTlv_S *pstParent = pstTlv->parent;
        
        pstTlv->length += usIncLength;

        pstTlv = pstParent;
    }

    return 0;
}

int Kami_Tlv_AddTlvToObject(KamiTlv_S *pstTlv, tlv_type usType, tlv_len usLength, void *pValue, int len)
{
    KamiTlv_S *pstItem = Kami_Tlv_CreateTlv(usType, usLength, pValue, len);
    if (!pstItem)
    {
        return -1;
    }

    if (0 == Kami_Tlv_AddItemToObject(pstTlv, pstItem))
    {
        return 0;
    }

    Kami_Tlv_Delete(pstItem);
    return -1;
}

void Kami_Tlv_UpdateOffset(KamiTlvPrintBuffer *buffer, KamiTlv_S *pstItem)
{
    switch ((pstItem->obj_type) & 0xFF)
    {
    case KamiTlv_Tlv:
        buffer->offset += (pstItem->type_len + sizeof(tlv_len) + pstItem->length);
        break;
    case KamiTlv_Array:
        break;
    case KamiTlv_Object:

    default:
        return;
    }
    return;
}

static unsigned char *Kami_Tlv_Ensure(KamiTlvPrintBuffer *const p, size_t needed)
{
    unsigned char *newbuffer = NULL;
    size_t newsize = 0;

    if ((p == NULL) || (p->buffer == NULL))
    {
        return NULL;
    }

    if ((p->length > 0) && (p->offset >= p->length))
    {
        return NULL;
    }

    if (needed > INT_MAX)
    {
        return NULL;
    }

    needed += p->offset + 1;
    if (needed <= p->length)
    {
        return p->buffer + p->offset;
    }

    if (needed > (INT_MAX / 2))
    {
        if (needed <= INT_MAX)
        {
            newsize = INT_MAX;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        newsize = needed * 2;
    }

    newbuffer = (unsigned char *)realloc(p->buffer, newsize);
    if (newbuffer == NULL)
    {
        free(p->buffer);
        p->length = 0;
        p->buffer = NULL;

        return NULL;
    }

    p->length = newsize;
    p->buffer = newbuffer;

    return newbuffer + p->offset;
}

int Kami_Tlv_PrintTlv(KamiTlv_S *input, KamiTlvPrintBuffer *const output_buffer)
{
    unsigned char *output = NULL;
    if (!input)
    {
        return -1;
    }

    output = Kami_Tlv_Ensure(output_buffer, input->length + input->type_len + sizeof(tlv_len));

    if (output == NULL)
    {
        return -1;
    }

    *((tlv_type *)output) = htons_type(input->type, input->type_len);
    *((tlv_len *)(output + input->type_len)) = htons_len(input->length);

    memcpy(output + input->type_len + sizeof(tlv_len), input->value, input->length);

    output_buffer->offset += (input->length + input->type_len + sizeof(tlv_len));

    return 0;
}

int Kami_Tlv_PrintObject(KamiTlv_S *item, KamiTlvPrintBuffer *const output_buffer)
{
    unsigned char *output_pointer = NULL;
    size_t length = 0;
    KamiTlv_S *current_item = item->child;

    if (output_buffer == NULL)
    {
        return -1;
    }

    while (current_item)
    {

        if (Kami_Tlv_PrintValue(current_item, output_buffer))
        {
            return -1;
        }

        current_item = current_item->next;
    }

    return 0;
}

int Kami_Tlv_PrintArray(KamiTlv_S *item, KamiTlvPrintBuffer *const output_buffer)
{
    unsigned char *output_pointer = NULL;
    size_t length = 0;
    KamiTlv_S *current_item = item->child;

    if (output_buffer == NULL)
    {
        return -1;
    }

    output_pointer = Kami_Tlv_Ensure(output_buffer, item->type_len + sizeof(tlv_len));
    if (output_pointer == NULL)
    {
        return -1;
    }
    *((tlv_type *)output_pointer) = htons_type(item->type, item->type_len);
    *((tlv_len *)(output_pointer + item->type_len)) = htons_len(item->length);

    output_buffer->offset += (item->type_len + sizeof(tlv_len));

    while (current_item)
    {


        if (Kami_Tlv_PrintValue(current_item, output_buffer))
        {
            return -1;
        }

        current_item = current_item->next;
    }

    return 0;
}

int Kami_Tlv_PrintValue(KamiTlv_S *pstItem, KamiTlvPrintBuffer *output_buffer)
{
    unsigned char *output = NULL;

    if ((pstItem == NULL) || (output_buffer == NULL))
    {
        return -1;
    }

    switch ((pstItem->obj_type) & 0xFF)
    {
    case KamiTlv_Tlv:
        return Kami_Tlv_PrintTlv(pstItem, output_buffer);
    case KamiTlv_Array:
        return Kami_Tlv_PrintArray(pstItem, output_buffer);
    case KamiTlv_Object:
        return Kami_Tlv_PrintObject(pstItem, output_buffer);

    default:
        return -1;
    }
}

unsigned char *Kami_Tlv_Print(KamiTlv_S *pstTlv)
{
    static const size_t default_buffer_size = 256;
    KamiTlvPrintBuffer buffer[1];
    unsigned char *printed = NULL;
    memset(buffer, 0, sizeof(buffer));

    buffer->buffer = (unsigned char *)malloc(default_buffer_size);
    buffer->length = default_buffer_size;

    if (buffer->buffer == NULL)
    {
        goto fail;
    }

    if (Kami_Tlv_PrintValue(pstTlv, buffer))
    {
        goto fail;
    }


    printed = (unsigned char *)realloc(buffer->buffer, buffer->offset + 1);
    if (printed == NULL)
    {
        goto fail;
    }
    buffer->buffer = NULL;

    return printed;

fail:
    if (buffer->buffer != NULL)
    {
        free(buffer->buffer);
    }

    if (printed != NULL)
    {
        free(printed);
    }

    return NULL;
}

KamiTlv_S *Kami_Tlv_ParseObject(void *pTlvData, int iLength, int len)
{
    if (!pTlvData || iLength < len + sizeof(tlv_len))
    {
        return NULL;
    }
    KamiTlv_S *pstRoot = Kami_Tlv_CreateObject(len);
    if (!pstRoot)
    {
        return NULL;
    }

    int iOffset = 0;
    char *pcTmp = (char *)pTlvData;

    while (iLength - iOffset > (len + sizeof(tlv_len)))
    {
        tlv_type *pusType = (tlv_type *)(pcTmp + iOffset);
        iOffset += len;
        tlv_len *pusLength = (tlv_len *)(pcTmp + iOffset);
        iOffset += sizeof(tlv_len);
        void *pValue = (void *)(pcTmp + iOffset);
        if (Kami_Tlv_AddTlvToObject(pstRoot, ntohs_type(*pusType, len), ntohs_len(*pusLength), pValue, len))
        {
            Kami_Tlv_Delete(pstRoot);
            pstRoot = NULL;
            break;
        }
        iOffset += ntohs_len(*pusLength);
    }

    return pstRoot;

}

KamiTlv_S *Kami_Tlv_GetObjectItem(KamiTlv_S *pstObject, tlv_type usType)
{
    KamiTlv_S *pstNext = NULL;
    KamiTlv_S *pstChild = pstObject->child;
    while (pstChild != NULL)
    {
        pstNext = pstChild->next;
        if (pstChild->type == usType)
        {
            break;
        }
        pstChild = pstNext;
    }

    return pstChild;
}

KamiTlv_S *Kami_Tlv_GetObjectItemIndex(KamiTlv_S *pstObject, int iIndex)
{
    KamiTlv_S *pstChild = NULL;
    if (!pstObject)
    {
        return NULL;
    }
    pstChild = pstObject->child;
    while (pstChild != NULL && iIndex > 0)
    {
        iIndex--;
        pstChild = pstChild->next;
    }

    return pstChild;
}

int Kami_Tlv_GetObjectNumber(KamiTlv_S *pstObject)
{
    KamiTlv_S *pstChild = NULL;
    int iIndex = 0;

    if (!pstObject)
    {
        return -1;
    }
    pstChild = pstObject->child;
    while (pstChild != NULL)
    {
        iIndex++;
        pstChild = pstChild->next;
    }

    return iIndex;
}

int Kami_Tlv_ObjectLength(KamiTlv_S *pstObject)
{
    return pstObject->length;
}

// int main()
// {
//     KamiTlv_S *pstRoot = Kami_Tlv_CreateObject();
//     KamiTlv_S *pstArr1 = Kami_Tlv_CreateArray(16);
//     Kami_Tlv_AddTlvToObject(pstArr1, 1, 2, (void *)"a");
//     Kami_Tlv_AddTlvToObject(pstArr1, 2, 2, (void *)"b");
    
//     KamiTlv_S *pstArr2 = Kami_Tlv_CreateArray(17);
//     Kami_Tlv_AddTlvToObject(pstArr2, 1, 2, (void *)"c");
//     Kami_Tlv_AddTlvToObject(pstArr2, 2, 2, (void *)"d");

//     KamiTlv_S *pstArr3 = Kami_Tlv_CreateArray(18);
//     Kami_Tlv_AddTlvToObject(pstArr3, 1, 2, (void *)"e");
//     Kami_Tlv_AddTlvToObject(pstArr3, 2, 2, (void *)"f");

//     Kami_Tlv_AddItemToObject(pstRoot, pstArr1);
//     Kami_Tlv_AddItemToObject(pstArr1, pstArr2);
//     Kami_Tlv_AddItemToObject(pstArr2, pstArr3);
    
    


//     unsigned char *pr = Kami_Tlv_Print(pstRoot);
//     int length = Kami_Tlv_ObjectLength(pstRoot);

//     Kami_Tlv_Delete(pstRoot);

//     KamiTlv_S *pstParsed1 = Kami_Tlv_ParseObject(pr, length);
//     for (int i = 0; i < Kami_Tlv_GetObjectNumber(pstParsed1); i++)
//     {
//         KamiTlv_S *pstTmp = Kami_Tlv_GetObjectItemIndex(pstParsed1, i);
//         printf("type: %d, length: %d\n", pstTmp->type, pstTmp->length);
//     }
//     KamiTlv_S *pstItem16 = Kami_Tlv_GetObjectItem(pstParsed1, 16);
//     KamiTlv_S *pstParsed2 = Kami_Tlv_ParseObject(pstItem16->value, pstItem16->length);
//     for (int i = 0; i < Kami_Tlv_GetObjectNumber(pstParsed2); i++)
//     {
//         KamiTlv_S *pstTmp = Kami_Tlv_GetObjectItemIndex(pstParsed2, i);
//         printf("type: %d, length: %d\n", pstTmp->type, pstTmp->length);
//     }
//     KamiTlv_S *pstItem17 = Kami_Tlv_GetObjectItem(pstParsed2, 17);
//     KamiTlv_S *pstParsed3 = Kami_Tlv_ParseObject(pstItem17->value, pstItem17->length);
//     for (int i = 0; i < Kami_Tlv_GetObjectNumber(pstParsed3); i++)
//     {
//         KamiTlv_S *pstTmp = Kami_Tlv_GetObjectItemIndex(pstParsed3, i);
//         printf("type: %d, length: %d\n", pstTmp->type, pstTmp->length);
//     }
//     KamiTlv_S *pstItem18 = Kami_Tlv_GetObjectItem(pstParsed3, 18);
//     KamiTlv_S *pstParsed4 = Kami_Tlv_ParseObject(pstItem18->value, pstItem18->length);
//     for (int i = 0; i < Kami_Tlv_GetObjectNumber(pstParsed4); i++)
//     {
//         KamiTlv_S *pstTmp = Kami_Tlv_GetObjectItemIndex(pstParsed4, i);
//         printf("type: %d, length: %d\n", pstTmp->type, pstTmp->length);
//     }

//     return 0;
// }
