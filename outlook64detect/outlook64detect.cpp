// outlook64detect.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "md5.h"
#include <UrlMon.h>
#include <WinSock.h>


#define DEFAULT_PORT 80
#define SUFIX_32_BIT "DiscoveryBOT.exe"
#define SUFIX_64_BIT "DiscoveryBOT_64.exe"
#define BASELINK L"http://download.totaldiscovery.com/"

#define DEFAULT_BUFLEN 4096

bool IsOutlook64bitInstaled()
{
      TCHAR pszaOutlookQualifiedComponents[][MAX_PATH] = {
        TEXT("{1E77DE88-BCAB-4C37-B9E5-073AF52DFD7A}"), // Outlook 2010
        TEXT("{24AAE126-0911-478F-A019-07B875EB9996}"), // Outlook 2007
        TEXT("{BC174BAD-2F53-4855-A1D5-0D575C19B1EA}")  // Outlook 2003
    };

    int nOutlookQualifiedComponents = sizeof(pszaOutlookQualifiedComponents)/(MAX_PATH*sizeof(TCHAR));   //Changed
    int i = 0;
    DWORD dwValueBuf = 0;
	UINT ret = ERROR_NOT_FOUND;

    for (i = 0; i < nOutlookQualifiedComponents; i++)
    {
        ret = MsiProvideQualifiedComponent(
            pszaOutlookQualifiedComponents[i],
            TEXT("outlook.x64.exe"),
            (DWORD) INSTALLMODE_DEFAULT,
            NULL,
            &dwValueBuf);
        if (ERROR_SUCCESS == ret) break;
    }

    if (ret != ERROR_SUCCESS){
		return false;
    }

    return true; 
}



bool getAmazonHeader(bool bitVersion, char* returnBuffer, int buffersize){
     WSADATA wsaData = {0};
    int iResult = 0;

    SOCKET sock = INVALID_SOCKET;
    int iFamily = AF_INET;
    int iType = SOCK_STREAM;
    int iProtocol = IPPROTO_TCP;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        wprintf(L"ERROR!!! WSAStartup failed: %d\n", iResult);
		return false;
    }

    sock = socket(iFamily, iType, iProtocol);
    if (sock == INVALID_SOCKET){ 
        wprintf(L"ERROR!!! socket function failed with error = %d\n", WSAGetLastError() );
		return false;
	}else {
        wprintf(L"socket function succeeded\n");

		struct sockaddr_in clientService; 

		char *sendbuf = NULL;
		char *sendbuf32 = "HEAD /"SUFIX_32_BIT" HTTP/1.1\nUser-Agent: Opera/9.80 (Windows NT 5.1; U; DepositFiles/FileManager 0.9.9.206 YB/5.0.3; ru) Presto/2.10.289 Version/12.01\nHost: download.totaldiscovery.com\nAccept: text/html, application/xml;q=0.9, application/xhtml+xml, image/png, image/webp, image/jpeg, image/gif, image/x-xbitmap, */*;q=0.1\nAccept-Language: ru,en;q=0.9,ru-RU;q=0.8\nAccept-Encoding: gzip, deflate\nConnection: Keep-Alive\r\n\r\n";
		char *sendbuf64 = "HEAD /"SUFIX_64_BIT" HTTP/1.1\nUser-Agent: Opera/9.80 (Windows NT 5.1; U; DepositFiles/FileManager 0.9.9.206 YB/5.0.3; ru) Presto/2.10.289 Version/12.01\nHost: download.totaldiscovery.com\nAccept: text/html, application/xml;q=0.9, application/xhtml+xml, image/png, image/webp, image/jpeg, image/gif, image/x-xbitmap, */*;q=0.1\nAccept-Language: ru,en;q=0.9,ru-RU;q=0.8\nAccept-Encoding: gzip, deflate\nConnection: Keep-Alive\r\n\r\n";
		
		if(bitVersion == true){
			sendbuf = sendbuf64;
		}else{
			sendbuf = sendbuf32;
		}
		//----------------------
		// The sockaddr_in structure specifies the address family,
		// IP address, and port of the server to be connected to.
		clientService.sin_family = AF_INET;
		clientService.sin_addr.s_addr = inet_addr( "216.137.61.168" );
		clientService.sin_port = htons( DEFAULT_PORT );

		//----------------------
		// Connect to server.
		iResult = connect( sock, (SOCKADDR*) &clientService, sizeof(clientService) );
		if (iResult == SOCKET_ERROR) {
			wprintf(L"ERROR!!! connect failed with error: %d\n", WSAGetLastError() );
			closesocket(sock);
			WSACleanup();
			return false;
		}

		//----------------------
		// Send an initial buffer
		iResult = send( sock, sendbuf, (int)strlen(sendbuf), 0 );
		if (iResult == SOCKET_ERROR) {
			wprintf(L"ERROR!!! send failed with error: %d\n", WSAGetLastError());
			closesocket(sock);
			WSACleanup();
			return false;
		}

		printf("Bytes Sent: %d\n", iResult);

		iResult = recv(sock, returnBuffer, buffersize, 0);
		if ( iResult > 0 ){
			wprintf(L"Bytes received: %d\n", iResult);
		}else if ( iResult == 0 ){
			wprintf(L"ERROR!!! Connection closed, nothing received.\n");
			return false;
		}else{
			wprintf(L"ERROR!!! recv failed with error: %d\n", WSAGetLastError());
			return false;
		}

        iResult = closesocket(sock);
        if (iResult == SOCKET_ERROR) {
            wprintf(L"ERROR!!! closesocket failed with error = %d\n", WSAGetLastError() );
            WSACleanup();
			return false;
        }    
    }

    WSACleanup();
	return true;

}

int runTool(LPCWSTR pathBuffer)
{
	 printf("Run tool...\n");
	 HINSTANCE nResult = ShellExecute( NULL, NULL, pathBuffer,NULL, NULL, SW_SHOWNORMAL);
	 if((int)nResult > 32){
		 return 0;
	 }else{
		 printf("\nERROR!!!!  ShellExecute return error number %i",(int)nResult);
		 return (int)nResult;
	 }
}

int downloadAndRunTool(LPCWSTR linkBuffer,LPCWSTR pathBuffer)
{
	printf("Downloading tool from Amazon, please wait...\n");
	HRESULT hr = URLDownloadToFile( NULL, linkBuffer, pathBuffer, 0, NULL );
	if(FAILED(hr)){
		printf("Can`t downloading tool from Amazon servers");
		return 1;
	}

	return runTool(pathBuffer);
}

int _tmain(int argc, _TCHAR* argv[])
{
	TCHAR szActualLink[DEFAULT_BUFLEN];

	printf("Program start work...\n");
	//Check Outlook version.
	bool isNeed64bit = IsOutlook64bitInstaled(); 

	wcscpy(szActualLink,BASELINK);
	if(isNeed64bit == false){
		//Outlook not found or 32b version was detected
		printf("Outlook 32bit detected or outlook not detected.. Use tool for 32bit. \n");
		wcscat(szActualLink,_T(SUFIX_32_BIT));
	}else{
		//64b version was detected.
		printf("Outlook 64bit detected. Use tool for 64bit. \n");
		wcscat(szActualLink,_T(SUFIX_64_BIT));
	}
	 _tprintf(_T("Link to tool on Amazon site: %s\n"),szActualLink);
	
	 //
	 //Check if tool already exists on local PC.
	 //

	 TCHAR lpTempPathBuffer[MAX_PATH+1];
	 DWORD dwRetVal = GetTempPath(MAX_PATH,lpTempPathBuffer);
	 if(dwRetVal == 0){
		 printf("\nERROR!!! Can`t receive user temp folder\n");
		 return 1;
	 }
	 if(dwRetVal > 240){
		 printf("\nERROR!!! Path to temp folder too long.\n");
		 return 1;
	 }
	 wcscat(lpTempPathBuffer,_T("DiscoveryBOT.exe"));
	 _tprintf(_T("Path to local tool: %s\n"),lpTempPathBuffer);

	//Check if tool exists on local PC.
	printf("Check: Does local version of tool exist?\n");
	bool isToolExist = false;
	FILE *fp = _tfopen(lpTempPathBuffer, _T("r"));

	if(fp != NULL){
		printf("Tool exists.\n");
		isToolExist = true;
		fclose(fp);
	}else{
		printf("Tool was not found.\n");
		isToolExist = false;
	}

	if(isToolExist == false){
		//Local tool not exist. Just download latest version.
		return downloadAndRunTool(szActualLink,lpTempPathBuffer);
	}

	 //Tool exists. Need compare MD5 hash. In case if not equal download latest tool from Amazon.
	MD5 md5;
	char* szLocalMD5Hash = md5.digestFile(lpTempPathBuffer);
	if(szLocalMD5Hash == NULL){
		return 1;
	}

	printf("Check local tool md5 hash : %s\n", szLocalMD5Hash);

	int recvbuflen = DEFAULT_BUFLEN;
	char recvbuf[DEFAULT_BUFLEN+1] = "";		

	printf("Received  tool header from Amazon\n");
	if (getAmazonHeader(isNeed64bit,recvbuf,recvbuflen) == true){
		printf("\n%s\n\n",recvbuf);

		if(strstr(recvbuf,"200 OK") == NULL){
			printf("ERROR!!! Server response in not valid!!!! Not \"200 OK\".");
			return 1;
		}
			//compare eTag and md5 hash
		printf("Compare Amazon eTag and Local md5 hash\n");
		if(strstr(recvbuf,szLocalMD5Hash) > 0){
			printf("Hash sums the same. We don`t need to download tool from Amazon.\n");
		}else{
			printf("Can`t find md5 hash in header from amazon. Will load latest version from Amazon.\n");
			printf("Delete local tool.\n");
			if( _wunlink(lpTempPathBuffer) == -1 ){
				printf( "ERROR!!! Could not delete local version of tool. May be it`s locked.Try delete it by hands.\n" );
				return 1;
			}else{
				printf( "Local version of tool was deleted.\n" );
			}

			return downloadAndRunTool(szActualLink,lpTempPathBuffer);
		}
	}
	return runTool(lpTempPathBuffer);
}

