#pragma once
#include <QString>
#include <stdio.h>
#include <iostream>

namespace XrApp
{
	// 结构体;
	struct IRI_SEARCH_SETTING_INFO 
	{
		IRI_SEARCH_SETTING_INFO()
		{
			nSearchModel = 1;
			strSearchDir = "D:/data/";
			nDmiHz = 2350;
			dWheelSize = 2.35;
			strSaveDir = "D:/save/";
			nSaveIRI10m = 1;
			nSaveIRI100m = 0;
			nSaveIRI1000m = 0;
			nDmiSerializeFromName = 0;
			nUseAutoSearchProc = 0;
		}

		// 检索类型0表示搜索已处理的信息，1表示搜索未处理的工程信息，2表示搜索全部;
		int nSearchModel;

		// 使用自动搜索处理功能;
		int nUseAutoSearchProc;

		// 检索路径;
		QString strSearchDir;

		// 编码器频率;
		int nDmiHz;

		// 车轮周长;
		double dWheelSize;

		// 保存路径;
		QString strSaveDir;

		// 设置保存IRI10M;
		int nSaveIRI10m;

		// 设置保存IRI100M;
		int nSaveIRI100m;

		// 设置保存IRI1000M;
		int nSaveIRI1000m;

		// 从标记是否需要从名称上解析编码器和车轮周长信息;
		int nDmiSerializeFromName;
	};

	class XrIRISetting
	{
	private:
		XrIRISetting();
		~XrIRISetting();

	public:
		// 获取单实例;
		static XrIRISetting* getIRISetting();

		// 删除单实例;
		static void destroyIRISetting();

		// 获取搜索配置信息;
		IRI_SEARCH_SETTING_INFO* getSearchInfo();

		// 将内存数据写入xml;
		bool saveData();

		// 从xml文件中读取配置文件参数信息值内存;
		bool readData();

	private:
		static XrIRISetting* m_pIRISetting;

		// 自动检索参数设置信息;
		IRI_SEARCH_SETTING_INFO* m_pSearchInfo;
	};
}



