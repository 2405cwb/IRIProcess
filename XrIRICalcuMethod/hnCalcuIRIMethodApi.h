#ifndef HNCALCU_IRI_METHOD_API_H
#define HNCALCU_IRI_METHOD_API_H
#include "xriricalcumethod_global.h"
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include "hnCalcuIRIDefine.h"
#include <QObject>

namespace hn
{
	class XRIRICALCUMETHOD_EXPORT hnCalcuIRIMethodApi : public QObject
	{
		Q_OBJECT
	public:
		hnCalcuIRIMethodApi();
		~hnCalcuIRIMethodApi();

		// 设置进度条回调函数;
		void(*loadCallback)(float, const char*);

		// 设置参数;
		void setParam(const char* strDaqPath, int nImuHz, int nDmiHz, double dDmiWheel);

		// 设置KB系数;
		void setKB(double dK, double dB);

		// 设置保存文件路径;
		void setSaveResultPath(const char* strSaveDir,const char* strSavePreName);

		// 设置车轮编码器安装在左侧还是右侧，为0表示在左侧，为1表示在右侧，安装不同惯导坐标系指向不一样;
		void setIsOnRight(int onRight);

		// 设置使用信号槽信息;
		void setUseThread(bool useThread);

		// 计算处理,返回为0表示成功，其他都是异常;
		int calcuIRI();

		// 计算激光平整度;
		int calcuIRIMTD();

		// 获取错误信息;
		std::string getErrMsg();

		// 测试代码，读取resample.txt文件，计算100m值对比结果;
		void testResample100(const char* strResamplePath);

		// 设置进度信息;
		void setProgress(float, const char*);

		// 设置是否保存10mIRI-100m-1000m值;
		void setSaveIRIs(int save10, int save100, int save1000);

		// 异或校验检查错误值;
		bool checkLineData(std::string strLine);

	signals:
		// 信号标记，主要用于进度处理;
		void progress(float p, QString msg);

	private:
		// DAQ数据解析;
		bool loadDaqData(std::vector<DAQ_STRUCT_INFO>& vecDaqInfo,std::vector<dmi_speed_type>& vecDmiSpeedInfo);

		// 根据DAQ记录时间信息，更新当前读取的resample里面的时间;
		bool CheckFrame(std::string instr,int framelen);
		bool UpdateTimeLoadDaq(std::vector<POSD_RESAMPLE250_INFO>& vecResample50);

		// 解析I300原始惯导数据;
		bool parseHGI300Data(const std::string& str, DAQ_STRUCT_INFO& daqInfo);

		// utc时间转换gps时间;
		bool utc2gps(const HNTIME& stTime, short& nGPSWeek, double& dGPSSeconds, double dGPSSubUTC = 0.0);

		// gps周秒转换utc;
		bool gps2utc(short nGpsWeek, double dGpsSeconds, HNTIME& stTime, double dGPSSubUTC = 0.0);

		// 航位推算位置计算;
		bool calcuPosBySinsDr(std::vector<DAQ_STRUCT_INFO>& vecDaqInfo, std::vector<dmi_speed_type>& vecDmiSpeedInfo, std::vector<POSD_STRUCT_INFO>& vecMatPos);

		// 计算平整度值;
		void calcuIRIMethod(double dIntervel, double DeltLen,std::vector<POSD_RESAMPLE250_INFO>& vecResample250, std::vector<IRI_RESULT_TIME_INFO>& vecIRIResult,double dPerTime = 0.00025);

		// 重采样250mm数据;
		bool resampleIRI250(std::vector<POSD_STRUCT_INFO>& vecMatPos,std::vector<POSD_RESAMPLE250_INFO>& vecResample250);

		// 从50重采样值250;
		bool resampleIRI250From50(std::vector<POSD_RESAMPLE250_INFO>& vecResample50, std::vector<POSD_RESAMPLE250_INFO>& vecResample250);

		// 给定间隔数据计算IRI值;
		double CalculateIRI(double dIntervel, std::vector<POSD_RESAMPLE250_INFO>& vecIRIInfo,
			double DeltLen = 0.25);

		// 保存给定数据IRI结果值;
		void saveIRIs(const char* strSavePath, std::vector<IRI_RESULT_TIME_INFO>& vecIRIResult);

		// 保存250mm重采样信息数据结果值;
		void saveReSample250(const char* strSavePath,std::vector<POSD_RESAMPLE250_INFO>& vecResample250);

		// 添加一个速度计算变量;
		double addSpeedBlong(double iriResult, double speedval);

		// 单轴加速度计计算速度和高程信息-测试代码;
		void calcAccDisIntegrate(std::vector<DAQ_STRUCT_INFO>& vecDaqInfo, std::vector<POSD_STRUCT_INFO>& vecMatPos);

		// 多段线拟合;
		void polyfit(int n, double *y, int poly_n, double *a, double *tempx, double *tempy, double *sumxx, double *sumxy, double *ata);//拟合
		void gauss_solve(int n, double *A, double *x, double *b);
		void polyfitVal(int n, double *y, int poly_n, double *a);
		void Acc2Vel(int n, double *acc, double *vel, double t,int fitnum);
		void Vel2Dis(int n, double *vel, double *dis, double t,int fitnum);

		void Acc2Vel2(int ptcount,double* ptAcc,double*& ptVel,double dTimeScale,int fitnum);
	private:
		// 记录原始DAQ文件路径;
		std::string m_strDaqPath;

		// 记录保存结果值文件夹路径;
		std::string m_strSaveResultDir;

		// 记录保存结果值文件名前缀信息;
		std::string m_strSavePreName;

		// 记录错误信息;
		std::string m_strErrMsg;

		// 惯导频率;
		int m_nImuHz;

		// 编码器频率;
		int m_nDmiHz;

		// 车轮周长;
		double m_dDmiWheel;

		// 记录计算的GPS周;
		int m_nCurGpsWeek;

		double* m_SZU;
		double* m_PZU;
		double* m_ZSU;
		double* m_oldZSU;

		// 标记设备安装在左侧还是右侧;
		int m_isOnRight;

		int m_bSaveIRI10;
		int m_bSaveIRI100;
		int m_bSaveIRI1000;

		// K B系数值;
		double m_dScaleK;
		double m_dScaleB;

		bool m_bUseSpeedParam;
		int m_nSpeedCnt;
		double* m_dSpeedParm;
		double* m_dSpeedK;
		double* m_dSpeedB;

	private:
		double *m_tempx, *m_tempy;
		double *m_sumxx_0, *m_sumxy_0, *m_ata_0;
		double *m_sumxx_1, *m_sumxy_1, *m_ata_1;
		double *m_sumxx_2, *m_sumxy_2, *m_ata_2;
		double m_poly_a[6];

		// 标记处理为使用线程，需要把进度信息通过信号槽的方式传递出去;
		bool m_use_thread;
	};
}



#endif // HNCALCU_IRI_METHOD_API_H
