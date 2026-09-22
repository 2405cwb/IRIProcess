//#include "stdafx.h"
#include "hnCalcuIRIMethodApi.h"
#include <corecrt_io.h>
//#include "ForSinsDR2.h"
//#include "engine.h"
//#include "mclmcr.h"
#include <direct.h>
#include <QFileInfo>
#include <QSettings>

using namespace hn;
using namespace std;

#define PI64       3.14159265358979323846
#define HGI300_RATE_LSB 0.00048828125
#define HGI300_ACC_LSB 0.009525

// 将字符串转换为浮点(float);
float str2float(const std::string& str)
{
	short int hg1120Result = 0;
	float fResult = 0.0f;

	// 定义初始变量，确定字符串长度;
	int i = (int)strlen(str.data());
	int j = 0;
	int counter = 0;
	char zc[2];
	unsigned int bytes[2];
	unsigned char strDest[128] = { 0 };
	for (j = 0; j < i; j += 2)
	{
		if (0 == j % 2)
		{
			zc[0] = str[j];
			zc[1] = str[j + 1];
			sscanf_s(zc, "%02x", &bytes[0]);
			strDest[counter] = bytes[0];
			counter++;
		}
	}

	fResult = 0.0f;
	memcpy(&hg1120Result, strDest, 2);
	fResult = hg1120Result;

	return fResult;
}

hnCalcuIRIMethodApi::hnCalcuIRIMethodApi()
{
	loadCallback = NULL;

	m_bUseSpeedParam = false;
	m_strDaqPath = "";
	m_nImuHz = 900;
	m_nDmiHz = 2350;
	m_dDmiWheel = 2.35;
	m_isOnRight = 0;
	m_nCurGpsWeek = 0;

	m_SZU = new double[16];
	m_SZU[0] = 0.9966071; m_SZU[1] = 1.091514e-02; m_SZU[2] = -2.083274e-03; m_SZU[3] = 3.190145e-04;
	m_SZU[4] = -0.5563044; m_SZU[5] = 0.9438768; m_SZU[6] = -0.8324718; m_SZU[7] = 5.064701e-02;
	m_SZU[8] = 2.153176e-02; m_SZU[9] = 2.126763e-03; m_SZU[10] = 0.7508714; m_SZU[11] = 8.221888e-03;
	m_SZU[12] = 3.335013; m_SZU[13] = 0.3376467; m_SZU[14] = -39.12762; m_SZU[15] = 0.4347664;

	m_PZU = new double[4];
	m_PZU[0] = 5.47610e-03; m_PZU[1] = 1.388776; m_PZU[2] = 0.2275968; m_PZU[3] = 35.79262;

	m_ZSU = new double[4];
	m_ZSU[0] = 0.0; m_ZSU[1] = 0.0; m_ZSU[2] = 0.0; m_ZSU[3] = 0.0;
	m_oldZSU = new double[4];
	m_oldZSU[0] = 0.0; m_oldZSU[1] = 0.0; m_oldZSU[2] = 0.0; m_oldZSU[3] = 0.0;

	// 拟合相关参数计算;
	int nIntenlen = 13500;
	m_tempx = new double[nIntenlen];
	m_tempy = new double[nIntenlen];
	memset(m_tempx,0,sizeof(double)*nIntenlen);
	memset(m_tempy, 0, sizeof(double)*nIntenlen);

	int maxFitSize = 4;
	m_sumxx_0 = new double[maxFitSize * 2 + 1];
	m_sumxy_0 = new double[maxFitSize + 1];
	m_ata_0 = new double[(maxFitSize + 1)*(maxFitSize + 1)];
	memset(m_sumxx_0, 0, sizeof(double)*(maxFitSize * 2 + 1));
	memset(m_sumxy_0, 0, sizeof(double)*(maxFitSize + 1));
	memset(m_ata_0, 0, sizeof(double)*(maxFitSize + 1)*(maxFitSize + 1));

	m_sumxx_1 = new double[maxFitSize * 2 + 1];
	m_sumxy_1 = new double[maxFitSize + 1];
	m_ata_1 = new double[(maxFitSize + 1)*(maxFitSize + 1)];
	memset(m_sumxx_1, 0, sizeof(double) * (maxFitSize * 2 + 1));
	memset(m_sumxy_1, 0, sizeof(double) * (maxFitSize + 1));
	memset(m_ata_1, 0, sizeof(double) * (maxFitSize + 1)*(maxFitSize + 1));

	m_sumxx_2 = new double[maxFitSize * 2 + 1];
	m_sumxy_2 = new double[maxFitSize + 1];
	m_ata_2 = new double[(maxFitSize + 1)*(maxFitSize + 1)];
	memset(m_sumxx_2, 0, sizeof(double) * (maxFitSize * 2 + 1));
	memset(m_sumxy_2, 0, sizeof(double) * (maxFitSize + 1));
	memset(m_ata_2, 0, sizeof(double) *(maxFitSize + 1)*(maxFitSize + 1));

	m_poly_a[0] = m_poly_a[1] = m_poly_a[2] = m_poly_a[3] = m_poly_a[4] = m_poly_a[5] = 0.0;

	m_dScaleK = 1.0;
	m_dScaleB = 0.0;
	m_bSaveIRI10 = 1;
	m_bSaveIRI100 = 0;
	m_bSaveIRI1000 = 0;
}


hnCalcuIRIMethodApi::~hnCalcuIRIMethodApi()
{
	delete[] m_SZU;
	m_SZU = NULL;

	delete[] m_PZU;
	m_PZU = NULL;
	delete[] m_ZSU;
	m_ZSU = NULL;
	delete[] m_oldZSU;
	m_oldZSU = NULL;

	delete[] m_sumxx_0;
	delete[] m_sumxy_0;
	delete[] m_ata_0;
	delete[] m_sumxx_1;
	delete[] m_sumxy_1;
	delete[] m_ata_1;
	delete[] m_sumxx_2;
	delete[] m_sumxy_2;
	delete[] m_ata_2;
}

void hnCalcuIRIMethodApi::setParam(const char* strDaqPath, int nImuHz, int nDmiHz, double dDmiWheel)
{
	m_strDaqPath = strDaqPath;
	m_nImuHz = nImuHz;
	m_nDmiHz = nDmiHz;
	m_dDmiWheel = dDmiWheel;
}

int hnCalcuIRIMethodApi::calcuIRI()
{
	// 加载数据;
	std::vector<DAQ_STRUCT_INFO> vecDaqInfo;
	std::vector<dmi_speed_type> vecDmiSpeedInfo;
	std::vector<POSD_STRUCT_INFO> vecMatPos;
	bool bSucc = loadDaqData(vecDaqInfo, vecDmiSpeedInfo);
	if (!bSucc || vecDaqInfo.size() <= 10 || vecDmiSpeedInfo.size() <= 10)
	{
		m_strErrMsg = "读取文件失败";
		return 1;
	}

	//int iIndex = 0;
	//std::vector<DAQ_STRUCT_INFO> vecDaqInfoNew;
	//for (int n = 1;n < vecDaqInfo.size();n++)
	//{
	//	DAQ_STRUCT_INFO& preInfo = vecDaqInfo[n-1];
	//	DAQ_STRUCT_INFO& curInfo = vecDaqInfo[n];

	//	// 判断初始编码器变化;
	//	int temp = curInfo.dim - preInfo.dim;
	//	if (curInfo.dim > 1000 && preInfo.dim < 100)
	//	{
	//		iIndex = n;
	//		break;
	//	}
	//}

	//int nTmpDelt = vecDaqInfo[iIndex].dim;
	//for (int n = iIndex;n < vecDaqInfo.size();n++)
	//{
	//	vecDaqInfo[n].dim = vecDaqInfo[n].dim - nTmpDelt;
	//	vecDaqInfoNew.push_back(vecDaqInfo[n]);
	//}

	//vecDaqInfo.clear();
	//vecDaqInfo = vecDaqInfoNew;

	//// 角度分量计算-测试代码;
	//int nStaticCount = 3.0 * 60 * m_nImuHz;

	//// 统计这段时间静止的分量变化，确定偏移角度;
	//double dAccTmpX, dAccTmpY, dAccTmpZ;
	//dAccTmpX = dAccTmpY = dAccTmpZ = 0.0;
	//for (int n = 0;n < nStaticCount;n++)
	//{
	//	DAQ_STRUCT_INFO& info = vecDaqInfo[n];
	//	dAccTmpX += info.xAccelerate;
	//	dAccTmpY += info.yAccelerate;
	//	dAccTmpZ += info.zAccelerate;
	//}
	//dAccTmpX = dAccTmpX / nStaticCount;
	//dAccTmpY = dAccTmpY / nStaticCount;
	//dAccTmpZ = dAccTmpZ / nStaticCount;

#if 0
	{
		// 测试代码，直接积分计算-开始;
		std::vector<POSD_STRUCT_INFO> vecMatPosSpeed;
		double dCurSpeed = 0.0;
		for (int n = 1; n < vecDaqInfo.size(); n++)
		{
			DAQ_STRUCT_INFO& preInfo = vecDaqInfo[n - 1];
			DAQ_STRUCT_INFO& curInfo = vecDaqInfo[n];

			// 相对于标准地球重力的变化量;
			double dtmpPre = preInfo.yAccelerate - dAccTmpY;
			double dtmpCur = curInfo.yAccelerate - dAccTmpY;

			// 获得当前这一刻的速度值;
			double dTmpSpeed = (dtmpCur + (dtmpCur - dtmpPre)/2.0) * (curInfo.gpsSecond - preInfo.gpsSecond);
			dCurSpeed = dCurSpeed + dTmpSpeed;

			POSD_STRUCT_INFO pos;
			pos.dGpsTime = curInfo.gpsSecond;
			pos.dH = dCurSpeed;
			pos.dDist = (curInfo.dim - vecDaqInfo[0].dim) * m_dDmiWheel / m_nDmiHz;

			vecMatPosSpeed.push_back(pos);
		}

		double dCurHeight = 0.0;
		for (int n = 1; n < vecMatPosSpeed.size(); n++)
		{
			POSD_STRUCT_INFO& preInfo = vecMatPosSpeed[n - 1];
			POSD_STRUCT_INFO& curInfo = vecMatPosSpeed[n];

			// 相对于标准地球重力的变化量;
			double dtmpPre = preInfo.dH;
			double dtmpCur = curInfo.dH;

			double dCurS = (dtmpCur + (dtmpCur - dtmpPre) / 2.0) * (curInfo.dGpsTime - preInfo.dGpsTime);

			dCurHeight = dCurHeight + dCurS;

			POSD_STRUCT_INFO pos;
			pos.dGpsTime = curInfo.dGpsTime;
			pos.dH = dCurHeight;
			pos.dDist = curInfo.dDist;

			vecMatPos.push_back(pos);
		}


		// 重采样数据;
		std::vector<POSD_RESAMPLE250_INFO> vecResample250;
		resampleIRI250(vecMatPos, vecResample250);
		if (vecResample250.size() < 10)
		{
			return false;
		}

		// 重采样计算;
		std::string strResamplePath = m_strSaveResultDir + "\\";
		strResamplePath = strResamplePath + m_strSavePreName + "_ReSample250.txt";
		saveReSample250(strResamplePath.data(), vecResample250);

		//// 计算IRI(平整度值) - 10m;
		//std::vector<double> vecIRIResult10;
		//calcuIRIMethod(10.0,0.25, vecResample250, vecIRIResult10);
		//std::string strIRIResultPath10 = m_strSaveResultDir + "\\";
		//strIRIResultPath10 = strIRIResultPath10 + m_strSavePreName + "_IRI_10m.txt";
		//saveIRIs(strIRIResultPath10.data(), vecIRIResult10);

		// 计算IRI(平整度值) - 100m;
		std::vector<double> vecIRIResult100;
		calcuIRIMethod(100.0, 0.25, vecResample250, vecIRIResult100);
		std::string strIRIResultPath100 = m_strSaveResultDir + "\\";
		strIRIResultPath100 = strIRIResultPath100 + m_strSavePreName + "_IRI_100m.txt";
		saveIRIs(strIRIResultPath100.data(), vecIRIResult100);

		//return true;
	}

#endif

	//// 测试代码，直接积分计算-结束;s

	////double dt1 = sqrt(dAccTmpZ * dAccTmpZ + dAccTmpY * dAccTmpY);
	//double tmpAngle = atan2(abs(dAccTmpZ), abs(dAccTmpY)); // 角度值为弧度;
	//tmpAngle = abs(tmpAngle);

	//double dt2 = sqrt(dAccTmpX * dAccTmpX + dAccTmpY * dAccTmpY);
	//double tmpAngleXY = atan2(dAccTmpX, dt2); // 角度值为弧度;

	//double dt2 = sqrt(dAccTmpX * dAccTmpX + dAccTmpZ * dAccTmpZ);
	//double tmpAngleXY = atan2(dt2, abs(dAccTmpY)); // 角度值为弧度;

	//// 根据角度变化更新计算;
	//double tmpX, tmpY, tmpZ;
	//tmpX = tmpY = tmpZ = 0.0;
	//for (unsigned int n = 0;n < vecDaqInfo.size();n++)
	//{
	//	DAQ_STRUCT_INFO& info = vecDaqInfo[n];
	//	//tmpY = info.yAccelerate * cos(tmpAngle) + sqrt(info.zAccelerate * info.zAccelerate) * sin(tmpAngle);
	//	tmpY = info.yAccelerate / cos(tmpAngle);

	//	info.yAccelerate = tmpY;
	//}


	//// 测试代码;
	//std::string strImuPath = m_strDaqPath + ".imu.txt";


	//FILE* ptrFileImu = NULL;
	//fopen_s(&ptrFileImu, strImuPath.data(), "wt");
	//for (unsigned int n = 0; n < vecDaqInfo.size(); n++)
	//{
	//	DAQ_STRUCT_INFO& daqInfo = vecDaqInfo[n];
	//	fprintf_s(ptrFileImu, "%04d%02d%02d,%02d%02d,%d,%d,%ld,%.6lf,%.6lf,%.6lf,%.6lf,%.6lf,%.6lf\n",
	//		daqInfo.utc.year, daqInfo.utc.mon, daqInfo.utc.day, daqInfo.utc.min, daqInfo.utc.sec,
	//		daqInfo.utc.microSec, daqInfo.utc.milliSec, daqInfo.dim,
	//		daqInfo.xVelocity, daqInfo.yVelocity, daqInfo.zVelocity,
	//		daqInfo.xAccelerate, daqInfo.yAccelerate, daqInfo.zAccelerate);
	//}
	//fclose(ptrFileImu);

	//return true;


	//// 测试代码;
	//FILE* ptrFileImu = NULL;
	//fopen_s(&ptrFileImu, "E:\\imu.txt", "wt+");
	//for (unsigned int n = 0;n < vecDaqInfo.size();n++)
	//{
	//	DAQ_STRUCT_INFO& daqInfo = vecDaqInfo[n];
	//	fprintf_s(ptrFileImu, "%.6lf	%.6lf	%.6lf	%.6lf	%.6lf	%.6lf	%.6lf\n",
	//		daqInfo.gpsSecond, daqInfo.xVelocity, daqInfo.yVelocity, daqInfo.zVelocity,
	//		daqInfo.xAccelerate, daqInfo.yAccelerate, daqInfo.zAccelerate);
	//}
	//fclose(ptrFileImu);

	//FILE* ptrFileDmr = NULL;
	//fopen_s(&ptrFileDmr, "E:\\dmr.txt", "wt+");
	//for (unsigned int n = 0; n < vecDmiSpeedInfo.size(); n++)
	//{
	//	dmi_speed_type& speedInfo = vecDmiSpeedInfo[n];
	//	fprintf_s(ptrFileDmr, "%.6lf	%.6lf\n",
	//		speedInfo.gpsSecond, speedInfo.speed);
	//}
	//fclose(ptrFileDmr);
	//return true;


	//// 航位推算数据传入;
	//bSucc = calcuPosBySinsDr(vecDaqInfo,vecDmiSpeedInfo, vecMatPos);
	//if (!bSucc)
	//{
	//	return false;
	//}

	// 单轴加速度计计算高程距离值,调用闫提供接口;
	calcAccDisIntegrate(vecDaqInfo,vecMatPos);

	//FILE* ptrFilePos = NULL;
	//fopen_s(&ptrFilePos, "M:\\NONGCUN_IRI_DATA\\20220301高配版农村路数据\\ceshi_001_上行_ceshi_湖北省_武汉市_江夏区_20220225_112841\\ceshi_001_上行_ceshi_湖北省_武汉市_江夏区_20220225_112841\\test-qt.pos", "wt+");
	//for (unsigned int n = 0; n < vecMatPos.size(); n++)
	//{
	//	// 180585.600   30.9492248495   114.3111321204   28.440    138.0992034358    0.6827989920    -0.0006495655    0.000   0.000   0.000
	//	POSD_STRUCT_INFO& posInfo = vecMatPos[n];
	//	double dtmp = 0.0;
	//	fprintf_s(ptrFilePos, "%.6lf\n",
	//		posInfo.dH);
	//}
	//fclose(ptrFilePos);

	// 重采样数据;
	std::vector<POSD_RESAMPLE250_INFO> vecResample250;
	resampleIRI250(vecMatPos, vecResample250);
	if (vecResample250.size() < 10)
	{
		if (m_use_thread)
		{
			setProgress(1.0, "计算完成");
		}

		m_strErrMsg = "重采样计算里程过短,不足1m,无法计算处理";

		return 2;
	}

	// 重采样计算;
	std::string strResamplePath = m_strSaveResultDir + "\\";
	strResamplePath = strResamplePath + m_strSavePreName + "_ReSample250.txt";
	for (unsigned int n = 0;n < vecResample250.size();n++)
	{
		POSD_RESAMPLE250_INFO& info = vecResample250[n];
		info.dLaserHeight = info.dHeight;
		info.dAccHeight = 0.0;
	}
	saveReSample250(strResamplePath.data(), vecResample250);

	// 计算IRI(平整度值) - 10m;
	if (m_bSaveIRI10)
	{
		std::vector<IRI_RESULT_TIME_INFO> vecIRIResult10;
		calcuIRIMethod(10.0, 0.25, vecResample250, vecIRIResult10);
		std::string strIRIResultPath10 = m_strSaveResultDir + "\\";
		strIRIResultPath10 = strIRIResultPath10 + m_strSavePreName + "_IRI_10m.txt";
		//strIRIResultPath10 = strIRIResultPath10 + "IRI_10m.txt";
		saveIRIs(strIRIResultPath10.data(), vecIRIResult10);
	}

	// 计算IRI(平整度值) - 100m;
	if (m_bSaveIRI100)
	{
		std::vector<IRI_RESULT_TIME_INFO> vecIRIResult100;
		calcuIRIMethod(100.0, 0.25, vecResample250, vecIRIResult100);
		std::string strIRIResultPath100 = m_strSaveResultDir + "\\";
		strIRIResultPath100 = strIRIResultPath100 + m_strSavePreName + "_IRI_100m.txt";
		//strIRIResultPath100 = strIRIResultPath100 + "IRI_100m.txt";
		saveIRIs(strIRIResultPath100.data(), vecIRIResult100);
	}


	// 计算IRI(平整度值) - 1000m;
	if (m_bSaveIRI1000)
	{
		std::vector<IRI_RESULT_TIME_INFO> vecIRIResult1000;
		calcuIRIMethod(1000.0, 0.25, vecResample250, vecIRIResult1000);
		std::string strIRIResultPath1000 = m_strSaveResultDir + "\\";
		strIRIResultPath1000 = strIRIResultPath1000 + m_strSavePreName + "_IRI_1000m.txt";
		//strIRIResultPath1000 = strIRIResultPath1000 + "IRI_1000m.txt";
		saveIRIs(strIRIResultPath1000.data(), vecIRIResult1000);
	}

	// 标记处理完成;
	std::string strIRIResultPathFinish = m_strDaqPath;
	strIRIResultPathFinish = strIRIResultPathFinish.substr(0,strIRIResultPathFinish.find_last_of('.'));
	strIRIResultPathFinish = strIRIResultPathFinish + "_finished.txt";
	FILE* ptrSaveFinish = NULL;
	int nFinsh = 1;
	fopen_s(&ptrSaveFinish, strIRIResultPathFinish.data(),"wt");
	fprintf_s(ptrSaveFinish, "%d\n", nFinsh);
	fclose(ptrSaveFinish);


	// 里程校正，输出结果值;

	if (m_use_thread)
	{
		setProgress(1.0, "计算完成");
	}

	return 0;
}

bool hnCalcuIRIMethodApi::loadDaqData(std::vector<DAQ_STRUCT_INFO>& vecDaqInfo, std::vector<dmi_speed_type>& vecDmiSpeedInfo)
{
	// 检查文件是否存在;
	if (_access(m_strDaqPath.data(), 00) != 0)
	{
		return false;
	}

	// 文件存在，则加载;
	vecDaqInfo.resize(50000);
	vecDmiSpeedInfo.resize(50000);

	int curValue = 0;
	int totalValue = 0;
	char strline[1024];
	int invalidCount = 0;
	FILE* ptrDaqFile = NULL;
	fopen_s(&ptrDaqFile, m_strDaqPath.data(), "rt");
	if (ptrDaqFile)
	{
		// 跳读1000字节;
		fseek(ptrDaqFile, 1000, SEEK_SET);
		fgets(strline, 1024, ptrDaqFile); // 舍弃第一行;
		while (!feof(ptrDaqFile))
		{
			// 读取文件;
			memset(strline, 0, 1024);
			fgets(strline, 1024, ptrDaqFile);
			totalValue++;
		}
	}
	fclose(ptrDaqFile);

	// 获取到的总行数需要判断;
	if (totalValue < 1 * 10 * m_nImuHz)
	{
		return false;
	}

	// 重新打开;
	bool bWriteDMI = false;  // 判断是否开始记录编码器数据;
	unsigned long preVal;  // 插值前一次编码器数据;
	double scale = 0.0;         // 插值比例
	bool is_first = true; 
	dmi_lrec_type pre_dmiRec;
	dmi_speed_type dmiSpeedInfo;

	// 统计设备内部记录的年信息，判断是否存在2000年的数据;
	bool isUnTimeEixst = false;
	bool isUnTimeAll = false;
	int dmiCount = 0;
	curValue = 0;
	DAQ_STRUCT_INFO daqInfo;
	short week;
	double secInWeek;
	double lastDayTime;
	double preSecInWeek = 0.0;
	bool isFirst = true;
	double imuRate = 1.0 / m_nImuHz;
	ptrDaqFile = NULL;

	int nTmp = 0;
	fopen_s(&ptrDaqFile,m_strDaqPath.data(), "rt");
	if (ptrDaqFile)
	{
		// 跳读1000字节;
		fseek(ptrDaqFile, 1000, SEEK_SET);
		//fgets(strline, 1024, ptrDaqFile); // 舍弃第一行;
		//fseek(ptrFile, 1000, SEEK_SET);
		for (int n = 0; n < 1000; n++)
		{
			fgets(strline, 1024, ptrDaqFile); // 舍弃1000行;
		}

		while (!feof(ptrDaqFile))
		{
			// 读取文件;
			memset(strline, 0, 1024);
			fgets(strline, 1024, ptrDaqFile);

			std::string str = strline;
			if (str.empty() || str.size() != 76)
			{
				continue;
			}

			//// 检查校验;
			//bool bsucc = checkLineData(str);
			//if (!bsucc)
			//{
			//	continue;
			//}

			QString strTmp0 = QString::fromLocal8Bit(str.data());
			QStringList strList = strTmp0.split(',');
			if (strList.size() != 8)
			{
				continue;
			}

			// 字符串解析;
			parseHGI300Data(str, daqInfo);

			// 存在2000年的时间，认为未同步;
			if (daqInfo.utc.year <= 2000)
			{
				isUnTimeEixst = true;
			}

			// 时间有效性判断;
			if (daqInfo.utc.year < 2000 || daqInfo.utc.mon < 0 || daqInfo.utc.mon > 12 || daqInfo.utc.day < 0 || daqInfo.utc.day > 31
				|| daqInfo.utc.hour < 0 || daqInfo.utc.hour > 24 )
			{
				continue;
			}


			// 时间转换;
			utc2gps(daqInfo.utc, week, secInWeek, 0);
			daqInfo.gpsSecond = secInWeek;
			daqInfo.gpsWeek = week;
			if (isFirst)
			{
				preSecInWeek = secInWeek;
				isFirst = false;
			}

			if (daqInfo.utc.year > 2000 && !isFirst)
			{
				if ((secInWeek - preSecInWeek) < -80000)
				{
					double dtmpInWeek = secInWeek + 86400.0;
					if (dtmpInWeek > 604800.0)
					{
						week = week + 1;
						dtmpInWeek = dtmpInWeek - 604800.0;
					}

					secInWeek = dtmpInWeek;
					daqInfo.gpsSecond = secInWeek;
					daqInfo.gpsWeek = week;
				}
			}
			preSecInWeek = secInWeek;
			

			// 计算速度;
			if (!bWriteDMI)
			{
				int iMod = abs(static_cast<int>((secInWeek - static_cast<int>(secInWeek)) * 1000) + 5) % 20;
				if (iMod < 5)
				{
					bWriteDMI = true;
				}
			}
			if (bWriteDMI)
			{
				int iMod = (abs(static_cast<int>((secInWeek - static_cast<int>(secInWeek)) * 1000) + 5) % 20) / 5;
				if (0 == iMod)
				{
					preVal = daqInfo.dim;
					scale = ((static_cast<int>(secInWeek * 1000 + 5) / 20) * 20 / 1000.0 - secInWeek);

				}
				if (1 == iMod)
				{
					// 写入 dmi 数据;
					dmi_lrec_type dmiRec;
					//dmiRec.sSync = 0xffee;
					dmiRec.sWeek = week;

					dmiRec.dTime = (static_cast<int>(secInWeek * 1000) / 20) * 20 / 1000.0;
					dmiRec.lValue[0] = static_cast<int>((scale * preVal + (5 - scale)*(daqInfo.dim)) / 5.0 + 0.5);

					double speed = 0.0;
					if (is_first)
					{
						pre_dmiRec = dmiRec;
						is_first = false;

						dmiSpeedInfo.speed = speed;
						dmiSpeedInfo.gpsSecond = secInWeek;
						//daqInfo.speed = speed;
						//fprintf_s(pDmrFile, "%.6lf	%.6lf\n", dmiRec.dTime, speed);

						if (dmiCount < vecDmiSpeedInfo.size())
						{
							vecDmiSpeedInfo[dmiCount] = dmiSpeedInfo;
						}

						if (dmiCount >= vecDmiSpeedInfo.size())
						{
							vecDmiSpeedInfo.resize(vecDmiSpeedInfo.size() + 50000);
						}

						dmiCount++;
					}
					else
					{
						double temp = 1.0 * (dmiRec.lValue[0] - pre_dmiRec.lValue[0]) * m_dDmiWheel / m_nDmiHz;
						double temp2 = dmiRec.dTime - pre_dmiRec.dTime;
						double temp3 = temp / temp2;
						if (temp3 > 0.0)
						{
							int testt = 0;
						}
						speed = (1.0 * (dmiRec.lValue[0] - pre_dmiRec.lValue[0]) * m_dDmiWheel / m_nDmiHz) / (dmiRec.dTime - pre_dmiRec.dTime);

						//fprintf_s(pDmrFile, "%.6lf	%.6lf\n", dmiRec.dTime, speed);
						dmiSpeedInfo.speed = speed;
						dmiSpeedInfo.gpsSecond = secInWeek;
						pre_dmiRec = dmiRec;

						// 实时更新大小;
						if (dmiCount < vecDmiSpeedInfo.size())
						{
							vecDmiSpeedInfo[dmiCount] = dmiSpeedInfo;
						}

						if (dmiCount >= vecDmiSpeedInfo.size())
						{
							vecDmiSpeedInfo.resize(vecDmiSpeedInfo.size() + 50000);
						}

						dmiCount++;
					}

					bWriteDMI = false;
				}
			}


			vecDaqInfo[curValue] = daqInfo;
			curValue++;

			if (curValue >= vecDaqInfo.size())
			{
				vecDaqInfo.resize(vecDaqInfo.size() + 50000);
			}

			if (curValue < 10)
			{
				m_nCurGpsWeek = week;
			}

			

			if (loadCallback && curValue % 10000 == 0)
			{
				loadCallback(curValue * 1.0 / totalValue, "读取DAQ数据...");
			}

			if (m_use_thread && curValue % 10000 == 0)
			{
				setProgress(curValue * 1.0 / totalValue, "读取DAQ数据...");
			}
		}
	}
	fclose(ptrDaqFile);
	vecDaqInfo.resize(curValue);
	vecDmiSpeedInfo.resize(dmiCount);

	// 判断最后一条记录是不是未同步;
	if (curValue > 0 && vecDaqInfo[curValue - 1].utc.year <= 2000)
	{
		isUnTimeAll = true;
	}

	// 如果存在需要校时,则在此处进行修复;
	if (isUnTimeEixst)
	{
		// 分为是全部都未授时，还是部分未授时;
		QString strDaqPath = QString::fromLocal8Bit(m_strDaqPath.data());
		if (isUnTimeAll) // 全部未授时，则直接从文件名称上获取;
		{
			// 存储的文件夹路径及文件名称;
			QString strDirPath = strDaqPath;
			strDirPath = strDirPath.left(strDirPath.lastIndexOf('/'));
			QString strPreName = strDaqPath;
			strPreName = strPreName.right(strPreName.length() - strPreName.lastIndexOf('/') - 1);
			strPreName = strPreName.left(strPreName.lastIndexOf('.'));

			
			// 需要从名称上解析;
			QString strDaqName = strPreName;
			QStringList qlist;
			qlist = strDaqName.split(QRegExp("[_*]"));
			int nsize = qlist.size();
			if (nsize >= 8)
			{
				// 解析获取时间信息;
				QString strTime = qlist[4];
				QString strDate = qlist[3];
				if (strDate.isEmpty() || strTime.isEmpty())
				{
					return true;
				}

				// 解析年月日;
				int nNewYear, nNewMonth, nNewDay;
				int nNewHour, nNewMin, nNewSec;
				nNewYear = strDate.left(4).toInt();
				nNewMonth = strDate.mid(4,2).toInt();
				nNewDay = strDate.mid(6, 2).toInt();

				// 解析时分秒;
				nNewHour = strTime.left(2).toInt();
				nNewMin = strTime.mid(2, 2).toInt();
				nNewSec = strTime.mid(4, 2).toInt();

				HNTIME zeroUtc;
				zeroUtc.year = nNewYear;
				zeroUtc.mon = nNewMonth;
				zeroUtc.day = nNewDay;
				zeroUtc.hour = nNewHour;
				zeroUtc.min = nNewMin;
				zeroUtc.sec = nNewSec;
				zeroUtc.microSec = 0;
				zeroUtc.milliSec = 0;
				short gpsWeekZero = 0;
				double gpsSecZero = 0.0;
				utc2gps(zeroUtc, gpsWeekZero, gpsSecZero,0.0);
				double oldGpsSecZero = vecDaqInfo[0].gpsSecond;
				double dOldSecCount = 0.0;

				// 前面获取的时间为北京时间,需要转换为UTC时间;
				gpsSecZero = gpsSecZero - 8.0 * 3600.0;
				if (gpsSecZero < 0.0)
				{
					gpsSecZero = gpsSecZero + 604800.0;
					gpsWeekZero = gpsWeekZero - 1;
				}

				double dTimeStep = 1.0 / 900.0;
				double curGpsTime = 0.0;

				// 根据惯导频率修改时间;
				for (int n = 0;n < vecDaqInfo.size();n++)
				{
					int curWeek = gpsWeekZero;
					HNTIME curUtc;
					DAQ_STRUCT_INFO& info = vecDaqInfo[n];

					double dOldTmpGpsSec = info.gpsSecond;
					dOldSecCount = dOldTmpGpsSec - oldGpsSecZero;

					curGpsTime = gpsSecZero + dOldSecCount;
					//curGpsTime = gpsSecZero + n * dTimeStep;
					if (curGpsTime > 604800.0)
					{
						curGpsTime = curGpsTime - 604800.0;
						curWeek = curWeek + 1;
					}

					if (curGpsTime > 280192 && curGpsTime < 280193)
					{
						int test = 0;
					}

					// gps时间转utc时间;
					gps2utc(curWeek, curGpsTime, curUtc,0.0);

					// 更新时间;
					info.gpsSecond = curGpsTime;
					info.gpsWeek = curWeek;
					info.utc = curUtc;
				}
			}
		}
		else // 存在部分数据未授时的情况;
		{
			double dTimeStep = 1.0 / 900.0;
			double curGpsTime = 0.0;
			int curWeek = 0;
			HNTIME curUtc;
			double nextGpsTime = 0.0;
			int nextWeek = 0;

			// 根据惯导频率修改时间;
			for (int n = vecDaqInfo.size() - 2; n >= 0; n--)
			{

				DAQ_STRUCT_INFO& info = vecDaqInfo[n];
				if (info.utc.year <= 2000)
				{
					// 需要更新时间;
					DAQ_STRUCT_INFO& nextInfo = vecDaqInfo[n+1];
					nextGpsTime = nextInfo.gpsSecond;
					nextWeek = nextInfo.gpsWeek;

					// 当前时间获取;
					curGpsTime = nextGpsTime - dTimeStep;
					if (curGpsTime < 0)
					{
						curGpsTime = curGpsTime + 604800.0;
						curWeek = nextWeek - 1;
					}
					else
					{
						curWeek = nextWeek;
					}

					// gps时间转utc时间;
					gps2utc(curWeek, curGpsTime, curUtc, 0.0);

					// 更新时间;
					info.gpsSecond = curGpsTime;
					info.gpsWeek = curWeek;
					info.utc = curUtc;
				} // if (info.utc.year <= 2000)
			}
		}
	}

	return true;
}

void hn::hnCalcuIRIMethodApi::setKB(double dK, double dB)
{
	m_dScaleK = dK;
	m_dScaleB = dB;
}

void hn::hnCalcuIRIMethodApi::setSaveResultPath(const char* strSaveDir, const char* strSavePreName)
{
	m_strSaveResultDir = strSaveDir;
	m_strSavePreName = strSavePreName;

	//// 检查文件夹路径是否存在，若不存在则创建;
	//CreateFolder(m_strSaveResultDir);
}

void hn::hnCalcuIRIMethodApi::setIsOnRight(int onRight)
{
	m_isOnRight = onRight;
}

void hn::hnCalcuIRIMethodApi::setUseThread(bool useThread)
{
	m_use_thread = useThread;
}

bool hn::hnCalcuIRIMethodApi::CheckFrame(std::string instr,int framelen)
{
	size_t len = instr.length();
	if (len == framelen)
	{
		char temp = instr[1];
		char checkval[32];

		for (size_t i = 2; i < len - 2; i++)
		{
			temp ^= instr[i];
		}

		int tp0 = temp;

		sprintf(checkval, "%02X", tp0);
		if (checkval[0] == instr[len - 2] && checkval[1] == instr[len - 1])
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

bool hn::hnCalcuIRIMethodApi::UpdateTimeLoadDaq(std::vector<POSD_RESAMPLE250_INFO>& vecResample50)
{
	// 暂定从DAQ名称上解析时间，与存储的同步板时间信息进行比较，时间超过5分钟，认为存在未同步现象，使用文件名称上的时间进行强制插值时间信息;
	QString strFileHms = "";
	QString strFileYmd = "";
	QString strDaqName = QString::fromLocal8Bit(m_strSavePreName.data());
	strDaqName = strDaqName.right(strDaqName.length() - strDaqName.lastIndexOf('/') - 1);
	if (strDaqName.contains(".daq") || strDaqName.contains(".DAQ")) // 为daq文件;
	{
		strDaqName = strDaqName.left(strDaqName.lastIndexOf('.'));
	}
	
	QString strDaqPreName = strDaqName.left(strDaqName.lastIndexOf('_'));
	strDaqPreName = strDaqPreName.left(strDaqPreName.lastIndexOf('_'));
	strFileHms = strDaqPreName.right(strDaqPreName.length() - strDaqPreName.lastIndexOf('_') - 1);
	strDaqPreName = strDaqPreName.left(strDaqPreName.lastIndexOf('_'));
	strFileYmd = strDaqPreName.right(strDaqPreName.length() - strDaqPreName.lastIndexOf('_') - 1);

	int ntmpYear, ntmpMonth, ntmpDay, ntmpH, ntmpMin, ntmpSec,ntmpMicro,ntmpMilli;
	ntmpYear = 2000;
	ntmpMonth = 1;
	ntmpDay = 1;
	ntmpH = ntmpMin = ntmpSec = 0;
	ntmpMicro = ntmpMilli = 0;
	bool bExistYmdName = false;
	bool bExistHmsName = false;
	if (strFileYmd.length() >= 8)
	{
		ntmpYear = strFileYmd.left(4).toInt();
		strFileYmd = strFileYmd.right(4);
		ntmpMonth = strFileYmd.left(2).toInt();
		strFileYmd = strFileYmd.right(2);
		ntmpDay = strFileYmd.toInt();
		bExistYmdName = true;
	}

	int nFileHour, nFileMin, nFileSec;
	if (strFileHms.length() >= 6)
	{
		nFileHour = strFileHms.left(2).toInt();
		strFileHms = strFileHms.right(4);
		nFileMin = strFileHms.left(2).toInt();
		strFileHms = strFileHms.right(2);
		nFileSec = strFileHms.toInt();
		bExistHmsName = true;
	}

	// 文件夹记录的时间信息记录;
	HNTIME curFileTime;
	curFileTime.year = ntmpYear;
	curFileTime.mon = ntmpMonth;
	curFileTime.day = ntmpDay;
	curFileTime.hour = nFileHour;
	curFileTime.min = nFileMin;
	curFileTime.sec = nFileSec;
	curFileTime.microSec = 0;
	curFileTime.milliSec = 0;

	short nWeek = 0;
	double dFileGpsInWeek = 0.0;
	double dCurGpsInWeek = 0.0;
	utc2gps(curFileTime,nWeek, dFileGpsInWeek);

	// 先记录年月日;
	HNTIME curTime;
	curTime.year = ntmpYear;
	curTime.mon = ntmpMonth;
	curTime.day = ntmpDay;

	// 解析DAQ文件;
	bool bChecked = false;
	QString strLine;
	QStringList qlist;
	char strline[1024];
	FILE* ptrDaqFile = NULL;
	fopen_s(&ptrDaqFile, m_strDaqPath.data(), "rt");
	if (!ptrDaqFile)
	{
		return false;
	}

	// 需要检查是否存在采集过程中未授时成功的情况，若出现，则后续全部采用文件名时间授时;
	char strTmpLine[1024];
	int framelen = strlen("%AD,011935150873,-1055580,+2528307,+0059063,+0053415,0E");
	if (ptrDaqFile)
	{
		while (!bChecked && !feof(ptrDaqFile))
		{
			memset(strline, 0, 1024);
			fgets(strline, 1024, ptrDaqFile);
			if (strlen(strline) <= 0)
			{
				continue;
			}
			memcpy(strTmpLine, strline,strlen(strline)-1);
			bChecked = CheckFrame(strTmpLine,framelen);

			// 解析第一有效帧;
			strLine = QString::fromLocal8Bit(strline);
			qlist = strLine.split(",");
			if (qlist.size() < 7)
			{
				continue;
			}

			// 获取时间信息;
			QString strTemp = qlist[1];
			unsigned long long ullTime = strTemp.toULongLong();
			ntmpH = (ullTime / 10000000000);
			ntmpMin = (ullTime / 100000000) % 100;
			ntmpSec = (ullTime / 1000000) % 100;
			ntmpMicro = (ullTime % 1000000);
			ntmpMicro = (ntmpMicro / 1000);
			ntmpMilli = (ullTime % 1000);

			// 方便转换，使用北京时间计算;
			curTime.hour = ntmpH + 8;
			curTime.min = ntmpMin;
			curTime.sec = ntmpSec;
			curTime.microSec = ntmpMicro;
			curTime.milliSec = ntmpMilli;

			utc2gps(curTime, nWeek, dCurGpsInWeek);
		}
	}
	fclose(ptrDaqFile);

	// 获取配置文件中的帧频;
	int nFrameHz = 4000;
	QString strDaqDirPath = QString::fromLocal8Bit(m_strDaqPath.data());
	strDaqDirPath = strDaqDirPath.left(strDaqDirPath.lastIndexOf('/'));
	QString strSettingPath = strDaqDirPath + "/Setting.ini";

	QSettings *pSetting = new QSettings(strSettingPath, QSettings::IniFormat);
	nFrameHz = pSetting->value("SampleFrequency/Frequency").toInt();
	delete pSetting;
	double dFrameSecTime = 1.0 / nFrameHz;

	// 时间判断，是从文件解析里面获取时间信息还是从文件名称获取(解决GPS时间不同步的问题);
	bool bNoSyned = false;
	double dFirstGpsInWeek = dCurGpsInWeek;
	double dTmpAddTime = 0.0;
	if (bExistYmdName && bExistHmsName && abs(dFileGpsInWeek - dFirstGpsInWeek) >= 60.0*5)
	{
		// 增加文件到同步板时间差，出现未同步的情况，则直接以数据记录间隔来更新时间,存在毫米级偏差不影响;
		dTmpAddTime = dFileGpsInWeek - dFirstGpsInWeek;
		bNoSyned = true;
	}

	// 根据帧号及帧频计算时间更新至resample结构体;
	double dtmpTime = 0.0;
	HNTIME timeInfo;
	unsigned long long ullTime = 0;
	for (unsigned int n = 0;n < vecResample50.size();n++)
	{
		POSD_RESAMPLE250_INFO& sampleInfo = vecResample50[n];
		if (bNoSyned)
		{
			dtmpTime = dFirstGpsInWeek + dTmpAddTime + 1.0 * dFrameSecTime * sampleInfo.ullFrameIdx - 8.0 * 3600.0;
			gps2utc(nWeek, dtmpTime, timeInfo, 0.0);
		}
		else
		{
			ullTime = sampleInfo.ullFrameTime;
			ntmpH = (ullTime / 10000000000);
			ntmpMin = (ullTime / 100000000) % 100;
			ntmpSec = (ullTime / 1000000) % 100;
			ntmpMicro = (ullTime % 1000000);
			ntmpMicro = (ntmpMicro / 1000);
			ntmpMilli = (ullTime % 1000);

			// 方便转换，使用北京时间计算;
			curTime.year = ntmpYear;
			curTime.mon = ntmpMonth;
			curTime.day = ntmpDay;
			curTime.hour = ntmpH + 8;
			curTime.min = ntmpMin;
			curTime.sec = ntmpSec;
			curTime.microSec = ntmpMicro;
			curTime.milliSec = ntmpMilli;
			utc2gps(curTime, nWeek, dCurGpsInWeek);
			dtmpTime = dCurGpsInWeek + dTmpAddTime - 8.0 * 3600.0;

			// 时间转换UTC时间系统;
			//dtmpTime = dFirstGpsInWeek + 1.0 * dFrameSecTime * sampleInfo.ullFrameIdx - 8.0 * 3600.0;
			gps2utc(nWeek, dtmpTime, timeInfo, 0.0);
		}

		sampleInfo.time = timeInfo;
	}

	return true;
}

bool hn::hnCalcuIRIMethodApi::parseHGI300Data(const std::string& str, DAQ_STRUCT_INFO& daqInfo)
{
	// 时间读取;
	// 年;
	string strYear = str.substr(5, 4);
	daqInfo.utc.year = atoi(strYear.data());

	//月;
	string strMonth = str.substr(9, 2);
	daqInfo.utc.mon = atoi(strMonth.data());

	//日;
	string strDay = str.substr(11, 2);
	daqInfo.utc.day = atoi(strDay.data());

	// 时;
	string strHour = str.substr(14, 2);
	daqInfo.utc.hour = atoi(strHour.data());

	// 分;
	string strMin = str.substr(16, 2);
	daqInfo.utc.min = atoi(strMin.data());

	// 秒;
	string strSec = str.substr(18, 2);
	daqInfo.utc.sec = atoi(strSec.data());

	// 毫秒;
	string strMSec = str.substr(21, 3);
	daqInfo.utc.microSec = atoi(strMSec.data());

	// 微秒;
	string strMillSec = str.substr(25, 3);
	daqInfo.utc.milliSec = atoi(strMillSec.data());

	// DIM数据读取;
	string strDimValue = str.substr(29, 10);
	daqInfo.dim = strtoul(strDimValue.data(),NULL,10);

	// rate x;
	string strXVelocity = str.substr(44, 4);
	daqInfo.xVelocity = str2float(strXVelocity) * HGI300_RATE_LSB * 180.0 / PI64;

	// rate y;
	string strYVelocity = str.substr(48, 4);
	daqInfo.yVelocity = str2float(strYVelocity) * HGI300_RATE_LSB * 180.0 / PI64;

	//  rate z;
	string strZVelocity = str.substr(52, 4);
	daqInfo.zVelocity = str2float(strZVelocity) * HGI300_RATE_LSB * 180.0 / PI64;

	// acc x;
	string strXAccelerate = str.substr(56, 4);
	daqInfo.xAccelerate = str2float(strXAccelerate) * HGI300_ACC_LSB;

	// acc y;
	string strYAccelerate = str.substr(60, 4);
	daqInfo.yAccelerate = str2float(strYAccelerate) * HGI300_ACC_LSB;

	// acc z;
	string strZAccelerate = str.substr(64, 4);
	daqInfo.zAccelerate = str2float(strZAccelerate) * HGI300_ACC_LSB;
	return true;

	// 安装在右侧时，惯导指向为X轴指右，Y轴指下，Z轴指前,统一调换为X轴指右，Y轴指前，Z轴指上;
	if (m_isOnRight)
	{
		// rate x-X轴不变;
		string strXVelocity = str.substr(44, 4);
		daqInfo.xVelocity = str2float(strXVelocity) * HGI300_RATE_LSB * 180.0 / PI64;

		// rate y-Y轴指下变为反向指上-Z轴;
		string strYVelocity = str.substr(48, 4);
		daqInfo.zVelocity = -1.0 * str2float(strYVelocity) * HGI300_RATE_LSB * 180.0 / PI64;

		//  rate z-Z轴指前变为Y轴;
		string strZVelocity = str.substr(52, 4);
		daqInfo.yVelocity = str2float(strZVelocity) * HGI300_RATE_LSB * 180.0 / PI64;

		// acc x;
		string strXAccelerate = str.substr(56, 4);
		daqInfo.xAccelerate = str2float(strXAccelerate) * HGI300_ACC_LSB;

		// acc y;
		string strYAccelerate = str.substr(60, 4);
		daqInfo.zAccelerate = -1.0 * str2float(strYAccelerate) * HGI300_ACC_LSB;

		// acc z;
		string strZAccelerate = str.substr(64, 4);
		daqInfo.yAccelerate = str2float(strZAccelerate) * HGI300_ACC_LSB;
	}
	else
	{
		// 安装在左侧时，惯导指向为X轴指左，Y轴指下，Z轴指后,统一调换为X轴指右，Y轴指前，Z轴指上;
		// rate x;
		string strXVelocity = str.substr(44, 4);
		daqInfo.xVelocity =  -1.0 * str2float(strXVelocity) * HGI300_RATE_LSB * 180.0 / PI64;

		// rate y;
		string strYVelocity = str.substr(48, 4);
		daqInfo.zVelocity = -1.0 * str2float(strYVelocity) * HGI300_RATE_LSB * 180.0 / PI64;

		//  rate z;
		string strZVelocity = str.substr(52, 4);
		daqInfo.yVelocity = -1.0 * str2float(strZVelocity) * HGI300_RATE_LSB * 180.0 / PI64;

		// acc x;
		string strXAccelerate = str.substr(56, 4);
		daqInfo.xAccelerate = -1.0 * str2float(strXAccelerate) * HGI300_ACC_LSB;

		// acc y;
		string strYAccelerate = str.substr(60, 4);
		daqInfo.zAccelerate = -1.0 * str2float(strYAccelerate) * HGI300_ACC_LSB;

		// acc z;
		string strZAccelerate = str.substr(64, 4);
		daqInfo.yAccelerate = -1.0 * str2float(strZAccelerate) * HGI300_ACC_LSB;
	}



	return true;
}

bool hn::hnCalcuIRIMethodApi::utc2gps(const HNTIME& stTime, short& nGPSWeek, double& dGPSSeconds, double dGPSSubUTC /*= 18.0*/)
{
	int dayofw = 0;
	int dayofy = 0;
	int yr = 0;
	int ttlday = 0;
	int m = 0;
	int weekno = 0;

	// 将年月日信息转换为 当年中的第几天
	const int dinmth[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	if (1 == stTime.mon)
	{
		dayofy = stTime.day;
	}
	else
	{
		dayofy = 0;
		for (m = 1; m <= (stTime.mon - 1); m++)
		{
			dayofy += dinmth[m];
			if (2 == m)
			{
				if (0 == stTime.year % 4
					&& 0 != stTime.year % 100
					|| 0 == stTime.year % 400)
				{
					dayofy += 1;
				}
			}
		}

		dayofy += stTime.day;
	}

	// 
	ttlday = 360;
	for (yr = 1981; yr <= (stTime.year - 1); yr++)
	{
		ttlday += 365;
		if (0 == yr % 4
			&& 0 != yr % 100
			|| 0 == yr % 400)
		{
			ttlday += 1;
		}
	}

	ttlday += dayofy;

	weekno = ttlday / 7;
	dayofw = ttlday - 7 * weekno;

	nGPSWeek = weekno;
	dGPSSeconds = dayofw * 86400.0 + stTime.hour * 3600
		+ stTime.min * 60 + stTime.sec
		+ stTime.microSec / 1000.0 + stTime.milliSec / 1000000.0;

	dGPSSeconds += dGPSSubUTC;
	if (dGPSSeconds > 7 * 24 * 3600)
	{
		dGPSSeconds -= 7 * 24 * 3600;
		nGPSWeek += 1;
	}

	return true;
}

bool hn::hnCalcuIRIMethodApi::gps2utc(short nGpsWeek, double dGpsSeconds, HNTIME& stTime, double dGPSSubUTC /*= 0.0*/)
{
	short wWeek = nGpsWeek;
	double dSec = dGpsSeconds - dGPSSubUTC;
	if (dSec < 0)
	{
		wWeek -= 1;
		dSec += 7 * 24 * 3600;
	}

	int dayofw(0), h(0), m(0);
	int isecs = (int)dSec;
	double fsec = dSec - isecs;
	int dayofy(0), yr(0), ttlday(0), mn(0);

	dayofw = isecs / 86400;
	isecs = isecs - 86400 * dayofw;
	h = isecs / 3600;
	isecs = isecs - 3600 * h;
	m = isecs / 60;
	stTime.sec = (int)(isecs - 60 * m + fsec);
	stTime.microSec = (int)((isecs - 60 * m + fsec - stTime.sec) * 1000);
	stTime.milliSec = (int)((isecs - 60 * m + fsec - stTime.sec) * 1000000 - stTime.microSec * 1000);

	stTime.hour = h;
	stTime.min = m;

	ttlday = dayofw + 7 * wWeek;
	ttlday -= 360;
	yr = 1981;
	while (ttlday > 366)
	{
		ttlday -= 365;
		if ((yr % 4 == 0 && yr % 100 != 0) || yr % 400 == 0)
		{
			ttlday -= 1;
		}
		yr += 1;
	}

	if (ttlday == 366)
	{
		if ((yr % 4 == 0 && yr % 100 != 0) || yr % 400 == 0)
		{
			stTime.year = yr;
			dayofy = 366;
		}
		else
		{
			stTime.year = yr + 1;
			dayofy = 1;
		}
	}
	else if (ttlday < 366)
	{
		stTime.year = yr;
		dayofy = ttlday;
	}

	const  int  dinmth[13] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	for (mn = 1; mn <= 12; mn++)
	{
		if ((ttlday <= dinmth[mn]) && (ttlday > 0))
		{
			stTime.day = ttlday;
			stTime.mon = mn;
			ttlday = 0;
		}
		else if (mn == 2)
		{
			if ((yr % 4 == 0 && yr % 100 != 0) || yr % 400 == 0)
			{
				if (ttlday > 29) ttlday -= 29;
				else
				{
					stTime.day = 29;
					stTime.mon = 2;
					ttlday = 0;
				}
			}
			else
				ttlday -= 28;
		}
		else
			ttlday -= dinmth[mn];
		if (ttlday == 0) break;
		if (wWeek == 0) stTime.year = 0;
	}

	return 1;
}

bool hn::hnCalcuIRIMethodApi::calcuPosBySinsDr(std::vector<DAQ_STRUCT_INFO>& vecDaqInfo, std::vector<dmi_speed_type>& vecDmiSpeedInfo, std::vector<POSD_STRUCT_INFO>& vecMatPos)
{
	//// 初始化;
	//if (!ForSinsDR2Initialize())
	//{
	//	system("pause");
	//	int test = 0;
	//	return false;
	//}

	//// 构建imu所需matlab数组，行列表现为（vecImu.size(),7）;
	//mxArray* arryImu = mxCreateDoubleMatrix(vecDaqInfo.size(), 7, mxREAL);
	//double* ptrImu = mxGetPr(arryImu);
	//for (unsigned int n = 0; n < vecDaqInfo.size(); n++)
	//{
	//	DAQ_STRUCT_INFO& info = vecDaqInfo[n];
	//	ptrImu[vecDaqInfo.size() * 0 + n] = info.gpsSecond;
	//	ptrImu[vecDaqInfo.size() * 1 + n] = info.xVelocity;
	//	ptrImu[vecDaqInfo.size() * 2 + n] = info.yVelocity;
	//	ptrImu[vecDaqInfo.size() * 3 + n] = info.zVelocity;
	//	ptrImu[vecDaqInfo.size() * 4 + n] = info.xAccelerate;
	//	ptrImu[vecDaqInfo.size() * 5 + n] = info.yAccelerate;
	//	ptrImu[vecDaqInfo.size() * 6 + n] = info.zAccelerate;
	//}

	//// 构建dmi所需matlab数组，行列表现为（vecDmi.size(),2）
	//mxArray* arryDmi = mxCreateDoubleMatrix(vecDmiSpeedInfo.size(), 2, mxREAL);
	//double* ptrDmi = mxGetPr(arryDmi);
	//for (unsigned int n = 0; n < vecDmiSpeedInfo.size(); n++)
	//{
	//	dmi_speed_type& info = vecDmiSpeedInfo[n];
	//	ptrDmi[vecDmiSpeedInfo.size() * 0 + n] = info.gpsSecond;
	//	ptrDmi[vecDmiSpeedInfo.size() * 1 + n] = info.speed;
	//}

	//// 标记IMU指向行进方向类型;
	//int caliType = 3;
	//mxArray* arryCaliType = mxCreateDoubleMatrix(1, 1, mxREAL);
	//double* ptrCaliType = mxGetPr(arryCaliType);
	//ptrCaliType[0] = (double)caliType;

	//// 标记起点经度,初始设置为114;
	//double dStartLongtitude = 114.0;
	//mxArray* arryStartLongtitude = mxCreateDoubleMatrix(1, 1, mxREAL);
	//double* ptrStartLong = mxGetPr(arryStartLongtitude);
	//ptrStartLong[0] = dStartLongtitude;

	//// 标记起点高程,初始设置为30.0;
	//double dStartHeight = 30.0;
	//mxArray* arryStartHeight = mxCreateDoubleMatrix(1, 1, mxREAL);
	//double* ptrStartHeight = mxGetPr(arryStartHeight);
	//ptrStartHeight[0] = dStartHeight;

	//// 标记静止行数，由C++部分代码计算;
	//double dStillCount = 30000.0;
	//mxArray* arryStillCount = mxCreateDoubleMatrix(1, 1, mxREAL);
	//double* ptrStillCount = mxGetPr(arryStillCount);
	//ptrStillCount[0] = dStillCount;

	//// 标度因子;
	//mxArray* arryCorr = mxCreateDoubleMatrix(4, 1, mxREAL);
	//double* ptrCorr = mxGetPr(arryCorr);
	//ptrCorr[0] = 0.0;
	//ptrCorr[1] = 0.0;
	//ptrCorr[2] = 0.0;
	//ptrCorr[3] = 1.0;

	//// 结果值存储;
	//mxArray* arrPosResult = mxCreateDoubleMatrix(vecDaqInfo.size(), 8, mxREAL);
	//mxArray* arrPosRows = mxCreateDoubleMatrix(1, 1, mxREAL);

	//// 调用matlab计算;
	//bool bSucc = mlfForSinsDR2(2, &arrPosResult, &arrPosRows, arryImu, arryDmi, arryStartLongtitude, arryStartHeight, arryCaliType, arryCorr, arryStillCount);
	//if (!bSucc)
	//{
	//	mxDestroyArray(arryImu);
	//	mxDestroyArray(arryDmi);
	//	mxDestroyArray(arryStartLongtitude);
	//	mxDestroyArray(arryStartHeight);
	//	mxDestroyArray(arryCaliType);
	//	mxDestroyArray(arrPosResult);
	//	mxDestroyArray(arrPosRows);
	//	mxDestroyArray(arryCorr);
	//	return false;
	//}

	//// 返回结果值;
	//double* ptrResultRows = mxGetPr(arrPosRows);
	//int posRows = ptrResultRows[0];

	//// 结果用vector容器记录;
	//double* ptrPosResult = mxGetPr(arrPosResult);
	//POSD_STRUCT_INFO info;
	//vecMatPos.resize(posRows);
	//int ncount = 0;
	//for (int n = 0; n < posRows; n++)
	//{
	//	info.dGpsTime = ptrPosResult[posRows * 0 + n];

	//	info.dB = ptrPosResult[posRows * 1 + n];
	//	info.dL = ptrPosResult[posRows * 2 + n];
	//	info.dH = ptrPosResult[posRows * 3 + n] * 1000.0;

	//	info.dYaw = ptrPosResult[posRows * 4 + n];
	//	info.dPitch = ptrPosResult[posRows * 5 + n];
	//	info.dRoll = ptrPosResult[posRows * 6 + n];
	//	info.dDist = ptrPosResult[posRows * 7 + n];

	//	// 判断值是否为非法值，非法值不予以记录;
	//	if (_isnan(info.dB) != 0 || _isnan(info.dL) != 0 || _isnan(info.dH) != 0 ||
	//		_isnan(info.dYaw) != 0 || _isnan(info.dPitch) != 0 || _isnan(info.dRoll) != 0 || _isnan(info.dDist) != 0)
	//	{
	//		continue;
	//	}

	//	// 无效时间过滤;
	//	if (info.dGpsTime <= 0.0)
	//	{
	//		continue;
	//	}

	//	vecMatPos[ncount] = info;
	//	ncount++;
	//}
	//vecMatPos.resize(ncount);

	//// 内存释放;
	//mxDestroyArray(arryImu);
	//mxDestroyArray(arryDmi);
	//mxDestroyArray(arryStartLongtitude);
	//mxDestroyArray(arryStartHeight);
	//mxDestroyArray(arryCaliType);
	//mxDestroyArray(arrPosResult);
	//mxDestroyArray(arrPosRows);
	//mxDestroyArray(arryCorr);
	return true;
}

void hn::hnCalcuIRIMethodApi::calcuIRIMethod(double dIntervel, double DeltLen,std::vector<POSD_RESAMPLE250_INFO>& vecResample250, std::vector<IRI_RESULT_TIME_INFO>& vecIRIResult, double dPerTime)
{
	vecIRIResult.resize(50000);

	m_ZSU[0] = 0.0; m_ZSU[1] = 0.0; m_ZSU[2] = 0.0; m_ZSU[3] = 0.0;
	//m_oldZSU[0] = 0.0; m_oldZSU[1] = 0.0; m_oldZSU[2] = 0.0; m_oldZSU[3] = 0.0;
	m_oldZSU[0] = vecResample250[1].dHeight - vecResample250[0].dHeight;
	m_oldZSU[1] = vecResample250[1].dHeight - vecResample250[0].dHeight;
	m_oldZSU[2] = 0.0; 
	m_oldZSU[3] = 0.0;


	//// 测试代码-speed;
	//std::string strSpeedPath = "M:\\NONGCUN_IRI_DATA\\2022-2-19惯导平整度测试\\_变速_上行_01_湖北省_武汉市_江夏区_20220219_112213\\IRIMTD\\DAQ1\\Speed_10m.txt";
	//FILE* ptrFileSpeed = NULL;
	//fopen_s(&ptrFileSpeed, strSpeedPath.data(), "rt");
	//std::vector<double> vecSpeed;
	//char strLine[1024];
	//while (!feof(ptrFileSpeed))
	//{
	//	memset(strLine,0,1024);
	//	fgets(strLine,1024,ptrFileSpeed);

	//	double dspeed;
	//	int index;
	//	int nret = sscanf_s(strLine, "%d	%lf\n",&index,&dspeed);
	//	if (nret >= 2)
	//	{
	//		vecSpeed.push_back(dspeed);
	//	}
	//}
	//fclose(ptrFileSpeed);
	// 测试代码-speed;

	// 总共需要点个数;
	HNTIME startTime;
	HNTIME endTime;
	IRI_RESULT_TIME_INFO resultInfo;
	double dCurSpeed = 0.0;
	int nIRIResultCount = 0;
	std::vector<POSD_RESAMPLE250_INFO> vecCalc;
	int needcounts = (int)(dIntervel / DeltLen);
	for (int n = 0;n < vecResample250.size();n += needcounts)
	{
		// 计算;
		int nCalcCount = 0;
		if ((n + needcounts) < vecResample250.size())
		{
			nCalcCount = n + needcounts;
		}
		else
		{
			continue;
			//nCalcCount = vecResample250.size();
		}

		vecCalc.clear();
		for (int m = n;m < nCalcCount;m++)
		{
			vecCalc.push_back(vecResample250[m]);
		}

		// 给定间隔计算平整度值;
		double iResult = CalculateIRI(dIntervel, vecCalc);
		if (m_bUseSpeedParam)
		{
			dCurSpeed = 3.6 * dIntervel / ((vecCalc[vecCalc.size() - 1].ullFrameIdx - vecCalc[0].ullFrameIdx)*dPerTime);

			//根据车速获取速度系数k、b;
			double kparm = m_dSpeedK[m_nSpeedCnt - 1];
			double bparm = m_dSpeedB[m_nSpeedCnt - 1];
			for (int pi = 0; pi < m_nSpeedCnt; ++pi)
			{
				if (dCurSpeed <= m_dSpeedParm[pi])
				{
					kparm = m_dSpeedK[pi];
					bparm = m_dSpeedB[pi];
					break;
				}
			}
			iResult = iResult * kparm + bparm;
		}
		
		if (vecCalc.size() < 2)
		{
			continue;
		}

		//// 根据速度计算系数值;
		//double speedval = vecSpeed[nIRIResultCount];
		//iResult = addSpeedBlong(iResult,speedval);
		resultInfo.startTime = vecCalc[0].time;
		resultInfo.endTime = vecCalc[vecCalc.size() - 1].time;
		resultInfo.dIriVal = iResult;
		vecIRIResult[nIRIResultCount] = resultInfo;
		nIRIResultCount++;

		if (nIRIResultCount >= vecIRIResult.size())
		{
			vecIRIResult.resize(vecIRIResult.size() + 50000);
		}

		if (m_use_thread && n % 10 == 0)
		{
			setProgress(n * 1.0 / vecResample250.size(), "计算IRI值...");
		}
	}

	vecIRIResult.resize(nIRIResultCount);
}

bool hn::hnCalcuIRIMethodApi::resampleIRI250(std::vector<POSD_STRUCT_INFO>& vecMatPos, std::vector<POSD_RESAMPLE250_INFO>& vecResample250)
{
	// 第一个点记录;
	double dDeltH = 0.0;
	POSD_RESAMPLE250_INFO first250;
	first250.dim = vecMatPos[0].dDist * m_nDmiHz / m_dDmiWheel;
	first250.dHeight = (vecMatPos[0].dH - dDeltH);
	gps2utc(vecMatPos[0].nGpsWeek, vecMatPos[0].dGpsTime, first250.time);
	vecResample250.resize(50000);
	vecResample250[0] = first250;

	// 进行插值确认;
	double dDist = 0.0;
	double dTimeDelt = 1.0 / 900.0;
	std::vector<POSD_STRUCT_INFO> vecMatPosNew;
	int len0 = vecMatPos.size();
	for (int i = 1; i < len0; ++i)
	{
		POSD_STRUCT_INFO& preInfo = vecMatPos[i - 1];
		POSD_STRUCT_INFO& curInfo = vecMatPos[i];

		double dTmpDist = curInfo.dDist - preInfo.dDist;
		double dTmpTime = curInfo.dGpsTime - preInfo.dGpsTime;
		int nTmpTimeCount = floor(0.5 + dTmpTime / dTimeDelt); // 四舍五入;
		if (nTmpTimeCount <= 1)
		{
			vecMatPosNew.push_back(curInfo);
			continue;
		}

		if (nTmpTimeCount > 10000)
		{
			continue;
		}

		double dTmpScaleDist = dTmpDist / nTmpTimeCount;
		double dTmpScaleH = (curInfo.dH - preInfo.dH) / nTmpTimeCount;

		// 若时间间隔超过两个标准时间间隔，认为需要插值;
		if (nTmpTimeCount > 2)
		{
			for (int n = 1;n < nTmpTimeCount;n++)
			{
				POSD_STRUCT_INFO tmpInfo = preInfo;
				tmpInfo.dGpsTime = tmpInfo.dGpsTime + 1.0 * n * dTimeDelt;
				tmpInfo.dDist = tmpInfo.dDist + 1.0 * n * dTmpScaleDist;
				tmpInfo.dH = tmpInfo.dH + 1.0 * n * dTmpScaleH;

				vecMatPosNew.push_back(tmpInfo);
			}
		}

		vecMatPosNew.push_back(curInfo);

		//vecMatPosNew[i].dH = (vecMatPos[i - 2].dH + vecMatPos[i - 1].dH
		//	+ vecMatPos[i].dH + vecMatPos[i + 1].dH + vecMatPos[i + 2].dH) / 5;
	}
	vecMatPos.clear();
	vecMatPos = vecMatPosNew;


	// 遍历;
	double dTargetDist = 0.0;
	int curStatCount = 1;
	double dDeltaDist = 0.05;
	double dTmpDistDelt = 0.0;
	double dTmpTotalDist = 0.0;
	POSD_RESAMPLE250_INFO needInfo250;
	HNTIME utcTime;
	dTargetDist = dDeltaDist;
	for (unsigned int n = 1;n < vecMatPos.size();n++)
	{
		POSD_STRUCT_INFO& preInfo = vecMatPos[n-1];
		POSD_STRUCT_INFO& curInfo = vecMatPos[n];

		// 目标距离值;
		dTargetDist = dDeltaDist * (curStatCount);

		// 判断该点是否符合条件;
		if (preInfo.dDist < dTargetDist && curInfo.dDist >= dTargetDist)
		{
			// 以前一个点为基准进行插值;
			dTmpDistDelt = dTargetDist - preInfo.dDist;
			dTmpTotalDist = curInfo.dDist - preInfo.dDist;
			double fscale = dTmpDistDelt / dTmpTotalDist;
			double gpsTime = preInfo.dGpsTime + fscale * (curInfo.dGpsTime - preInfo.dGpsTime);

			// 时间计算;
			gps2utc(preInfo.nGpsWeek, gpsTime, needInfo250.time);

			// 赋值计算;
			needInfo250.dHeight = (preInfo.dH + fscale * (curInfo.dH - preInfo.dH) - dDeltH);
			needInfo250.dim = dTargetDist * m_nDmiHz / m_dDmiWheel;

			vecResample250[curStatCount] = needInfo250;
			curStatCount++;

			if (curStatCount >= vecResample250.size())
			{
				vecResample250.resize(vecResample250.size() + 50000);
			}

			--n;
		}

		// 进度条;
		if (loadCallback && n % 10000 == 0)
		{
			loadCallback(n * 1.0f / vecMatPos.size(), "重采样数据...");
		}

		if (m_use_thread && n % 10000 == 0)
		{
			setProgress(n * 1.0 / vecMatPos.size(), "重采样数据...");
		}
	}

	// 更新大小;
	vecResample250.resize(curStatCount);

	//FILE* dd = fopen("D:\\111.txt", "w");
	//if (dd)
	//{
	//	for (unsigned int n = 1; n < vecMatPos.size(); n++)
	//	{
	//		fprintf_s(dd, "%lf,%lf,%lf\n", vecMatPos[n].dGpsTime, vecMatPos[n].dH, vecMatPos[n].dDist);
	//	}
	//	fclose(dd);
	//}

	//FILE* ptrFileSaveH = NULL;
	////fopen_s(&ptrFileSaveH, "M:\\NONGCUN_IRI_DATA\\2022-2-19惯导平整度测试\\20220219惯导测试123匀速456变速\\比较matlab\\mfc_1h_dist.txt", "wt+");
	//fopen_s(&ptrFileSaveH, "M:\\NONGCUN_IRI_DATA\\2022-2-19惯导平整度测试\\20220219惯导测试123匀速456变速\\比较matlab\\resample.txt", "wt+");
	//for (unsigned int n = 0;n < vecResample250.size();n++)
	//{
	//	short gpsWeek = 0;
	//	double gpsSec = 0.0;
	//	POSD_RESAMPLE250_INFO& info = vecResample250[n];
	//	utc2gps(info.time,gpsWeek,gpsSec);
	//	//fprintf_s(ptrFileSaveH, "%.3lf %.3lf %.5lf\n", info.dim * m_dDmiWheel / m_nDmiHz,info.dHeight/1000.0, gpsSec);

	//	double dtmp = 0.0;
	//	fprintf_s(ptrFileSaveH, "%.10lf	%.10lf	%.10lf	%ld\n",
	//		info.dHeight, dtmp,
	//		info.dHeight, info.dim);
	//}
	//fclose(ptrFileSaveH);


	//取5个点做均值滤波;
	std::vector<POSD_RESAMPLE250_INFO> vecResample250New;
	vecResample250New = vecResample250;
	int len = vecResample250.size();
	for (int i = 2; i < len - 2; ++i)
	{
		vecResample250New[i].dHeight = (vecResample250[i - 2].dHeight + vecResample250[i - 1].dHeight
			+ vecResample250[i].dHeight + vecResample250[i + 1].dHeight + vecResample250[i + 2].dHeight) / 5;
	}
	vecResample250.clear();
	for (unsigned int n = 0;n < vecResample250New.size();n += 5)
	{
		vecResample250.push_back(vecResample250New[n]);
	}

	return true;
}

bool hn::hnCalcuIRIMethodApi::resampleIRI250From50(std::vector<POSD_RESAMPLE250_INFO>& vecResample50, std::vector<POSD_RESAMPLE250_INFO>& vecResample250)
{
	//取5个点做均值滤波;
	int len = vecResample50.size();
	for (int i = 2; i < len - 2; ++i)
	{
		vecResample50[i].dHeight = (vecResample50[i - 2].dHeight + vecResample50[i - 1].dHeight 
			+ vecResample50[i].dHeight + vecResample50[i + 1].dHeight + vecResample50[i + 2].dHeight) / 5.0;
	}

	vecResample250.clear();
	for (unsigned int n = 0; n < vecResample50.size(); n += 5)
	{
		vecResample250.push_back(vecResample50[n]);
	}

	return true;
}

double hn::hnCalcuIRIMethodApi::CalculateIRI(double dIntervel, std::vector<POSD_RESAMPLE250_INFO>& vecIRIInfo, double DeltLen /*= 0.25*/)
{
	//计算10m的平整度;
	double irival = 0.0;
	double irisum = 0.0;
	int plusenum = (int)(dIntervel / DeltLen);
	double YSU = 0.0;

	//iridata: 250mm采样间距纵断面;
	for (int i = 1; i < vecIRIInfo.size(); ++i)
	{
		YSU = (vecIRIInfo[i].dHeight - vecIRIInfo[i - 1].dHeight) / DeltLen;
		for (int zi = 0; zi < 4; ++zi)
		{
			m_ZSU[zi] = 0;
			for (int zj = 0; zj < 4; ++zj)
			{
				m_ZSU[zi] += m_SZU[zi * 4 + zj] * m_oldZSU[zj];
			}
			m_ZSU[zi] += m_PZU[zi] * YSU;
		}
		irisum += fabs(m_ZSU[0] - m_ZSU[2]);

		for (int zi = 0; zi < 4; ++zi)
		{
			m_oldZSU[zi] = m_ZSU[zi];
		}
	}

	irival = irisum / plusenum;
	return irival;
}

void hn::hnCalcuIRIMethodApi::saveIRIs(const char* strSavePath, std::vector<IRI_RESULT_TIME_INFO>& vecIRIResult)
{
	FILE* ptrFile = NULL;
	fopen_s(&ptrFile, strSavePath, "wt");
	if (!ptrFile)
	{
		return;
	}

	int ncount = (int)(vecIRIResult.size());
	for (int n = 0;n < ncount;n++)
	{
		double dTmp = vecIRIResult[n].dIriVal * m_dScaleK + m_dScaleB;
		fprintf_s(ptrFile, "%d %.14lf\n", n + 1, dTmp);

		//fprintf_s(ptrFile, "%d %.14lf %04d:%02d:%02d:%02d:%02d:%02d.%03d%03d %04d:%02d:%02d:%02d:%02d:%02d.%03d%03d\n",n+1, dTmp,
		//	vecIRIResult[n].startTime.year, vecIRIResult[n].startTime.mon, vecIRIResult[n].startTime.day,
		//	vecIRIResult[n].startTime.hour, vecIRIResult[n].startTime.min, vecIRIResult[n].startTime.sec, vecIRIResult[n].startTime.microSec, vecIRIResult[n].startTime.milliSec,
		//	vecIRIResult[n].endTime.year, vecIRIResult[n].endTime.mon, vecIRIResult[n].endTime.day,
		//	vecIRIResult[n].endTime.hour, vecIRIResult[n].endTime.min, vecIRIResult[n].endTime.sec, vecIRIResult[n].endTime.microSec, vecIRIResult[n].endTime.milliSec);

		if (m_use_thread)
		{
			setProgress((n+1) * 1.0 / ncount, "保存文件...");
		}
	}
	fclose(ptrFile);

	if (m_use_thread)
	{
		setProgress(1.0f, "保存完成");
	}
}

void hn::hnCalcuIRIMethodApi::saveReSample250(const char* strSavePath, std::vector<POSD_RESAMPLE250_INFO>& vecResample250)
{
	FILE* ptrFile = NULL;
	fopen_s(&ptrFile, strSavePath, "wt");
	if (!ptrFile)
	{
		return;
	}

	double dtmp = 0.0;
	int ncount = (int)(vecResample250.size());
	for (int n = 0; n < ncount; n++)
	{
		POSD_RESAMPLE250_INFO& info = vecResample250[n];
		fprintf_s(ptrFile, "%.10lf	%.10lf	%.10lf	%ld	%02d%02d%02d%03d\n", 
			info.dLaserHeight,info.dAccHeight,
			info.dHeight,info.dim,
			info.time.hour, info.time.min, info.time.sec, info.time.microSec);

		// 进度条;
		if (loadCallback && n % 5 == 0)
		{
			loadCallback((n+1) * 1.0f / ncount, "保存重采样数据...");
		}

		if (m_use_thread && n % 5 == 0)
		{
			setProgress((n + 1) * 1.0f / ncount, "保存重采样数据...");
		}
	}
	fclose(ptrFile);

	if (m_use_thread)
	{
		setProgress(1.0, "保存重采样数据完成");
	}
}

double hn::hnCalcuIRIMethodApi::addSpeedBlong(double iriResult,double speedval)
{
	double kparms[6];
	double bparms[6];
	double speedparms[6];
	kparms[0] = 0.957956619;
	kparms[1] = 0.985742817;
	kparms[2] = 0.964108396;
	kparms[3] = 0.92684222;
	kparms[4] = 1.022933165;
	kparms[5] = 0.991272218;
	bparms[0] = 0.212270559;
	bparms[1] = 0.076500917;
	bparms[2] = 0.083499027;
	bparms[3] = 0.054732906;
	bparms[4] = -0.073520777;
	bparms[5] = -0.047490142;

	speedparms[0] = 15;
	speedparms[1] = 25;
	speedparms[2] = 40;
	speedparms[3] = 60;
	speedparms[4] = 75;
	speedparms[5] = 130;
	

	//根据车速获取速度系数k、b;
	int parmnum = 6;
	double kparm = kparms[parmnum - 1];
	double bparm = bparms[parmnum - 1];
	for (int pi = 0; pi < parmnum; ++pi)
	{
		if (speedval <= speedparms[pi])
		{
			kparm = kparms[pi];
			bparm = bparms[pi];
			break;
		}
	}

	iriResult = iriResult * kparm + bparm;
	return iriResult;
}

void hn::hnCalcuIRIMethodApi::calcAccDisIntegrate(std::vector<DAQ_STRUCT_INFO>& vecDaqInfo, std::vector<POSD_STRUCT_INFO>& vecMatPos)
{
	double dtime = 1.0 / m_nImuHz;
	double lenAcc = vecDaqInfo.size();

	int ulen = 900;
	double part = floor(lenAcc / ulen) - 2;
	vecMatPos.resize(vecDaqInfo.size());
	for (int n = 0;n < vecDaqInfo.size();n++)
	{
		POSD_STRUCT_INFO& posInfo = vecMatPos[n];
		DAQ_STRUCT_INFO& daqInfo = vecDaqInfo[n];

		posInfo.dH = 0.0;
		posInfo.dGpsTime = daqInfo.gpsSecond;
		posInfo.nGpsWeek = daqInfo.gpsWeek;
		posInfo.dDist = (daqInfo.dim - vecDaqInfo[0].dim) * m_dDmiWheel / m_nDmiHz;
	}

	// 计算初始速度;
	std::vector<DAQ_STRUCT_INFO> tAcc;
	tAcc.resize(ulen * 2);
	int lenthtAcc = tAcc.size();
	for (unsigned int n = 0;n < lenthtAcc;n++)
	{
		tAcc[n] = vecDaqInfo[n];
	}
	double speed = (vecDaqInfo[lenthtAcc - 1].dim - vecDaqInfo[0].dim) * 3.6 / 1000 / (lenthtAcc / m_nImuHz);
	int fitnum = ceil(speed / 10);
	if (fitnum > 3)
	{
		fitnum = 3;
	}

	double* pAcc0 = new double[lenthtAcc];
	double* pVel0 = new double[lenthtAcc];
	double* ptDis0 = new double[lenthtAcc];
	for (int n = 0;n < lenthtAcc;n++)
	{
		pAcc0[n] = vecDaqInfo[n+1].yAccelerate * 1000.0;
	}


	Acc2Vel(lenthtAcc, pAcc0, pVel0, dtime,fitnum);
	Vel2Dis(lenthtAcc, pVel0, ptDis0, dtime,fitnum);

	// 前1秒数据;
	//double* pDis = new double[ulen];
	int ncount = 0;
	//std::vector<double> vecHeights;
	for (int n = 0;n < ulen;n++)
	{
		//pDis[n] = ptDis0[n];
		vecMatPos[n].dH = ptDis0[n];
		//vecHeights.push_back(ptDis0[n]);
		ncount++;
	}
	double right_dis = vecMatPos[ulen-1].dH;


	// 增加;
	double* pAcc = new double[3 * ulen];
	double* pVel = new double[3 * ulen];
	double* ptDis = new double[3 * ulen];
	double* ptDisNew = new double[3 * ulen];

	
	for (int ii = 1;ii < part;ii++)
	{
		// 原始加速度赋值-3秒的数据;
		for (int n = (ii - 1)*ulen;n < (ii + 2)*ulen;n++)
		{
			pAcc[n - (ii - 1)*ulen] = vecDaqInfo[n].yAccelerate * 1000.0;
		}

		if (ii == 345)
		{
			int test = 0;
		}

		try
		{
			// 3秒的数据计算一个速度值;
			int lengthtAcc = 3 * ulen;
			speed = (vecDaqInfo[(ii + 2)*ulen - 1].dim - vecDaqInfo[(ii - 1)*ulen + 0].dim) * 3.6 / 1000.0 / (lengthtAcc / m_nImuHz);
			fitnum = ceil(speed / 10);
			if (fitnum > 0)
			{
				int test = 0;
			}

			if (fitnum > 3)
			{
				fitnum = 3;
			}

			// 加速度值积分为速度;
			Acc2Vel(lengthtAcc, pAcc, pVel, dtime, fitnum);

			// 速度值积分为距离变化;
			Vel2Dis(lengthtAcc, pVel, ptDis, dtime, fitnum);

			// 每条记录积分的变化值加上中间变量量;
			//tDis = tDis - tDis(ulen) + right_dis;
			for (int n = 0; n < lengthtAcc; n++)
			{
				ptDisNew[n] = ptDis[n] - ptDis[ulen - 1] + right_dis;
			}

			//Dis(ii*ulen + 1:(ii + 1)*ulen) = tDis(ulen + 1:ulen * 2);
			for (int n = ulen; n < ulen * 2; n++)
			{
				// 最后一秒超过部分舍弃;
				if (((ii - 1)*ulen + n) >= vecMatPos.size())
				{
					continue;
				}

				vecMatPos[(ii-1)*ulen + n].dH = ptDisNew[n];
				ncount++;

				//vecHeights.push_back(ptDisNew[n]);
				//int test1 = 0;
			}

			if ((ii + 1)*ulen >= vecMatPos.size())
			{
				break;
			}
			right_dis = vecMatPos[(ii + 1)*ulen - 1].dH;

			//right_dis = vecHeights[(ii + 1)*ulen - 1];
			//right_dis = ptDisNew[ulen * 2 - 1];
		}
		catch (...)
		{
			int test = 0;
		}

		if (loadCallback && ii % 5 == 0)
		{
			loadCallback(ii * 1.0 / part, "计算线性...");
		}

		if (m_use_thread && ii % 5 == 0)
		{
			setProgress(ii * 1.0 / part, "计算线性...");
		}
		
	}

	vecMatPos.resize(ncount);

	//FILE* ptrFile = fopen("E:\\test0427.txt", "wt");
	//for (int n = 0;n < ncount;n++)
	//{
	//	POSD_STRUCT_INFO& posInfo = vecMatPos[n];
	//	fprintf_s(ptrFile, "%.6lf,%.6lf,%.6lf\n",posInfo.dGpsTime,posInfo.dDist,posInfo.dH);
	//}
	//fclose(ptrFile);


	if (m_use_thread)
	{
		setProgress(1.0, "计算线性完成");
	}

	delete[] pAcc;
	pAcc = NULL;
	delete[] pVel;
	pVel = NULL;
	delete[] ptDis;
	ptDis = NULL;
	delete[] ptDisNew;
	ptDisNew = NULL;

	delete[] pAcc0;
	delete[] pVel0;
	delete[] ptDis0;

	//for ii = 1:part
	//	tAcc = Acc((ii - 1)*ulen + 1:(ii + 2)*ulen);
	//speed = (Dmi((ii + 2)*ulen) - Dmi((ii - 1)*ulen + 1)) * 3.6 / 1000 / (length(tAcc) / frequency);
	//fitnum = ceil(speed / 10);
	//tDis = Acc2Dis(tAcc, dtime, fitnum);
	//tDis = tDis - tDis(ulen) + right_dis;
	//Dis(ii*ulen + 1:(ii + 1)*ulen) = tDis(ulen + 1:ulen * 2);
	//right_dis = Dis((ii + 1)*ulen);
	//end
}

void hn::hnCalcuIRIMethodApi::polyfit(int n, double *y, int poly_n, double *a, double *tempx, double *tempy, double *sumxx, double *sumxy, double *ata)
{
	int i, j;

	memset(a, 0, 4 * sizeof(double));
	memset(tempx, 0, n * sizeof(double));
	memset(tempy, 0, n * sizeof(double));

	memset(sumxx, 0, (poly_n * 2 + 1) * sizeof(double));
	memset(sumxy, 0, (poly_n + 1) * sizeof(double));
	memset(ata, 0, (poly_n + 1)*(poly_n + 1) * sizeof(double));

	for (i = 0; i < n; i++)
	{
		tempx[i] = 1;
		tempy[i] = y[i];
	}
	for (i = 0; i < 2 * poly_n + 1; i++)
	{
		for (sumxx[i] = 0, j = 0; j < n; j++)
		{
			sumxx[i] += tempx[j];
			tempx[j] *= j;
		}
	}
	for (i = 0; i < poly_n + 1; i++)
	{
		for (sumxy[i] = 0, j = 0; j < n; j++)
		{
			sumxy[i] += tempy[j];
			tempy[j] *= j;
		}
	}
	for (i = 0; i < poly_n + 1; i++)
	{
		for (j = 0; j < poly_n + 1; j++)
		{
			ata[i*(poly_n + 1) + j] = sumxx[i + j];
		}
	}
	gauss_solve(poly_n + 1, ata, a, sumxy);
}

void hn::hnCalcuIRIMethodApi::gauss_solve(int n, double *A, double *x, double *b)
{
	int i, j, k, r;
	double max;
	for (k = 0; k < n - 1; k++)
	{
		max = fabs(A[k*n + k]); /*find maxmum*/
		r = k;
		for (i = k + 1; i < n - 1; i++)
			if (max < fabs(A[i*n + i]))
			{
				max = fabs(A[i*n + i]);
				r = i;
			}
		if (r != k)
			for (i = 0; i < n; i++)         /*change array:A[k]&A[r] */
			{
				max = A[k*n + i];
				A[k*n + i] = A[r*n + i];
				A[r*n + i] = max;
			}
		max = b[k];                    /*change array:b[k]&b[r]     */
		b[k] = b[r];
		b[r] = max;
		for (i = k + 1; i < n; i++)
		{
			for (j = k + 1; j < n; j++)
				A[i*n + j] -= A[i*n + k] * A[k*n + j] / A[k*n + k];
			b[i] -= A[i*n + k] * b[k] / A[k*n + k];
		}
	}

	for (i = n - 1; i >= 0; x[i] /= A[i*n + i], i--)
		for (j = i + 1, x[i] = b[i]; j < n; j++)
			x[i] -= A[i*n + j] * x[j];
}

void hn::hnCalcuIRIMethodApi::polyfitVal(int n, double *y, int poly_n, double *a)
{
	double sumy = 0;
	for (int i = 0; i < n; ++i)
	{
		sumy = 0;
		for (int j = poly_n; j >= 0; --j)
		{
			sumy = i * sumy + a[j];
		}
		y[i] = y[i] - sumy;
	}
}

void hn::hnCalcuIRIMethodApi::Acc2Vel(int n, double *acc, double *vel, double t, int fitnum)
{
	polyfit(n, acc, 0, m_poly_a, m_tempx, m_tempy, m_sumxx_0, m_sumxy_0, m_ata_0);
	polyfitVal(n, acc, 0, m_poly_a);

	memset(vel, 0, n * sizeof(double));
	for (int i = 1; i < n; ++i)
	{
		vel[i] = vel[i - 1] + (acc[i] + acc[i - 1]) * t * 0.5;
	}

	polyfit(n, vel, fitnum, m_poly_a, m_tempx, m_tempy, m_sumxx_1, m_sumxy_1, m_ata_1);
	polyfitVal(n, vel, fitnum, m_poly_a);
}

void hn::hnCalcuIRIMethodApi::Vel2Dis(int n, double *vel, double *dis, double t, int fitnum)
{
	memset(dis, 0, n * sizeof(double));
	for (int i = 1; i < n; ++i)
	{
		dis[i] = dis[i - 1] + (vel[i] + vel[i - 1]) * t * 0.5;
	}

	polyfit(n, dis, fitnum, m_poly_a, m_tempx, m_tempy, m_sumxx_2, m_sumxy_2, m_ata_2);
	polyfitVal(n, dis, fitnum, m_poly_a);

	for (int i = 0; i < n; ++i)
	{
		dis[i] = dis[i];
	}
}

void hn::hnCalcuIRIMethodApi::Acc2Vel2(int ptcount, double* ptAcc, double*& ptVel, double dTimeScale, int fitnum)
{
	int len = ptcount;

	//x = 1:1 : len;
	//x = x';
	int* x = new int[len];
	for (int n = 0;n < len;n++)
	{
		x[n] = n + 1;
	}

	polyfit(ptcount, ptAcc, 0, m_poly_a, m_tempx, m_tempy, m_sumxx_0, m_sumxy_0, m_ata_0);

	//P = polyfit(x, Acc, 0);
	//Acc = Acc - polyval(P, x);

	//Vel = zeros(len, 1);
	//for ii = 2:len
	//	Vel(ii) = Vel(ii - 1) + 0.5 * (Acc(ii) + Acc(ii - 1))*T;
	//end

	//	%%
	//	x = 1:1 : len;
	//x = x';
	//	P = polyfit(x, Vel, n);
	//Vel = Vel - polyval(P, x);
}

int hn::hnCalcuIRIMethodApi::calcuIRIMTD()
{
	// 获取resample.txt信息;
	QString strDaqDirPath = QString::fromLocal8Bit(m_strDaqPath.data());
	strDaqDirPath = strDaqDirPath.left(strDaqDirPath.lastIndexOf('/'));
	QString strResamplePath = strDaqDirPath + "/Resample.txt";
	QString strCoeffPath = strDaqDirPath + "/Coeff.dat";
	bool isCoeffFileExist = false;
	QFileInfo qfile;
	if (qfile.exists(strCoeffPath))
	{
		isCoeffFileExist = true;
	}

	// 读取速度补偿参数;
	m_dSpeedParm = NULL;
	m_dSpeedK = NULL;
	m_dSpeedB = NULL;
	char strline[1024];
	int nSpeedCount = 0;
	FILE* ptrFile = fopen(strCoeffPath.toLocal8Bit().data(), "rt");
	if (ptrFile)
	{
		fgets(strline, 1024, ptrFile);
		sscanf_s(strline, "%d\n",&nSpeedCount);
		if (nSpeedCount > 0)
		{
			m_dSpeedParm = new double[nSpeedCount];
			m_dSpeedK = new double[nSpeedCount];
			m_dSpeedB = new double[nSpeedCount];
			m_bUseSpeedParam = true;
			m_nSpeedCnt = nSpeedCount;
		}

		int ntmp = 0;
		double dTmp = 0.0;
		for (int n = 0;n < nSpeedCount;n++)
		{
			memset(strline, 0, 1024);
			fgets(strline, 1024, ptrFile);
			sscanf_s(strline, "%d\n", &ntmp);
			m_dSpeedParm[n] = ntmp;
		}

		for (int n = 0; n < nSpeedCount; n++)
		{
			memset(strline, 0, 1024);
			fgets(strline, 1024, ptrFile);
			sscanf_s(strline, "%lf\n", &dTmp);
			m_dSpeedK[n] = dTmp;
		}

		for (int n = 0; n < nSpeedCount; n++)
		{
			memset(strline, 0, 1024);
			fgets(strline, 1024, ptrFile);
			sscanf_s(strline, "%lf\n", &dTmp);
			m_dSpeedB[n] = dTmp;
		}

		fclose(ptrFile);
	}
	
	// 读取resample值;
	int nret = 0;
	double dtmp0, dtmp1, dtmp2;
	dtmp0 = dtmp1 = dtmp2 = 0.0;
	unsigned long long ullTmp0, ullTmp1;
	ullTmp0 = ullTmp1 = 0;
	std::vector<POSD_RESAMPLE250_INFO> vecResample50;
	vecResample50.resize(5000);
	int nPerSize = 0;
	POSD_RESAMPLE250_INFO sampleInfo;
	ptrFile = fopen(strResamplePath.toLocal8Bit().data(), "rt");
	while (!feof(ptrFile))
	{
		// 313.7116383823	-1.1811917630	314.8928301452	23424	   733657306
		memset(strline, 0, 1024);
		fgets(strline, 1024, ptrFile);
		nret = sscanf_s(strline, "%lf	%lf	%lf	%llu	%llu\n", &dtmp0,&dtmp1,&dtmp2,&ullTmp0,&ullTmp1);
		if (nret < 5)
		{
			continue;
		}

		// 记录高程值和帧号;
		sampleInfo.dLaserHeight = dtmp0;
		sampleInfo.dAccHeight = dtmp1;
		sampleInfo.dHeight = dtmp2;
		sampleInfo.ullFrameIdx = ullTmp0;
		sampleInfo.dim = ullTmp0;
		sampleInfo.ullFrameTime = ullTmp1;
		vecResample50[nPerSize] = sampleInfo;
		nPerSize++;

		if (nPerSize >= vecResample50.size())
		{
			vecResample50.resize(vecResample50.size() + 5000);
		}
	}
	fclose(ptrFile);
	vecResample50.resize(nPerSize);

	// 读取DAQ数据,更新时间信息;
	bool bRet = UpdateTimeLoadDaq(vecResample50);
	if (!bRet)
	{
		m_strErrMsg = "读取文件失败";
		delete[] m_dSpeedParm;
		m_dSpeedParm = NULL;
		delete[] m_dSpeedK;
		m_dSpeedK = NULL;
		delete[] m_dSpeedB;
		m_dSpeedB = NULL;
		return 1;
	}

	// 重采样;
	std::vector<POSD_RESAMPLE250_INFO> vecResample250;
	resampleIRI250From50(vecResample50, vecResample250);
	if (vecResample250.size() < 10)
	{
		if (m_use_thread)
		{
			setProgress(1.0, "计算完成");
		}

		m_strErrMsg = "重采样计算里程过短,不足1m,无法计算处理";
		delete[] m_dSpeedParm;
		m_dSpeedParm = NULL;
		delete[] m_dSpeedK;
		m_dSpeedK = NULL;
		delete[] m_dSpeedB;
		m_dSpeedB = NULL;
		return 2;
	}

	// 重采样计算;
	std::string strResamplePath250 = m_strSaveResultDir + "/";
	std::string strSaveResamplePath = strResamplePath250 + m_strSavePreName + "_ReSample250.txt";
	saveReSample250(strSaveResamplePath.data(), vecResample250);

	// 计算IRI(平整度值) - 10m;
	if (m_bSaveIRI10)
	{
		std::vector<IRI_RESULT_TIME_INFO> vecIRIResult10;
		calcuIRIMethod(10.0, 0.25, vecResample250, vecIRIResult10);
		std::string strIRIResultPath10 = m_strSaveResultDir + "/";
		strIRIResultPath10 = strIRIResultPath10 + m_strSavePreName + "_IRI_10m.txt";
		saveIRIs(strIRIResultPath10.data(), vecIRIResult10);
	}

	// 计算IRI(平整度值) - 100m;
	if (m_bSaveIRI100)
	{
		std::vector<IRI_RESULT_TIME_INFO> vecIRIResult100;
		calcuIRIMethod(100.0, 0.25, vecResample250, vecIRIResult100);
		std::string strIRIResultPath100 = m_strSaveResultDir + "/";
		strIRIResultPath100 = strIRIResultPath100 + m_strSavePreName + "_IRI_100m.txt";
		//strIRIResultPath100 = strIRIResultPath100 + "IRI_100m.txt";
		saveIRIs(strIRIResultPath100.data(), vecIRIResult100);
	}


	// 计算IRI(平整度值) - 1000m;
	if (m_bSaveIRI1000)
	{
		std::vector<IRI_RESULT_TIME_INFO> vecIRIResult1000;
		calcuIRIMethod(1000.0, 0.25, vecResample250, vecIRIResult1000);
		std::string strIRIResultPath1000 = m_strSaveResultDir + "\\";
		strIRIResultPath1000 = strIRIResultPath1000 + m_strSavePreName + "_IRI_1000m.txt";
		//strIRIResultPath1000 = strIRIResultPath1000 + "IRI_1000m.txt";
		saveIRIs(strIRIResultPath1000.data(), vecIRIResult1000);
	}

	// 标记处理完成;
	std::string strIRIResultPathFinish = m_strDaqPath;
	strIRIResultPathFinish = strIRIResultPathFinish.substr(0, strIRIResultPathFinish.find_last_of('.'));
	strIRIResultPathFinish = strIRIResultPathFinish + "_finished.txt";
	FILE* ptrSaveFinish = NULL;
	int nFinsh = 1;
	fopen_s(&ptrSaveFinish, strIRIResultPathFinish.data(), "wt");
	fprintf_s(ptrSaveFinish, "%d\n", nFinsh);
	fclose(ptrSaveFinish);


	// 里程校正，输出结果值;

	if (m_use_thread)
	{
		setProgress(1.0, "计算完成");
	}

	delete[] m_dSpeedParm;
	m_dSpeedParm = NULL;
	delete[] m_dSpeedK;
	m_dSpeedK = NULL;
	delete[] m_dSpeedB;
	m_dSpeedB = NULL;
	return 0;
}

std::string hn::hnCalcuIRIMethodApi::getErrMsg()
{
	return m_strErrMsg;
}

void hn::hnCalcuIRIMethodApi::testResample100(const char* strResamplePath)
{
	FILE* ptrFileResample = NULL;
	fopen_s(&ptrFileResample, strResamplePath, "rt");
	std::vector<POSD_RESAMPLE250_INFO> vecResample250;
	char strLine[1024];
	while (!feof(ptrFileResample))
	{
		memset(strLine,0,1024);
		fgets(strLine,1024,ptrFileResample);

		// 200.4118159056	-0.0705153811	200.4823312867	353827
		POSD_RESAMPLE250_INFO info;
		double t1, t2, t3;
		unsigned long t4 = 0;
		int nret = sscanf_s(strLine,"%lf	%ld\n",&t3,&t4);
		if (nret >= 2)
		{
			info.dHeight = t3;
			info.dim = t4;

			vecResample250.push_back(info);
		}
	}
	fclose(ptrFileResample);

	////取5个点做均值滤波;
	//std::vector<POSD_RESAMPLE250_INFO> vecResample250New;
	//std::vector<POSD_RESAMPLE250_INFO> vecResample250Orient;
	//vecResample250Orient = vecResample250;
	//int len = vecResample250.size();
	//for (int i = 2; i < len - 2; ++i)
	//{
	//	vecResample250Orient[i].dHeight = (vecResample250[i - 2].dHeight + vecResample250[i - 1].dHeight 
	//		+ vecResample250[i].dHeight + vecResample250[i + 1].dHeight + vecResample250[i + 2].dHeight) / 5;
	//}

	//int qplusenum = 5;
	//for (int i = 0, j = 0; i < len; i += qplusenum, ++j)
	//{
	//	vecResample250New.push_back(vecResample250Orient[i]);
	//}

	// 计算IRI(平整度值) - 10m;
	std::vector<IRI_RESULT_TIME_INFO> vecIRIResult10;
	calcuIRIMethod(10.0, 0.25, vecResample250, vecIRIResult10);
	std::string strIRIResultPath10 = m_strSaveResultDir + "\\";
	strIRIResultPath10 = strIRIResultPath10 + m_strSavePreName + "_IRI_10m.txt";
	saveIRIs(strIRIResultPath10.data(), vecIRIResult10);

	// 计算IRI(平整度值) - 100m;
	std::vector<IRI_RESULT_TIME_INFO> vecIRIResult100;
	calcuIRIMethod(100.0, 0.25, vecResample250, vecIRIResult100);
	std::string strIRIResultPath100 = m_strSaveResultDir + "\\";
	strIRIResultPath100 = strIRIResultPath100 + m_strSavePreName + "_IRI_100m.txt";
	saveIRIs(strIRIResultPath100.data(), vecIRIResult100);
}

void hn::hnCalcuIRIMethodApi::setProgress(float p,  const char* str_msg)
{
	QString str_mgs_t = QString::fromLocal8Bit(str_msg);
	emit progress(p, str_mgs_t);
}

void hn::hnCalcuIRIMethodApi::setSaveIRIs(int save10, int save100, int save1000)
{
	m_bSaveIRI10 = save10;
	m_bSaveIRI100 = save100;
	m_bSaveIRI1000 = save1000;
}

bool hn::hnCalcuIRIMethodApi::checkLineData(std::string strLine)
{

	// 进行异或校验判断;
	std::string strData = strLine;
	int lineLength = strLine.length();
	if (lineLength < 20)
	{
		return false;
	}

	// 去除换行符;
	int npos = strLine.find_last_of('\r\n');
	if (npos >= 0)
	{
		strData = strLine.substr(1, npos);
	}
	else
	{
		strData = strLine.substr(1);
	}
	if (strData.length() <= 2)
	{
		return false;
	}

	npos = strData.find_last_of(',');
	std::string strVck = strData.substr(npos + 1, 2);
	std::string strVckCal = strVck;
	strData = strData.substr(0, npos + 1);

	// 异或校验检查;
	char cTemp = strData[0];
	int lentg = strData.length();
	//if (strData[lentg-3] == '\0')
	//{
	//	for (int n = 1;n < lentg-3;n++)
	//	{
	//		cTemp ^= strData[n];
	//	}
	//}
	//else
	{
		for (int n = 1; n < lentg; n++)
		{
			cTemp ^= strData[n];
		}
	}




	int checkval[2];
	checkval[0] = cTemp / 16;
	checkval[1] = cTemp % 16;

	for (int i = 0; i < 2; i++)
	{
		strVckCal[i] = checkval[i] < 10 ? checkval[i] + '0' : checkval[i] + 'A' - 10;
	}

	bool bsame = false;
	if (strcmp(strVck.data(), strVckCal.data()) == 0)
	{
		bsame = true;
	}
	else
	{
		bsame = false;
	}

	return bsame;
}
