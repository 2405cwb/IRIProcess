#pragma once
#include <QDialog>
#include <QWidget>
#include "ui_HnAboutUsDlg.h"

class HnAboutUsDlg : public QDialog
{
	Q_OBJECT

public:
	HnAboutUsDlg(QWidget *parent = Q_NULLPTR);
	~HnAboutUsDlg();

private:
	Ui::HnAboutUsDlg ui;
private slots:
	// 联系我们;
	void connectButtonClick();

	// 确定按钮;
	void btnClick_ok();
};
