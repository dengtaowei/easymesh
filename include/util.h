#ifndef __UTIL_H__
#define __UTIL_H__

int get_if_mac(const char *ifname, unsigned char *mac);

void hex_dump(char *str, unsigned char *pSrcBufVA, unsigned int SrcBufLen);
#endif