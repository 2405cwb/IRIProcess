/*! @file
********************************************************************************
<PRE>
模块名       :XrIRIProcess
文件名       : XrIRIProcessRunnable.cpp
相关文件     : XrIRIProcessRunnable.h,QRunnable
文件实现功能 : 用于处理IRI计算过程，继承至QRunnable，便于构建线程;
作者         : 朱旭波
版本         : 软件部，朱旭波
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 :
日 期        版本     修改人              修改内容
2022/02/28	 1.0	 朱旭波		         创建
</PRE>
*******************************************************************************/
#include "XrIRIProcessRunnable.h"
#include "..\XrIRICalcuMethod\hnCalcuIRIMethodApi.h"
#include <QThread>
#include <QDir>
#include <QDateTime>
#include <QCoreApplication>

#define PI64       3.14159265358979323846
#define HGI300_RATE_LSB 0.00048828125
#define HGI300_ACC_LSB 0.009525

XrIRIProcessRunnable::XrIRIProcessRunnable(QObject *parent)
	: QObject(parent)
{
	m_nProcType = 0;
	m_nDmiHz = 2350;
	m_dWheelSize = 2.35;
}

XrIRIProcessRunnable::~XrIRIProcessRunnable()
{
}

void XrIRIProcessRunnable::setDaqPath(QString strDaqPath)
{
	m_strDaqPath = strDaqPath;
}

void XrIRIProcessRunnable::setProcType(int type)
{
	m_nProcType = type;
}

void XrIRIProcessRunnable::setDaqDirPath(QString strDirPath)
{
	m_strOrientDaqDir = strDirPath;
}

void XrIRIProcessRunnable::setDmiWheel(int dmiHz, double dWheelSize)
{
	m_nDmiHz = dmiHz;
	m_dWheelSize = dWheelSize;
}

void XrIRIProcessRunnable::setKB(double dK, double dB)
{
	m_dK = dK;
	m_dB = dB;
}

void XrIRIProcessRunnable::setSaveIRIs(int save10, int save100, int save1000)
{
	m_bSaveIRI10 = save10;
	m_bSaveIRI100 = save100;
	m_bSaveIRI1000 = save1000;
}

void XrIRIProcessRunnable::setSaveDir(QString strSaveDir)
{
	m_strSaveDir = strSaveDir;
}

//void XrIRIProcessRunnable::setLoadCallback(float p, const char* msg)
//{
//	// 更新界面进度条;
//	QString strMsg = QString::fromLocal8Bit(msg);
//	emit progress(p,strMsg);
//}

void XrIRIProcessRunnable::run()
{
	switch (m_nProcType)
	{
	case 0:
		// 处理单个数据;
		processData(m_strDaqPath);
		break;
	case 1:
		// 处理文件夹下所有数据;
		processAllDataDir(m_strOrientDaqDir);
		break;
	case 2:
		// 处理激光平整度数据;
		processIriMtdData(m_strDaqPath);
	}



	// 处理完成，退出线程;
	QThread::currentThread()->quit();
}

void XrIRIProcessRunnable::getProgress(float p, QString msg)
{
	emit progress(p, msg);
}

void XrIRIProcessRunnable::processData(QString strDaqPath)
{
	int imuHz = 900;
	int dmrHz = m_nDmiHz;
	double dDmiWheel = m_dWheelSize;

	// 存储的文件夹路径;
	QString strDirPath = strDaqPath;
	strDirPath = strDirPath.left(strDirPath.lastIndexOf('/'));
	QString strPreName = strDaqPath;
	strPreName = strPreName.right(strPreName.length() - strPreName.lastIndexOf('/') - 1);
	strPreName = strPreName.left(strPreName.lastIndexOf('.'));

	//// 获取原始DAQ文件夹名称;
	//QString strTmpDirName = strDirPath;
	//int nret = strTmpDirName.lastIndexOf('/');
	//if (nret > 0)
	//{
	//	strTmpDirName = strTmpDirName.right(strTmpDirName.length() - strTmpDirName.lastIndexOf('/') - 1);
	//}
	//else
	//{
	//	strTmpDirName = "/";
	//}

	//QString strTargDir = m_strSaveDir + "/" + strTmpDirName + "/";

	hn::hnCalcuIRIMethodApi* calcuIRIApi = new hn::hnCalcuIRIMethodApi;
	calcuIRIApi->loadCallback = NULL;
	calcuIRIApi->setUseThread(true);
	connect(calcuIRIApi, SIGNAL(progress(float, QString)), this, SLOT(getProgress(float, QString)));
	calcuIRIApi->setParam(strDaqPath.toLocal8Bit().data(), imuHz, dmrHz, dDmiWheel);
	calcuIRIApi->setKB(m_dK,m_dB);

	calcuIRIApi->setSaveIRIs(m_bSaveIRI10,m_bSaveIRI100,m_bSaveIRI1000);
	calcuIRIApi->setSaveResultPath(m_strSaveDir.toLocal8Bit().data(), strPreName.toLocal8Bit().data());
	calcuIRIApi->setIsOnRight(1);

	// 计算处理;
	QString strMsg0 = QString("%1%2").arg(strDaqPath).arg(":startProc\n");
	writeLog(strMsg0);
	int bErrNum = 0;
	bErrNum = calcuIRIApi->calcuIRI();
	std::string strMsg = calcuIRIApi->getErrMsg();
	strMsg0 = QString("%1%2%3").arg(strDaqPath).arg(":endProc,ErrMsg:").arg(strMsg.data());
	writeLog(strMsg0);

	disconnect(calcuIRIApi, SIGNAL(progress(float, QString)), this, SLOT(getProgress(float, QString)));

	// 内存释放;
	delete calcuIRIApi;
	calcuIRIApi = NULL;

	// 通知处理完成;
	emit process_finished(strDaqPath, bErrNum);
}

void XrIRIProcessRunnable::processIriMtdData(QString strDaqPath)
{
	int imuHz = 900;
	int dmrHz = m_nDmiHz;
	double dDmiWheel = m_dWheelSize;

	// 存储的文件夹路径;
	QString strDirPath = strDaqPath;
	strDirPath = strDirPath.left(strDirPath.lastIndexOf('/'));
	QString strPreName = strDaqPath;
	strPreName = strPreName.right(strPreName.length() - strPreName.lastIndexOf('/') - 1);
	strPreName = strPreName.left(strPreName.lastIndexOf('.'));

	QString strTmpDirName = strDirPath;
	int nret = strTmpDirName.lastIndexOf('/');
	strTmpDirName = strTmpDirName.left(strTmpDirName.lastIndexOf('/'));
	strTmpDirName = strTmpDirName.left(strTmpDirName.lastIndexOf('/'));
	nret = strTmpDirName.lastIndexOf('/');
	if (nret > 0)
	{
		// 同步把DAQ配合subname构建一个新的前置名称用于IRI输出名称;
		QString strSubPreName = strTmpDirName.right(strTmpDirName.length() - strTmpDirName.lastIndexOf('/') - 1);
		QString strSubPreName0 = strSubPreName + "_" + strPreName;
		strPreName = strSubPreName0;
	}
	else
	{
		strTmpDirName = "/";
	}

	hn::hnCalcuIRIMethodApi* calcuIRIApi = new hn::hnCalcuIRIMethodApi;
	calcuIRIApi->loadCallback = NULL;
	calcuIRIApi->setUseThread(true);
	connect(calcuIRIApi, SIGNAL(progress(float, QString)), this, SLOT(getProgress(float, QString)));
	calcuIRIApi->setParam(strDaqPath.toLocal8Bit().data(), imuHz, dmrHz, dDmiWheel);
	calcuIRIApi->setKB(m_dK, m_dB);

	calcuIRIApi->setSaveIRIs(m_bSaveIRI10, m_bSaveIRI100, m_bSaveIRI1000);
	calcuIRIApi->setSaveResultPath(m_strSaveDir.toLocal8Bit().data(), strPreName.toLocal8Bit().data());
	calcuIRIApi->setIsOnRight(1);

	// 计算处理;
	int bErrNum = 0;
	QString strMsg = QString("%1%2").arg(strDaqPath).arg(":startProc\n");
	writeLog(strMsg);
	bErrNum = calcuIRIApi->calcuIRIMTD();
	std::string strErr = calcuIRIApi->getErrMsg();
	strMsg = QString("%1%2%3").arg(strDaqPath).arg(":endProc,ErrMsg:").arg(strErr.data());
	writeLog(strMsg);

	disconnect(calcuIRIApi, SIGNAL(progress(float, QString)), this, SLOT(getProgress(float, QString)));

	// 内存释放;
	delete calcuIRIApi;
	calcuIRIApi = NULL;

	// 通知处理完成;
	emit process_finished(strDaqPath, bErrNum);
}

void XrIRIProcessRunnable::processAllDataDir(QString strDirPath)
{
	// 搜索文件夹下所有DAQ文件;
	std::vector<QString> vecSearchResult;
	getDaqFiles(strDirPath, vecSearchResult);
	if (vecSearchResult.size() <= 0)
	{
		// 不处理，返回;
		emit process_dir_finished(strDirPath, m_strSaveDir ,-1);
		return;
	}

	// 逐个进行处理;
	for (unsigned int n= 0;n < vecSearchResult.size();n++)
	{
		// 逐个获取有效的DAQ文件路径;
		QString strDaqPath = vecSearchResult[n];

		// 存储的文件夹路径及文件名称;
		bool isIrimtdEixst = false; // 通过判断IRIMTD确定当前为激光平整度数据还是惯导平整度数据;
		QString strDirPath = strDaqPath;
		strDirPath = strDirPath.left(strDirPath.lastIndexOf('/'));

		// 从目录结构中查找;
		QString strMtdExistPath = strDirPath;
		if (strMtdExistPath.lastIndexOf('/') > 0 && strMtdExistPath.contains("IRIMTD"))
		{
			strMtdExistPath = strMtdExistPath.left(strMtdExistPath.lastIndexOf('/'));
			if (strMtdExistPath.lastIndexOf('/') > 0)
			{
				strMtdExistPath = strMtdExistPath.right(strMtdExistPath.length() - strMtdExistPath.lastIndexOf('/') - 1);
				if (strMtdExistPath.compare("IRIMTD") == 0 || strMtdExistPath.compare("irimtd") == 0)
				{
					// 表明为激光平整度数据;
					isIrimtdEixst = true;
				}
			}
		}

		// 名称信息解析;
		QString strPreName = strDaqPath.right(strDaqPath.length() - strDaqPath.lastIndexOf('/') - 1);
		strPreName = strPreName.left(strPreName.lastIndexOf('.'));

		// 获取原始DAQ文件夹名称-IRIMTD里面的文件夹前缀名;
		QString strTmpDirName = strDirPath;
		int nret = strTmpDirName.lastIndexOf('/');
		if (isIrimtdEixst)
		{
			strTmpDirName = strTmpDirName.left(strTmpDirName.lastIndexOf('/'));
			strTmpDirName = strTmpDirName.left(strTmpDirName.lastIndexOf('/'));
			nret = strTmpDirName.lastIndexOf('/');
			if (nret > 0)
			{
				// 同步把DAQ配合subname构建一个新的前置名称用于IRI输出名称;
				QString strSubPreName = strTmpDirName.right(strTmpDirName.length() - strTmpDirName.lastIndexOf('/') - 1);
				QString strSubPreName0 = strSubPreName + "_" + strPreName;
				strPreName = strSubPreName0;

				//// 子目录上一级;
				//strTmpDirName = strTmpDirName.left(strTmpDirName.lastIndexOf('/'));
				//strTmpDirName = strTmpDirName.right(strTmpDirName.length() - strTmpDirName.lastIndexOf('/') - 1);
			}
			//else
			//{
			//	strTmpDirName = "/";
			//}
		}


		// 需要从名称上解析获取车轮周长、编码器频率、标定的KB系数;
		int imuHz = 900;
		double dWheelSize = 2.35;
		int nDmiHz = 2350;
		double scaleK = 1.0;
		double scaleB = 0.0;
		bool bNeedFromName = true;
		if (!isIrimtdEixst)
		{
			if (bNeedFromName)
			{
				// 需要从名称上解析;
				QString strDaqName = strPreName;
				QStringList qlist;
				qlist = strDaqName.split(QRegExp("[_*]"));
				int nsize = qlist.size();
				if (nsize >= 8)
				{
					// 解析获取编码器信息;
					QString strB = qlist[nsize - 1];
					QString strK = qlist[nsize - 2];
					QString strDmi = qlist[nsize - 3];
					QString strWheel = qlist[nsize - 4];

					// 获取信息,第一个值为符号标记;
					if (!strB.isEmpty())
					{
						// 千位值表达为0时表示为负号，为1表示为正数;
						int tmpNum = strB.toInt();
						if (tmpNum < 1000)
						{
							scaleB = -1.0 * tmpNum * 0.01;
						}
						else
						{
							scaleB = (tmpNum - 1000) * 0.01;
						}
					}

					if (!strK.isEmpty())
					{
						int tmpNum = strK.toInt();
						scaleK = tmpNum * 0.001;
					}

					if (!strDmi.isEmpty())
					{
						int tmpNum = strDmi.toInt();
						nDmiHz = tmpNum;
					}

					if (!strWheel.isEmpty())
					{
						int tmpNum = strWheel.toInt();
						dWheelSize = tmpNum * 0.001;
					}
				}
			}// if (bNeedFromName)
		}

		// 逐个进行处理;
		hn::hnCalcuIRIMethodApi* calcuIRIApi = new hn::hnCalcuIRIMethodApi;
		calcuIRIApi->loadCallback = NULL;
		calcuIRIApi->setUseThread(true);
		connect(calcuIRIApi, SIGNAL(progress(float, QString)), this, SLOT(getProgress(float, QString)));
		calcuIRIApi->setParam(strDaqPath.toLocal8Bit().data(), imuHz, nDmiHz, dWheelSize);
		calcuIRIApi->setKB(scaleK, scaleB);

		calcuIRIApi->setSaveIRIs(m_bSaveIRI10, m_bSaveIRI100, m_bSaveIRI1000);
		calcuIRIApi->setSaveResultPath(m_strSaveDir.toLocal8Bit().data(), strPreName.toLocal8Bit().data());
		calcuIRIApi->setIsOnRight(1);

		QString strMsg0 = QString("%1%2").arg(strDaqPath).arg(":startProc\n");
		writeLog(strMsg0);

		// 计算处理;
		int bErrNum = 0;
		if (isIrimtdEixst)
		{
			bErrNum = calcuIRIApi->calcuIRIMTD();
		}
		else
		{
			bErrNum = calcuIRIApi->calcuIRI();
		}
		std::string strMsg = calcuIRIApi->getErrMsg();

		strMsg0 = QString("%1%2%3").arg(strDaqPath).arg(":endProc,ErrMsg:").arg(strMsg.data());
		writeLog(strMsg0);
		
		

		disconnect(calcuIRIApi, SIGNAL(progress(float, QString)), this, SLOT(getProgress(float, QString)));

		// 处理失败，则写入日志信息;
		if (bErrNum != 0)
		{
			// 获取路径信息;
			QString strDaqName = strDaqPath.right(strDaqPath.length() - strDaqPath.lastIndexOf('/') - 1);
			QString strDirName = strDaqPath.left(strDaqPath.lastIndexOf('/'));

			// 写入日志的路径,文件夹不存在则创建;
			QString strSaveDir = m_strSaveDir;
			QString strSaveLogPath = strSaveDir + "/" + "iri_error_record.txt";

			// 采用全局锁,追加模式写入文件;
			g_saveLog_mutex.lock();
			FILE* ptrLogFile = NULL;
			fopen_s(&ptrLogFile, strSaveLogPath.toLocal8Bit().data(), "at+");
			if (ptrLogFile)
			{
				fprintf_s(ptrLogFile, "%s,%d,%s\n", strDaqName.toLocal8Bit().data(), bErrNum, strMsg.data());
				fclose(ptrLogFile);
			}
			g_saveLog_mutex.unlock();
		}

		// 内存释放;
		delete calcuIRIApi;
		calcuIRIApi = NULL;
	}

	emit process_dir_finished(strDirPath, m_strSaveDir, 0);
}

void XrIRIProcessRunnable::getDaqFiles(QString strDirPath, std::vector<QString>& vecSearchResult)
{
	// 设置dirlujing;
	QDir* dir = new QDir(strDirPath);
	QStringList filter;
	//filter << QString("*.daq");

	// 获取列表下所有文件信息;
	QList<QFileInfo>* fileInfo = new QList<QFileInfo>(dir->entryInfoList(filter));
	for (int i = 0; i < fileInfo->count(); i++)
	{
		const QFileInfo info_tmp = fileInfo->at(i);
		QString path_tmp = info_tmp.filePath();
		if (info_tmp.fileName() == ".." || info_tmp.fileName() == ".")
		{

		}
		else if (info_tmp.isFile())
		{
			// 检查文件后缀;
			int index = path_tmp.lastIndexOf('.');
			if (index > 0)
			{
				QString strExp = path_tmp.right(path_tmp.length() - index - 1);
				strExp = strExp.toLower();
				if (strExp.compare("daq") == 0) // 后缀判断;
				{
					// 判断是否为有效的DAQ数据，包括数据协议、数据大小、数据是否已处理（根据设置信息判断，当前要求加载的为已处理工程还是未处理工程或全部工程信息）;
					int nValid = isValidProject(path_tmp);
					if (nValid != 0)
					{
						// -1表示不符合处理条件的，比如要求只解算未处理的数据，但是已标记处理之类,在成果文件夹记录失败原因;
						if (nValid != -1)
						{
							// 获取路径信息;
							QString strDaqPath = path_tmp;
							QString strDaqName = strDaqPath.right(strDaqPath.length() - strDaqPath.lastIndexOf('/') - 1);
							QString strDirName = strDaqPath.left(strDaqPath.lastIndexOf('/'));
							//strDirName = strDirName.right(strDirName.length() - strDirName.lastIndexOf('/') - 1);
							//QString strLogName = strDirName + ".log";

							// 写入日志的路径,文件夹不存在则创建;
							QString strSaveDir = m_strSaveDir;
							QString strSaveLogPath = strSaveDir + "/" + "iri_error_record.txt";

							// 采用全局锁,追加模式写入文件;
							g_saveLog_mutex.lock();
							FILE* ptrLogFile = NULL;
							fopen_s(&ptrLogFile, strSaveLogPath.toLocal8Bit().data(), "at+");
							if (ptrLogFile)
							{
								fprintf_s(ptrLogFile, "%s,%d,%s\n", strDaqName.toLocal8Bit().data(), nValid, m_strMsgLog.toLocal8Bit().data());
								fclose(ptrLogFile);
							}
							g_saveLog_mutex.unlock();
						}


						continue;
					}

					// 应读取文件判断一下是否为有效惯导数据;
					vecSearchResult.push_back(path_tmp);
				}
			}
		}
		else if (info_tmp.isDir())// 为文件夹;
		{
			//// 不进行迭代遍历;
			//continue;

			// 迭代搜索;
			getDaqFiles(path_tmp, vecSearchResult);
		}
	}// for (int i = 0;i < fileInfo->count();i++)

	delete fileInfo;
	delete dir;
}

int XrIRIProcessRunnable::isValidProject(QString strDaqPath)
{
	// 检查文件大小，是否为有效的工程;
	bool isValid = false;
	QFileInfo fileInfo(strDaqPath);
	int nFileSize = fileInfo.size();
	if (nFileSize < 3 * 1024 * 1024)
	{
		m_strMsgLog = QString::fromLocal8Bit("采集数据过小,不进行处理");
		return 3;
	}

	// 检查是否为符合要求的数据(判断数据已处理标准为检查同名的resample250mm文件是否存在);
	QString strTmpPath = strDaqPath;
	strTmpPath = strTmpPath.left(strTmpPath.lastIndexOf('.'));
	QString strResamplePath = strTmpPath + "_finished.txt";
	int curFileModel = 0;
	if (fileInfo.exists(strResamplePath) == false) // 文件不存在，表示未处理
	{
		curFileModel = 1;
	}
	int nSearchTarget = 2;
	switch (nSearchTarget)
	{
	case 0:
		if (curFileModel > 0)
		{
			isValid = false;
		}
		else
		{
			isValid = true;
		}
		break;
	case 1:
		if (curFileModel > 0)
		{
			isValid = true;
		}
		else
		{
			isValid = false;
		}
		break;
	case 2:
		isValid = true;
		break;
	}

	// 当前已不符合条件，不再进行打开文件解析操作;
	if (!isValid)
	{
		return -1;
	}

	// 检查是否存在Resample.txt文件，存在则认为激光平整度;
	QString strResamplePath0 = strDaqPath;
	strResamplePath0 = strResamplePath0.left(strTmpPath.lastIndexOf('/'));
	strResamplePath0 = strResamplePath0 + "/Resample.txt";
	if (fileInfo.exists(strResamplePath0) == true || strResamplePath0.contains("IRIMTD") || strResamplePath0.contains("irimtd"))
	{
		if (fileInfo.exists(strResamplePath0) == true)
		{
			return 0;
		}

		return -1;
	}

	// 最后检查数据协议;
	FILE* ptrDaqFile = NULL;
	fopen_s(&ptrDaqFile, strDaqPath.toLocal8Bit().data(), "rt");
	if (!ptrDaqFile)
	{
		return -1;
	}
	char strLine[1024];
	fseek(ptrDaqFile, 1000, SEEK_SET);
	//fgets(strLine, 1024, ptrDaqFile); // 舍弃第一行;
	for (int n = 0; n < 1000; n++)
	{
		fgets(strLine, 1024, ptrDaqFile); // 舍弃1000行;
	}

									  // 解析部分数据;
	double dValueTol = 0.0;
	int ncount = 0;
	while (!feof(ptrDaqFile))
	{
		// 读取文件;
		memset(strLine, 0, 1024);
		fgets(strLine, 1024, ptrDaqFile);

		std::string str = strLine;
		if (str.empty() || str.size() < 73)
		{
			continue;
		}

		float fTmpAccY = 0.0;
		bool bret = parseHGI300DataPart(str, fTmpAccY);
		dValueTol += fTmpAccY;
		ncount++;

		if (ncount >= 5)
		{
			break;
		}
	}
	fclose(ptrDaqFile);

	// 在这个范围内都认为正常;
	double dtmp = dValueTol / ncount;
	dtmp = abs(dtmp);
	//if (dtmp > -15.0 && dtmp < -5.0)
	if (dtmp > 1.0 && dtmp < 20.0)
	{
		//isValid = true;
		return 0;
	}
	else
	{
		//isValid = false;
		m_strMsgLog = QString::fromLocal8Bit("惯导初始数据不正常,未进行正常静止采集,无法处理");
		return 4;
	}

	return 0;
}

// 将字符串转换为浮点(float);
float str2float0(const std::string& str)
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

using namespace std;
bool XrIRIProcessRunnable::parseHGI300DataPart(const std::string& str, float& yAccValue)
{
	string strYear = str.substr(5, 4);
	int year = atoi(strYear.data());

	//月;
	string strMonth = str.substr(9, 2);
	int mon = atoi(strMonth.data());

	//日;
	string strDay = str.substr(11, 2);
	int day = atoi(strDay.data());

	// 时;
	string strHour = str.substr(14, 2);
	int hour = atoi(strHour.data());

	// 分;
	string strMin = str.substr(16, 2);
	int min = atoi(strMin.data());

	// 秒;
	string strSec = str.substr(18, 2);
	int sec = atoi(strSec.data());

	// 毫秒;
	string strMSec = str.substr(21, 3);
	int microSec = atoi(strMSec.data());

	// 微秒;
	string strMillSec = str.substr(25, 3);
	int milliSec = atoi(strMillSec.data());

	// DIM数据读取;
	string strDimValue = str.substr(29, 10);
	unsigned long dim = strtoul(strDimValue.data(), NULL, 10);

	// rate x;
	string strXVelocity = str.substr(44, 4);
	float xVelocity = str2float0(strXVelocity) * HGI300_RATE_LSB * 180.0 / PI64;

	// rate y;
	string strYVelocity = str.substr(48, 4);
	float yVelocity = str2float0(strYVelocity) * HGI300_RATE_LSB * 180.0 / PI64;

	//  rate z;
	string strZVelocity = str.substr(52, 4);
	float zVelocity = str2float0(strZVelocity) * HGI300_RATE_LSB * 180.0 / PI64;

	// acc x;
	string strXAccelerate = str.substr(56, 4);
	float xAccelerate = str2float0(strXAccelerate) * HGI300_ACC_LSB;

	// acc y;
	string strYAccelerate = str.substr(60, 4);
	float yAccelerate = str2float0(strYAccelerate) * HGI300_ACC_LSB;

	// acc z;
	string strZAccelerate = str.substr(64, 4);
	float zAccelerate = str2float0(strZAccelerate) * HGI300_ACC_LSB;

	yAccValue = yAccelerate;
	return true;
}

void XrIRIProcessRunnable::writeLog(QString strMsg)
{
	QDateTime curdateTime = QDateTime::currentDateTime();
	QString strdatetime = curdateTime.toString("yyyy-MM-dd hh:mm:ss.zzz");

	QString strAppDir = QCoreApplication::applicationDirPath();
	QString strLogDir = strAppDir + "/LOG";
	QDir dir(strLogDir);
	if (!dir.exists())
	{
		if (dir.mkpath(strLogDir))
		{
			return;
		}
	}

	QString strDate = curdateTime.toString("yyyy-MM-dd");
	strDate += ".log";
	QString strLogPath = strLogDir + "/" + strDate;
	FILE* ptrFile = fopen(strLogPath.toLocal8Bit().data(),"at+");
	fprintf_s(ptrFile, "%s:%s\n",strdatetime.toLocal8Bit().data(),strMsg.toLocal8Bit().data());
	fclose(ptrFile);
}
