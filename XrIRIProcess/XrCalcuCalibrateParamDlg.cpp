#include "XrCalcuCalibrateParamDlg.h"
#include <QStandardItemModel>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessagebox>
#include <QDesktopServices>
#include <QUrl>

XrCalcuCalibrateParamDlg::XrCalcuCalibrateParamDlg(QWidget *parent)
	: QDialog(parent)
{
	ui.setupUi(this);
	initial();
}

XrCalcuCalibrateParamDlg::~XrCalcuCalibrateParamDlg()
{
}

bool sortByName(QString p1, QString p2)
{
	QString strName1 = p1.right(p1.length() - p1.lastIndexOf('/') - 1);
	QString strName2 = p2.right(p2.length() - p2.lastIndexOf('/') - 1);
	strName1 = strName1.toLower();
	strName2 = strName2.toLower();

	if (strName1.compare(strName2) < 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void XrCalcuCalibrateParamDlg::slotOnFindDir()
{
	// 选择文件夹路径;
	QString proj_dir = QFileDialog::getExistingDirectory(this, QString::fromLocal8Bit("打开激光平整度目录"));
	if (proj_dir.isEmpty())
	{
		return;
	}

	// 遍历检索;
	std::vector<QString> vecLaserList;
	autoSearchIriData(proj_dir, vecLaserList);

	// 判断条件;
	if (vecLaserList.size() <= 0)
	{
		QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("文件夹下不存在有效的IRI_10m文件，请检查！"), QMessageBox::Ok);
		return;
	}

	ui.lineEdit_laser_iri_dir->setText(proj_dir);

	// 将vecLaserList从小到大排序;
	std::sort(vecLaserList.begin(), vecLaserList.end(), sortByName);

	// 添加到左侧列表;
	addDataToTable(vecLaserList, true);
}

void XrCalcuCalibrateParamDlg::slotOnFindDir2()
{
	// 选择文件夹路径;
	QString proj_dir = QFileDialog::getExistingDirectory(this, QString::fromLocal8Bit("打开惯导平整度目录"));
	if (proj_dir.isEmpty())
	{
		return;
	}

	// 遍历检索;
	std::vector<QString> vecLaserList;
	autoSearchIriData(proj_dir, vecLaserList);

	// 判断条件;
	if (vecLaserList.size() <= 0)
	{
		QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("文件夹下不存在有效的IRI_10m文件，请检查！"), QMessageBox::Ok);
		return;
	}

	ui.lineEdit_imu_iri_dir->setText(proj_dir);

	// 将vecLaserList从小到大排序;
	std::sort(vecLaserList.begin(), vecLaserList.end(), sortByName);

	// 添加到左侧列表;
	addDataToTable(vecLaserList, false);
}

void XrCalcuCalibrateParamDlg::slotOnclickOk()
{
	// 获取激光列表中所有信息;
	std::vector<QString> vecLasers;
	std::vector<QString> vecImus;

	// 从列表中获取路径;
	getDataFromTable(vecLasers, 1);
	getDataFromTable(vecImus, 0);
	if (vecLasers.size() <= 0 || vecImus.size() <= 0)
	{
		//返回;
		QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit("不存在有效的IRI_10m文件，请检查！"), QMessageBox::Ok);
		return;
	}

	////// map列表关联;
	////std::map<QString, std::vector<double>> mapLaserIRI;
	////std::map<QString, std::vector<double>> mapImuIRI;

	std::vector<std::vector<double>> vecTotals;

	// 读取数据;
	int nLaserCount = vecLasers.size();
	int nImuCount = vecImus.size();
	int nMaxLen = 100000000;
	char strLine[1024];
	int iIndex = 0;
	double dIri = 0.0;
	for (unsigned int n = 0;n < vecLasers.size();n++)
	{
		QString strPath = vecLasers[n];
		FILE* ptrFile = NULL;
		fopen_s(&ptrFile, strPath.toLocal8Bit().data(), "rt");
		if (!ptrFile)
		{
			continue;
		}

		// 读取数据;
		std::vector<double> vecData;
		while (!feof(ptrFile))
		{
			memset(strLine,0,1024);
			fgets(strLine, 1024, ptrFile);

			// 解析数据;
			int nret = sscanf_s(strLine, "%d	%lf",&iIndex,&dIri);
			if (nret >= 2)
			{
				vecData.push_back(dIri);
			}
		}// while (!feof(ptrFile))
		fclose(ptrFile);

		//mapLaserIRI[strPath] = vecData;
		if (vecData.size() < nMaxLen)
		{
			nMaxLen = vecData.size();
		}

		vecTotals.push_back(vecData);
	}

	// 读取IMU计算的IRI值;
	for (unsigned int n = 0; n < vecImus.size(); n++)
	{
		QString strPath = vecImus[n];
		FILE* ptrFile = NULL;
		fopen_s(&ptrFile, strPath.toLocal8Bit().data(), "rt");
		if (!ptrFile)
		{
			continue;
		}

		// 读取数据;
		std::vector<double> vecData;
		while (!feof(ptrFile))
		{
			memset(strLine, 0, 1024);
			fgets(strLine, 1024, ptrFile);

			// 解析数据;
			int nret = sscanf_s(strLine, "%d	%lf", &iIndex, &dIri);
			if (nret >= 2)
			{
				vecData.push_back(dIri);
			}
		}// while (!feof(ptrFile))
		fclose(ptrFile);

		//mapImuIRI[strPath] = vecData;
		if (vecData.size() < nMaxLen)
		{
			nMaxLen = vecData.size();
		}

		vecTotals.push_back(vecData);
	}

	// 保存文件记录;
	QString strSavePath = ui.lineEdit_imu_iri_dir->text();
	QString strSaveDir = strSavePath;
	strSavePath += "/result.csv";
	FILE* ptrFile = NULL;
	fopen_s(&ptrFile, strSavePath.toLocal8Bit().data(), "wt");
	if (!ptrFile)
	{
		return;
	}

	std::vector<double> vecLaserAverage;
	std::vector<double> vecImuAverage;

	// 写入文件头;
	iIndex = 0;
	for (unsigned int m = 0; m < vecTotals.size(); m++)
	{
		if (m < nLaserCount)
		{
			iIndex = m;
			fprintf_s(ptrFile, "%s%d%s,", "激光第", iIndex + 1, "组");
		}
		else if (m == nLaserCount) // 当前为惯导平整度;
		{
			iIndex = m - nLaserCount;
			fprintf_s(ptrFile, "%s,,,,,%s%d%s,","平均值", "惯导第", iIndex + 1, "组");
		}
		else
		{
			iIndex = m - nLaserCount;
			fprintf_s(ptrFile, "%s%d%s,", "惯导第", iIndex + 1, "组");
		}

		if (m == (vecTotals.size()-1))
		{
			fprintf_s(ptrFile, "%s,\n", "平均值");
		}
	}
	

	// 写入文件;
	double dImuAverage = 0.0;
	double dLaserAverage = 0.0;
	for (int n = 0;n < nMaxLen;n++)
	{
		dImuAverage = 0.0;
		dLaserAverage = 0.0;
		for (unsigned int m = 0; m < vecTotals.size(); m++)
		{
			std::vector<double>& vec = vecTotals[m];
			dIri = vec[n];

			if (m < nLaserCount)
			{
				dLaserAverage += dIri;
			}
			else if (m == nLaserCount)
			{
				dLaserAverage = dLaserAverage / nLaserCount;
				fprintf_s(ptrFile, "%.6lf,,,,,",dLaserAverage);

				vecLaserAverage.push_back(dLaserAverage);
			}

			// 大于，为后续的惯导数据;
			if (m >= nLaserCount)
			{
				dImuAverage += dIri;
			}
			fprintf_s(ptrFile, "%.6lf,", dIri);

			if (m == (vecTotals.size()-1))
			{
				dImuAverage = dImuAverage / nImuCount;
				fprintf_s(ptrFile, "%.6lf\n", dImuAverage);

				vecImuAverage.push_back(dImuAverage);
			}
		}
	}// for (int n = 0;n < nMaxLen;n++)
	

	// 激光和惯导的平均值进行最小二乘拟合直线值;
	double dK, dB, dR;
	dK = dB = dR = 0.0;
	LineFitLeastSquares(vecImuAverage._Myfirst(), vecLaserAverage._Myfirst(), nMaxLen,dK,dB,dR);

	// 保存KB值;
	fprintf_s(ptrFile, "K,B,Scale\n");
	fprintf_s(ptrFile, "%.4lf,%.4lf,%.4lf\n",dK,dB,dR);
	fclose(ptrFile);

	memset(strLine, 0, 1024);
	sprintf_s(strLine, "%s","处理完成，保存文件路径为惯导平整度文件夹下 result.csv 文件");
	QMessageBox::information(this, QString::fromLocal8Bit("提示"), QString::fromLocal8Bit(strLine), QMessageBox::Ok);

	// 打开文件夹;
	QFile file_obj(strSaveDir);
	if (file_obj.exists())
	{
		// 文件存在，则打开文件路径;
		QString file_path = "file:///" + strSaveDir;
		QDesktopServices::openUrl(QUrl::fromLocalFile(file_path));
	}

	accept();
}

void XrCalcuCalibrateParamDlg::slotOnclickDel()
{
	// 获取其model
	QStandardItemModel* model = static_cast<QStandardItemModel*>(ui.tableView_laser_iri->model());
	if (!model)
	{
		return;
	}

	// 获取选择索引
	QItemSelectionModel* selections = ui.tableView_laser_iri->selectionModel();
	QModelIndexList selected = selections->selectedIndexes();

	// 列表获取
	QMap<int, int> row_map;
	foreach(QModelIndex index, selected)
	{
		row_map.insert(index.row(), 0);
	}

	// 迭代删除
	QMapIterator<int, int> iter(row_map);
	iter.toBack();
	while (iter.hasPrevious())
	{
		iter.previous();
		int row_to_del = iter.key();
		model->removeRow(row_to_del);
	}
}

void XrCalcuCalibrateParamDlg::slotOnclickDelImu()
{
	// 获取其model;
	QStandardItemModel* model = static_cast<QStandardItemModel*>(ui.tableView_imu_iri->model());
	if (!model)
	{
		return;
	}

	// 获取选择索引;
	QItemSelectionModel* selections = ui.tableView_imu_iri->selectionModel();
	QModelIndexList selected = selections->selectedIndexes();

	// 列表获取;
	QMap<int, int> row_map;
	foreach(QModelIndex index, selected)
	{
		row_map.insert(index.row(), 0);
	}

	// 迭代删除;
	QMapIterator<int, int> iter(row_map);
	iter.toBack();
	while (iter.hasPrevious())
	{
		iter.previous();
		int row_to_del = iter.key();
		model->removeRow(row_to_del);
	}
}

void XrCalcuCalibrateParamDlg::slotOnclickCancel()
{
	accept();
}

void XrCalcuCalibrateParamDlg::initial()
{
	// 初始化table;
	initialTables();

	// 绑定信号槽;
	connect(this->ui.pushButton_find_laser_dir, SIGNAL(clicked()), this, SLOT(slotOnFindDir()));
	connect(this->ui.pushButton_find_imu_dir, SIGNAL(clicked()), this, SLOT(slotOnFindDir2()));
	connect(this->ui.pushButton_ok, SIGNAL(clicked()), this, SLOT(slotOnclickOk()));
	connect(this->ui.pushButton_del, SIGNAL(clicked()), this, SLOT(slotOnclickDel()));
	connect(this->ui.pushButton_imu_del, SIGNAL(clicked()), this, SLOT(slotOnclickDelImu()));
	connect(this->ui.pushButton_cancel, SIGNAL(clicked()), this, SLOT(slotOnclickCancel()));
}

void XrCalcuCalibrateParamDlg::initialTables()
{
	// 设置表头;
	QStandardItemModel* model = new QStandardItemModel();
	model->setColumnCount(3);
	model->setHeaderData(0, Qt::Horizontal, QStringLiteral("索引"));
	model->setHeaderData(1, Qt::Horizontal, QStringLiteral("名称"));
	model->setHeaderData(2, Qt::Horizontal, QStringLiteral("路径"));

	// 设置表格属性，设置居中; 
	ui.tableView_laser_iri->setSelectionBehavior(QAbstractItemView::SelectRows); // 选择行
	ui.tableView_laser_iri->setTextElideMode(Qt::ElideMiddle); // 居中显示
	ui.tableView_laser_iri->setEditTriggers(QAbstractItemView::NoEditTriggers); // 不可编辑
	ui.tableView_laser_iri->setSelectionMode(QAbstractItemView::MultiSelection); // 支持多行选中
	ui.tableView_laser_iri->setContextMenuPolicy(Qt::NoContextMenu); // 不设置右键菜单;
	ui.tableView_laser_iri->setModel(model);

	// QHeaderView::ResizeToContents根据列内容来定列宽;Stretch 扩展自适应宽度;
	ui.tableView_laser_iri->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui.tableView_laser_iri->verticalHeader()->hide();
	//ui.tableView_laser_iri->setColumnWidth(0,30);


	// 设置表头;
	QStandardItemModel* model0 = new QStandardItemModel();
	model0->setColumnCount(3);
	model0->setHeaderData(0, Qt::Horizontal, QStringLiteral("索引"));
	model0->setHeaderData(1, Qt::Horizontal, QStringLiteral("名称"));
	model0->setHeaderData(2, Qt::Horizontal, QStringLiteral("路径"));

	// 设置表格属性，设置居中; 
	ui.tableView_imu_iri->setSelectionBehavior(QAbstractItemView::SelectRows); // 选择行
	ui.tableView_imu_iri->setTextElideMode(Qt::ElideMiddle); // 居中显示
	ui.tableView_imu_iri->setEditTriggers(QAbstractItemView::NoEditTriggers); // 不可编辑
	ui.tableView_imu_iri->setSelectionMode(QAbstractItemView::MultiSelection); // 支持多行选中
	ui.tableView_imu_iri->setContextMenuPolicy(Qt::NoContextMenu); // 不设置右键菜单;
	ui.tableView_imu_iri->setModel(model0);

	// QHeaderView::ResizeToContents根据列内容来定列宽;Stretch 扩展自适应宽度;
	ui.tableView_imu_iri->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui.tableView_imu_iri->verticalHeader()->hide();
}

void XrCalcuCalibrateParamDlg::autoSearchIriData(QString strSearchDir, std::vector<QString>& vecSearchResult)
{
	// 设置dirlujing;
	QDir* dir = new QDir(strSearchDir);
	QStringList filter;

	// 获取列表下所有文件信息;
	QList<QFileInfo>* fileInfo = new QList<QFileInfo>(dir->entryInfoList(filter));
	for (int i = 0; i < fileInfo->count(); i++)
	{
		const QFileInfo info_tmp = fileInfo->at(i);
		QString path_tmp = info_tmp.filePath();
		if (info_tmp.fileName() == ".." || info_tmp.fileName() == ".")
		{

		}
		else if (info_tmp.isFile())
		{
			// 检查文件后缀;
			int index = path_tmp.lastIndexOf('.');
			if (index > 0)
			{
				QString strExp = path_tmp.right(path_tmp.length() - index - 1);
				strExp = strExp.toLower();
				if (strExp.compare("txt") == 0) // 后缀判断;
				{
					int nlength = path_tmp.length();
					if (nlength < 11)
					{
						continue;
					}

					// 去掉后缀，获取文件名;
					QString strIriPre = path_tmp.left(path_tmp.lastIndexOf('.'));
					strIriPre = strIriPre.right(strIriPre.length() - strIriPre.lastIndexOf('/') - 1);
					//strIriPre = strIriPre.right(7);
					strIriPre = strIriPre.toLower();

					// 判断名称是否包含该字段;
					if (strIriPre.contains("iri_10m"))
					{
						// 应读取文件判断一下是否为有效惯导数据;
						vecSearchResult.push_back(path_tmp);
					}
					else
					{
						continue;
					}

				}
			}
		}
		else if (info_tmp.isDir())// 为文件夹;
		{
			// 迭代搜索;
			autoSearchIriData(path_tmp, vecSearchResult);
		}
	}// for (int i = 0;i < fileInfo->count();i++)

	delete fileInfo;
	delete dir;
}

void XrCalcuCalibrateParamDlg::addDataToTable(std::vector<QString>& vecList, int isLaser)
{
	QStandardItemModel* model = NULL;
	if (isLaser)
	{
		model = static_cast<QStandardItemModel*>(ui.tableView_laser_iri->model());
	}
	else
	{
		model = static_cast<QStandardItemModel*>(ui.tableView_imu_iri->model());
	}

	// 移除所有已显示信息;
	model->removeRows(0, model->rowCount());

	// 添加到列表;
	QStandardItem* pItem = NULL;
	QString str = "";
	int nTableListCount = 0;
	for (unsigned int n = 0;n < vecList.size();n++)
	{
		// 每次更新;
		QString strPath = vecList[n];
		nTableListCount = model->rowCount();

		// 序号;
		str = str.sprintf("%d", nTableListCount+1);
		pItem = new QStandardItem(str);
		pItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		model->setItem(nTableListCount, 0, pItem);

		// 获取名称;
		QString strFileName = strPath.right(strPath.length() - strPath.lastIndexOf('/') - 1);
		strFileName = strFileName.left(strFileName.lastIndexOf('.'));

		// IRI_10m文件名称;
		pItem = new QStandardItem(strFileName);
		pItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		model->setItem(nTableListCount, 1, pItem);

		// IRI_10m文件路径;
		pItem = new QStandardItem(strPath);
		pItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		model->setItem(nTableListCount, 2, pItem);
	}
}

void XrCalcuCalibrateParamDlg::getDataFromTable(std::vector<QString>& vecList, int isLaser)
{
	// 从列表中获取;
	int table_list_count = 0;
	QStandardItemModel* model = NULL;
	if (isLaser)
	{
		model = static_cast<QStandardItemModel*>(ui.tableView_laser_iri->model());
	}
	else
	{
		model = static_cast<QStandardItemModel*>(ui.tableView_imu_iri->model());
	}

	if (!model)
	{
		return;
	}

	//遍历;
	QString str;
	table_list_count = model->rowCount();
	for (int i = 0; i < table_list_count; i++)
	{
		QModelIndex model_index_e = model->index(i, 2);
		str = model->data(model_index_e).toString();

		vecList.push_back(str);
	}
}

void XrCalcuCalibrateParamDlg::LineFitLeastSquares(double *data_x, double *data_y, int data_n, double& rK, double& rB, double& rScale)
{
	double A = 0.0;
	double B = 0.0;
	double C = 0.0;
	double D = 0.0;
	double E = 0.0;
	double F = 0.0;

	for (int i = 0; i < data_n; i++)
	{
		A += data_x[i] * data_x[i];
		B += data_x[i];
		C += data_x[i] * data_y[i];
		D += data_y[i];
	}

	// 计算斜率a和截距b; 
	double a, b, temp = 0;
	if (temp = (data_n*A - B*B))// 判断分母不为0  
	{
		a = (data_n*C - B*D) / temp;
		b = (A*D - B*C) / temp;
	}
	else
	{
		a = 1;
		b = 0;
	}

	// 计算相关系数r; 
	double Xmean, Ymean;
	Xmean = B / data_n;
	Ymean = D / data_n;

	double tempSumXX = 0.0, tempSumYY = 0.0;
	for (int i = 0; i < data_n; i++)
	{
		tempSumXX += (data_x[i] - Xmean) * (data_x[i] - Xmean);
		tempSumYY += (data_y[i] - Ymean) * (data_y[i] - Ymean);
		E += (data_x[i] - Xmean) * (data_y[i] - Ymean);
	}
	F = sqrt(tempSumXX) * sqrt(tempSumYY);

	double r;
	r = E / F;

	rK = a;
	rB = b;
	rScale = r;

}
