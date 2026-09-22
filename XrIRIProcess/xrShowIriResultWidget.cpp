/*! @file
********************************************************************************
<PRE>
模块名       :XrIRIProcess
文件名       : xrShowIriResultWidget.cpp
相关文件     : xrShowIriResultWidget.h,QWidget
文件实现功能 : 用于显示惯导平整度结果值的面板，停靠面板采用 hn::CDockWidget，
统一由hn::CDockManager进行管理;
作者         : 朱旭波
版本         : 软件部，朱旭波
--------------------------------------------------------------------------------
备注         : <其它说明>
--------------------------------------------------------------------------------
修改记录 :
日 期        版本     修改人              修改内容
2021/10/15	 1.0	  朱旭波		         创建
</PRE>
*******************************************************************************/
#include "xrShowIriResultWidget.h"
#include <QStandardItemModel>
#include <QAbstractItemView>
#include <QHeaderView>

xrShowIriResultWidget::xrShowIriResultWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);

	initialTableView();
}

xrShowIriResultWidget::~xrShowIriResultWidget()
{
}

void xrShowIriResultWidget::initialTableView()
{
	//// 设置表头;
	//QStandardItemModel* modelResample = new QStandardItemModel();
	//modelResample->setColumnCount(2);
	//modelResample->setHeaderData(0, Qt::Horizontal, QStringLiteral("索引"));
	//modelResample->setHeaderData(1, Qt::Horizontal, QStringLiteral("IRI值"));

	//// 设置表格属性，设置居中; 
	//ui.tableView_iriResample->setSelectionBehavior(QAbstractItemView::SelectRows); // 选择行
	//ui.tableView_iriResample->setTextElideMode(Qt::ElideMiddle); // 居中显示
	//ui.tableView_iriResample->setEditTriggers(QAbstractItemView::NoEditTriggers); // 不可编辑
	//ui.tableView_iriResample->setSelectionMode(QAbstractItemView::SingleSelection); // 支持单行选中
	//ui.tableView_iriResample->setContextMenuPolicy(Qt::NoContextMenu); // 不设置右键菜单;
	//ui.tableView_iriResample->setModel(modelResample);

	//// QHeaderView::ResizeToContents根据列内容来定列宽;Stretch 扩展自适应宽度;
	//ui.tableView_iriResample->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	//ui.tableView_iriResample->verticalHeader()->hide();

	// 设置表头;
	QStandardItemModel* model = new QStandardItemModel();
	model->setColumnCount(2);
	model->setHeaderData(0, Qt::Horizontal, QStringLiteral("索引"));
	model->setHeaderData(1, Qt::Horizontal, QStringLiteral("IRI值"));

	// 设置表格属性，设置居中; 
	ui.tableView_iriResult->setSelectionBehavior(QAbstractItemView::SelectRows); // 选择行
	ui.tableView_iriResult->setTextElideMode(Qt::ElideMiddle); // 居中显示
	ui.tableView_iriResult->setEditTriggers(QAbstractItemView::NoEditTriggers); // 不可编辑
	ui.tableView_iriResult->setSelectionMode(QAbstractItemView::SingleSelection); // 支持单行选中
	ui.tableView_iriResult->setContextMenuPolicy(Qt::NoContextMenu); // 不设置右键菜单;
	ui.tableView_iriResult->setModel(model);

	// QHeaderView::ResizeToContents根据列内容来定列宽;Stretch 扩展自适应宽度;
	ui.tableView_iriResult->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	ui.tableView_iriResult->verticalHeader()->hide();
}

struct IRI_DATA_SHOW 
{
	IRI_DATA_SHOW()
	{
		index = 0;
		dIriValue = 0.0;
	}
	int index;
	double dIriValue;
};

void xrShowIriResultWidget::setDataIRI(QString strIriPath100)
{
	// 打开文件信息;
	FILE* ptrFile = NULL;
	fopen_s(&ptrFile, strIriPath100.toLocal8Bit().data(), "rt");
	if (!ptrFile)
	{
		return;
	}

	// 读取文件解析;
	std::vector<IRI_DATA_SHOW> vecIriShows;
	char strLine[1024];
	while (!feof(ptrFile))
	{
		memset(strLine,0,1024);
		fgets(strLine,1024,ptrFile);

		IRI_DATA_SHOW info;
		int nret = sscanf_s(strLine, "%d	%lf\n", &info.index, &info.dIriValue);
		if (nret < 2)
		{
			continue;
		}
		vecIriShows.push_back(info);
	}
	fclose(ptrFile);
	if (vecIriShows.size() <= 0)
	{
		return;
	}

	// 列表信息清空;
	QStandardItemModel* model = static_cast<QStandardItemModel*>(ui.tableView_iriResult->model());
	if (!model)
	{
		return;
	}

	// 移除所有已显示信息;
	model->removeRows(0, model->rowCount());

	// 添加到列表中;
	QString str = "";
	int nTableListCount = 0;
	QStandardItem* pItem = NULL;
	for (int n = 0; n < vecIriShows.size(); n++)
	{
		IRI_DATA_SHOW& info = vecIriShows[n];

		// 每次更新;
		nTableListCount = model->rowCount();

		// 序号;
		str = str.sprintf("%d", info.index);
		pItem = new QStandardItem(str);
		pItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		model->setItem(nTableListCount, 0, pItem);

		// IRI值;
		str = str.sprintf("%.3lf", info.dIriValue);
		pItem = new QStandardItem(str);
		pItem->setTextAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		model->setItem(nTableListCount, 1, pItem);
	}
}
