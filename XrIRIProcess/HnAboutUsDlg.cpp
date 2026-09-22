#include "HnAboutUsDlg.h"
#include "HnConnectUsDlg.h"

HnAboutUsDlg::HnAboutUsDlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	Qt::WindowFlags flags = Qt::Dialog | Qt::WindowCloseButtonHint;
	this->setWindowFlags(flags);

	connect(ui.connectPushButton, SIGNAL(clicked()), this, SLOT(connectButtonClick()));
	connect(ui.okButton, SIGNAL(clicked()), this, SLOT(btnClick_ok()));
}

HnAboutUsDlg::~HnAboutUsDlg()
{
}

void HnAboutUsDlg::connectButtonClick()
{
	// 新建联系我们对话框并显示;
	HnConnectUsDlg connect_dlg;
	connect_dlg.exec();
}

void HnAboutUsDlg::btnClick_ok()
{
	// 关闭对话框
	accept();
}
