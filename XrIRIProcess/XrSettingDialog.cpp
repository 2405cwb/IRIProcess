#include "XrSettingDialog.h"
#include "XrIRISetting.h"
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>

using namespace XrApp;
XrSettingDialog::XrSettingDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	initialDlg();
}

XrSettingDialog::~XrSettingDialog()
{
}

void XrSettingDialog::slotOnFindDir()
{
	// 打开文件夹路径;
	QString proj_dir = QFileDialog::getExistingDirectory(this, QString::fromLocal8Bit("打开搜索目录"));
	if (proj_dir.isEmpty())
	{
		return;
	}

	// 路径存在则写入文本框显示;
	this->ui.lineEdit_searchpath->setText(proj_dir);
}

void XrSettingDialog::slotOnFindDir2()
{
	// 打开文件夹路径;
	QString proj_dir = QFileDialog::getExistingDirectory(this, QString::fromLocal8Bit("打开保存目录"));
	if (proj_dir.isEmpty())
	{
		return;
	}

	// 路径存在则写入文本框显示;
	this->ui.lineEdit_save_path->setText(proj_dir);
}

void XrSettingDialog::slotOnclickOk()
{
	// 获取设置信息;
	IRI_SEARCH_SETTING_INFO* pSearchInfo = XrIRISetting::getIRISetting()->getSearchInfo();
	if (!pSearchInfo)
	{
		return;
	}

	// 搜索类型判断;
	bool is_do = ui.radioButton_search_do->isChecked();
	bool is_undo = ui.radioButton_search_undo->isChecked();
	bool is_full = ui.radioButton_search_full->isChecked();
	if (is_do)
	{
		pSearchInfo->nSearchModel = 0;
	}
	if (is_undo)
	{
		pSearchInfo->nSearchModel = 1;
	}
	if (is_full)
	{
		pSearchInfo->nSearchModel = 2;
	}

	// 搜索路径更新;
	QString strSearchDir = this->ui.lineEdit_searchpath->text();
	QDir dir(strSearchDir);
	if (dir.exists())
	{
		// 更新至内存记录;
		pSearchInfo->strSearchDir = strSearchDir;
	}

	// 编码器频率和车轮周长更新;
	QString strTmp = ui.lineEdit_dmi_hz->text();
	if (!strTmp.isEmpty())
	{
		int ntmp = strTmp.toInt();
		if (ntmp > 0)
		{
			pSearchInfo->nDmiHz = ntmp;
		}
	}

	// 车轮周长更新;
	strTmp = ui.lineEdit_wheel_size->text();
	if (!strTmp.isEmpty())
	{
		double dtmp = strTmp.toDouble();
		if (dtmp > 0.0)
		{
			pSearchInfo->dWheelSize = dtmp;
		}
	}

	// 保存路径更新;
	QString strSaveDir = this->ui.lineEdit_save_path->text();
	QDir dir0(strSaveDir);
	if (dir0.exists())
	{
		// 更新至内存记录;
		pSearchInfo->strSaveDir = strSaveDir;
	}

	bool isCheckOnName = ui.checkBox_serialize_from_name->isChecked();
	if (isCheckOnName) // 是否需要从名称上获取信息;
	{
		pSearchInfo->nDmiSerializeFromName = 1;
	}
	else
	{
		pSearchInfo->nDmiSerializeFromName = 0;
	}


	// 标记保存IRI10m;
	bool is_10 = ui.checkBox_export_10m->isChecked();
	bool is_100 = ui.checkBox_export_100m->isChecked();
	bool is_1000 = ui.checkBox_export_1000m->isChecked();
	pSearchInfo->nSaveIRI10m = is_10 ? 1:0;
	pSearchInfo->nSaveIRI100m = is_100 ? 1 : 0;
	pSearchInfo->nSaveIRI1000m = is_1000 ? 1 : 0;

	// 自动处理;
	bool isCheckAutoSave = ui.checkBox_auto_proc->isChecked();
	if (isCheckAutoSave)
	{
		// 之前为不自动处理,给予提示;
		if (pSearchInfo->nUseAutoSearchProc == 0)
		{
			QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("配置已更改，请重启软件！"), QMessageBox::Ok);
			pSearchInfo->nUseAutoSearchProc = 1;
		}
	}
	else
	{
		if (pSearchInfo->nUseAutoSearchProc == 1)
		{
			QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("配置已更改，请重启软件！"), QMessageBox::Ok);
			pSearchInfo->nUseAutoSearchProc = 0;
		}
	}

	// 保存xml文件;
	XrIRISetting::getIRISetting()->saveData();
	accept();
}

void XrSettingDialog::slotOnclickCancel()
{
	// 取消;
	reject();
}

void XrSettingDialog::slotOnRadioCheckDo()
{
	ui.radioButton_search_undo->setChecked(false);
	ui.radioButton_search_full->setChecked(false);
}

void XrSettingDialog::slotOnRadioCheckUndo()
{
	ui.radioButton_search_do->setChecked(false);
	ui.radioButton_search_full->setChecked(false);
}

void XrSettingDialog::slotOnRadioCheckFull()
{
	ui.radioButton_search_do->setChecked(false);
	ui.radioButton_search_undo->setChecked(false);
}

void XrSettingDialog::slotOnCheckSaveIRI10m()
{

}

void XrSettingDialog::slotOnCheckSaveIRI100m()
{

}

void XrSettingDialog::slotOnCheckSaveIRI1000m()
{

}

void XrSettingDialog::slotOnCheckFromName()
{
	// 是否解析名称;
	bool isCheckOnName = ui.checkBox_serialize_from_name->isChecked();
	if (isCheckOnName)
	{
		ui.lineEdit_dmi_hz->setEnabled(false);
		ui.lineEdit_wheel_size->setEnabled(false);
		ui.label_2->setEnabled(false);
		ui.label_3->setEnabled(false);
	}
	else
	{
		ui.lineEdit_dmi_hz->setEnabled(true);
		ui.lineEdit_wheel_size->setEnabled(true);
		ui.label_2->setEnabled(true);
		ui.label_3->setEnabled(true);
	}
}

void XrSettingDialog::slotOnCheckAutoProc()
{

}

void XrSettingDialog::initialDlg()
{
	// 获取设置信息;
	IRI_SEARCH_SETTING_INFO* pSearchInfo = XrIRISetting::getIRISetting()->getSearchInfo();
	if (!pSearchInfo)
	{
		return;
	}

	switch (pSearchInfo->nSearchModel)
	{
	case 0:
		ui.radioButton_search_do->setChecked(true);
		ui.radioButton_search_undo->setChecked(false);
		ui.radioButton_search_full->setChecked(false);
		break;
	case 1:
		ui.radioButton_search_do->setChecked(false);
		ui.radioButton_search_undo->setChecked(true);
		ui.radioButton_search_full->setChecked(false);
		break;
	case 2:
		ui.radioButton_search_do->setChecked(true);
		ui.radioButton_search_undo->setChecked(false);
		ui.radioButton_search_full->setChecked(true);
		break;
	}

	// 设置搜索路径;
	ui.lineEdit_searchpath->setText(pSearchInfo->strSearchDir);
	ui.lineEdit_searchpath->setReadOnly(true);
	ui.lineEdit_save_path->setText(pSearchInfo->strSaveDir);
	ui.lineEdit_save_path->setReadOnly(true);

	// 是否需要从名称上解析获取编码器频率和车轮周长;
	if (pSearchInfo->nDmiSerializeFromName) // 需要从名称上解析获取编码器频率和车轮周长;
	{
		ui.checkBox_serialize_from_name->setChecked(true);
		ui.lineEdit_dmi_hz->setEnabled(false);
		ui.lineEdit_wheel_size->setEnabled(false);
		ui.label_2->setEnabled(false);
		ui.label_3->setEnabled(false);
	}
	else
	{
		ui.checkBox_serialize_from_name->setChecked(false);
		ui.lineEdit_dmi_hz->setEnabled(true);
		ui.lineEdit_wheel_size->setEnabled(true);
		ui.label_2->setEnabled(true);
		ui.label_3->setEnabled(true);
	}

	// 设置编码器频率和车轮周长;
	ui.lineEdit_dmi_hz->setText(QString::number(pSearchInfo->nDmiHz, 10));
	ui.lineEdit_wheel_size->setText(QString::number(pSearchInfo->dWheelSize, 10,3));

	// 设置check状态;
	if (pSearchInfo->nSaveIRI10m)
	{
		ui.checkBox_export_10m->setChecked(true);
	}
	else
	{
		ui.checkBox_export_10m->setChecked(false);
	}

	if (pSearchInfo->nSaveIRI100m)
	{
		ui.checkBox_export_100m->setChecked(true);
	}
	else
	{
		ui.checkBox_export_100m->setChecked(false);
	}

	if (pSearchInfo->nSaveIRI1000m)
	{
		ui.checkBox_export_1000m->setChecked(true);
	}
	else
	{
		ui.checkBox_export_1000m->setChecked(false);
	}

	// 自动保存;
	if (pSearchInfo->nUseAutoSearchProc)
	{
		ui.checkBox_auto_proc->setChecked(true);
	}
	else
	{
		ui.checkBox_auto_proc->setChecked(false);
	}

	connect(this->ui.pushButton_find_path, SIGNAL(clicked()), this, SLOT(slotOnFindDir()));
	connect(this->ui.pushButton_ok, SIGNAL(clicked()), this, SLOT(slotOnclickOk()));
	connect(this->ui.pushButton_cancel, SIGNAL(clicked()), this, SLOT(slotOnclickCancel()));

	connect(ui.radioButton_search_do, SIGNAL(clicked()), this, SLOT(slotOnRadioCheckDo()));
	connect(ui.radioButton_search_undo, SIGNAL(clicked()), this, SLOT(slotOnRadioCheckUndo()));
	connect(ui.radioButton_search_full, SIGNAL(clicked()), this, SLOT(slotOnRadioCheckFull()));

	connect(ui.pushButton_find_path2, SIGNAL(clicked()), this, SLOT(slotOnFindDir2()));

	connect(ui.checkBox_export_10m, SIGNAL(clicked()), this, SLOT(slotOnCheckSaveIRI10m()));
	connect(ui.checkBox_export_100m, SIGNAL(clicked()), this, SLOT(slotOnCheckSaveIRI100m()));
	connect(ui.checkBox_export_1000m, SIGNAL(clicked()), this, SLOT(slotOnCheckSaveIRI1000m()));
	connect(ui.checkBox_serialize_from_name, SIGNAL(clicked()), this, SLOT(slotOnCheckFromName()));
	connect(ui.checkBox_auto_proc, SIGNAL(clicked()), this, SLOT(slotOnCheckAutoProc()));
}
