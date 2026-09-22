#include "HnConnectUsDlg.h"

HnConnectUsDlg::HnConnectUsDlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	Qt::WindowFlags flags = Qt::Dialog | Qt::WindowCloseButtonHint;
	this->setWindowFlags(flags);

	connect(ui.pushButton, SIGNAL(clicked()), this, SLOT(btnClick_ok()));
}

HnConnectUsDlg::~HnConnectUsDlg()
{
}

void HnConnectUsDlg::btnClick_ok()
{
	// ¹Ø±Õ¶Ô»°¿ò;
	accept();
}
