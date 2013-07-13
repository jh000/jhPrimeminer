
#ifdef _SYSLIB_USE_SIMPLEHTTP

#include<winsock2.h>
#include<Windows.h>
#include<stdio.h>
#include"simpleHTTP.h"

int _httpReadLine(SOCKET s, char *buffer, int limit)
{
	int i = 0;
	limit--; // because of '\0'
	while( i < limit )
	{
		int r = recv(s, buffer+i, 1, 0);
		if( r != 1 )
			break;
		i++;
		if( i>=2 )
		{
			if( buffer[i-2] == '\r' && buffer[i-1] == '\n' )
			{
				i -= 2;
				break;
			}
		}
	}
	buffer[i] = '\0';
	return i;
}

int hexAtio(char *s, int errorCode)
{
	int number = 0;
	int m = 0;
	while( *s )
	{
		if( *s >= 'a' && *s <= 'f' )
		{
			number *= 16;
			number += 0xA + (*s-'a');
		}
		else if( *s >= 'A' && *s <= 'F' )
		{
			number *= 16;
			number += 0xA + (*s-'A');
		}
		else if( *s >= '0' && *s <= '9' )
		{
			number *= 16;
			number += (*s-'0');
		}
		else
		{
			if( m ) // already found a valid number/digit
				return number;
			return errorCode;
		}
		s++;
		m++;
	}
	return number;
}

int simpleHTTP_directGET(char *url, char *outBuffer, int *pLength)
{
	char host[1024];
	// parse url
	if( memcmp(url, "http://", 7) )
		return HTTPERR_INVALIDURL;
	url += 7;
	// find next '/' or '\0'
	int hl = 0;
	while( url[hl] && url[hl] != '/' ) hl++;
	// copy hostpart
	for(int i=0; i<hl; i++)
		host[i] = url[i];
	host[hl] = '\0';
	url += hl;
	// get path
	char *path = url;
	//if( *path == '/' ) path++;
	// get host port (optional)
	char *hostPortStr = NULL;
	int hostPort = -1;
	int hpi = 0;
	while( host[hpi] )
	{
		if( host[hpi] == ':' )
		{
			host[hpi] = '\0';
			hostPortStr = host+hpi+1;
			hostPort = atoi(hostPortStr);
			if( hostPort <= 0 || hostPort > 0xFFFF )
				return HTTPERR_INVALIDURL;
			break;
		}
		hpi++;
	}
	// get host ip
	struct  hostent *hp;
	struct  sockaddr_in     server;
	hp = gethostbyname(host);
	if( hp == NULL )
		return HTTPERR_HOSTERROR;
	memset((char *) &server,0, sizeof(server));
	memmove((char *) &server.sin_addr, hp->h_addr, hp->h_length);
	server.sin_family = hp->h_addrtype;
	if( hostPortStr )
		server.sin_port = (unsigned short) htons( hostPort );
	else
		server.sin_port = (unsigned short) htons( 80 );
	/* create socket */
	SOCKET s;
	s = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, 0, 0);
	if( connect(s, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR )
	{
		closesocket(s);
		return HTTPERR_COULDNOTCONNECT;
	}
	// create header
	char header[1024];
	header[0] = '\0';
	// GET ...
	strcat(header, "GET ");
	strcat(header, path);
	strcat(header, " HTTP/1.1\r\n");
	// Host ...
	strcat(header, "Host: ");
	strcat(header, host);
	strcat(header, "\r\n");
	// Useragent ...
	strcat(header, "User-Agent: ");
	strcat(header, "Mozilla/5.0 (Windows; U; Windows NT 6.1; de; rv:1.9.2.14) Gecko/20110218 Firefox/3.6.14");
	strcat(header, "\r\n");
	strcat(header, "\r\n");
	
	// send header
	int hLen = strlen(header);
	if( send(s, (const char*)header, hLen, 0) != hLen )
	{
		closesocket(s);
		return HTTPERR_NETERROR;
	}	
	char lineBuffer[1024];
	memset(lineBuffer, 0x00, sizeof(lineBuffer));
	// read HTTP response type
	_httpReadLine(s, lineBuffer, sizeof(lineBuffer));
	char *lb = lineBuffer;
	int HTTPVersion = 0; // 1 --> 1.0, 2 --> 1.1
	if( memcmp(lb, "HTTP/1.0 ", 9) == 0 )
		HTTPVersion = 1;
	else if( memcmp(lb, "HTTP/1.1 ", 9) == 0 )
		HTTPVersion = 2;
	else
	{
		closesocket(s);
		return HTTPERR_PARSEERROR;
	}
	lb += 9;
	int httpRetCode = -1;
	httpRetCode = atoi(lb);
	if( httpRetCode == -1 )
	{
		closesocket(s);
		return HTTPERR_PARSEERROR;
	}
	if( httpRetCode != HTTPERR_OK ) // success
	{
		closesocket(s);
		return httpRetCode;
	}
	// read other
	int dataLength = -1;
	while( _httpReadLine(s, lineBuffer, sizeof(lineBuffer)) > 0 )
	{
		// Content-Length: 2337
		if( memcmp(lineBuffer, "Content-Length: ", 16) == 0 )
		{
			char *plb = lineBuffer+16;
			dataLength = atoi(plb);
		}
		//__debugbreak();
		//puts(lineBuffer);
	}
	// read content length
	if( dataLength == -1 ) // no content-length passed
	{
		if( HTTPVersion == 2 )
		{
			// HTTP 1.1 supports hexadecimal encoded size
			if( _httpReadLine(s, lineBuffer, sizeof(lineBuffer)) <= 0 )
				return HTTPERR_PARSEERROR;
			dataLength = hexAtio(lineBuffer, -1);
		}
		else
		{
			// HTTP 1.0 supports "length by duration of connection"
			// in this case, do the special handling here
			char *httpData = outBuffer;
			int index = 0;
			int lengthLimit = *pLength;
			*pLength = 0;
			while( index < lengthLimit )
			{
				int r = recv(s, httpData+index, (lengthLimit-index), 0);
				if( r == 0 )
					break; // connection closed by server
				if( r < 0 )
				{
					closesocket(s);
					return HTTPERR_PARSEERROR;
				}
				index += r;
			}
			*pLength = index;
			httpData[index] = '\0';
			// dont need to close connection clientside
			return HTTPERR_OK;
		}
	}
	if( dataLength == -1 )
	{
		closesocket(s);
		return HTTPERR_PARSEERROR;
	}
	if( dataLength == 0 )
	{
		// no data..
		*pLength = 0;
		closesocket(s);
		return httpRetCode;
	}
	// read data
	char *httpData = outBuffer;
	int index = 0;
	dataLength = min(dataLength, (*pLength-1));
	*pLength = dataLength;
	while( index < dataLength )
	{
		int r = recv(s, httpData+index, (dataLength-index), 0);
		if( r <= 0 )
			return HTTPERR_PARSEERROR;
		index += r;
	}
	httpData[index] = '\0';
	closesocket(s);
	//GET /tr/content/server/auth_request_form.php HTTP/1.1
		//Host: jhwork.net
		//User-Agent: Mozilla/5.0 (Windows; U; Windows NT 6.1; de; rv:1.9.2.14) Gecko/20110218 Firefox/3.6.14
		//Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
		//Accept-Language: de-de,de;q=0.8,en-us;q=0.5,en;q=0.3
		//Accept-Encoding: gzip,deflate
		//Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7
		//Keep-Alive: 115
		//Connection: keep-alive


	return HTTPERR_OK;
}


int simpleHTTP_directPOST(char *url, char *post, char *outBuffer, int *pLength)
{
	char host[1024];
	// parse url
	if( memcmp(url, "http://", 7) )
		return HTTPERR_INVALIDURL;
	url += 7;
	// find next '/' or '\0'
	int hl = 0;
	while( url[hl] && url[hl] != '/' ) hl++;
	// copy hostpart
	for(int i=0; i<hl; i++)
		host[i] = url[i];
	host[hl] = '\0';
	url += hl;
	// get path
	char *path = url;
	//if( *path == '/' ) path++;
	// get host ip
	struct  hostent *hp;
	struct  sockaddr_in     server;
	hp = gethostbyname(host);
	if( hp == NULL )
		return HTTPERR_HOSTERROR;
	memset((char *) &server,0, sizeof(server));
	memmove((char *) &server.sin_addr, hp->h_addr, hp->h_length);
	server.sin_family = hp->h_addrtype;
	server.sin_port = (unsigned short) htons( 80 );
	/* create socket */
	SOCKET s;
	s = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, 0, 0);
	if( connect(s, (sockaddr*)&server, sizeof(server)) == SOCKET_ERROR )
	{
		closesocket(s);
		return HTTPERR_COULDNOTCONNECT;
	}
	// create header
	char header[1024*4];
	header[0] = '\0';
	// GET ...
	strcat(header, "POST ");
	strcat(header, path);
	strcat(header, " HTTP/1.1\r\n");
	// Host ...
	strcat(header, "Host: ");
	strcat(header, host);
	strcat(header, "\r\n");
	// Accept
	strcat(header, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n");
	// Useragent ...
	strcat(header, "User-Agent: ");
	strcat(header, "Mozilla/5.0 (Windows; U; Windows NT 6.1; de; rv:1.9.2.14) Gecko/20110218 Firefox/3.6.14");
	strcat(header, "\r\n");
	// content info
	strcat(header, "Content-Type: application/x-www-form-urlencoded\r\n");
	strcat(header, "Content-Length: ");
	char contentLengthStr[16];
	wsprintf(contentLengthStr, "%d", strlen(post));
	strcat(header, contentLengthStr);
	strcat(header, "\r\n");
	// content data
	strcat(header, "\r\n");
	strcat(header, post);



	// send header
	int hLen = strlen(header);
	if( send(s, (const char*)header, hLen, 0) != hLen )
	{
		closesocket(s);
		return HTTPERR_NETERROR;
	}	
	char lineBuffer[1024];
	memset(lineBuffer, 0x00, sizeof(lineBuffer));
	// read HTTP response type
	_httpReadLine(s, lineBuffer, sizeof(lineBuffer));
	char *lb = lineBuffer;
	if( memcmp(lb, "HTTP/1.1 ", 9) )
	{
		closesocket(s);
		return HTTPERR_PARSEERROR;
	}
	lb += 9;
	int httpRetCode = -1;
	httpRetCode = atoi(lb);
	if( httpRetCode == -1 )
	{
		closesocket(s);
		return HTTPERR_PARSEERROR;
	}
	if( httpRetCode != HTTPERR_OK ) // success
	{
		closesocket(s);
		return httpRetCode;
	}
	// read other
	int dataLength = -1;
	while( _httpReadLine(s, lineBuffer, sizeof(lineBuffer)) > 0 )
	{
		// Content-Length: 2337
		if( memcmp(lineBuffer, "Content-Length: ", 16) == 0 )
		{
			char *plb = lineBuffer+16;
			dataLength = atoi(plb);
		}
		//__debugbreak();
		//puts(lineBuffer);
	}
	// read content length
	if( dataLength == -1 ) // no content-length passed
	{
		if( _httpReadLine(s, lineBuffer, sizeof(lineBuffer)) <= 0 )
			return HTTPERR_PARSEERROR;
		dataLength = hexAtio(lineBuffer, -1);
	}
	if( dataLength == -1 )
	{
		closesocket(s);
		return HTTPERR_PARSEERROR;
	}
	if( dataLength == 0 )
	{
		// no data..
		*pLength = 0;
		closesocket(s);
		return httpRetCode;
	}
	// read data
	char *httpData = outBuffer;
	int index = 0;
	dataLength = min(dataLength, (*pLength-1));
	*pLength = dataLength;
	while( index < dataLength )
	{
		int r = recv(s, httpData+index, (dataLength-index), 0);
		if( r <= 0 )
			return HTTPERR_PARSEERROR;
		index += r;
	}
	httpData[index] = '\0';
	closesocket(s);
	//GET /tr/content/server/auth_request_form.php HTTP/1.1
		//Host: jhwork.net
		//User-Agent: Mozilla/5.0 (Windows; U; Windows NT 6.1; de; rv:1.9.2.14) Gecko/20110218 Firefox/3.6.14
		//Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
		//Accept-Language: de-de,de;q=0.8,en-us;q=0.5,en;q=0.3
		//Accept-Encoding: gzip,deflate
		//Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7
		//Keep-Alive: 115
		//Connection: keep-alive


	return HTTPERR_OK;
}

#endif