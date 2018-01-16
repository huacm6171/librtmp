// filecut.cpp : 定义控制台应用程序的入口点。
//
#include <stdio.h>
#include "librtmp/rtmp_sys.h"
#include "librtmp/log.h"
#include <time.h>
#include <string>
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

int main(int argc, char* argv[])
{


	double duration=-1;
	int nRead;
	//is live stream ?
	bool bLiveStream=true;				


	int bufsize=1024*1024*10;			
	char *buf=(char*)malloc(bufsize);
	memset(buf,0,bufsize);
	long countbufsize=0;


	while(true){

		time_t tt = time(NULL);
		struct tm * nt = localtime(&tt);
		if ( nt->tm_hour > 18 || nt->tm_hour < 5)
		{
			RTMP_LogPrintf("sleep 10 secends...\n");
			sleep(10);
			continue;
		}
		/* set log level */
		//RTMP_LogLevel loglvl=RTMP_LOGDEBUG;
		//RTMP_LogSetLevel(loglvl);
		InitSockets();
		RTMP *rtmp=RTMP_Alloc();
		RTMP_Init(rtmp);
		//set connection timeout,default 30s
		rtmp->Link.timeout=10;	
		// HKS's live URL
		if(!RTMP_SetupURL(rtmp,"rtmp://live.qab.co.jp:1935/live/livenahacity.sdp"))
		{
			RTMP_Log(RTMP_LOGERROR,"SetupURL Err\n");
			RTMP_Free(rtmp);
			CleanupSockets();
			return -1;
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
			return -1;
		}

		if(!RTMP_ConnectStream(rtmp,0)){
			RTMP_Log(RTMP_LOGERROR,"ConnectStream Err\n");
			RTMP_Free(rtmp);
			RTMP_Close(rtmp);
			CleanupSockets();
			return -1;
		}

		//get current time
		//gen filename

		int prevTime = nt->tm_min;

		char fileName[256];
		strftime(fileName, 256,"%Y-%m-%d-%H-%M",nt);
		strcat(fileName, ".flv");
		string strFilePath = SavePath + string(fileName);

		FILE *fp=fopen(strFilePath.c_str(),"wb");
		if (!fp){
			RTMP_LogPrintf("Open File Error.\n");
			CleanupSockets();
			return -1;
		}
		char bufHeader[1000];
		int lenHeader = RTMP_Read(rtmp, bufHeader, 1000);
		fwrite(bufHeader,1,lenHeader, fp);
		countbufsize += lenHeader;
		//////////////////////////////////////
		RTMP * rtmpServer = RTMP_Alloc();
		RTMP_Init(rtmpServer);
		rtmpServer->Link.timeout = 5;
		if ( !RTMP_SetupURL(rtmpServer, "rtmp://localhost/live/livestream"))
		{
			RTMP_Log(RTMP_LOGERROR, "SetupURL Error!\n");
			RTMP_Free(rtmpServer);
			CleanupSockets();
			return -1;
		}

		RTMP_EnableWrite(rtmpServer);
		RTMP_SetBufferMS(rtmpServer, 3600*1000);
		if ( !RTMP_Connect(rtmpServer, NULL))
		{
			RTMP_Log(RTMP_LOGERROR, "Connect Rtmp Server Error!\n");
			RTMP_Free(rtmpServer);
			CleanupSockets();
			return -1;
		}
		if(! RTMP_ConnectStream(rtmpServer, 0))
		{
			RTMP_Log(RTMP_LOGERROR,"ConnectStream Err\n");
			RTMP_Close(rtmp);
			RTMP_Free(rtmp);
			CleanupSockets();
			return -1;
		}
		//////////////////////////////////////
		while(nRead=RTMP_Read(rtmp,buf,bufsize)){

			/////////////start send data//////////
			if (!RTMP_IsConnected(rtmp)){
				RTMP_Log(RTMP_LOGERROR,"rtmp is not connect\n");
				break;
			}
			if ( ! RTMP_Write(rtmpServer, buf, nRead))
			{
				RTMP_Log(RTMP_LOGERROR, "Rtmp Write Error!\n");
				break;
			}
			///////////////////////////////////////
			fwrite(buf,1,nRead,fp);

			countbufsize+=nRead;
			RTMP_LogPrintf("Receive: %5dByte, Total: %5.2fkB\n",nRead,countbufsize*1.0/1024);
			//update file name
			tt = time(NULL);
			nt = localtime(&tt);

			if ( nt->tm_hour > 18 || nt->tm_hour < 5)
			{
				fclose(fp);
				break;
			}
			// 
			// 			if ( nt->tm_min%5==0 && nt->tm_min!=prevTime) 
			// 			{
			// 				prevTime = nt->tm_min;
			// 				fclose(fp);
			// 				//////////////
			// 				//5 mins
			// 				RTMP_UpdateBufferMS(rtmp);	
			// 				//////////////
			// 
			// 				strftime(fileName, 256, "%Y%m%d%H%M%S", nt);
			// 				strcat(fileName, ".flv");
			// 				string strFilePath = SavePath + string(fileName);
			// 				fp = fopen(strFilePath.c_str(), "wb");
			// 				fwrite(bufHeader, 1, lenHeader, fp);
			// 				countbufsize = lenHeader;
			// 
			// 				if (!fp)
			// 				{
			// 					RTMP_LogPrintf("Open File Error!\n");
			// 					CleanupSockets();
			// 					return -1;
			// 				}
			// 			}
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
	return 0;
}