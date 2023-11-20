// 05.03.2019 created by Daniel Lachner
#pragma once

#ifndef __AbstractReader__H__
#define __AbstractReader__H__

#include <vector>
#include <omp.h>
#include <sstream>

#include "DataTypes.h"

class AbstractReader
{
public:

	AbstractReader(std::string strFilename, std::vector<std::string > selectedUnipolarChanns, std::vector<std::string > selectedBipolarChanns, bool useScalpEEG);
	virtual ~AbstractReader();

public:
	// public method to get data
	virtual bool  ReadSamples(long long lStartSample, long long lNumberSamples, matrixStd& signalSamples) = 0;
	virtual bool  ReadReformattedSamples(long long lStartSample, long long lNumberSamples, matrixStd& reformattedSamples) = 0;

	virtual bool  WriteReformattedSamples(long long lStartTime, long long lEndTime) = 0;

	virtual bool isValidImporter() = 0;

	virtual double getSamplingRate() = 0;
	virtual bool setSamplingRate(double samplingRate) = 0;

	virtual unsigned long getSampleCount() = 0;
	virtual bool setSampleCount(_int64 lSampleCount) = 0;

	DateAndTime getStartDate() { return m_fileStartDateAndTime; };

	virtual unsigned int getNumberUnipolarChannels() = 0;
	virtual unsigned int getNumberReformattedMontages() = 0;

	virtual std::string getFileName() = 0;

	void setSelectedUnipolarChannels(std::vector<std::string > selectedChanns) {
		m_selectedUnipolarChannels = selectedChanns;
	}
	void setSelectedBipolarChannels(std::vector<std::string > selectedChanns) {
		m_selectedBipolarChannels = selectedChanns;
	}

	virtual bool readUnipolarLabels(std::vector<ContactNames> &unipolarContactNames) = 0;
	virtual bool readMontageLabels(std::vector<MontageNames> &montageLabels) = 0;

	DateAndTime getDateAndTime() { return m_fileStartDateAndTime; }
	//virtual std::string getPatientNameAndPath(void) = 0;

	//virtual bool readHeader() = 0;

protected:
	inline _int64 getBlockLength() { return m_nBlockLength; };
	inline bool setBlockLength(_int64 nBlockLength) { m_nBlockLength = nBlockLength; return true; };

protected:
	_int64			m_nBlockLength;	// Block lenght in seconds
	long			m_lMinDig;			// minimum digital value
	long			m_lMaxDig;			// maximum digital value
	double			m_dMinPhys;			// minimum physical value
	double			m_dMaxPhys;			// maximum physical value
	double			m_dSamplingRate;	// signal sampling rate
	_int64	m_lSampleCount;		// sample count
	std::string m_strFilename;			// file name of signal file 
	std::string m_patientName;
	std::string m_patientPath;

	bool m_bValidImporter;
	std::vector<ContactNames> m_unipolarContacts;
	std::vector<MontageNames> m_montages;
	std::vector<std::string> m_selectedUnipolarChannels;
	std::vector<std::string> m_selectedBipolarChannels;
	std::vector<std::string> m_readChannelLabels;

	DateAndTime m_fileStartDateAndTime;

	bool m_useScalpEEG;
};

#endif // __AbstractReader__H__
