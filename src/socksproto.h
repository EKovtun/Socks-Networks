#ifndef SOCKSPROTO_H
#define SOCKSPROTO_H
#include <set>
#include <cstring>

#pragma pack(push, 1)
namespace SOCKS {
	typedef unsigned char	byte;

    #ifdef _WIN32
      typedef unsigned __int16 uint16;
      typedef unsigned __int32 uint32;
    #else
      typedef unsigned short uint16;
      typedef unsigned int   uint32;
    #endif
	const int MAX_SOCSK5_CMD = 262;
	
	struct SOCKS4req {
		byte	version;
		byte	command;
		uint16	port;		// IP port
		uint32	addr;		// v4 IP address in 
		byte	userid[];	// zero-terminated string
		//byte	dnsname[];	// zero-terminated dns-name (SOCKS4a extension, if address like 0.0.0.x)
		SOCKS4req(): version(0x04), command(0), port(0), addr(0){}
	};

	struct SOCKS4resp {
		byte	nullb;
		byte	status;
		uint16	port;
		uint32	addr;	
		SOCKS4resp(): nullb(0), status(0), port(0), addr(0){}
	};

	struct SOCKS5NegotiationRequest{
		byte version;
		byte nMethods;
		byte methods[];
		SOCKS5NegotiationRequest(): version(0x05), nMethods(0){}
	};

	struct SOCKS5NegotiationResponse{
		byte version;
		byte method;
		SOCKS5NegotiationResponse(): version(0), method(0){}
	};



	struct IPv4Addr{
		uint32 addr;
		IPv4Addr(): addr(0){}
	};
	
	struct IPv6Addr{
		byte addr[16];
		IPv6Addr() { memset(addr, 0, sizeof(addr)); }
	};

	struct DNSName{
		byte length;
		byte addr[255];
		DNSName(){ memset(addr, 0, sizeof(addr));}
	};
	
	struct SOCKS5CommandRequest{
		byte version;
		byte command;
		byte reserved;
		byte addr_type;
		byte addr[];
		//uint16 port[]; 
		SOCKS5CommandRequest(): version(0x05), command(0), reserved(0), addr_type(0){}
	};

	struct SOCKS5CommandResponse{
		byte	version;
		byte	reply;
		byte	reserved;
		byte	addr_type;
		byte	addr[];
		//uint16 port[]; 
		SOCKS5CommandResponse(): version(0x05), reply(0), reserved(0), addr_type(0){}
	};

}

#pragma pack(pop)
#endif
