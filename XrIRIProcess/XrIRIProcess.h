#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_XrIRIProcess.h"
#include "..\hnQtRibbonUI\hnRibbonMainWindow.h"
#include "..\QtAdvancedDocking\DockManager.h"
#include <vector>
#include <QMutex>

// 前置声明;
class QTreeView;
class QTreeWidget;
class QStandardItemModel;
class QStandardItem;
class QTreeWidgetItem;
class QDockWidget;
class QAction;
class hnRibbonCategory;
class hnRibbonContextCategory;
class QLabel;
class QProgressBar;
class xrShowIriResultWidget;
class XrIRIProcessRunnable;
class XrIRILoggingWidget;

extern QMutex g_saveLog_mutex;

namespace hn
{
	class CDockWidget;
}

// 进度回调函数;
void progress_callback(float p, const char* msg);
class XrIRIProcess : public hnRibbonMainWindow
{
    Q_OBJECT

public:

	// 构造;
    XrIRIProcess(QWidget *parent = Q_NULLPTR);

	// 析构函数;
	~XrIRIProcess();

	static XrIRIProcess* AppMain;

	// 进度条信息;
	QProgressBar* getProgressBar() { return m_status_progress; }
	QLabel* getMsgLabel() { return m_status_label2; }

protected:
	// 程序退出时保存之前的布局;
	void closeEvent(QCloseEvent * e);

private slots:
	// 打开路径，检索所有惯导平整度数据;
	void openProjectSlot();

	// 工程参数设置;
	void setConfigSlot();

	// 计算IRI;
	void calcIriSlot();

	// 自动搜索;
	void autoSearchPathSlot();

	// 标定检查;
	void calibrIriSlot();

	// 车轮编码器标定;
	void calibrWheelSlot();

	// 清空列表;
	void clearListSlot();

	// 用户手册;
	void helpbookSlot();

	// 关于;
	void aboutUsSlot();

	// 双击树节点，显示结果信息-暂时不实现;
	void treeviewItemDbClick(QTreeWidgetItem *item, int column);

	// 主框架进度条显示控制;
	void updateProgress(float p, QString msg);

	// 更新容器;
	void updateProcessFinished(QString strDaqPath, int succ);

	// 更新处理信息;
	void updateProcessDirFinished(QString strDaqDirPath,QString strSaveDirPath, int succ);

	// 打开文件夹操作;
	void openProjDirSlot();

	// 树目录工作区响应消息;
	void onCustomContextMenuRequestedSlot(const QPoint& pos);

	// 定时器，定时搜索目录并计算处理;
	void timer_handleAutoProcTimeout();
private:
	// 保存布局;
	void saveLayout();

	// 读取布局;
	void readLayout();

	// 创建视图;
	void createView();

	// 创建工具栏;
	void createAction();

	// 创建连接;
	void createConnect();

	// 树状视图连接;
	void createTreeConnect();

	// 初始化状态栏;
	void setupStatusBar();

	// 创建工程管理应用模块工具栏;
	void createProCategory(hnRibbonCategory* page);

	//// 创建工具模块工具栏;
	//void createToolCategory(hnRibbonCategory* page);

	// 创建树节点面板;
	void createTreeDockPane();

	// 创建成果展示面板-绘图显示;
	void createShowIriDockPane();

	// 创建日志面板;
	void createLoggingDockPane();

	// 设置获取类型，自动搜索路径下所有符合条件文件;
	void autoSearchDaqData(QString strSearchDir, std::vector<QString>& vecSearchResult);

	// 解析日志文件，获取所有记录信息;
	void serializeLogFile(QString strSearchDir, QString strLogName,QString strUsedName, std::vector<QString>& vecLogDirNames);

	// 处理给定文件夹目录下所有数据;
	bool calcuIriFromDirs(std::vector<QString>& vecDirs);

	// 设置获取类型，自动搜索路径下所有符合条件的文件,传入一个年月日文件夹路径以及;
	void autoProcSearchDaqData(QString strSearchDir,QString strLogName, std::vector<QString>& vecSearchResult);

	// 判断该字符串是否存在于vector中;
	bool isNameExist(QString strName, std::vector<QString>& vecNames);

	// 更新树节点;
	void updateTreeNode(std::vector<QString>& vecPaths);

	// 删除所有工程item;
	void clearAllItem();

	// 判断DAQ工程是否有效;
	int isValidProject(QString strDaqPath);

	// 解析I300原始惯导数据;
	bool parseHGI300DataPart(const std::string& str,float& yAccValue);

	// 开始启动线程池处理列表中所有数据,传入列表为DAQ文件;
	void startProcessIRIData(std::vector<QString>& vecPaths);

	// 开始启动线程池处理列表中所有数据,传入列表为文件夹路径，由runnable内部遍历获取进行计算;
	void startProcessIRIDataFromDir(std::vector<QString>& vecDirPaths);

	// 获取当前线程池中正在处理的对象;
	XrIRIProcessRunnable* getIRICalcuWorker(QString strDaqPath);

	// 写入错误日志;
	void writeErrLog(QString strMsg);
private:
    Ui::XrIRIProcessClass ui;

	// dock面板管理器;
	hn::CDockManager* m_DockManager;

	// 进度条;
	QProgressBar *m_status_progress;
	QLabel* m_status_label;
	QLabel* m_status_label2;

	// 打开工程;
	QAction* m_openProjectAct;

	// 参数设置;
	QAction* m_setProjectParamAct;

	// 计算处理;
	QAction* m_calcIriAct;

	// 更新检索;
	QAction* m_aotuUpdateAct;

	// 标定检查;
	QAction* m_caliParamAct;

	// 车轮编码器标定;
	QAction* m_caliWheelAct;

	// 清空列表;
	QAction* m_clearListAct;

	// 用于控制面板显示隐藏;
	QMenu* m_pShowPaneMenu;

	// 用户手册;
	QAction* m_helpAct;

	// 关于;
	QAction* m_aboutAct;

	// 打开文件夹操作;
	QAction* m_treeOpenTreeAct;

	// 树状视图;
	QTreeWidget* m_docTreeWidget;

	// 树状视图停靠窗;
	hn::CDockWidget *m_DoctreeViewDock;

	// IRI结果值显示;
	xrShowIriResultWidget* m_showIriWidget;

	// 日志面板;
	XrIRILoggingWidget* m_showLoggingWidget;

	// 右键菜单;
	QMenu* m_tree_proj_menu;

	// 定时器;
	QTimer* m_pAutoProcTimer;

	// 线程池对象;
	std::map<QString, XrIRIProcessRunnable*> m_map_pool_threads;
	int m_nCurProcCount;
};
