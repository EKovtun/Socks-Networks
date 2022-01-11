#include "hton.h"

quint16 Htons(quint16 v)
{
    return (((v >> 8) & 0xff) | ((v << 8) & 0xff00));
}

quint16 Ntohs(quint16 x) 
{
	return ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8));
}

quint32 Htonl(quint32 v) 
{
	return ((v<<24)&0xff000000)|((v<<8)&0x00ff0000)|((v>>8)&0x0000ff00)|((v>>24)&0x000000ff);
}

quint32 Ntohl(quint32 v){
	return ((v<<24)&0xff000000)|((v<<8)&0x00ff0000)|((v>>8)&0x0000ff00)|((v>>24)&0x000000ff);
}

void HtonIPv6(quint8* addr) 
{
	quint16 *v_as_short = (quint16 *) addr;
	for (int i=0; i<8; ++i)
		v_as_short[i] = Htons(v_as_short[i]);
}