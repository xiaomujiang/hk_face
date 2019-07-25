/*
* Copyright(C) 2011,Hikvision Digital Technology Co., Ltd 
* 
* File   name��main.cpp
* Discription��demo for muti thread get stream
* Version    ��1.0
* Author     ��luoyuhua
* Create Date��2011-12-10
* Modification History��
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "HCNetSDK.h"
#include "iniFile.h"
#include <stdlib.h>
#include <sys/io.h>
#include <iostream>
#include <iconv.h>
#include <memory.h>
#include <time.h>
#include<typeinfo>
#include<fstream>
#include<string>
#include<iostream>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
using namespace std;
#ifdef _WIN32
#elif defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#endif
//时间解释宏定义
#define GET_YEAR(_time_) (((_time_) >> 26) + 2000)
#define GET_MONTH(_time_) (((_time_) >> 22) & 15)
#define GET_DAY(_time_) (((_time_) >> 17) & 31)
#define GET_HOUR(_time_) (((_time_) >> 12) & 31)
#define GET_MINUTE(_time_) (((_time_) >> 6) & 63)
#define GET_SECOND(_time_) (((_time_) >> 0) & 63)
#define HPR_OK 0
#define HPR_ERROR -1


// struct ParamsCam  
// {  
// 	char *_img_save_path;
// 	char *_camera_id;
// 	char *_face_dir_name;
// };

// 代码转换操作类
class CodeConverter
{
  private:
	iconv_t cd;

  public:
	// 构造
	CodeConverter(const char *from_charset, const char *to_charset)
	{
		cd = iconv_open(to_charset, from_charset);
	}
	// 析构
	~CodeConverter()
	{
		iconv_close(cd);
	}
	// 转换输出
	int convert(char *inbuf, int inlen, char *outbuf, int outlen)
	{
		char **pin = &inbuf;
		char **pout = &outbuf;
		memset(outbuf, 0, outlen);
		return iconv(cd, pin, (size_t *)&inlen, pout, (size_t *)&outlen);
	}
};

void CALLBACK MessageCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void *pUser)
{

	int i;
	NET_DVR_ALARMINFO_V30 struAlarmInfo;
	memcpy(&struAlarmInfo, pAlarmInfo, sizeof(NET_DVR_ALARMINFO_V30));
	// struct ParamsCam *newPcam=(struct ParamsCam*)pUser;
	char *img_save_path=(char *)pUser;
	// cout << newPcam->_camera_id<<endl;
	switch (lCommand)
	{
		case COMM_SNAP_MATCH_ALARM:
		{

			NET_VCA_FACESNAP_MATCH_ALARM struFaceMatchAlarm = {0};
			memcpy(&struFaceMatchAlarm, pAlarmInfo, sizeof(NET_VCA_FACESNAP_MATCH_ALARM));
			
			NET_DVR_TIME struAbsTime = {0};
			struAbsTime.dwYear = GET_YEAR(struFaceMatchAlarm.struSnapInfo.dwAbsTime);
			struAbsTime.dwMonth = GET_MONTH(struFaceMatchAlarm.struSnapInfo.dwAbsTime);
			struAbsTime.dwDay = GET_DAY(struFaceMatchAlarm.struSnapInfo.dwAbsTime);
			struAbsTime.dwHour = GET_HOUR(struFaceMatchAlarm.struSnapInfo.dwAbsTime);
			struAbsTime.dwMinute = GET_MINUTE(struFaceMatchAlarm.struSnapInfo.dwAbsTime);
			struAbsTime.dwSecond = GET_SECOND(struFaceMatchAlarm.struSnapInfo.dwAbsTime);
			// BYTE sex = struFaceMatchAlarm.struSnapInfo.bySex;
			char strName[32];
			memcpy(strName, struFaceMatchAlarm.struBlackListInfo.struBlackListInfo.struAttribute.byName,
			   sizeof(struFaceMatchAlarm.struBlackListInfo.struBlackListInfo.struAttribute.byName));
			strName[sizeof(struFaceMatchAlarm.struBlackListInfo.struBlackListInfo.struAttribute.byName)] = '\0';

			char NameOut[255];
			CodeConverter cc = CodeConverter("gb2312", "utf-8");
			cc.convert(strName, strlen(strName), NameOut, 255);

			char strID[32]; 
			memcpy(strID, struFaceMatchAlarm.struBlackListInfo.struBlackListInfo.struAttribute.byCertificateNumber,
				sizeof(struFaceMatchAlarm.struBlackListInfo.struBlackListInfo.struAttribute.byCertificateNumber));

			if (struFaceMatchAlarm.fSimilarity < 0.7)
			{
				sprintf(strID, "00000000");
				sprintf(NameOut, "未知人员");
			}
			
			if (struFaceMatchAlarm.struSnapInfo.dwSnapFacePicLen > 0 && struFaceMatchAlarm.struSnapInfo.pBuffer1!= NULL)
			{	
				char cFilename[256] = {0};
				char chFile[256] = {0};
				char chFilePWD[256] = {0};
				char cnowday[256];
				char cSimilariry[128]={0};
				char chTime_back[1024];
				HANDLE hFile;
				DWORD dwReturn;
				sprintf(chTime_back, "%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d", struAbsTime.dwYear, struAbsTime.dwMonth, struAbsTime.dwDay, struAbsTime.dwHour, struAbsTime.dwMinute, struAbsTime.dwSecond);
				sprintf(cnowday, "%4.4d-%2.2d-%2.2d", struAbsTime.dwYear, struAbsTime.dwMonth, struAbsTime.dwDay);
				sprintf(cSimilariry, "%1.3f", struFaceMatchAlarm.fSimilarity);
				sprintf(cFilename, "%s_%s_%s.jpg",chTime_back,strID, cSimilariry);
				sprintf(chFilePWD, "%s/%s/%s",img_save_path,cnowday,cFilename);
				sprintf(chFile, "%s/%s",img_save_path,cnowday);

				if (access(chFile, 0) == -1){
					int flag = mkdir(chFile,S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
				}
				FILE *pFile;
				pFile = fopen(chFilePWD, "wb");
				fwrite(
					struFaceMatchAlarm.struSnapInfo.pBuffer1,
					sizeof(char),
					struFaceMatchAlarm.struSnapInfo.dwSnapFacePicLen,
					pFile
				);
				fclose(pFile);
				// 判断图片是否存在
				if (access(chFilePWD, 0) != -1){
					cout << "CPP match face:"<< cFilename<< endl;
				}
			}
		}break;
		default:
			break;
	}
}

int Demo_AlarmFortify(char *DeviceIP, int port, char *userName, char *password,char *img_save_path)
{
	LONG lUserID;
	NET_DVR_DEVICEINFO_V30 struDeviceInfo;
	NET_DVR_Init();
	lUserID = NET_DVR_Login_V30(DeviceIP, port, userName, password, &struDeviceInfo);
	if (lUserID < 0)
	{
		printf("Login error, %d\n", NET_DVR_GetLastError());
		NET_DVR_Cleanup();
		return HPR_ERROR;
	}
	// 传递多个参数有问题
	// struct ParamsCam pcam;
	// pcam._img_save_path=img_save_path;
	// pcam._camera_id=camera_id;
	// pcam._face_dir_name=face_dir_name;

	cout << "设置回调函数！！！" << endl;
	// NET_DVR_SetDVRMessageCallBack_V30(MessageCallback,camera_id);
	NET_DVR_SetDVRMessageCallBack_V30(MessageCallback,img_save_path);
	cout << "回调设置完成！！！" << endl;
	LONG lHandle;
	lHandle = NET_DVR_SetupAlarmChan_V30(lUserID);
	if (lHandle < 0)
	{
		printf("NET_DVR_SetupAlarmChan_V30 error, %d\n", NET_DVR_GetLastError());
		NET_DVR_Logout(lUserID);
		NET_DVR_Cleanup();
		return HPR_ERROR;
	}
}

extern "C" {

	int FaceDetectAndContrast(char *DeviceIP, int port, char *userName, char *password,char *img_save_path)
	{
		Demo_AlarmFortify(DeviceIP, port, userName, password,img_save_path);
		return 0;
	}


	int UploadFile(char *DeviceIP, int port, char *userName, char *password, char *picPath,char *sdk_path)
	{
		NET_DVR_Init();
		NET_DVR_LOCAL_SDK_PATH struLocalPath;
		strcpy(struLocalPath.sPath, sdk_path);
		bool set_path = NET_DVR_SetSDKInitCfg(NET_SDK_INIT_CFG_SDK_PATH, &struLocalPath);

		NET_DVR_DEVICEINFO_V30 struDeviceInfo;

		// printf("传进来的参数,IP:%s,端口:%d,用户名：%s,密码：%s,图片路径：%s\n", DeviceIP, port, userName, password, picPath);
		int iUserID = NET_DVR_Login_V30(DeviceIP, port, userName, password, &struDeviceInfo);
		if (iUserID >= 0)
		{
			NET_DVR_FACELIB_COND FaceInBuffer = {0};
			FaceInBuffer.dwSize = sizeof(FaceInBuffer);
			
			strcpy(FaceInBuffer.szFDID, "1");
			FaceInBuffer.byCover = 1;
			FaceInBuffer.byConcurrent = 0;
			int m_lUploadHandle = NET_DVR_UploadFile_V40(iUserID, IMPORT_DATA_TO_FACELIB, &FaceInBuffer, FaceInBuffer.dwSize, NULL, NULL, 0);
			printf("上传文件：%d\n", m_lUploadHandle);
	
			BYTE *pSendAppendData;
			BYTE *pSendPicData;
			//printf("BYTE*定义完成!!\n");
			NET_DVR_SEND_PARAM_IN m_struSendParam;
			
			printf("数据类型定义完成！\n");
			memset(&m_struSendParam, 0, sizeof(m_struSendParam));
			printf("内存清零操作完成！！\n");
			char szLan[1280] = {0};
			char szFileName[255]; //注，MFC综合实例中给出的长度是MAX_PATH,未找到定义
			DWORD dwFileSize = 0;
			//读取XML文件
			printf("开始读取xml文件！！\n");
			sprintf(szFileName, "%s/tmp.xml",sdk_path);
			FILE *fp;
			if ((fp = fopen(szFileName, "r")) == NULL)
			{
				printf("文件打开不成功！\n");
			}
			fseek(fp, 0L, SEEK_END);
			dwFileSize = ftell(fp);
			// printf("XML文件大小：%d\n", dwFileSize);
			if (dwFileSize == 0)
			{
				printf("XML文件为空！！\n");
			}
			fseek(fp, 0L, SEEK_SET);
			pSendAppendData = new BYTE[dwFileSize];
			fread(pSendAppendData, sizeof(BYTE), dwFileSize, fp);
			m_struSendParam.pSendAppendData = pSendAppendData;
			m_struSendParam.dwSendAppendDataLen = dwFileSize;
			fclose(fp);
			printf("xml文件读取完成，开始读取图片文件！！\n");
			//读取图片文件
			strcpy(szFileName, picPath);
			FILE *picFP;
			if ((picFP = fopen(szFileName, "r")) == NULL)
			{
				printf("图片打开失败！！\n");
			}
			fseek(picFP, 0L, SEEK_END);
			dwFileSize = ftell(picFP);
			fseek(picFP, 0L, SEEK_SET);
			printf("图片文件大小：%d\n", dwFileSize);
			if (dwFileSize == 0)
			{
				printf("图片文件为空！！\n");
			}
			pSendPicData = new BYTE[dwFileSize];
			fread(pSendPicData, sizeof(BYTE), dwFileSize, picFP);
			m_struSendParam.pSendData = pSendPicData;
			m_struSendParam.dwSendDataLen = dwFileSize;
			m_struSendParam.byPicType = 1;
			m_struSendParam.byPicURL = 0;
			fclose(picFP);

			printf("开始上传图片数据！！！\n");

			if (NET_DVR_UploadSend(m_lUploadHandle, &m_struSendParam, NULL) < 0)
			{
				printf("\n上传失败,错误代号：%d\n", NET_DVR_GetLastError());
				return 0;
			}
			else
			{
				uint pProgress = 0;
				
				while (pProgress < 100)
				{
					int iStatus = NET_DVR_GetUploadState(m_lUploadHandle, &pProgress);
					if (iStatus == -1)
					{
						printf("上传失败,错误号:%d\n", NET_DVR_GetLastError());
						usleep(100000);
						break;
					}
					else if (iStatus == 2)
					{
						printf("文件正在上传，进度：%d\n", pProgress);
						usleep(100000);
					}
					else if (iStatus == 1)
					{
						usleep(100000);
						printf("上传成功！\n");
						return 1;
						break;
					}
					else
					{
						printf("上传失败，状态标志位：%d,错误标志号：%d\n", iStatus, NET_DVR_GetLastError());
						break;
					}
				}
			}
		}
		return 0;
	}

}





