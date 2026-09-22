#include "XrIRIProcess.h"
#include "..\hnQtRibbonUI\hnRibbonBar.h"
#include "..\hnQtRibbonUI\hnRibbonCategory.h"
#include "..\hnQtRibbonUI\hnRibbonPannel.h"
#include "..\hnQtRibbonUI\hnRibbonToolButton.h"
#include "xrShowIriResultWidget.h"
#include "XrCalcuCalibrateParamDlg.h"
#include <QProgressBar>
#include <QLabel>
#include <QFile>
#include <QFileInfo>
#include <QTreeWidget>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include "XrIRISetting.h"
#include "XrSettingDialog.h"
#include <windows.h>
#include <QThreadPool>
#include "XrIRILoggingWidget.h"
#include "XrIRIProcessRunnable.h"
#include "HnAboutUsDlg.h"
#include <corecrt_io.h>
#include <direct.h>
#include <QTimer>
#include <QDateTime>
#include <QSettings>
#include "XrIRICalibraDmiDialog.h"

#define PI64       3.14159265358979323846
#define HGI300_RATE_LSB 0.00048828125
#define HGI300_ACC_LSB 0.009525

XrIRIProcess* XrIRIProcess::AppMain = NULL;
using namespace std;

void SplitString(const std::string &s, std::vector<std::string> &v, const std::string &c)
{
	std::string::size_type pos1, pos2;
	pos2 = s.find(c);
	pos1 = 0;
	while (std::string::npos != pos2)
	{
		v.push_back(s.substr(pos1, pos2 - pos1));

		pos1 = pos2 + c.size();
		pos2 = s.find(c, pos1);
	}
	if (pos1 != s.length())
	{
		v.push_back(s.substr(pos1));
	}
}

void CreateFolder(string folderpath)
{
	std::vector<std::string> vecSegTag;
	SplitString(folderpath, vecSegTag, "/");

	string PartDir = "";
	//std::vector<std::string>::iterator it;
	//for(it=vecSegTag.begin(); it!=vecSegTag.end(); ++it)
	for (int nn = 0; nn < vecSegTag.size(); nn++)
	{
		std::string str_temp = vecSegTag[nn];
		if (nn != 0)
		{
			PartDir += "/";
		}

		PartDir += str_temp;

		//PartDir = PartDir+"/"+*it;
		if (_access(PartDir.c_str(), 0) == -1)
		{
			if (_mkdir(PartDir.c_str()) == 0)
			{
				//                cout<<"folder "<<PartDir<<" creat successed!"<<endl;
			}
			else
			{
				//char Message[128];
				//sprintf(Message, "folder %s creat failed!", PartDir.c_str());
				//WriteLog(Message);
				//printf("%s\n", Message);
			}
			//system(("echo 'whuwhu'|sudo -S chmod 777 "+PartDir).c_str());
		}
	}
}

using namespace XrApp;
XrIRIProcess::XrIRIProcess(QWidget *parent)
	: hnRibbonMainWindow(parent)
{
	//ui.setupUi(this);

	// 面板管理器对象;
	m_DockManager = new hn::CDockManager(this);

	// 创建工具栏;
	createAction();

	// 创建视图;
	createView();

	// 创建连接;
	createConnect();

	// 树状视图连接;
	createTreeConnect();

	// 创建状态栏;
	setupStatusBar();

	// 设置软件图标;
	setWindowIcon(QIcon(":/Resources/icons/xr.ico"));

	//状态栏显示;
	statusBar()->show();

	// 读取已有布局;
	readLayout();

	//最大化;
	showMaximized();

	// 读取参数设置;
	XrIRISetting::getIRISetting()->readData();

	// 定时器，定时搜索并处理;
	m_pAutoProcTimer = new QTimer(this);
	connect(m_pAutoProcTimer,SIGNAL(timeout()),this,SLOT(timer_handleAutoProcTimeout()));
	if (XrIRISetting::getIRISetting()->getSearchInfo()->nUseAutoSearchProc) // 使用定时搜索处理，创建时启动;
	{
		m_pAutoProcTimer->start(30 * 60 * 1000);// 30 * 
	}

	// 获取当前计算机可用线程内核数;
	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	int num_processors = sys_info.dwNumberOfProcessors;
	QThreadPool::globalInstance()->setMaxThreadCount(num_processors - 1); // num_processors - 1
}

XrIRIProcess::~XrIRIProcess()
{
	if (m_DockManager)
	{
		delete m_DockManager;
		m_DockManager = NULL;
	}

	// 释放线程池资源,在此之前应检查确定所有线程已推出;
	QThreadPool::globalInstance()->releaseThread();
	QThreadPool::globalInstance()->clear();
}

void XrIRIProcess::closeEvent(QCloseEvent * e)
{
	saveLayout();
}

void XrIRIProcess::openProjectSlot()
{
	// 选择文件夹路径;
	QString proj_dir = QFileDialog::getExistingDirectory(this, QString::fromLocal8Bit("打开工程目录"));
	if (proj_dir.isEmpty())
	{
		return;
	}

	// 搜索目录下所有DAQ文件信息;
	std::vector<QString> vecDaqPaths;
	autoSearchDaqData(proj_dir, vecDaqPaths);
	if (vecDaqPaths.size() <= 0)
	{
		// 无文件存在则给予提示;
		QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("文件夹下不存在有效的IRI工程数据，请检查！"), QMessageBox::Ok);
		return;
	}

	// 列出所有文件路径;
	updateTreeNode(vecDaqPaths);
}

void XrIRIProcess::setConfigSlot()
{
	// 参数设置传入;
	XrSettingDialog iriSettingDlg;
	int ret = iriSettingDlg.exec();
}

void XrIRIProcess::calcIriSlot()
{
	// 获取所有树节点中信息;
	std::vector<QString> vecDaqPaths;
	QTreeWidgetItem* get_data_item = NULL;
	int ncount = m_docTreeWidget->topLevelItemCount();
	bool existed = false;
	QTreeWidgetItem* get_child_item = NULL;
	for (int n = 0; n < ncount; n++)
	{
		QTreeWidgetItem* parent_item = m_docTreeWidget->topLevelItem(n);
		if (parent_item)
		{
			// 获取节点记录的文件路径;
			QString strTmpTxt = parent_item->text(1);

			// 判断该文件路径是否已存在于列表中;
			XrIRIProcessRunnable* pRunnable = getIRICalcuWorker(strTmpTxt);
			if (pRunnable) // 如果已经存在，则不添加进去;
			{
				continue;
			}

			vecDaqPaths.push_back(strTmpTxt);

			//if (strTmpTxt.compare(strDaqPath) == 0)
			//{
			//	// 找到该节点;
			//	get_data_item = parent_item;
			//	break;
			//}
		} // if (parent_item)
	} // for (int n = 0; n < ncount; n++)

	if (vecDaqPaths.size() <= 0)
	{
		return;
	}

	// 开启线程进行处理;
	startProcessIRIData(vecDaqPaths);

	// 开始处理，设置按钮不可用，避免重复点击;
	m_calcIriAct->setEnabled(false);
	m_openProjectAct->setEnabled(false);
	m_aotuUpdateAct->setEnabled(false);
	m_clearListAct->setEnabled(false);
	m_setProjectParamAct->setEnabled(false);
}

void XrIRIProcess::autoSearchPathSlot()
{
	QString strSearchDir = XrIRISetting::getIRISetting()->getSearchInfo()->strSearchDir;

	// 搜索目录下所有DAQ文件信息;
	std::vector<QString> vecDaqPaths;
	autoSearchDaqData(strSearchDir, vecDaqPaths);
	int nsize = vecDaqPaths.size();
	if (nsize > 0)
	{
		// 给予提示是否导入;
		QString str_information;
		char cstr_temp[1024];
		memset(cstr_temp, 0, 1024);
		sprintf_s(cstr_temp, "检测到当前共 %d 组工程，是否导入?", nsize);

		// 输出到日志信息里面;
		m_showLoggingWidget->slotAddMsg(QString::fromLocal8Bit(cstr_temp));

		//QMessageBox::StandardButton reply;
		//reply = QMessageBox::question(this, QString::fromLocal8Bit("注意"), QString::fromLocal8Bit(cstr_temp), QMessageBox::Yes | QMessageBox::No);
		//if (reply != QMessageBox::Yes)
		//{
		//	return;
		//}

		// 添加到面板列表中;
		updateTreeNode(vecDaqPaths);
	}
}

void XrIRIProcess::calibrIriSlot()
{
	XrCalcuCalibrateParamDlg dlg;
	dlg.exec();
}

void XrIRIProcess::calibrWheelSlot()
{
	XrIRICalibraDmiDialog dlg;
	dlg.exec();
}

void XrIRIProcess::clearListSlot()
{
	// 清空列表前，先检查是否存在正在处理的数据;
	int activeAcount = QThreadPool::globalInstance()->activeThreadCount();
	if (activeAcount > 0) // 存在，给予提示，不清空操作;
	{
		QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("当前列表中存在正在处理的工作，请等待计算完成！"), QMessageBox::Ok);
		return;
	}

	clearAllItem();
}

void XrIRIProcess::helpbookSlot()
{
	QString path = QCoreApplication::applicationDirPath();
	path = QDir::toNativeSeparators(path);

	path = path + QString::fromLocal8Bit("\\惯导平整度后处理软件用户手册.pdf");

	//	启动外部程序;
	QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void XrIRIProcess::aboutUsSlot()
{
	HnAboutUsDlg dlg;
	dlg.exec();
}

void XrIRIProcess::treeviewItemDbClick(QTreeWidgetItem *item, int column)
{
	// 获取当前节点记录的DAQ路径;
	QString strPath = item->text(1);
	//QString strTmpPath = strPath;

	// 存储的文件夹路径及文件名称;
	QString strDirPath = strPath;
	strDirPath = strDirPath.left(strDirPath.lastIndexOf('/'));
	QString strPreName = strPath;
	strPreName = strPreName.right(strPreName.length() - strPreName.lastIndexOf('/') - 1);
	strPreName = strPreName.left(strPreName.lastIndexOf('.'));

	// 获取原始DAQ文件夹名称;
	QString strTmpDirName = strDirPath;
	int nret = strTmpDirName.lastIndexOf('/');
	if (nret > 0)
	{
		strTmpDirName = strTmpDirName.right(strTmpDirName.length() - strTmpDirName.lastIndexOf('/') - 1);
	}
	else
	{
		strTmpDirName = "/";
	}
	// 保存文件夹路径;
	QString strSaveDir = XrIRISetting::getIRISetting()->getSearchInfo()->strSaveDir;
	QString strTargDir = strSaveDir + "/" + strTmpDirName + "/";

	QString strIriPath10 = strTargDir + strPreName + "_IRI_10m.txt";
	QString strIriPath100 = strTargDir + strPreName + "_IRI_100m.txt";
	QString strIriPath1000 = strTargDir + strPreName + "_IRI_100m.txt";

	QFileInfo qfileInfo;
	if (qfileInfo.exists(strIriPath10))
	{
		// 更新列表显示;
		m_showIriWidget->setDataIRI(strIriPath10);
		return;
	}

	if (qfileInfo.exists(strIriPath100))
	{
		// 更新列表显示;
		m_showIriWidget->setDataIRI(strIriPath100);
		return;
	}

	if (qfileInfo.exists(strIriPath1000))
	{
		// 更新列表显示;
		m_showIriWidget->setDataIRI(strIriPath1000);
		return;
	}
}

void XrIRIProcess::updateProgress(float p, QString msg)
{
	if (p == 0.0f || p == 1.0f)
	{
		getProgressBar()->setVisible(false);
		getMsgLabel()->setText(msg);
		return;
	}
	if (p > 0.0f && p < 1.0f)
	{
		getProgressBar()->setVisible(true);
		getProgressBar()->setValue(p * 1000);
		getMsgLabel()->setText(msg);
	}
}

void XrIRIProcess::updateProcessFinished(QString strDaqPath, int succ)
{
	// 获取路径信息;
	QString strDaqName = strDaqPath.right(strDaqPath.length() - strDaqPath.lastIndexOf('/') - 1);
	QString strDirName = strDaqPath.left(strDaqPath.lastIndexOf('/'));
	strDirName = strDirName.right(strDirName.length() - strDirName.lastIndexOf('/') - 1);
	QString strLogName = strDirName + ".log";

	// 写入日志的路径;
	QString strSaveDir = XrIRISetting::getIRISetting()->getSearchInfo()->strSaveDir;
	QString strSaveLogPath = strSaveDir + "/" + strDirName + "/" + strLogName;

	// 迭代遍历;
	XrIRIProcessRunnable* currentThread = NULL;
	std::map<QString, XrIRIProcessRunnable*>::iterator iter = m_map_pool_threads.find(strDaqPath);
	if (iter != m_map_pool_threads.end())
	{
		// 找到后移除;
		m_map_pool_threads.erase(iter);
	}

	//QTreeWidgetItem* get_data_item = NULL;
	int ncount = m_docTreeWidget->topLevelItemCount();
	//bool existed = false;
	//for (int n = 0; n < ncount; n++)
	//{
	//	QTreeWidgetItem* parent_item = m_docTreeWidget->topLevelItem(n);
	//	if (parent_item)
	//	{
	//		QString strTmpTxt = parent_item->text(1);
	//		if (strTmpTxt.compare(strDaqPath) == 0)
	//		{
	//			// 找到该节点;
	//			get_data_item = parent_item;
	//			break;
	//		}
	//	} // if (parent_item)
	//} // for (int n = 0; n < ncount; n++)

	// 开始处理，设置按钮不可用，避免重复点击;
	m_nCurProcCount++;

	// 状态设置更改;
	//if (get_data_item)
	{
		// 若成功，则标记加黑;
		if (succ == 0)
		{
			//QFont serif_font;
			//serif_font.setBold(true);
			////get_child_item->setf
			//QColor color(0, 0, 0);
			//get_data_item->setFont(0, serif_font);
			//get_data_item->setTextColor(0, color);

			// 日志信息显示;
			char strLine[1024];
			sprintf_s(strLine, "%s %s %d/%d", strDaqPath.toLocal8Bit().data(),"处理完成", m_nCurProcCount, ncount);
			QString strMsg = QString::fromLocal8Bit(strLine);
			m_showLoggingWidget->slotAddMsg(strMsg);

			// 采用追加模式写入文件;
			g_saveLog_mutex.lock();
			FILE* ptrLogFile = NULL;
			fopen_s(&ptrLogFile, strSaveLogPath.toLocal8Bit().data(),"at+");
			if (ptrLogFile)
			{
				fprintf_s(ptrLogFile, "%s,%d\n", strDaqName.toLocal8Bit().data(), succ);
				fclose(ptrLogFile);
			}
			g_saveLog_mutex.unlock();
		}
		else
		{
			//QFont serif_font;
			//serif_font.setBold(false);
			//QColor color(255, 0, 0);
			//get_data_item->setFont(0, serif_font);
			//get_data_item->setTextColor(0, color);

			// 日志信息显示;
			char strLine[1024];
			sprintf_s(strLine, "%s %s %d/%d", strDaqPath.toLocal8Bit().data(), "处理失败", m_nCurProcCount, ncount);
			QString strMsg = QString::fromLocal8Bit(strLine);
			writeErrLog(strMsg);
			m_showLoggingWidget->slotAddMsg(strMsg);

			// 采用追加模式写入文件;
			g_saveLog_mutex.lock();
			FILE* ptrLogFile = NULL;
			fopen_s(&ptrLogFile, strSaveLogPath.toLocal8Bit().data(), "at+");
			if (ptrLogFile)
			{
				fprintf_s(ptrLogFile, "%s,%d\n", strDaqName.toLocal8Bit().data(), succ);
				fclose(ptrLogFile);
			}
			g_saveLog_mutex.unlock();
		}
	}

	// 更新记录处理总数，如果当前处理总数达到列表总数，认为处理完成，状态更新可用;
	if (m_nCurProcCount >= ncount)
	{
		m_calcIriAct->setEnabled(true);
		m_openProjectAct->setEnabled(true);
		m_aotuUpdateAct->setEnabled(true);
		m_clearListAct->setEnabled(true);
		m_setProjectParamAct->setEnabled(true);
	}

	updateProgress(1.0,QString::fromLocal8Bit("处理完成"));
}

void XrIRIProcess::updateProcessDirFinished(QString strDaqDirPath, QString strSaveDirPath, int succ)
{
	// 获取路径信息;
	QString strSecDirName = strDaqDirPath.right(strDaqDirPath.length() - strDaqDirPath.lastIndexOf('/') - 1);
	QString strFirstDirPath = strDaqDirPath.left(strDaqDirPath.lastIndexOf('/'));
	QString strLogName = strFirstDirPath + "/iriOriginal_use_record.txt";
	QString strOrigLogPath = strLogName;

	// 写入日志的路径;
	QString strSaveTmpPath = strSaveDirPath;
	strSaveTmpPath = strSaveTmpPath.left(strSaveTmpPath.lastIndexOf('/'));
	QString strSaveLogPath = strSaveTmpPath + "/iri_file.txt";

	// 迭代遍历;
	XrIRIProcessRunnable* currentThread = NULL;
	std::map<QString, XrIRIProcessRunnable*>::iterator iter = m_map_pool_threads.find(strDaqDirPath);
	if (iter != m_map_pool_threads.end())
	{
		// 找到后移除;
		m_map_pool_threads.erase(iter);
	}

	// 若成功，则标记加黑;
	if (succ == 0)
	{
		// 日志信息显示;
		char strLine[1024];
		sprintf_s(strLine, "%s %s", strDaqDirPath.toLocal8Bit().data(), "处理完成");
		QString strMsg = QString::fromLocal8Bit(strLine);
		m_showLoggingWidget->slotAddMsg(strMsg);

		// 采用追加模式写入文件;
		g_saveLog_mutex.lock();
		FILE* ptrLogFile = NULL;
		fopen_s(&ptrLogFile, strOrigLogPath.toLocal8Bit().data(), "at+");
		if (ptrLogFile)
		{
			fprintf_s(ptrLogFile, "%s\n", strSecDirName.toLocal8Bit().data());
			fclose(ptrLogFile);
		}

		fopen_s(&ptrLogFile, strSaveLogPath.toLocal8Bit().data(), "at+");
		if (ptrLogFile)
		{
			fprintf_s(ptrLogFile, "%s\n", strSecDirName.toLocal8Bit().data());
			fclose(ptrLogFile);
		}
		g_saveLog_mutex.unlock();
	}
	else
	{
		//QFont serif_font;
		//serif_font.setBold(false);
		//QColor color(255, 0, 0);
		//get_data_item->setFont(0, serif_font);
		//get_data_item->setTextColor(0, color);

		// 日志信息显示;
		char strLine[1024];
		sprintf_s(strLine, "%s %s", strDaqDirPath.toLocal8Bit().data(), "处理失败");
		QString strMsg = QString::fromLocal8Bit(strLine);
		writeErrLog(strMsg);
		m_showLoggingWidget->slotAddMsg(strMsg);

		// 采用追加模式写入文件;
		g_saveLog_mutex.lock();
		FILE* ptrLogFile = NULL;
		fopen_s(&ptrLogFile, strOrigLogPath.toLocal8Bit().data(), "at+");
		if (ptrLogFile)
		{
			fprintf_s(ptrLogFile, "%s\n", strSecDirName.toLocal8Bit().data());
			fclose(ptrLogFile);
		}

		fopen_s(&ptrLogFile, strSaveLogPath.toLocal8Bit().data(), "at+");
		if (ptrLogFile)
		{
			fprintf_s(ptrLogFile, "%s\n", strSecDirName.toLocal8Bit().data());
			fclose(ptrLogFile);
		}

		g_saveLog_mutex.unlock();
	}

	updateProgress(1.0, QString::fromLocal8Bit("处理完成"));
}

void XrIRIProcess::openProjDirSlot()
{
	// 获取当前选中节点;
	QTreeWidgetItem* item = m_docTreeWidget->currentItem();
	if (!item)
	{
		return;
	}

	// 获取节点记录的DAQ文件路径;
	QString strDaqPath = item->text(1);

	// 截取文件夹路径;
	QString strFilePath = strDaqPath.left(strDaqPath.lastIndexOf('/'));

	// 打开文件夹;
	// 打开文件路径;
	QFile file_obj(strFilePath);
	if (file_obj.exists())
	{
		// 文件存在，则打开文件路径;
		QString file_path = "file:///" + strFilePath;
		QDesktopServices::openUrl(QUrl::fromLocalFile(file_path));
	}
}

void XrIRIProcess::onCustomContextMenuRequestedSlot(const QPoint& pos)
{
	// 获取当前点击处菜单项;
	QTreeWidgetItem* item = m_docTreeWidget->currentItem();
	if (!item)
	{
		return;
	}

	// 右键菜单添加;
	m_tree_proj_menu->clear();
	m_tree_proj_menu->addAction(m_treeOpenTreeAct);
	m_tree_proj_menu->exec(QCursor::pos());
}

void XrIRIProcess::timer_handleAutoProcTimeout()
{
	// 是否存在正在处理的线程;
	int ncount = QThreadPool::globalInstance()->activeThreadCount();
	if (ncount > 0)
	{
		return;
	}

	QString strSearchDir = XrIRISetting::getIRISetting()->getSearchInfo()->strSearchDir;

	// 搜索当前两天的信息;
	char strLine[1024];
	QDateTime curDateTime = QDateTime::currentDateTime();
	QString strToday = curDateTime.toString("yyyyMMdd");

	QString strExeDir = QCoreApplication::applicationDirPath();
	QString strConfig = strExeDir + "/" + "Setting.ini";
	QSettings* setIni = new QSettings(strConfig,QSettings::IniFormat);
	int nSearchDays = setIni->value("Parm/SearchDays",30).toInt();
	delete setIni;

	// 前推一个月进行检索;
	
	//std::vector<QString> vecLogDirNames;
	std::vector<QString> vecDaqDirPaths;
	for (int n = 0;n < nSearchDays;n++)
	{
		QString strCurDayName = curDateTime.addDays(-n).toString("yyyyMMdd");

		QString strCurDayDirName = "iriOrigin_" + strCurDayName; // 新修改协议为iriOrigin_20220330
		QString strSearchDirYesd = strSearchDir + "/" + strCurDayDirName;// 前一天的文件夹路径;
		QString strLogNameYesd = "iriOriginal_file.txt";
		QString strUsedNameYesd = "iriOriginal_use_record.txt";

		// 先查询确定前一天的哪些文件夹需要处理;
		serializeLogFile(strSearchDirYesd, strLogNameYesd, strUsedNameYesd, vecDaqDirPaths);
	}

	//// 搜索前一天的数据;
	//QString strYestodayName = curDateTime.addDays(-1).toString("yyyyMMdd");
	////std::vector<QString> vecDaqPathsYesday;
	//QString strYestodayDirName = "iriOrigin_"+ strYestodayName; // 新修改协议为iriOrigin_20220330
	//QString strSearchDirYesd = strSearchDir + "/" + strYestodayDirName;// 前一天的文件夹路径;
	//QString strLogNameYesd = "iriOriginal_file.txt";
	//QString strUsedNameYesd = "iriOriginal_use_record.txt";

	//// 先查询确定前一天的哪些文件夹需要处理;
	//std::vector<QString> vecLogDirNames;
	//serializeLogFile(strSearchDirYesd, strLogNameYesd, strUsedNameYesd, vecLogDirNames);

	//autoProcSearchDaqData(strSearchDirYesd, strLogNameYesd, vecDaqPathsYesday);

	//// 搜索今天的数据;
	//QString strTodayName = "iriOrigin_" + strToday;
	////std::vector<QString> vecDaqPathsToday;
	//QString strSearchDirTod = strSearchDir + "/" + strTodayName;
	//QString strLogNameTod = "iriOriginal_file.txt";
	//QString strUsedNameTod = "iriOriginal_use_record.txt";
	//std::vector<QString> vecLogDirTodayNames;
	//serializeLogFile(strSearchDirTod, strLogNameTod, strUsedNameTod, vecLogDirTodayNames);
	////autoProcSearchDaqData(strSearchDirTod, strLogNameTod, vecDaqPathsToday);

	//// 汇总记录;
	//std::vector<QString> vecDaqDirPaths;
	//for (unsigned int n = 0;n < vecLogDirNames.size();n++)
	//{
	//	vecDaqDirPaths.push_back(vecLogDirNames[n]);
	//}
	//for (unsigned int n = 0; n < vecLogDirTodayNames.size(); n++)
	//{
	//	vecDaqDirPaths.push_back(vecLogDirTodayNames[n]);
	//}

	// 搜索目录下所有DAQ文件信息;
	int nsize = vecDaqDirPaths.size();
	if (nsize > 0)
	{
		// 给予提示是否导入;
		QString str_information;
		char cstr_temp[1024];
		memset(cstr_temp, 0, 1024);
		sprintf_s(cstr_temp, "检测到当前共 %d 组工程数据，导入中...", nsize);

		// 输出到日志信息里面;
		m_showLoggingWidget->slotAddMsg(QString::fromLocal8Bit(cstr_temp));

		//QMessageBox::StandardButton reply;
		//reply = QMessageBox::question(this, QString::fromLocal8Bit("注意"), QString::fromLocal8Bit(cstr_temp), QMessageBox::Yes | QMessageBox::No);
		//if (reply != QMessageBox::Yes)
		//{
		//	return;
		//}

		//// 添加到面板列表中;
		//updateTreeNode(vecDaqPaths);
	}
	else
	{
		return;
	}

	// 搜索完成后自动处理;
	calcuIriFromDirs(vecDaqDirPaths);
}

void XrIRIProcess::saveLayout()
{
	// 设置配置参数路径;
	QString strAppDirPath = QCoreApplication::applicationDirPath();
	QString strLayoutPath = strAppDirPath + "/Layout.ini";
	QFile file(strLayoutPath);
	if (file.open(QIODevice::WriteOnly))
	{
		QDataStream out(&file);
		out << m_DockManager->saveState();
		file.close();
	}
}

void XrIRIProcess::readLayout()
{
	QString strAppDirPath = QCoreApplication::applicationDirPath();
	QString strLayoutPath = strAppDirPath + "/Layout.ini";
	if (QFileInfo::exists(strLayoutPath) == false)
	{
		return;
	}

	// 读取数据;
	QFile file(strLayoutPath);
	if (file.open(QIODevice::ReadOnly))
	{
		QByteArray arry;
		QDataStream out(&file);
		out >> arry;
		file.close();

		m_DockManager->restoreState(arry);
	}
}

void XrIRIProcess::createView()
{
	// 创建树节点面板;
	createTreeDockPane();

	// 创建IRI结果显示面板;
	createShowIriDockPane();

	// 添加日志;
	createLoggingDockPane();

}

void XrIRIProcess::createAction()
{
	// 软件名称;
	setWindowTitle(QStringLiteral("惯导平整度后处理软件"));

	// 设置字体信息;
	hnRibbonBar* ribbon = ribbonBar();
	QFont f = ribbon->font();
	f.setFamily("Microsoft YaHei");
	f.setPixelSize(15);
	ribbon->setFont(f);

	// 工程管理;
	hnRibbonCategory* categoryProject = ribbon->addCategoryPage(QStringLiteral("工程管理"));
	createProCategory(categoryProject);

	//// 工具-设置-帮助工具栏;
	//hnRibbonCategory* categoryTool = ribbon->addCategoryPage(QStringLiteral("帮助"));
	//createToolCategory(categoryTool);
}

void XrIRIProcess::createConnect()
{
	// 打开工程;
	connect(m_openProjectAct, &QAction::triggered, this, &XrIRIProcess::openProjectSlot);

	// 工程参数设置;
	connect(m_setProjectParamAct, &QAction::triggered, this, &XrIRIProcess::setConfigSlot);

	// 计算IRI;
	connect(m_calcIriAct, &QAction::triggered, this, &XrIRIProcess::calcIriSlot);

	// 自动检索;
	connect(m_aotuUpdateAct, &QAction::triggered, this, &XrIRIProcess::autoSearchPathSlot);

	// 标定检查;
	connect(m_caliParamAct, &QAction::triggered, this, &XrIRIProcess::calibrIriSlot);

	// 车轮编码器标定;
	connect(m_caliWheelAct, &QAction::triggered, this, &XrIRIProcess::calibrWheelSlot);

	// 清空列表;
	connect(m_clearListAct, &QAction::triggered, this, &XrIRIProcess::clearListSlot);

	// 帮助-用户手册;
	connect(m_helpAct, &QAction::triggered, this, &XrIRIProcess::helpbookSlot);

	// 关于我们;
	connect(m_aboutAct, &QAction::triggered, this, &XrIRIProcess::aboutUsSlot);
}

void XrIRIProcess::createTreeConnect()
{
	// 双击treeWidget中节点;
	connect(m_docTreeWidget, &QTreeWidget::itemDoubleClicked, this, &XrIRIProcess::treeviewItemDbClick);
}

void XrIRIProcess::setupStatusBar()
{
	m_status_label = new QLabel(this);
	m_status_label->setFixedWidth(230);
	m_status_label->setText("");
	statusBar()->insertWidget(0, m_status_label);

	m_status_label2 = new QLabel(this);
	m_status_label2->setFixedWidth(230);
	m_status_label2->setText("");
	statusBar()->insertWidget(1, m_status_label2);

	m_status_progress = new QProgressBar;
	m_status_progress->setFixedSize(300, 20);
	m_status_progress->setRange(0, 1000);
	m_status_progress->setValue(0);
	m_status_progress->setVisible(false);
	statusBar()->insertWidget(2, m_status_progress);
}

void XrIRIProcess::createProCategory(hnRibbonCategory* page)
{
	// 添加面板;
	hnRibbonPannel* pannel = page->addPannel(QStringLiteral("工程管理"));

	// 打开工程文件;
	const QIcon openProjectIcon = QIcon::fromTheme("projectIcon", QIcon(QStringLiteral(":/Resources/icons/打开工程.png")));
	m_openProjectAct = new QAction(openProjectIcon, QStringLiteral("&导入工程"), this);
	pannel->addLargeAction(m_openProjectAct);

	// IRI惯导平整度计算;
	const QIcon setCalcIriIcon = QIcon::fromTheme("iriIcon", QIcon(QStringLiteral(":/Resources/icons/坐标点转换.png")));
	m_calcIriAct = new QAction(setCalcIriIcon, QStringLiteral("&平整度计算"), this);
	pannel->addLargeAction(m_calcIriAct);

	// 清理列表;
	const QIcon setClearIcon = QIcon::fromTheme("iriIcon", QIcon(QStringLiteral(":/Resources/icons/清空.png")));
	m_clearListAct = new QAction(setClearIcon, QStringLiteral("&清空列表"), this);
	pannel->addLargeAction(m_clearListAct);

	// 工程参数设置;
	const QIcon setProjectParamIcon = QIcon::fromTheme("removeIcon", QIcon(QStringLiteral(":/Resources/icons/参数设置.png")));
	m_setProjectParamAct = new QAction(setProjectParamIcon, QStringLiteral("&检索设置"), this);
	pannel->addLargeAction(m_setProjectParamAct);

	// 自动更新检索;
	const QIcon setUpdateIcon = QIcon::fromTheme("iriIcon", QIcon(QStringLiteral(":/Resources/icons/更新.png")));
	m_aotuUpdateAct = new QAction(setUpdateIcon, QStringLiteral("&自动检索"), this);
	pannel->addLargeAction(m_aotuUpdateAct);

	// 比较计算;
	const QIcon compareIcon = QIcon::fromTheme("compareIcon", QIcon(QStringLiteral(":/Resources/icons/Excel写入.png")));
	m_caliParamAct = new QAction(compareIcon, QStringLiteral("&标定检查"), this);
	pannel->addLargeAction(m_caliParamAct);

	// 车轮编码器标定;
	const QIcon wheelIcon = QIcon::fromTheme("wheelIcon", QIcon(QStringLiteral(":/Resources/icons/扣件检查.png")));
	m_caliWheelAct = new QAction(wheelIcon, QStringLiteral("&标定车轮"), this);
	pannel->addLargeAction(m_caliWheelAct);

	// 菜单项，用于管理面板显示隐藏;
	const QIcon showPaneIcon = QIcon::fromTheme("projectIcon", QIcon(QStringLiteral(":/Resources/icons/slider.png")));
	m_pShowPaneMenu = new QMenu(this);
	//m_pShowPaneMenu->setStyleSheet();
	m_pShowPaneMenu->setTitle(QString::fromLocal8Bit("显隐面板"));
	m_pShowPaneMenu->setIcon(showPaneIcon);
	m_pShowPaneMenu->menuAction()->setStatusTip(QString::fromLocal8Bit("显隐面板"));
	pannel->addLargeAction(m_pShowPaneMenu->menuAction());

	// 用户手册;
	const QIcon helpIcon = QIcon::fromTheme("projectIcon", QIcon(QStringLiteral(":/Resources/icons/用户手册.png")));
	m_helpAct = new QAction(helpIcon, QStringLiteral("&用户手册"), this);
	pannel->addLargeAction(m_helpAct);

	// 关于;
	const QIcon aboutIcon = QIcon::fromTheme("projectIcon", QIcon(QStringLiteral(":/Resources/icons/帮助.png")));
	m_aboutAct = new QAction(aboutIcon, QStringLiteral("&关于"), this);
	pannel->addLargeAction(m_aboutAct);
}


//void XrIRIProcess::createToolCategory(hnRibbonCategory* page)
//{
//	//hnRibbonPannel* sysPannel = page->addPannel(QStringLiteral("系统"));
//
//	//// 菜单项，用于管理面板显示隐藏;
//	//const QIcon showPaneIcon = QIcon::fromTheme("projectIcon", QIcon(QStringLiteral(":/Resources/icons/帮助.png")));
//	//m_pShowPaneMenu = new QMenu(this);
//	////m_pShowPaneMenu->setStyleSheet();
//	//m_pShowPaneMenu->setTitle(QString::fromLocal8Bit("显隐面板"));
//	//m_pShowPaneMenu->setIcon(showPaneIcon);
//	//m_pShowPaneMenu->menuAction()->setStatusTip(QString::fromLocal8Bit("显隐面板"));
//	//sysPannel->addLargeAction(m_pShowPaneMenu->menuAction());
//
//
//	//// 用户手册;
//	//const QIcon helpIcon = QIcon::fromTheme("projectIcon", QIcon(QStringLiteral(":/Resources/icons/用户手册.png")));
//	//m_helpAct = new QAction(helpIcon, QStringLiteral("&用户手册"), this);
//	//sysPannel->addLargeAction(m_helpAct);
//
//	//// 关于;
//	//const QIcon aboutIcon = QIcon::fromTheme("projectIcon", QIcon(QStringLiteral(":/Resources/icons/帮助.png")));
//	//m_aboutAct = new QAction(aboutIcon, QStringLiteral("&关于"), this);
//	//sysPannel->addLargeAction(m_aboutAct);
//}

void XrIRIProcess::createTreeDockPane()
{
	// 设置对象停靠属性;
	hn::CDockWidget::DockWidgetFeatures tFeatures = hn::CDockWidget::NoDockWidgetFeatures;
	tFeatures |= hn::CDockWidget::DockWidgetFloatable;
	tFeatures |= hn::CDockWidget::DockWidgetMovable;

	// 创建树状视图,设置右键菜单;
	m_docTreeWidget = new QTreeWidget(this);
	m_docTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

	// 树状图设置为两列， 第二列记录工程路径并隐藏;
	m_docTreeWidget->setColumnCount(2);
	m_docTreeWidget->setColumnHidden(1, true);

	m_docTreeWidget->setHeaderLabel(QStringLiteral("工程列表"));
	m_docTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

	// 创建Dock
	m_DoctreeViewDock = new hn::CDockWidget(QStringLiteral("工程管理视图"), this);
	m_DoctreeViewDock->setFeatures(tFeatures);
	m_DoctreeViewDock->setWidget(m_docTreeWidget);
	m_DoctreeViewDock->setHidden(false);

	// 根据设定的停靠位置，将dockpane添加到widget中进行管理;
	m_DockManager->addDockWidget(hn::LeftDockWidgetArea, m_DoctreeViewDock);
	m_pShowPaneMenu->addAction(m_DoctreeViewDock->toggleViewAction());

	// 右键菜单，添加打开文件夹;
	m_tree_proj_menu = new QMenu(this);

	// 右键菜单
	connect(m_docTreeWidget, &QTreeView::customContextMenuRequested, this, &XrIRIProcess::onCustomContextMenuRequestedSlot);

	m_treeOpenTreeAct = new QAction(QStringLiteral("打开文件夹"), this);
	m_treeOpenTreeAct->setCheckable(false);
	connect(m_treeOpenTreeAct, &QAction::triggered, this, &XrIRIProcess::openProjDirSlot);
}

void XrIRIProcess::createShowIriDockPane()
{
	// 设置对象停靠属性;
	hn::CDockWidget::DockWidgetFeatures tFeatures = hn::CDockWidget::NoDockWidgetFeatures;
	tFeatures |= hn::CDockWidget::DockWidgetFloatable;
	tFeatures |= hn::CDockWidget::DockWidgetMovable;

	// 创建IRI展示面板,由CDockManager进行管理;
	m_showIriWidget = new xrShowIriResultWidget(this);
	hn::CDockWidget* pIriShowDockPane = new hn::CDockWidget(QStringLiteral("IRI结果显示"), this);
	pIriShowDockPane->setFeatures(tFeatures);
	pIriShowDockPane->setWidget(m_showIriWidget);
	pIriShowDockPane->setHidden(false);

	// 根据设定的停靠位置，将dockpane添加到widget中进行管理;
	m_DockManager->addDockWidget(hn::TopDockWidgetArea, pIriShowDockPane);
	m_pShowPaneMenu->addAction(pIriShowDockPane->toggleViewAction());
}

void XrIRIProcess::createLoggingDockPane()
{
	// 设置对象停靠属性;
	hn::CDockWidget::DockWidgetFeatures tFeatures = hn::CDockWidget::NoDockWidgetFeatures;
	tFeatures |= hn::CDockWidget::DockWidgetFloatable;
	tFeatures |= hn::CDockWidget::DockWidgetMovable;

	// 创建几何参数展示面板,由CDockManager进行管理;
	m_showLoggingWidget = new XrIRILoggingWidget(this);
	hn::CDockWidget* pLogShowDockPane = new hn::CDockWidget(QStringLiteral("处理日志"), this);
	pLogShowDockPane->setFeatures(tFeatures);
	pLogShowDockPane->setWidget(m_showLoggingWidget);
	pLogShowDockPane->setHidden(false);

	// 根据设定的停靠位置，将dockpane添加到widget中进行管理;
	m_DockManager->addDockWidget(hn::TopDockWidgetArea, pLogShowDockPane);
	m_pShowPaneMenu->addAction(pLogShowDockPane->toggleViewAction());
}

void XrIRIProcess::autoSearchDaqData(QString strSearchDir, std::vector<QString>& vecSearchResult)
{
	// 设置dirlujing;
	QDir* dir = new QDir(strSearchDir);
	QStringList filter;
	//filter << QString("*.daq");

	// 获取列表下所有文件信息;
	QList<QFileInfo>* fileInfo = new QList<QFileInfo>(dir->entryInfoList(filter));
	for (int i = 0;i < fileInfo->count();i++)
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
					int isValid = isValidProject(path_tmp);
					if (isValid != 0)
					{
						continue;
					}

					// 应读取文件判断一下是否为有效惯导数据;
					vecSearchResult.push_back(path_tmp);
				}
			}
		}
		else if(info_tmp.isDir())// 为文件夹;
		{
			// 迭代搜索;
			autoSearchDaqData(path_tmp,vecSearchResult);
		}
	}// for (int i = 0;i < fileInfo->count();i++)

	delete fileInfo;
	delete dir;
}

void XrIRIProcess::serializeLogFile(QString strSearchDir, QString strLogName, QString strUsedName, std::vector<QString>& vecLogDirNames)
{
	// 解析已处理文件信息记录;
	std::vector<QString> vecUsedDirNames;
	QString strUsedPath = strSearchDir + "/" + strUsedName;
	FILE* ptrFileSearch = NULL;
	fopen_s(&ptrFileSearch, strUsedPath.toLocal8Bit().data(), "rt+");
	if (ptrFileSearch)
	{
		char strLine[1024];
		char strLine2[256];
		while (!feof(ptrFileSearch))
		{
			memset(strLine2, 0, 256);
			memset(strLine, 0, 1024);
			fgets(strLine, 1024, ptrFileSearch);
			int nret = sscanf_s(strLine, "%s\n", strLine2, 256);
			if (nret > 0)
			{
				QString strTmp = QString::fromLocal8Bit(strLine2);
				if (strTmp.length() > 5)
				{
					strTmp = strTmp.toLower();
					vecUsedDirNames.push_back(strTmp);
				}
			}
		}
		fclose(ptrFileSearch);
	}

	// 获取日志记录信息;
	QString strLogPath = strSearchDir + "/" + strLogName;
	FILE* ptrFileLog = NULL;
	fopen_s(&ptrFileLog, strLogPath.toLocal8Bit().data(), "rt+");
	if (ptrFileLog)
	{
		char strLine[1024];
		char strLine2[256];
		while (!feof(ptrFileLog))
		{
			memset(strLine2, 0, 256);
			memset(strLine, 0, 1024);
			fgets(strLine, 1024, ptrFileLog);
			int nret = sscanf_s(strLine, "%s\n", strLine2, 256);
			if (nret > 0)
			{
				QString strTmp = QString::fromLocal8Bit(strLine2);
				if (strTmp.length() > 5)
				{
					QString strTmpLower = strTmp.toLower();
					bool isValid = isNameExist(strTmpLower, vecUsedDirNames);
					if (!isValid)
					{
						// 不存在，则记录处理;
						QString strProcDirPath = strSearchDir + "/" + strTmp;
						vecLogDirNames.push_back(strProcDirPath);
					}
				}
			}
		}
		fclose(ptrFileLog);
	}
}

bool XrIRIProcess::calcuIriFromDirs(std::vector<QString>& vecDirs)
{
	// 传入为文件夹，以文件夹为单位开启线程进行处理;
	// 获取所有树节点中信息;
	std::vector<QString> vecNeedProcDirPaths;
	bool existed = false;
	for (int n = 0; n < vecDirs.size(); n++)
	{
		// 判断该文件路径是否已存在于列表中;
		QString strTmpTxt = vecDirs[n];
		XrIRIProcessRunnable* pRunnable = getIRICalcuWorker(strTmpTxt);
		if (pRunnable) // 如果已经存在，则不添加进去;
		{
			continue;
		}

		vecNeedProcDirPaths.push_back(strTmpTxt);
	} // for (int n = 0; n < ncount; n++)

	if (vecNeedProcDirPaths.size() <= 0)
	{
		return false;
	}

	// 开启线程进行处理;
	startProcessIRIDataFromDir(vecNeedProcDirPaths);
	return true;

	//// 开始处理，设置按钮不可用，避免重复点击;
	//m_calcIriAct->setEnabled(false);
	//m_openProjectAct->setEnabled(false);
	//m_aotuUpdateAct->setEnabled(false);
	//m_clearListAct->setEnabled(false);
	//m_setProjectParamAct->setEnabled(false);
}

void XrIRIProcess::autoProcSearchDaqData(QString strSearchDir, QString strLogName, std::vector<QString>& vecSearchResult)
{
	// 获取日志记录信息;
	std::vector<QString> vecLogList;
	QString strLogPath = strSearchDir + "/" + strLogName;
	FILE* ptrFileLog = NULL;
	fopen_s(&ptrFileLog,strLogPath.toLocal8Bit().data(),"rt+");
	if (ptrFileLog)
	{
		char strLine[1024];
		char strLine2[256];
		while (!feof(ptrFileLog))
		{
			memset(strLine2, 0, 256);
			memset(strLine, 0, 1024);
			fgets(strLine, 1024, ptrFileLog);
			int nret = sscanf_s(strLine, "%s\n", strLine2, 256);
			if (nret > 0)
			{
				QString strTmp = QString::fromLocal8Bit(strLine2);
				if (strTmp.length() > 5)
				{
					strTmp = strTmp.toLower();
					vecLogList.push_back(strTmp);
				}
			}
		}
		fclose(ptrFileLog);
	}

	// 设置dirlujing;
	QDir* dir = new QDir(strSearchDir);
	QStringList filter;
	filter << QString("*.daq");

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
					// 判断是否存在;
					QString strName = path_tmp.right(path_tmp.length() - path_tmp.lastIndexOf('/') - 1);
					bool isValid = isNameExist(strName,vecLogList);
					if (!isValid)
					{
						continue;
					}

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
							strDirName = strDirName.right(strDirName.length() - strDirName.lastIndexOf('/') - 1);
							QString strLogName = strDirName + ".log";

							// 写入日志的路径,文件夹不存在则创建;
							QString strSaveDir = XrIRISetting::getIRISetting()->getSearchInfo()->strSaveDir;
							QString strSaveDirLog = strSaveDir + "/" + strDirName;
							QString strSaveLogPath = strSaveDir + "/" + strDirName + "/" + strLogName;

							// 判断文件夹是否存在，不存在则创建;
							CreateFolder(strSaveDirLog.toLocal8Bit().data());

							// 采用全局锁,追加模式写入文件;
							g_saveLog_mutex.lock();
							FILE* ptrLogFile = NULL;
							fopen_s(&ptrLogFile, strSaveLogPath.toLocal8Bit().data(), "at+");
							if (ptrLogFile)
							{
								fprintf_s(ptrLogFile, "%s,%d\n", strDaqName.toLocal8Bit().data(), nValid);
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
			// 迭代搜索;
			autoSearchDaqData(path_tmp, vecSearchResult);
		}
	}// for (int i = 0;i < fileInfo->count();i++)

	delete fileInfo;
	delete dir;
}

bool XrIRIProcess::isNameExist(QString strName, std::vector<QString>& vecNames)
{
	QString strTarg = NULL;
	bool bFind = false;
	strName = strName.toLower();
	for (unsigned int n = 0;n < vecNames.size();n++)
	{
		strTarg = vecNames[n];
		if (strTarg.compare(strName) == 0) // 相同;
		{
			bFind = true;
			break;
		}
	}

	return bFind;
}

void XrIRIProcess::updateTreeNode(std::vector<QString>& vecPaths)
{
	// 清理所有节点;
	clearAllItem();

	// 添加节点信息;
	for (unsigned int n = 0;n < vecPaths.size();n++)
	{
		QString daqPath = vecPaths[n];

		// 获取上一级文件夹的名称;
		QString strDirName = daqPath.left(daqPath.lastIndexOf('/'));
		QString strDirPath = strDirName;
		strDirName = strDirName.right(strDirName.length() - strDirName.lastIndexOf('/') - 1);

		// 文件夹路径名称;
		QTreeWidgetItem* projectRootItem = new QTreeWidgetItem(m_docTreeWidget, QStringList(strDirName));
		projectRootItem->setText(0, strDirName);
		projectRootItem->setText(1, daqPath);

		// 获取文件名;
		QString strDaqName = daqPath.right(daqPath.length() - daqPath.lastIndexOf('/') - 1);
		//strDaqName = strDaqName.left(strDaqName.lastIndexOf('.'));

		// 文件名称;
		QTreeWidgetItem* pDaqItem = new QTreeWidgetItem(projectRootItem, QStringList(strDaqName));
		pDaqItem->setText(0, strDaqName);
		pDaqItem->setText(1, daqPath);
	}

	// 节点收起;
	m_docTreeWidget->collapseAll();
}

void XrIRIProcess::clearAllItem()
{
	if (!m_docTreeWidget)
	{
		return;
	}

	m_docTreeWidget->clear();
	if (!m_showLoggingWidget)
	{
		return;
	}
	m_showLoggingWidget->clearMsg();
}

int XrIRIProcess::isValidProject(QString strDaqPath)
{
	// 检查文件大小，是否为有效的工程;
	bool isValid = false;
	QFileInfo fileInfo(strDaqPath);
	int nFileSize = fileInfo.size();
	if (nFileSize < 1024)
	{
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
	int nSearchTarget = XrIRISetting::getIRISetting()->getSearchInfo()->nSearchModel;
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
	char strLine[1024];
	fseek(ptrDaqFile, 1000, SEEK_SET);
	fgets(strLine, 1024, ptrDaqFile); // 舍弃第一行;

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
		return 4;
	}

	return 0;
}

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

using namespace std;
bool XrIRIProcess::parseHGI300DataPart(const std::string& str, float& yAccValue)
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
	float xVelocity = str2float(strXVelocity) * HGI300_RATE_LSB * 180.0 / PI64;

	// rate y;
	string strYVelocity = str.substr(48, 4);
	float yVelocity = str2float(strYVelocity) * HGI300_RATE_LSB * 180.0 / PI64;

	//  rate z;
	string strZVelocity = str.substr(52, 4);
	float zVelocity = str2float(strZVelocity) * HGI300_RATE_LSB * 180.0 / PI64;

	// acc x;
	string strXAccelerate = str.substr(56, 4);
	float xAccelerate = str2float(strXAccelerate) * HGI300_ACC_LSB;

	// acc y;
	string strYAccelerate = str.substr(60, 4);
	float yAccelerate = str2float(strYAccelerate) * HGI300_ACC_LSB;

	// acc z;
	string strZAccelerate = str.substr(64, 4);
	float zAccelerate = str2float(strZAccelerate) * HGI300_ACC_LSB;

	yAccValue = yAccelerate;
	return true;
}

void XrIRIProcess::startProcessIRIData(std::vector<QString>& vecPaths)
{
	// 每一个DAQ数据新建一个线程，由线程池进行管理计算;
	m_nCurProcCount = 0;
	for (unsigned int n = 0;n < vecPaths.size();n++)
	{
		// 获取路径;
		QString strDaqPath = vecPaths[n];

		// 构建线程对象进行处理;
		XrIRIProcessRunnable* ptrWorker = new XrIRIProcessRunnable;
		ptrWorker->setAutoDelete(false);

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
				if ( strMtdExistPath.compare("IRIMTD") == 0 || strMtdExistPath.compare("irimtd") == 0 )
				{
					// 表明为激光平整度数据;
					isIrimtdEixst = true;
				}
			}
		}

		// daq名称获取;
		QString strPreName = strDaqPath;
		strPreName = strPreName.right(strPreName.length() - strPreName.lastIndexOf('/') - 1);
		strPreName = strPreName.left(strPreName.lastIndexOf('.'));

		// 获取原始DAQ文件夹名称;
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

				// 子目录上一级;
				strTmpDirName = strTmpDirName.left(strTmpDirName.lastIndexOf('/'));
				strTmpDirName = strTmpDirName.right(strTmpDirName.length() - strTmpDirName.lastIndexOf('/') - 1);
			}
			else
			{
				strTmpDirName = "/";
			}
		}
		else
		{
			// 惯导平整度目录;
			if (nret > 0)
			{
				strTmpDirName = strTmpDirName.right(strTmpDirName.length() - strTmpDirName.lastIndexOf('/') - 1);
			}
			else
			{
				strTmpDirName = "/";
			}
		}

		// 保存文件夹路径;
		QString strSaveDir = XrIRISetting::getIRISetting()->getSearchInfo()->strSaveDir;
		QString strTargDir = strSaveDir + "/" + strTmpDirName + "/";

		// 判断文件夹是否存在，不存在则创建;
		CreateFolder(strTargDir.toLocal8Bit().data());

		// 尝试从DAQ名称上解析编码器频率和车轮周长以及KB系数;
		double scaleK,scaleB;
		scaleK = 1.0;
		scaleB = 0.0;
		int nDmiHz = XrIRISetting::getIRISetting()->getSearchInfo()->nDmiHz;
		double dWheelSize = XrIRISetting::getIRISetting()->getSearchInfo()->dWheelSize;
		int bNeedFromName = XrIRISetting::getIRISetting()->getSearchInfo()->nDmiSerializeFromName;
		if (isIrimtdEixst)
		{
			ptrWorker->setProcType(2);
		}
		else
		{
			ptrWorker->setProcType(0);
			if (bNeedFromName)
			{
				// 需要从名称上解析;
				QString strDaqName = strPreName;
				QStringList qlist;
				qlist = strDaqName.split(QRegExp("[_*]"));
				int nsize = qlist.size();
				if (nsize >= 5)
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
			}
		}


		// 设置参数信息;
		ptrWorker->setDaqPath(strDaqPath);
		ptrWorker->setSaveDir(strTargDir);
		ptrWorker->setDmiWheel(nDmiHz,dWheelSize);
		ptrWorker->setKB(scaleK, scaleB);
		ptrWorker->setSaveIRIs(XrIRISetting::getIRISetting()->getSearchInfo()->nSaveIRI10m, 
			XrIRISetting::getIRISetting()->getSearchInfo()->nSaveIRI100m,
			XrIRISetting::getIRISetting()->getSearchInfo()->nSaveIRI1000m);

		// 绑定信号槽函数;
		connect(ptrWorker, SIGNAL(progress(float, QString)), this, SLOT(updateProgress(float, QString)), Qt::QueuedConnection);
		connect(ptrWorker, SIGNAL(process_finished(QString, int)), this, SLOT(updateProcessFinished(QString, int)), Qt::QueuedConnection);

		// 添加记录管理;
		m_map_pool_threads[strDaqPath] = ptrWorker;

		// 添加至线程池;
		QThreadPool::globalInstance()->start((QRunnable*)(ptrWorker));

		//ptrWorker->run();
	}

	int test = 0;
}

void XrIRIProcess::startProcessIRIDataFromDir(std::vector<QString>& vecDirPaths)
{
	// 每一个DAQ数据新建一个线程，由线程池进行管理计算;
	m_nCurProcCount = 0;
	for (unsigned int n = 0; n < vecDirPaths.size(); n++)
	{
		// 获取路径;
		QString strDaqDirPath = vecDirPaths[n];

		// 构建线程对象进行处理;
		XrIRIProcessRunnable* ptrWorker = new XrIRIProcessRunnable;
		ptrWorker->setAutoDelete(false);

		// 存储的文件夹路径及文件名称;
		QString strDirPath = strDaqDirPath;
		QString strCurDirName = strDirPath;
		strCurDirName = strCurDirName.right(strCurDirName.length() - strCurDirName.lastIndexOf('/') - 1); // 当前二级文件夹名称;
		QString strPreDirName = strDirPath.left(strDirPath.lastIndexOf('/'));
		strPreDirName = strPreDirName.right(strPreDirName.length() - strPreDirName.lastIndexOf('/') - 1);// 当前一级文件夹名称;
		strPreDirName = strPreDirName.right(strPreDirName.length() - strPreDirName.lastIndexOf('_') - 1); // 此处获取为协议规定的上传年月日信息;
		QString strSaveFirstDirName = "iri_" + strPreDirName;

		// 保存文件夹路径;
		QString strSaveDir = XrIRISetting::getIRISetting()->getSearchInfo()->strSaveDir;
		QString strTargDir = strSaveDir + "/" + strSaveFirstDirName + "/" + strCurDirName;

		// 判断文件夹是否存在，不存在则创建;
		CreateFolder(strTargDir.toLocal8Bit().data());

		// 设置参数信息;
		ptrWorker->setProcType(1);
		ptrWorker->setDaqDirPath(strDaqDirPath);
		ptrWorker->setSaveDir(strTargDir);
		ptrWorker->setSaveIRIs(XrIRISetting::getIRISetting()->getSearchInfo()->nSaveIRI10m,
			XrIRISetting::getIRISetting()->getSearchInfo()->nSaveIRI100m,
			XrIRISetting::getIRISetting()->getSearchInfo()->nSaveIRI1000m);

		// 绑定信号槽函数;
		connect(ptrWorker, SIGNAL(progress(float, QString)), this, SLOT(updateProgress(float, QString)), Qt::QueuedConnection);
		connect(ptrWorker, SIGNAL(process_dir_finished(QString,QString, int)), this, SLOT(updateProcessDirFinished(QString,QString, int)), Qt::QueuedConnection);

		// 添加记录管理;
		m_map_pool_threads[strDaqDirPath] = ptrWorker;

		// 添加至线程池;
		QThreadPool::globalInstance()->start((QRunnable*)(ptrWorker));
	}

	int test = 0;
}

XrIRIProcessRunnable* XrIRIProcess::getIRICalcuWorker(QString strDaqPath)
{
	// 迭代遍历;
	XrIRIProcessRunnable* currentThread = NULL;
	std::map<QString, XrIRIProcessRunnable*>::iterator iter = m_map_pool_threads.find(strDaqPath);
	if (iter != m_map_pool_threads.end())
	{
		// 找到;
		currentThread = iter->second;
	}

	return currentThread;
}

void XrIRIProcess::writeErrLog(QString strMsg)
{
	QDateTime curdateTime = QDateTime::currentDateTime();
	QString strdatetime = curdateTime.toString("yyyy-MM-dd hh:mm:ss.zzz");

	QString strAppDir = QCoreApplication::applicationDirPath();
	QString strLogDir = strAppDir + "/ERRLOG";
	QDir dir(strLogDir);
	if (!dir.exists())
	{
		if (dir.mkpath(strLogDir))
		{
			return;
		}
	}

	QString strDate = curdateTime.toString("yyyy-MM-dd");
	strDate += "_err.log";
	QString strLogPath = strLogDir + "/" + strDate;
	FILE* ptrFile = fopen(strLogPath.toLocal8Bit().data(), "at+");
	fprintf_s(ptrFile, "%s:%s\n", strdatetime.toLocal8Bit().data(), strMsg.toLocal8Bit().data());
	fclose(ptrFile);
}

void progress_callback(float p, const char* msg)
{
	XrIRIProcess* pMain = XrIRIProcess::AppMain;//dynamic_cast<hdStudioQT*>(QApplication::activeWindow());
	if (pMain == NULL)
	{
		return;
	}
	if (p == 0.0f || p == 1.0f)
	{
		pMain->getProgressBar()->setVisible(false);
		pMain->getMsgLabel()->setText(QString::fromLocal8Bit(msg));
		return;
	}
	if (p > 0.0f && p < 1.0f)
	{
		pMain->getProgressBar()->setVisible(true);
		pMain->getProgressBar()->setValue(p * 1000);
		pMain->getMsgLabel()->setText(QString::fromLocal8Bit(msg));
	}
}
