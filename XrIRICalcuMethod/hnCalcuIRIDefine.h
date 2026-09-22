#ifndef HNCALCU_IRI_DEFINE_HEADER_H
#define HNCALCU_IRI_DEFINE_HEADER_H
#include <iosfwd>

// 将字符串转换为整型 包括 (unsigned) int/short/long;
template <class T>
T str2INT(const std::string& str)
{
	T num;
	std::istringstream iss(str);

	iss >> num;

	return num;
}

namespace hn
{
	struct HNTIME
	{
		HNTIME()
		{
			year = 2000;
			mon = 1;
			day = 1;
			hour = 0;
			min = 0;
			sec = 0;
			microSec = 0;
			milliSec = 0;
		}
		uint16_t year;
		uint16_t mon;
		uint16_t day;
		uint16_t hour;
		uint16_t min;
		uint16_t sec;
		uint16_t microSec;
		uint16_t milliSec;

	};

	struct DAQ_STRUCT_INFO
	{
		DAQ_STRUCT_INFO()
		{
			gpsWeek = 0;
			gpsSecond = 0.0;
			dim = 0;
			xVelocity = yVelocity = zVelocity = 0.0;
			xAccelerate = yAccelerate = zAccelerate = 0.0;
		}

		short gpsWeek;
		double gpsSecond;
		unsigned long dim;

		double xVelocity;
		double yVelocity;
		double zVelocity;
		double xAccelerate;
		double yAccelerate;
		double zAccelerate;

		HNTIME utc;
	};

	struct dmi_speed_type
	{
		double gpsSecond;
		double speed;
	};

	// 编码器单条数据结构 LONG型;
	struct dmi_lrec_type
	{
		short    sSync;  // set to 0xffee
		short    sWeek;  // set to -1 if not known
		double   dTime;  // GPS time of week,in seconds
		unsigned long lValue[1];  // values(counts) should be equal to sDim
	};

	// 编码器单条数据结构 DOUBLE型;
	struct dmi_drec_type
	{
		short    sSync;      // set to 0xffee
		short    sWeek;      // set to -1 if not known
		double   dTime;      // GPS time of week,in seconds
		double   dValue[1];  // values(double precision) should be equal to sDim
	};

	struct POSD_STRUCT_INFO 
	{
		POSD_STRUCT_INFO()
		{
			dGpsTime = 0.0;
			dL = dB = dH = 0.0;
			dYaw = dPitch = dRoll = 0.0;
			dDist = 0.0;
		}

		int nGpsWeek;
		double dGpsTime;
		double dL;
		double dB;
		double dH;
		double dYaw;
		double dPitch;
		double dRoll;
		double dDist;
	};

	struct POSD_RESAMPLE250_INFO 
	{
		POSD_RESAMPLE250_INFO()
		{
			dAccHeight = 0.0;
			dLaserHeight = 0.0;
			ullFrameIdx = 0;
			dim = 0;
			dHeight = 0;
			ullFrameTime = 0;
		}

		HNTIME time;
		unsigned long dim;
		double dLaserHeight;
		double dAccHeight;
		double dHeight;
		unsigned long long ullFrameIdx;
		unsigned long long ullFrameTime;
	};

	struct IRI_RESULT_TIME_INFO
	{
		IRI_RESULT_TIME_INFO()
		{
			dIriVal = 0;
		}

		double dIriVal;
		HNTIME startTime;
		HNTIME endTime;
	};
}



#endif // HNCALCU_IRI_METHOD_API_H
