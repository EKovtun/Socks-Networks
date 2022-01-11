#ifndef HTON_H
#define HTON_H

#include <QtGlobal>



quint16 Ntohs(quint16); // Network to host short byte order

quint16 Htons(quint16); // Host to network short byte order

quint32 Ntohl(quint32);	// Network to host long byte order

quint32 Htonl(quint32);	// Host to network long byte order

void HtonIPv6(quint8* addr);



#endif // CHAINMAKER_H