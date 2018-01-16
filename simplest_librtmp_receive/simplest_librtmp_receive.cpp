#include <stdio.h>
#include "librtmp/rtmp_sys.h"
#include "librtmp/log.h"
#include <time.h>
#include <string>
#include <direct.h>
#include <io.h>
using namespace std;

#define SavePath "D:\\vedio\\"

int InitSockets()
{
	WORD version;
	WSADATA wsaData;
	version = MAKEWORD(1, 1);
	return (WSAStartup(version, &wsaData) == 0);
}

void CleanupSockets()
{
	WSACleanup();
}

RTMP * ConnectRtmp(char* Uri)
{
	//is live stream ?
	bool bLiveStream=true;		
	InitSockets();
	RTMP *rtmp=RTMP_Alloc();
	RTMP_Init(rtmp);
	//set connection timeout,default 30s
	rtmp->Link.timeout=10;	
	// HKS's live URL
	if(!RTMP_SetupURL(rtmp,Uri))
	{
		RTMP_Log(RTMP_LOGERROR,"SetupURL Err\n");
		RTMP_Free(rtmp);
		CleanupSockets();
		return NULL;
	}
	if (bLiveStream){
		rtmp->Link.lFlags|=RTMP_LF_LIVE;
	}

	//5 mins
	RTMP_SetBufferMS(rtmp, 300*1000);		

	if(!RTMP_Connect(rtmp,NULL)){
		RTMP_Log(RTMP_LOGERROR,"Connect Err\n");
		RTMP_Free(rtmp);
		CleanupSockets();
		return NULL;
	}

	if(!RTMP_ConnectStream(rtmp,0)){
		RTMP_Log(RTMP_LOGERROR,"ConnectStream Err\n");
		RTMP_Free(rtmp);
		RTMP_Close(rtmp);
		CleanupSockets();
		return NULL;
	}
	return rtmp;
}

void CloseRtmp(RTMP* rtmp)
{
	if ( rtmp == NULL)
	{
		return ;
	}
	RTMP_Close(rtmp);
	RTMP_Free(rtmp);
	CleanupSockets();
	return;
}

//create path or file
FILE * CreateFile(string strFilePath)
{
	FILE * fp=NULL;
	fp = fopen(strFilePath.c_str(), "wb");
	//path not exist,create it
	int indexStart = 0;
	int indexEnd = 0;
	if ( ! fp)
	{
		indexEnd = strFilePath.find("\\",indexStart);
		while(indexEnd != string::npos){
			string strPath = strFilePath.substr(0,indexEnd);
			if ( -1 == access(strPath.c_str(),0))
			{
				int iRes;
				iRes = mkdir(strPath.c_str());
			}
			indexStart = indexEnd+1;
			indexEnd = strFilePath.find("\\",indexStart);
		}
		fp = fopen(strFilePath.c_str(), "wb");
	}
	return fp;
}

int main(int argc, char* argv[])
{
	RTMP * rtmp = NULL;
	double duration=-1;
	int nRead;
	int bufsize = 1024*1024*10;
	char *buf=(char*)malloc(bufsize);
	memset(buf,0,bufsize);
	long countbufsize=0;
	

	while(true){

		time_t tt = time(NULL);
		struct tm * nt = localtime(&tt);

		/* set log level */
		//RTMP_LogLevel loglvl=RTMP_LOGDEBUG;
		//RTMP_LogSetLevel(loglvl);
		char Uri[512] = "rtmp://192.168.20.230/live/livestream";
		rtmp = ConnectRtmp( Uri );

		//get current time
		//gen filename

		int prevTime = nt->tm_min;

		char fileName[256];
		strftime(fileName, 256,"%Y\\%m\\%d\\%Y%m%d-%H%M%S",nt);
		strcat(fileName, ".flv");
		string strFilePath = SavePath + string(fileName);

		//FILE *fp=fopen(strFilePath.c_str(),"wb");
		FILE * fp = CreateFile(strFilePath);
		if (!fp){
			RTMP_LogPrintf("Open File Error.\n");
			CleanupSockets();
			return -1;
		}
		char bufHeader[1000];
		int lenHeader = RTMP_Read(rtmp, bufHeader, 1000);
		fwrite(bufHeader,1,lenHeader, fp);
		countbufsize += lenHeader;
		
		while(nRead=RTMP_Read(rtmp,buf,bufsize)){

			fwrite(buf,1,nRead,fp);

			countbufsize+=nRead;
			RTMP_LogPrintf("Receive: %5dByte, Total: %5.2fkB\n",nRead,countbufsize*1.0/1024);
			//update file name
			tt = time(NULL);
			nt = localtime(&tt);

// 
			if ( nt->tm_min%5==0 && nt->tm_min!=prevTime) 
			{
				prevTime = nt->tm_min;
				fclose(fp);
				//////////////
				//5 mins
				CloseRtmp(rtmp);
				rtmp = ConnectRtmp(Uri);
				//////////////

				strftime(fileName, 256, "%Y\\%m\\%d\\%Y%m%d-%H%M%S", nt);
				strcat(fileName, ".flv");
				string strFilePath = SavePath + string(fileName);
				fp = CreateFile(strFilePath.c_str());
				//fwrite(bufHeader, 1, lenHeader, fp);
				countbufsize = 0;

				if (!fp)
				{
					RTMP_LogPrintf("Open File Error!\n");
					CleanupSockets();
					return -1;
				}
			}
		}

		if(fp)
			fclose(fp);

		if(buf){
			free(buf);
		}

		if(rtmp){
			RTMP_Close(rtmp);
			RTMP_Free(rtmp);
			CleanupSockets();
			rtmp=NULL;
		}	
	}
	getchar();
	return 0;
}