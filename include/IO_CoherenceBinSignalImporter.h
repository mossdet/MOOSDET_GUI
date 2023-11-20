#pragma once
//$1 01.08.2008 Matthias D. created
//30.07.2019 DLP imported

#ifndef __IO_CoherenceBinSignalImporter_H__
#define __IO_CoherenceBinSignalImporter_H__

#include <vector>
#include <omp.h>
#include <sstream>

#include "AbstractReader.h"
#include "DataTypes.h"

class  IO_CoherenceBinSignalImporter : public AbstractReader
{
public:
	IO_CoherenceBinSignalImporter(const std::string strFilename, size_t SampleType, std::vector<std::string > selectedUnipolarChanns, std::vector<std::string > selectedBipolarChanns, bool useScalpEEG);
	virtual ~IO_CoherenceBinSignalImporter();

public:
	// public method to get data
	//virtual bool ReadSamples(unsigned long lStartSample, unsigned long lNumberSamples, matrixStd& signalSamples);

	// public method to get channel labels
	//virtual bool getLabels(std::vector< std::pair< std::string, std::string> >& Labels) const;

	// start date
	//virtual bool getStartDate(DateAndTime& startDate) const;

	/*************************/
	virtual bool  ReadSamples(long long lStartSample, long long lNumberSamples, matrixStd& signalSamples);
	virtual bool  ReadReformattedSamples(long long lStartSample, long long lNumberSamples, matrixStd& reformattedSamples);
	virtual bool  WriteReformattedSamples(long long lStartTime, long long lEndTime);
	virtual bool isValidImporter();


	virtual bool readUnipolarLabels(std::vector<ContactNames> &unipolarContactNames);
	virtual bool readMontageLabels(std::vector<MontageNames> &montageLabels);
	
	virtual unsigned int getNumberUnipolarChannels() {	return (unsigned int)m_unipolarContacts.size();}
	virtual unsigned int getNumberReformattedMontages() { return (unsigned int)m_montages.size(); }
	virtual std::string getFileName() { return m_strFilename; }
	virtual void setSelectedUnipolarChannels(std::vector<std::string > selectedChanns) {m_selectedUnipolarChannels = selectedChanns;}
	virtual void setSelectedBipolarChannels(std::vector<std::string > selectedChanns) { m_selectedBipolarChannels = selectedChanns;}
	
	
	DateAndTime getDateAndTime() { return m_fileStartDateAndTime; }
	DateAndTime getStartDate() { return m_fileStartDateAndTime; };

	/*************************/



	virtual double getSamplingRate();
	virtual bool setSamplingRate(double samplingRate);
	virtual unsigned long getSampleCount();
	virtual bool setSampleCount(_int64 lSampleCount);

	std::string getPatientNameAndPath(void);

	// get annotations
	//virtual bool getAnnotations(std::vector< Annotation_c >& Annotations);

private:
	std::vector< std::pair< std::string, std::string> > m_Labels;	// channel labels
	bool getUnipolarAndBipolarChannels();
	enum sampleType_e { sampleType32bitInt, sampleType16bitInt, sampleType32bitFloat };

protected:
	bool readHeader();
	bool SearchItem(const std::string &Identifier, FILE* fp);
	bool SearchItem(const std::string &SectionIdentifier, const std::string &Identifier, FILE* fp);
	bool fillEventList(FILE* fp);


protected:
	std::vector<double>										m_Scaling;					// factor from int values to SI units
	int														m_nNumberTotalChannels;
	DateAndTime												m_StartDate;				// start date
	//std::vector< Annotation_c >							m_Annotations;				// annotations
	size_t m_SampleType;
};

#endif // __IOAbstractSignalImporter_c__H__
