/*! @file
********************************************************************************
<PRE>
模块名       :XrIRIProcess
文件名       : xrShowIriResultWidget.h
相关文件     : xrShowIriResultWidget.cpp,QWidget
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
#ifndef __HN_IRI_SHOW_WIDGET_H_INCLUDED__
#define __HN_IRI_SHOW_WIDGET_H_INCLUDED__

#include <QWidget>
#include "ui_xrShowIriResultWidget.h"

class xrShowIriResultWidget : public QWidget
{
	Q_OBJECT

public:
	xrShowIriResultWidget(QWidget *parent = Q_NULLPTR);
	~xrShowIriResultWidget();

	// 设置更新显示列表信息;
	void setDataIRI(QString strIriPath100);
private:
	void initialTableView();
private:
	Ui::xrShowIriResultWidget ui;
};

#endif