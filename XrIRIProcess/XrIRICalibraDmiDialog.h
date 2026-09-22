#pragma once

#include <QDialog>
#include "ui_XrIRICalibraDmiDialog.h"

class XrIRICalibraDmiDialog : public QDialog
{
	Q_OBJECT

public:
	XrIRICalibraDmiDialog(QWidget *parent = Q_NULLPTR);
	~XrIRICalibraDmiDialog();

private:
	// 初始化;
	void initialDlg();

	// 加载读取DAQ文件;
	bool loadDaqDmiValue(QString strDaqPath,unsigned long& uStartDmi,unsigned long& uEndDmi);

	// 解析I300原始惯导数据;
	bool parseHGI300DataPart(const std::string& str, unsigned long& uDmiValue);

public slots:
	// 打开DAQ路径;
	void slotOnFindPath();

	// checkbox自动读取起始、终止编码器值;
	void slotOnCheckAutoRead();

	// checkbox需要更新编码器值;
	void slotOnCheckUpdateDmi();

	// checkbox需要更新KB系数;
	void slotOnCheckUpdateKb();

	// 点击确定;
	void slotOnclickOk();

	// 点击取消;
	void slotOnclickCancel();

private:
	Ui::XrIRICalibraDmiDialog ui;
};
