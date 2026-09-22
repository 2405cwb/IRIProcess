#pragma once

#include <QDialog>
#include "ui_HnConnectUsDlg.h"

class HnConnectUsDlg : public QDialog
{
	Q_OBJECT

public:
	HnConnectUsDlg(QWidget *parent = Q_NULLPTR);
	~HnConnectUsDlg();

private:
	Ui::HnConnectUsDlg ui;
private slots:
	// È·¶¨°´Å¥;
	void btnClick_ok();
};
