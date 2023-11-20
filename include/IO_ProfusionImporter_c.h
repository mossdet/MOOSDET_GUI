#pragma once

// 06.08.2018 Matthias D. and Danie LP created

#ifndef __IO_ProfusionImporter_c_H__
#define __IO_ProfusionImporter_c_H__

#include <vector>
#include <omp.h>
#include <sstream>

#include "DataTypes.h"
//#import "..\..\..\ProFusionCommonComponents\CMEEGStudyV4.dll"


class  IO_ProFusionImporter_c{

public:
	IO_ProFusionImporter_c(std::string strFilename, std::string strMontageFilename, bool bHasTriggerChannel, int nTriggerChannelIndex);
	virtual ~IO_ProFusionImporter_c();

	
public:
	// public method to get data
	//virtual bool ReadSamples(unsigned long lStartSample, unsigned long lNumberSamples, matrixStd signalSamples);

	// public method to get channel labels
	/*virtual bool getLabels(std::vector< std::pair< std::string, std::string> >& Labels) const;

	// and also reformatting labels, in case exist
	virtual bool getReformattingLabels(std::vector< std::pair< std::string, std::string> >& Labels) const;

	// get Annotations
	//virtual bool getAnnotations(std::vector< Annotation_c >& Annotations);

	// trigger
	//virtual bool getTrigger(utMatrix_t<long>& outTrigger);

	// start date
	//virtual bool getStartDate(Date_c& startDate) const;

	// segements
	virtual int getNumberSegments() const;

	virtual bool getSectionSampleInfo(std::vector< std::pair< long, long> > SectionSampleInfo) const;

	// events
	//virtual bool getEvents(utMatrix_t<double>& outEvents);
	virtual bool getEventDescriptions(std::vector< std::string > & Descriptions);
	//virtual bool getEventsForDescription(std::string Description, utMatrix_t<double>& outEvents);
	*/

protected:
	bool getRecordingInformation();

	// date handling
	//bool DateToSystemTime(SYSTEMTIME& st, DATE date) const;

	// adapt start date to first section
	//bool adaptStartDate(Date_c& startDate, double dOffset) const;

	/*long getSampleCountForTimeSpan(double dTimeSpan) const;
	long getSampleForTimeFromStart(double dTime) const;

	// trigger channel
	bool hasTriggerChannel() const;
	int  getTriggerChannelIndex() const;

	// get annotations from sync file, aligned to trigge
	//bool getAnnotationsFromSyncFile(std::vector< Annotation_c >& Annotations);
	void str_replace(std::string &s, const std::string &search, const std::string &replace);*/

protected:
	// smart pointer to data access eeg study object
	//CMEEGStudyV4Lib::IEEGStudyPtr m_StudyPtr;
	// handy data reader
	//CMEEGStudyV4Lib::IDataReaderPtr m_DataReaderPtr;

	bool m_bValidImporter;
	std::string m_strFilename;
	std::string m_strMontageFilename;

	std::vector< std::pair< std::string, std::string> > m_Labels;				// channel labels
	std::vector< std::pair< long, long> >				m_SectionSampleInfos;	// section start and durations in samples
	//std::vector< Date_c >								m_sectionStarts;		// start dates of sections
	bool												m_bHasTriggerChannel;	// flag for existing trigger channel
	int													m_nTriggerChannelIndex;	// index of trigger channel
	std::vector< EOI_Event >							m_Annotations;

	double m_samplingRate;
};

#endif // __IO_ProfusionImporter_c_H__
