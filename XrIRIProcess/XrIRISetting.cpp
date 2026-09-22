#include "XrIRISetting.h"
#include <QFile>
#include <QtXml/QtXml>

namespace XrApp
{
	XrIRISetting* XrIRISetting::m_pIRISetting = NULL;
	XrIRISetting::XrIRISetting():m_pSearchInfo(NULL)
	{
	}


	XrIRISetting::~XrIRISetting()
	{
		if (m_pSearchInfo)
		{
			delete m_pSearchInfo;
			m_pSearchInfo = NULL;
		}
	}

	XrIRISetting* XrIRISetting::getIRISetting()
	{
		if (!m_pIRISetting)
		{
			m_pIRISetting = new XrIRISetting();
		}

		return m_pIRISetting;
	}

	void XrIRISetting::destroyIRISetting()
	{
		if (m_pIRISetting)
		{
			delete m_pIRISetting;
			m_pIRISetting = NULL;
		}
	}

	IRI_SEARCH_SETTING_INFO* XrIRISetting::getSearchInfo()
	{
		return m_pSearchInfo;
	}

	bool XrIRISetting::saveData()
	{
		// 获取配置文件路径;
		QString currentPath = QCoreApplication::applicationDirPath();
		currentPath += "/systemSetting.xml";

		// 保存文件，无法写入;
		QFile file(currentPath);
		if (!file.open(QIODevice::WriteOnly))
		{
			return false;
		}

		// 写入流;
		QXmlStreamWriter xmlWriter(&file);
		xmlWriter.setAutoFormatting(true);
		xmlWriter.writeStartDocument();
		xmlWriter.writeStartElement("xr");

		// 自动搜索相关;
		QStringList xmlNameList;
		xmlNameList << QString::fromUtf8("searchModel")
			<< QString::fromUtf8("searchPath")
			<< QString::fromUtf8("dmiHz")
			<< QString::fromUtf8("wheelSize")
			<< QString::fromUtf8("savePath")
			<< QString::fromUtf8("iriTen")
			<< QString::fromUtf8("iriHan")
			<< QString::fromUtf8("iriThro")
			<< QString::fromUtf8("serialDmi")
			<< QString::fromUtf8("autoSearchProc");

		// 自动搜索相关;
		QStringList xmlParamlist;
		xmlParamlist << QString::number(m_pSearchInfo->nSearchModel, 10)
			<< m_pSearchInfo->strSearchDir
			<< QString::number(m_pSearchInfo->nDmiHz, 10)
			<< QString::number(m_pSearchInfo->dWheelSize, 10,3)
			<< m_pSearchInfo->strSaveDir
			<< QString::number(m_pSearchInfo->nSaveIRI10m, 10)
			<< QString::number(m_pSearchInfo->nSaveIRI100m, 10)
			<< QString::number(m_pSearchInfo->nSaveIRI1000m, 10)
			<< QString::number(m_pSearchInfo->nDmiSerializeFromName, 10)
			<< QString::number(m_pSearchInfo->nUseAutoSearchProc, 10);

		// 自动搜索参数设置;
		xmlWriter.writeStartElement("search");
		for (int i = 0; i < xmlNameList.size(); i++)
		{
			xmlWriter.writeTextElement(xmlNameList.at(i), xmlParamlist.at(i));
		}
		xmlWriter.writeEndElement();

		// 构建下一个结构体，清除之前的信息;
		xmlNameList.clear();
		xmlParamlist.clear();

		// xr节点结束;
		xmlWriter.writeEndElement();
		xmlWriter.writeEndDocument();
		file.close();
		return true;
	}

	bool XrIRISetting::readData()
	{
		// 搜索数据不存在则创建;
		if (NULL == m_pSearchInfo)
		{
			m_pSearchInfo = new IRI_SEARCH_SETTING_INFO;
		}

		// 读取数据;
		QString currentPath = QCoreApplication::applicationDirPath();
		currentPath += "/systemSetting.xml";

		// 检查文件存在;
		QFile file(currentPath);
		if (!file.open(QFile::ReadOnly | QFile::Text))
		{
			// 打开文件失败;
			//qDebug() << QObject::tr("error file %s failed\n") << currentPath;
			return false;
		}

		QXmlStreamReader xmlReader(&file);
		xmlReader.readNextStartElement();

		xmlReader.readNext();
		while (!xmlReader.atEnd())
		{
			if (xmlReader.isStartElement())
			{
				QString str = xmlReader.name().toString();

				// laser
				if (xmlReader.name() == "search")
				{
					xmlReader.readNext();
				}
				else if (xmlReader.name() == "searchModel")
				{
					m_pSearchInfo->nSearchModel = xmlReader.readElementText().toInt();
					xmlReader.readNext();
				}
				else if (xmlReader.name() == "searchPath")
				{
					m_pSearchInfo->strSearchDir = xmlReader.readElementText();
					xmlReader.readNext();
				}
				else if (xmlReader.name() == "dmiHz")
				{
					m_pSearchInfo->nDmiHz = xmlReader.readElementText().toInt();
					xmlReader.readNext();
				}
				else if (xmlReader.name() == "wheelSize")
				{
					m_pSearchInfo->dWheelSize = xmlReader.readElementText().toDouble();
					xmlReader.readNext();
				}
				else if (xmlReader.name() == "savePath")
				{
					m_pSearchInfo->strSaveDir = xmlReader.readElementText();
					xmlReader.readNext();
				}
				else if (xmlReader.name() == "iriTen")
				{
					m_pSearchInfo->nSaveIRI10m = xmlReader.readElementText().toInt();
					xmlReader.readNext();
				}
				else if (xmlReader.name() == "iriHan")
				{
					m_pSearchInfo->nSaveIRI100m = xmlReader.readElementText().toInt();
					xmlReader.readNext();
				}
				else if (xmlReader.name() == "iriThro")
				{
					m_pSearchInfo->nSaveIRI1000m = xmlReader.readElementText().toInt();
					xmlReader.readNext();
				}
				else if (xmlReader.name() == "serialDmi")
				{
					m_pSearchInfo->nDmiSerializeFromName = xmlReader.readElementText().toInt();
					xmlReader.readNext();
				}
				else if (xmlReader.name() == "autoSearchProc")
				{
					m_pSearchInfo->nUseAutoSearchProc = xmlReader.readElementText().toInt();
					xmlReader.readNext();
				}
			}
			else
			{
				xmlReader.readNext();
			}
		}//while (!xmlReader.atEnd())

		file.close();
		if (xmlReader.hasError())
		{
			qDebug() << QObject::tr("parse file failed,error msg is %s \n") << xmlReader.errorString();
		}

		return true;
	}

}


