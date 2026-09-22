#pragma once

#include <QDialog>
#include "ui_XrSettingDialog.h"

class XrSettingDialog : public QDialog
{
	Q_OBJECT

public:
	XrSettingDialog(QWidget *parent = Q_NULLPTR);
	~XrSettingDialog();

public slots:
	// 打开文件夹路径;
	void slotOnFindDir();

	// 设置保存文件夹路径;
	void slotOnFindDir2();

	// 点击确定;
	void slotOnclickOk();

	// 点击取消;
	void slotOnclickCancel();

	// 选择检索DO;
	void slotOnRadioCheckDo();

	// 选择检索undo;
	void slotOnRadioCheckUndo();

	// 选择检索full;
	void slotOnRadioCheckFull();

	// 设置保存10m;
	void slotOnCheckSaveIRI10m();

	// 设置保存100m;
	void slotOnCheckSaveIRI100m();

	// 设置保存1000m;
	void slotOnCheckSaveIRI1000m();

	// 设置是否需要从名称上读取编码器频率和车轮周长;
	void slotOnCheckFromName();

	// 设置自动处理模式;
	void slotOnCheckAutoProc();

private:
	void initialDlg();

private:
	Ui::XrSettingDialog ui;
};
