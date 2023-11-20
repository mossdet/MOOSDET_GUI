// 06.08.2018 created by Daniel Lachner-Piza


#include "IO_ProfusionImporter_c.h"
#include "windows.h" 

IO_ProFusionImporter_c::IO_ProFusionImporter_c(std::string strFilename, std::string strMontageFilename, bool bHasTriggerChannel, int nTriggerChannelIndex)
{
	m_strFilename = strFilename;
	m_strMontageFilename = strMontageFilename;
	m_bHasTriggerChannel = false;
	m_nTriggerChannelIndex = -1;

	/*CoInitialize(NULL);

	if (m_StudyPtr.CreateInstance(__uuidof(CMEEGStudyV4Lib::Study), NULL, CLSCTX_INPROC_SERVER) != S_OK)
	{
		m_bValidImporter = false;
		return;
	}

	if (m_StudyPtr == NULL)
	{
		m_bValidImporter = false;
		return;
	}

	if (m_StudyPtr->GetOpened())
	{
		m_bValidImporter = false;
		return;
	}

	try
	{
		// calls through smart pointer will throw _com_error on com failure
		//get file name
		_bstr_t bstrFileName(_T(m_strFilename.c_str()));


		// open study
		m_StudyPtr->Open(bstrFileName, VARIANT_TRUE);
		m_DataReaderPtr = m_StudyPtr->CreateDataReader();

		// connect to event source for getting data events
		m_DataReaderPtr->Enable();
	}
	catch (_com_error)
	{
		m_bValidImporter = false;
		return;
	}*/

	m_bValidImporter = getRecordingInformation();
}

IO_ProFusionImporter_c::~IO_ProFusionImporter_c()
{
}

bool IO_ProFusionImporter_c::getRecordingInformation()
{
	
	/*
	// get sampling rate
	long m_samplingRate = m_StudyPtr->GetEEGSampleRate();

	CMEEGStudyV4Lib::IChannelsPtr channelsPtr = m_StudyPtr->GetChannels();
	long nNumChannels = channelsPtr->GetCount();

	// get sample count
	// add samples from all segments
	// Need to Reload Data Segments 
	m_StudyPtr->ReloadDataSegments();
	CMEEGStudyV4Lib::IEEGDataSegmentsPtr segmentsPtr = m_StudyPtr->GetDataSegments();

	int nNumSegments = segmentsPtr->GetCount();
	if (nNumSegments < 1)
		return false;
	*/

	/*Date_c startDate;
	if (!getStartDate(startDate))
		return false;*/

	/*
	// allocate section sample and start date information
	m_SectionSampleInfos.resize(nNumSegments);
	m_sectionStarts.resize(nNumSegments);

	m_sectionStarts.at(0) = startDate;


	// loop over sections
	long lSampleCount = 0;
	for (int nSeg = 0; nSeg < nNumSegments; nSeg++)
	{
		CMEEGStudyV4Lib::IEEGDataSegmentPtr segmentPtr = segmentsPtr->GetItem(nSeg + 1);
		LONGLONG llStart = segmentPtr->GetSampleStart();
		LONGLONG llDuration = segmentPtr->GetSampleDuration();

		m_SectionSampleInfos.at(nSeg).first = llStart;
		m_SectionSampleInfos.at(nSeg).second = llDuration;

		if (nSeg > 0)
		{
			const double dBillion = 1000000000;
			double dRelSegmentStartTime = segmentPtr->GetTime();
			//double dTimeSec = (double)dRelSegmentStartTime/dBillion;
			Date_c segemntStartDate = startDate;
			segemntStartDate.addDouble(dRelSegmentStartTime);
			m_sectionStarts.at(nSeg) = segemntStartDate;
		}
		else
		{
			if (!getStartDate(startDate))
				;// return false;

			m_sectionStarts.at(0) = startDate;
		}

		lSampleCount += llDuration;
	}


	setSampleCount(lSampleCount);

	// get channels
	CMEEGStudyV4Lib::IChannelsPtr channelsPtr = m_StudyPtr->GetChannels();
	CMEEGStudyV4Lib::IChannel* channelPtr = NULL;

	long nNumChannels = channelsPtr->GetCount();
	m_Labels.resize(nNumChannels);
	std::string strTriggerName1(_T("tr"));
	std::string strTriggerName2(_T("Tr"));
	std::string strTriggerName3(_T("Trigger"));
	std::string strTriggerName4(_T("TRIGGER"));
	std::string strTriggerName5(_T("trigger"));

	for (long nChan = 0; nChan < nNumChannels; nChan++)
	{
		channelPtr = channelsPtr->GetItem(nChan + 1);
		BSTR pStrName = channelPtr->GetName();
		_bstr_t bstrN = pStrName;
		std::string strActiveName = bstrN;
		std::string strRefName = _T("Ref");

		// replace blanks in name
		int blank_index = strActiveName.rfind(" ");
		if (blank_index > -1)
			strActiveName.replace(strActiveName.rfind(" "), 1, "_");
		m_Labels.at(nChan).first = strActiveName;
		m_Labels.at(nChan).second = strRefName;
		long channeltype = channelPtr->GetType();
		BSTR pStrChannleType = channelPtr->GetUnitOfMeasure();
		_bstr_t bstrChannleType = pStrChannleType;
		std::string strChannleType = bstrChannleType;
		BSTR pStrUnit = channelPtr->GetUnitOfMeasure();
		_bstr_t bstrUnit = pStrUnit;
		std::string strUnit = bstrUnit;
		double dSensitivity = channelPtr->GetSensitivity();
		if (strActiveName == strTriggerName1 || strActiveName == strTriggerName2 || strActiveName == strTriggerName3 || strActiveName == strTriggerName4 || strActiveName == strTriggerName5)
		{
			m_bHasTriggerChannel = true;
			m_nTriggerChannelIndex = nChan;
		}
	}*/

	return true;
}
