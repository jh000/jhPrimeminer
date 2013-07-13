#ifdef _SYSLIB_USE_SIMPLEHTTP

int simpleHTTP_directGET(char *url, char *outBuffer, int *pLength);
int simpleHTTP_directPOST(char *url, char *post, char *outBuffer, int *pLength);

#define HTTPERR_OK			200
#define HTTPERR_NOTFOUND	404


#define HTTPERR_INVALIDURL			1001
#define HTTPERR_COULDNOTCONNECT		1002
#define HTTPERR_HOSTERROR			1003
#define HTTPERR_NETERROR			1004
#define HTTPERR_PARSEERROR			1005

#endif