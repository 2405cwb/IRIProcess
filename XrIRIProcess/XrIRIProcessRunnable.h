/*! @file
********************************************************************************
<PRE>
模块名       :XrIRIProcess
文件名       : XrIRIProcessRunnable.h
相关文件     : XrIRIProcessRunnable.cpp,QRunnable
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
#ifndef __HN_IRI_PROCESS_RUNNABLE_H_INCLUDED__
#define __HN_IRI_PROCESS_RUNNABLE_H_INCLUDED__

#include <QObject>
#include <QRunnable>
#include <QMutex>

extern QMutex g_saveLog_mutex;
class XrIRIProcessRunnable : public QObject, public QRunnable
{
	Q_OBJECT

public:
	XrIRIProcessRunnable(QObject *parent = NULL);
	~XrIRIProcessRunnable();

	// 设置路径;
	void setDaqPath(QString strDaqPath);

	// 设置处理类型;
	void setProcType(int type);

	// 设置原始数据二级文件夹路径;
	void setDaqDirPath(QString strDirPath);

	// 设置编码器频率和车轮周长;
	void setDmiWheel(int dmiHz,double dWheelSize);

	// 设置KB系数;
	void setKB(double dK,double dB);

	// 设置是否保存10mIRI-100m-1000m值;
	void setSaveIRIs(int save10, int save100, int save1000);

	// 设置保存文件夹路径;
	void setSaveDir(QString strSaveDir);

	//// 设置进度条回调函数;
	//void setLoadCallback(float p, const char* msg);
public:
	virtual void run();

signals:
	// 信号标记，主要用于进度处理;
	void progress(float p, QString msg);

	// 信号标记，标记处理完成;
	void process_finished(QString strDaqPath, int succ);

	// 信号标记，标记文件夹处理完成;
	void process_dir_finished(QString strDirPath, QString strSaveDirPath, int succ);

public slots:
	void getProgress(float p, QString msg);

private:
	void processData(QString strDaqPath);
	
	// 处理激光平整度数据;
	void processIriMtdData(QString strDaqPath);

	// 处理文件夹下所有数据,传入为二级文件夹路径;
	void processAllDataDir(QString strDirPath);

	// 获取文件夹下所有DAQ文件;
	void getDaqFiles(QString strDirPath, std::vector<QString>& vecSearchResult);

	// 判断DAQ工程是否有效;
	int isValidProject(QString strDaqPath);

	// 解析I300原始惯导数据;
	bool parseHGI300DataPart(const std::string& str, float& yAccValue);

	// 写入日志;
	void writeLog(QString strMsg);
private:
	// 处理类型,默认为0，表示处理单个DAQ数据,为1表示为定时处理，为文件夹下所有DAQ数据;
	int m_nProcType;

	// 记录DAQ路径;
	QString m_strDaqPath;

	// 保存文件夹路径;
	QString m_strSaveDir;

	// 判断错误信息记录输出;
	QString m_strMsgLog;

	// 记录编码器频率和车轮周长;
	int m_nDmiHz;
	double m_dWheelSize;

	// 记录传入KB系数值;
	double m_dK;
	double m_dB;

	int m_bSaveIRI10;
	int m_bSaveIRI100;
	int m_bSaveIRI1000;

	// 记录原始数据文件夹路径;
	QString m_strOrientDaqDir;
};

#endif