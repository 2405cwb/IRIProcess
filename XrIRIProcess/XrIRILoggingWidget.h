/*! @file
********************************************************************************
<PRE>
模块名       :XrIRIProcess
文件名       : XrIRILoggingWidget.h
相关文件     : XrIRILoggingWidget.cpp,QWidget
文件实现功能 : 用于显示惯导平整度结果值的面板，停靠面板采用 hn::CDockWidget，
统一由hn::CDockManager进行管理;
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
#ifndef __HN_IRI_PROCESS_LOGGING_H_INCLUDED__
#define __HN_IRI_PROCESS_LOGGING_H_INCLUDED__

#include <QWidget>
#include "ui_XrIRILoggingWidget.h"

class XrIRILoggingWidget : public QWidget
{
	Q_OBJECT

public:
	XrIRILoggingWidget(QWidget *parent = Q_NULLPTR);
	~XrIRILoggingWidget();

	void clearMsg();
public slots:
	// 日志信息更新显示;
	void slotAddMsg(QString msg);

private:
	Ui::XrIRILoggingWidget ui;
};

#endif