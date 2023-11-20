// 26.09.2016 created by Daniel Lachner

#ifndef __IO_EDFPlus_SignalImporter_c_H__
#define __IO_EDFPlus_SignalImporter_c_H__

#include <vector>
#include <omp.h>
#include <sstream>

#include "AbstractReader.h"
#include "DataTypes.h"

class  IO_EDFPlus_SignalImporter_c : public AbstractReader
{
public:
	IO_EDFPlus_SignalImporter_c(const std::string strFilename, std::vector<std::string > selectedUnipolarChanns, std::vector<std::string > selectedBipolarChanns, bool useScalpEEG);
    virtual ~IO_EDFPlus_SignalImporter_c();

	// public method to get data
	virtual bool  ReadSamples(long long lStartSample, long long lNumberSamples, matrixStd& signalSamples);
	virtual bool  ReadReformattedSamples(long long lStartSample, long long lNumberSamples, matrixStd& reformattedSamples);

	virtual bool  WriteReformattedSamples(long long lStartTime, long long lEndTime);

	//virtual bool setSelectedChannels(std::vector<unsigned> selection);

	// start date
	unsigned getChannelLength(unsigned ch);
	unsigned getChannelSamplingRate(unsigned ch);

	virtual bool isValidImporter();

	virtual double getSamplingRate();
	virtual bool setSamplingRate(double samplingRate);

	virtual unsigned long getSampleCount();
	virtual bool setSampleCount(_int64 lSampleCount);

	DateAndTime getStartDate() { return m_fileStartDateAndTime; };

	virtual unsigned int getNumberUnipolarChannels();
	virtual unsigned int getNumberReformattedMontages();

	virtual std::string getFileName();

	void setSelectedUnipolarChannels(std::vector<std::string > selectedChanns) {
		m_selectedUnipolarChannels = selectedChanns;
	}
	void setSelectedBipolarChannels(std::vector<std::string > selectedChanns) {
		m_selectedBipolarChannels = selectedChanns;
	}

	virtual bool readUnipolarLabels(std::vector<ContactNames> &unipolarContactNames);
	virtual bool readMontageLabels(std::vector<MontageNames> &montageLabels);

	DateAndTime getDateAndTime() { return m_fileStartDateAndTime; }
	virtual std::string getPatientNameAndPath(void);

	virtual bool readHeader();

protected:
	inline _int64 getBlockLength() { return m_nBlockLength; };
	inline bool setBlockLength(_int64 nBlockLength) { m_nBlockLength = nBlockLength; return true; };
};

#endif // __IOAbstractSignalImporter_c__H__
