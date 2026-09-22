#include "XrIRICalibraDmiDialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <string>
#include <QDesktopServices>

XrIRICalibraDmiDialog::XrIRICalibraDmiDialog(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);

	initialDlg();
}

XrIRICalibraDmiDialog::~XrIRICalibraDmiDialog()
{
}

void XrIRICalibraDmiDialog::initialDlg()
{
	// 默认自动从文件读取，不需要用户手动输入DAQ;
	ui.checkBox_read_from_file->setChecked(true);
	ui.lineEdit_start_dmi->setEnabled(false);
	ui.lineEdit_end_dmi->setEnabled(false);
	ui.label->setEnabled(false);
	ui.label_2->setEnabled(false);
	ui.label_3->setEnabled(true);
	ui.lineEdit_daq_path->setEnabled(true);
	ui.pushButton_find_daq_path->setEnabled(true);

	// 默认需要更新计算KB值;
	ui.checkBox_need_kb->setChecked(true);
	ui.label_7->setEnabled(true);
	ui.label_8->setEnabled(true);
	ui.lineEdit_cali_k->setEnabled(true);
	ui.lineEdit_cali_b->setEnabled(true);

	// 默认不更新编码器值;
	ui.checkBox_need_update_dmi->setChecked(false);
	ui.label_9->setEnabled(false);
	ui.lineEdit_SMP_value->setEnabled(false);

	// 设置初始值;
	QString strTemp = "";
	int dmiValue = 2350;
	ui.lineEdit_orient_dmi_hz->setText(QString::number(dmiValue, 10));

	// 设置SMP值;
	int smpValue = 851;
	ui.lineEdit_SMP_value->setText(QString::number(smpValue, 10));

	// 绑定信号槽函数;
	connect(this->ui.pushButton_find_daq_path, SIGNAL(clicked()), this, SLOT(slotOnFindPath()));
	connect(this->ui.pushButton_calc, SIGNAL(clicked()), this, SLOT(slotOnclickOk()));
	connect(this->ui.pushButton_cancel, SIGNAL(clicked()), this, SLOT(slotOnclickCancel()));

	connect(ui.checkBox_read_from_file, SIGNAL(clicked()), this, SLOT(slotOnCheckAutoRead()));
	connect(ui.checkBox_need_update_dmi, SIGNAL(clicked()), this, SLOT(slotOnCheckUpdateDmi()));
	connect(ui.checkBox_need_kb, SIGNAL(clicked()), this, SLOT(slotOnCheckUpdateKb()));
}

bool XrIRICalibraDmiDialog::loadDaqDmiValue(QString strDaqPath, unsigned long& uStartDmi, unsigned long& uEndDmi)
{
	// 判断文件存在;
	uStartDmi = 9999999999;
	uEndDmi = 0;
	QFileInfo qfile(strDaqPath);
	if (false == qfile.exists())
	{
		return false;
	}

	// 读取文件;
	FILE* ptrFile = NULL; 
	fopen_s(&ptrFile,strDaqPath.toLocal8Bit().data(),"rt");
	if (NULL == ptrFile)
	{
		return false;
	}

	// 跳读1000字节;
	char strLine[1024];
	fseek(ptrFile, 1000, SEEK_SET);
	for (int n = 0;n < 1000;n++)
	{
		fgets(strLine, 1024, ptrFile); // 舍弃1000行;
	}
	

	// 逐行读取;
	int ncount = 0;
	
	while (!feof(ptrFile))
	{
		// 读取文件;
		memset(strLine, 0, 1024);
		fgets(strLine, 1024, ptrFile);

		std::string str = strLine;
		if (str.empty() || str.size() < 73)
		{
			continue;
		}

		//ncount++;
		//if (ncount < 2000)
		//{
		//	continue;
		//}

		unsigned long uTmpDmi = 0;
		bool bret = parseHGI300DataPart(str, uTmpDmi);
		if (uTmpDmi < uStartDmi)
		{
			uStartDmi = uTmpDmi;
		}
		if (uTmpDmi > uEndDmi)
		{
			uEndDmi = uTmpDmi;
		}

	}
	fclose(ptrFile);

	return true;
}

using namespace std;
bool XrIRICalibraDmiDialog::parseHGI300DataPart(const std::string& str, unsigned long& uDmiValue)
{
	string strYear = str.substr(5, 4);
	int year = atoi(strYear.data());

	//月;
	string strMonth = str.substr(9, 2);
	int mon = atoi(strMonth.data());

	//日;
	string strDay = str.substr(11, 2);
	int day = atoi(strDay.data());

	// 时;
	string strHour = str.substr(14, 2);
	int hour = atoi(strHour.data());

	// 分;
	string strMin = str.substr(16, 2);
	int min = atoi(strMin.data());

	// 秒;
	string strSec = str.substr(18, 2);
	int sec = atoi(strSec.data());

	// 毫秒;
	string strMSec = str.substr(21, 3);
	int microSec = atoi(strMSec.data());

	// 微秒;
	string strMillSec = str.substr(25, 3);
	int milliSec = atoi(strMillSec.data());

	// DIM数据读取;
	string strDimValue = str.substr(29, 10);
	unsigned long dim = strtoul(strDimValue.data(), NULL, 10);
	uDmiValue = dim;

	return true;
}

void XrIRICalibraDmiDialog::slotOnFindPath()
{
	// 选择DAQ文件路径;
	QString str_filter_pos = "Support Files(*.daq *.daq *.daq)";
	QString str_new_pos_file = QFileDialog::getOpenFileName(this, QString::fromLocal8Bit("POS文件"), "", str_filter_pos);
	if (str_new_pos_file.isEmpty())
	{
		return;
	}

	// 路径存在则写入文本框显示;
	this->ui.lineEdit_daq_path->setText(str_new_pos_file);

	unsigned long uStartDmi, uEndDmi;
	uStartDmi = uEndDmi = 0;

	// 从文件中读取;
	bool bRet = loadDaqDmiValue(str_new_pos_file,uStartDmi,uEndDmi);
	if (bRet)
	{
		// 路径存在则写入文本框显示;
		this->ui.lineEdit_start_dmi->setText(QString::number(uStartDmi, 10));
		this->ui.lineEdit_end_dmi->setText(QString::number(uEndDmi, 10));
	}
}

void XrIRICalibraDmiDialog::slotOnCheckAutoRead()
{
	// 是否解析文件;
	bool isCheckOnFile = ui.checkBox_read_from_file->isChecked();
	if (isCheckOnFile)
	{
		ui.lineEdit_start_dmi->setEnabled(false);
		ui.lineEdit_end_dmi->setEnabled(false);
		ui.label->setEnabled(false);
		ui.label_2->setEnabled(false);
		ui.label_3->setEnabled(true);
		ui.lineEdit_daq_path->setEnabled(true);
		ui.pushButton_find_daq_path->setEnabled(true);
	}
	else
	{
		ui.lineEdit_start_dmi->setEnabled(true);
		ui.lineEdit_end_dmi->setEnabled(true);
		ui.label->setEnabled(true);
		ui.label_2->setEnabled(true);
		ui.label_3->setEnabled(false);
		ui.lineEdit_daq_path->setEnabled(false);
		ui.pushButton_find_daq_path->setEnabled(false);
	}
}

void XrIRICalibraDmiDialog::slotOnCheckUpdateDmi()
{
	// 是否需要更新编码器信息,给予提示;
	bool isChecked = ui.checkBox_need_update_dmi->isChecked();
	if (isChecked)
	{
		ui.label_9->setEnabled(true);
		ui.lineEdit_SMP_value->setEnabled(true);

		QMessageBox::information(this, QString::fromLocal8Bit("注意"), 
			QString::fromLocal8Bit("用户需确定该编码器实际可编程，且应在计算完成后根据计算结果通过编程工具更改编码器频率！"), QMessageBox::Ok);
	}
	else
	{
		ui.label_9->setEnabled(false);
		ui.lineEdit_SMP_value->setEnabled(false);
	}
}

void XrIRICalibraDmiDialog::slotOnCheckUpdateKb()
{
	// 是否计算KB;
	bool isChecked = ui.checkBox_need_kb->isChecked();
	if (isChecked)
	{
		ui.label_7->setEnabled(true);
		ui.label_8->setEnabled(true);
		ui.lineEdit_cali_k->setEnabled(true);
		ui.lineEdit_cali_b->setEnabled(true);
	}
	else
	{
		ui.label_7->setEnabled(false);
		ui.label_8->setEnabled(false);
		ui.lineEdit_cali_k->setEnabled(false);
		ui.lineEdit_cali_b->setEnabled(false);
	}
}

void XrIRICalibraDmiDialog::slotOnclickOk()
{
	QString strTemp = "";
	bool isCheckOnFile = ui.checkBox_read_from_file->isChecked();
	unsigned long dStartDmi, dEndDmi;
	dStartDmi = dEndDmi = 0;
	if (!isCheckOnFile)
	{
		// 从界面获取;
		strTemp = ui.lineEdit_start_dmi->text();
		if (strTemp.isEmpty())
		{
			QMessageBox::information(this, QString::fromLocal8Bit("注意"),
				QString::fromLocal8Bit("用户需输入起始位置的编码器记录值！"), QMessageBox::Ok);
			return;
		}
		else
		{
			dStartDmi = strTemp.toULong();
		}

		// 结束位置的编码器值;
		strTemp = ui.lineEdit_end_dmi->text();
		if (strTemp.isEmpty())
		{
			QMessageBox::information(this, QString::fromLocal8Bit("注意"),
				QString::fromLocal8Bit("用户需输入结束位置的编码器记录值！"), QMessageBox::Ok);
			return;
		}
		else
		{
			dEndDmi = strTemp.toULong();
		}
	}
	else
	{
		// 从界面获取;
		strTemp = ui.lineEdit_start_dmi->text();
		if (strTemp.isEmpty())
		{
			QMessageBox::information(this, QString::fromLocal8Bit("注意"),
				QString::fromLocal8Bit("用户需导入有效的惯导里程计数据！"), QMessageBox::Ok);
			return;
		}
		else
		{
			dStartDmi = strTemp.toULong();
		}

		// 结束位置的编码器值;
		strTemp = ui.lineEdit_end_dmi->text();
		if (strTemp.isEmpty())
		{
			QMessageBox::information(this, QString::fromLocal8Bit("注意"),
				QString::fromLocal8Bit("用户需导入有效的惯导里程计数据！"), QMessageBox::Ok);
			return;
		}
		else
		{
			dEndDmi = strTemp.toULong();
		}
	}

	// 获取实际量测值;
	double dMeasDist = 0.0;
	strTemp = ui.lineEdit_true_mile->text();
	if (strTemp.isEmpty())
	{
		QMessageBox::information(this, QString::fromLocal8Bit("注意"),
			QString::fromLocal8Bit("用户需输入有效的实际量测距离值，单位为米！"), QMessageBox::Ok);
		return;
	}
	else
	{
		dMeasDist = strTemp.toDouble();
	}

	// 获取原始的编码器值;
	int nOrientDmiHz = 0;
	strTemp = ui.lineEdit_orient_dmi_hz->text();
	if (strTemp.isEmpty())
	{
		QMessageBox::information(this, QString::fromLocal8Bit("注意"),
			QString::fromLocal8Bit("用户需输入有效的初始编码器值，一般为2350或者2000！"), QMessageBox::Ok);
		return;
	}
	else
	{
		nOrientDmiHz = strTemp.toInt();
	}

	// 获取原始的SMP值;
	int nSmpValue = 0;
	strTemp = ui.lineEdit_SMP_value->text();
	if (strTemp.isEmpty())
	{
		QMessageBox::information(this, QString::fromLocal8Bit("注意"),
			QString::fromLocal8Bit("用户需输入有效的初始SMP值，一般为851或者1000！"), QMessageBox::Ok);
		return;
	}
	else
	{
		nSmpValue = strTemp.toInt();
	}

	// 是否需要更新计算编码器频率;
	bool bNeedUpdateDMI = ui.checkBox_need_update_dmi->isChecked();

	// 计算车轮周长;
	double dNewWheel = dMeasDist * nOrientDmiHz / (dEndDmi - dStartDmi);
	int nParaWheel = dNewWheel * 1000;

	// 如果需要更新计算编码器频率;
	int nDmiNewValue = nOrientDmiHz;
	if (bNeedUpdateDMI)
	{
		nDmiNewValue = dNewWheel * nSmpValue;
	}
	
	// 是否需要更新KB系数;
	int nUpdateK, nUpdateB;
	nUpdateK = 1000; 
	nUpdateB = 0;
	bool bNeedUpdateKb = ui.checkBox_need_kb->isChecked();
	if (bNeedUpdateKb)
	{
		double dk, dB;
		dk = dB = 0.0;

		// 获取K值;
		strTemp = ui.lineEdit_cali_k->text();
		if (strTemp.isEmpty())
		{
			QMessageBox::information(this, QString::fromLocal8Bit("注意"),
				QString::fromLocal8Bit("用户需输入有效的K值！"), QMessageBox::Ok);
			return;
		}
		else
		{
			dk = strTemp.toDouble();
		}

		// 获取B值;
		strTemp = ui.lineEdit_cali_b->text();
		if (strTemp.isEmpty())
		{
			QMessageBox::information(this, QString::fromLocal8Bit("注意"),
				QString::fromLocal8Bit("用户需输入有效的B值！"), QMessageBox::Ok);
			return;
		}
		else
		{
			dB = strTemp.toDouble();
		}

		// 转换整形;
		nUpdateK = floor(dk * 1000 + 0.5);
		if (dB > 0.0)
		{
			nUpdateB = floor(1000 + dB * 100 + 0.5);
		}
		else
		{
			nUpdateB = floor( abs(dB) * 100 + 0.5);
		}
	}

	// 保存文件;
	// 选择保存原始数据文件路径;
	QString str_filter_txt = "Support Files(*.txt);;TXT Files(*.txt)";
	QString str_new_save_result_file = QFileDialog::getSaveFileName(this, QString::fromLocal8Bit("保存计算结果文件"), "", str_filter_txt);
	if (str_new_save_result_file.isEmpty())
	{
		return;
	}

	// 结果值记录;
	FILE* ptr_save = NULL;
	fopen_s(&ptr_save,str_new_save_result_file.toLocal8Bit().data(), "wt+");
	fprintf_s(ptr_save, "CLZC = %04d\n", nParaWheel);
	fprintf_s(ptr_save, "BMQ = %04d\n", nDmiNewValue);
	fprintf_s(ptr_save, "K = %04d\n", nUpdateK);
	fprintf_s(ptr_save, "B = %04d\n", nUpdateB);
	fclose(ptr_save);

	if (bNeedUpdateDMI)
	{
		QMessageBox::information(this, QString::fromLocal8Bit("提示"),
			QString::fromLocal8Bit("车轮标定完成，编码器频率存在更新，请先更新编码器频率，再将该文件中参数更新到服务端配置参数文件中！"), QMessageBox::Ok);
	}
	else
	{
		QMessageBox::information(this, QString::fromLocal8Bit("提示"),
			QString::fromLocal8Bit("车轮标定完成，请将该文件中参数更新到服务端配置参数文件中！"), QMessageBox::Ok);
	}

	//	启动外部程序;
	QDesktopServices::openUrl(QUrl::fromLocalFile(str_new_save_result_file));
}

void XrIRICalibraDmiDialog::slotOnclickCancel()
{
	// 关闭对话框;
	QDialog::reject();
}
