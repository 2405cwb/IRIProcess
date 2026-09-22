#pragma once

#include <QDialog>
#include "ui_XrCalcuCalibrateParamDlg.h"

class XrCalcuCalibrateParamDlg : public QDialog
{
	Q_OBJECT

public:
	XrCalcuCalibrateParamDlg(QWidget *parent = Q_NULLPTR);
	~XrCalcuCalibrateParamDlg();

public slots:
	// 打开文件夹路径;
	void slotOnFindDir();

	// 设置保存文件夹路径;
	void slotOnFindDir2();

	// 点击确定;
	void slotOnclickOk();

	// 点击删除激光列表;
	void slotOnclickDel();

	// 点击删除IMU列表;
	void slotOnclickDelImu();

	// 点击取消;
	void slotOnclickCancel();

private:
	void initial();

	// 初始化table;
	void initialTables();

	// 根据给定目录检索所有子目录下的IRI10的值;
	void autoSearchIriData(QString strDir, std::vector<QString>& vecList);

	// 将数据添加到列表;
	void addDataToTable(std::vector<QString>& vecList,int isLaser);

	// 从激光列表中获取路径;
	void getDataFromTable(std::vector<QString>& vecList, int isLaser);

	// 最小二乘拟合线性;
	void LineFitLeastSquares(double *data_x, double *data_y, int data_n,double& rK,double& rB,double& rScale);

private:
	Ui::XrCalcuCalibrateParamDlg ui;
};
