// 07.2019 DLP created

#include <fstream>
#include <sstream>
#include <algorithm>

#include "IO_CoherenceBinSignalImporter.h"

using namespace std;

#define ScanLength 1024
#define REM '#'


IO_CoherenceBinSignalImporter::IO_CoherenceBinSignalImporter(const std::string strFilename, size_t SampleType, std::vector<std::string > selectedUnipolarChanns, std::vector<std::string > selectedBipolarChanns, bool useScalpEEG) :
	AbstractReader::AbstractReader(strFilename, selectedUnipolarChanns, selectedBipolarChanns, useScalpEEG) {
	
	m_SampleType = sampleType16bitInt; //sampleType16bitInt, sampleType32bitInt, sampleType32bitFloat
	// read header and fill necesarry members
	m_strFilename = strFilename;
	m_selectedUnipolarChannels = selectedUnipolarChanns;
	m_selectedBipolarChannels = selectedBipolarChanns;

	getPatientNameAndPath();

	m_bValidImporter = readHeader();
}

IO_CoherenceBinSignalImporter::~IO_CoherenceBinSignalImporter()
{
}


bool IO_CoherenceBinSignalImporter::ReadSamples(long long lStartSample, long long lNumberSamples, matrixStd& signalSamples) {
	// check for valid importer
	if (!isValidImporter())
		return false;

	// check requested samples against available samples
	if (lStartSample + lNumberSamples > getSampleCount())
		return false;

	// open file and skip header
	const std::string &filename = getFileName();
	FILE* fp = fopen(filename.c_str(), _T("rb"));
	if (fp == NULL)
		return false;

	// position file pointer
	unsigned int nNumberTotChannels = m_nNumberTotalChannels;
	long lOffset = 0;

	switch (m_SampleType)
	{
	case sampleType16bitInt:
		lOffset = lStartSample * nNumberTotChannels * 2;
		break;
	case sampleType32bitInt:
		lOffset = lStartSample * nNumberTotChannels * 4;
		break;
	case sampleType32bitFloat:
		lOffset = lStartSample * nNumberTotChannels * 4;
		break;
	default:
		return false;
	}

	fseek(fp, lOffset, SEEK_SET);
	
	// allocate data matrix
	matrixStd tempSignalSamples;
	tempSignalSamples.resize(nNumberTotChannels);
	for (unsigned int nChannel = 0; nChannel < nNumberTotChannels; nChannel++) {
		tempSignalSamples[nChannel].resize(lNumberSamples);
	}

	// fill matrix with data
	double dScale = -1;// 1E-6;
	_int16 nValue = 0;
	__int32 n32Value = 0;
	float fValue = 0.0;
	for (unsigned long lSample = 0; lSample < lNumberSamples; lSample++) {
		for (unsigned int nChannel = 0; nChannel < nNumberTotChannels; nChannel++) {
			switch (m_SampleType) {
			case sampleType16bitInt:
				fread(&nValue, sizeof(_int16), 1, fp);
				tempSignalSamples[nChannel][lSample] = ((double)(nValue * m_Scaling[nChannel]))*dScale;
				break;
			case sampleType32bitInt:
				fread(&n32Value, sizeof(__int32), 1, fp);
				tempSignalSamples[nChannel][lSample] = ((double)(n32Value * m_Scaling[nChannel]))*dScale;
				break;
			case sampleType32bitFloat:
				fread(&fValue, sizeof(float), 1, fp);
				tempSignalSamples[nChannel][lSample] = ((double)(fValue * m_Scaling[nChannel]))*dScale;
				break;
			default:
				return false;
			}
		}
	}

	// close file pointer
	fclose(fp);

	// save the selected channels
	unsigned int nrSelectedChannels = m_unipolarContacts.size();
	signalSamples.resize(nrSelectedChannels);
	for (unsigned int selCh = 0; selCh < nrSelectedChannels; selCh++) {
		signalSamples[selCh].resize(lNumberSamples);
	}
	for (unsigned int selChIdx = 0; selChIdx < nrSelectedChannels; selChIdx++) {
		unsigned int globalChIdx = m_unipolarContacts[selChIdx].contactGlobalIdx;
		for (long long si = 0; si < lNumberSamples; si++) {
			signalSamples[selChIdx][si] = tempSignalSamples[globalChIdx][si];
		}
	}

	return true;
}

bool IO_CoherenceBinSignalImporter::ReadReformattedSamples(long long lStartSample, long long lNumberSamples, matrixStd& reformattedSamples) {

	//Read Unipolar samples
	// check for valid importer
	if (!isValidImporter())
		return false;

	// check requested samples against available samples
	if (lStartSample + lNumberSamples > getSampleCount())
		return false;

	// open file and skip header
	const std::string &filename = getFileName();
	FILE* fp = fopen(filename.c_str(), _T("rb"));
	if (fp == NULL)
		return false;

	// position file pointer
	unsigned int nNumberTotChannels = m_nNumberTotalChannels;
	long lOffset = 0;

	switch (m_SampleType)
	{
	case sampleType16bitInt:
		lOffset = lStartSample * nNumberTotChannels * 2;
		break;
	case sampleType32bitInt:
		lOffset = lStartSample * nNumberTotChannels * 4;
		break;
	case sampleType32bitFloat:
		lOffset = lStartSample * nNumberTotChannels * 4;
		break;
	default:
		return false;
	}

	fseek(fp, lOffset, SEEK_SET);

	// allocate data matrix
	matrixStd tempSignalSamples;
	tempSignalSamples.resize(nNumberTotChannels);
	for (unsigned int nChannel = 0; nChannel < nNumberTotChannels; nChannel++) {
		tempSignalSamples[nChannel].resize(lNumberSamples);
	}

	// fill matrix with data
	double dScale = -1;// 1E-6;
	_int16 nValue = 0;
	__int32 n32Value = 0;
	float fValue = 0.0;
	for (unsigned long lSample = 0; lSample < lNumberSamples; lSample++) {
		for (unsigned int nChannel = 0; nChannel < nNumberTotChannels; nChannel++) {
			switch (m_SampleType) {
			case sampleType16bitInt:
				fread(&nValue, sizeof(_int16), 1, fp);
				tempSignalSamples[nChannel][lSample] = ((double)(nValue * m_Scaling[nChannel]))*dScale;
				break;
			case sampleType32bitInt:
				fread(&n32Value, sizeof(__int32), 1, fp);
				tempSignalSamples[nChannel][lSample] = ((double)(n32Value * m_Scaling[nChannel]))*dScale;
				break;
			case sampleType32bitFloat:
				fread(&fValue, sizeof(float), 1, fp);
				tempSignalSamples[nChannel][lSample] = ((double)(fValue * m_Scaling[nChannel]))*dScale;
				break;
			default:
				return false;
			}
		}
	}

	// close file pointer
	fclose(fp);


	/**********Read Sampels from Montages************/
	int nrMontages = m_montages.size();

	//Allocate data matrix
	reformattedSamples.resize(nrMontages);
	for (int mi = 0; mi < nrMontages; mi++) {
		reformattedSamples[mi].resize(lNumberSamples);
	}

	for (int mi = 0; mi < nrMontages; mi++) {
		for (unsigned long lSample = 0; lSample < lNumberSamples; lSample++) {
			double val = (double)(tempSignalSamples[m_montages[mi].firstContactGlobalIdx][lSample] - tempSignalSamples[m_montages[mi].secondContactGlobalIdx][lSample]);
			reformattedSamples[mi][lSample] = val;
		}
	}

	return true;
}

bool IO_CoherenceBinSignalImporter::WriteReformattedSamples(long long lStartTime, long long lEndTime) {
	return true;
}

bool IO_CoherenceBinSignalImporter::isValidImporter() {
	double samplingRate = getSamplingRate();
	unsigned sampleCount = getSampleCount();
	unsigned nrChs = getNumberUnipolarChannels();
	// check sampling rate, sample count and number channels
	if (getSamplingRate() <= 0.0 || getSampleCount() == 0 || getNumberUnipolarChannels() == 0)
		m_bValidImporter = false;

	return m_bValidImporter;
}


bool IO_CoherenceBinSignalImporter::readUnipolarLabels(std::vector<ContactNames> &unipolarContactNames) {

	if (m_unipolarContacts.size() == 0)
		return false;

	for (int i = 0; i < m_unipolarContacts.size(); ++i)
		unipolarContactNames.push_back(m_unipolarContacts[i]);

	return true;
}

bool IO_CoherenceBinSignalImporter::readMontageLabels(std::vector<MontageNames> &montageLabels) {
	if (m_montages.size() == 0)
		return false;

	for (int i = 0; i < m_montages.size(); ++i)
		montageLabels.push_back(m_montages[i]);

	return true;
}

// read header
bool IO_CoherenceBinSignalImporter::readHeader()
{
	
	// get .txt filename from .bin filename
	std::string strTxtFileName = getFileName();
	int dot_index = strTxtFileName.rfind('.');
	if (dot_index == -1) dot_index = strTxtFileName.size();
	std::string baseName = strTxtFileName.substr(0, dot_index);
	strTxtFileName = baseName + _T(".txt");

	// open file and skip header
	FILE* fp = fopen(strTxtFileName.c_str(), _T("rt"));
	if (fp == NULL)
		return false;

	// some helpers for file parsing
	char        *Word;
	Word = (char *)malloc(ScanLength);
	std::string f;
	f = "%s";

	// look for number of channels	
	std::string searchString = _T("NbOfChannels=");
	SearchItem(searchString, fp);
	if (fscanf(fp, f.c_str(), Word) < 1) return false;

	// strip identifier
	std::string strLineString(Word);
	std::string strNbOfChannels = strLineString.substr(searchString.length());

	m_nNumberTotalChannels = atoi(strNbOfChannels.c_str());
	if (m_nNumberTotalChannels < 1)
		return false;

	// look for number of samples
	long lSampleCount = 0;
	searchString = _T("DurationInSamples=");
	SearchItem(searchString, fp);
	if (fscanf(fp, f.c_str(), Word) < 1) return false;

	// strip identifier
	strLineString = Word;
	std::string strlSampleCount = strLineString.substr(searchString.length());

	lSampleCount = atol(strlSampleCount.c_str());
	if (lSampleCount < 1)
		return false;

	setSampleCount(lSampleCount);

	// look for sampling rate
	long lSamplingRate = 0;
	searchString = _T("Sampling=");
	SearchItem(searchString, fp);
	if (fscanf(fp, f.c_str(), Word) < 1) return false;

	// strip identifier
	strLineString = Word;
	std::string strlSamplingRate = strLineString.substr(searchString.length());

	lSamplingRate = atol(strlSamplingRate.c_str());
	if (lSamplingRate < 1)
		return false;

	setSamplingRate(double(lSamplingRate));

	// read channel labels
	searchString = _T("Channels=");
	SearchItem(searchString, fp);
	if (fscanf(fp, f.c_str(), Word) < 1) return false;

	// strip identifier
	strLineString = Word;
	std::string strLabelString = strLineString.substr(searchString.length());

	// loop over channels
	size_t pos = 0;
	size_t length = 0;
	m_Labels.resize(m_nNumberTotalChannels);
	for (unsigned int nLabel = 0; nLabel < m_nNumberTotalChannels; nLabel++)
	{
		// find separator ","
		length = strLabelString.find(",");
		std::string strLabel = strLabelString.substr(pos, length);
		m_Labels.at(nLabel).first = strLabel;

		// reduce string with labels
		strLabelString = strLabelString.substr(length + 1);

		// add ref label
		m_Labels.at(nLabel).second = _T("Ref");
	}

	// read scaling factors
	searchString = _T("Gainx1000=");
	SearchItem(searchString, fp);
	if (fscanf(fp, f.c_str(), Word) < 1) return false;

	// strip identifier
	strLineString = Word;
	std::string strScalingString = strLineString.substr(searchString.length());

	// loop over channels
	m_Scaling.resize(m_nNumberTotalChannels);
	for (unsigned int nChannel = 0; nChannel < m_nNumberTotalChannels; nChannel++)
	{
		// find separator ","
		length = strScalingString.find(",");
		std::string strScaling = strScalingString.substr(pos, length);
		m_Scaling[nChannel] = atof(strScaling.c_str())*1e-3;

		// reduce string with labels
		strScalingString = strScalingString.substr(length + 1);
	}

	// adapt scaling factors to unit
	searchString = _T("unit=");
	SearchItem(searchString, fp);
	if (fscanf(fp, f.c_str(), Word) < 1) return false;

	// strip identifier
	strLineString = Word;
	std::string strUNITString = strLineString.substr(searchString.length());

	// loop over channels
	for (unsigned int nChannel = 0; nChannel < m_nNumberTotalChannels; nChannel++)
	{
		// find separator ","
		length = strUNITString.find(",");
		std::string strUNIT = strUNITString.substr(pos, length);
		std::string strUV1 = _T("µV");
		std::string strUV2 = _T("uV");
		std::string strMV = _T("mV");

		if (strUNIT == strUV1 || strUNIT == strUV2)
			m_Scaling[nChannel] = m_Scaling[nChannel] * 1e-6;

		if (strUNIT == strMV)
			m_Scaling[nChannel] = m_Scaling[nChannel] * 1e-3;

		// reduce string with labels
		strUNITString = strUNITString.substr(length + 1);
	}

	// start date and start time
	std::string strSectionString = _T("[SEQUENCE]");
	searchString = _T("Date=");
	SearchItem(strSectionString, searchString, fp);
	if (fscanf(fp, f.c_str(), Word) < 1) return false;

	// strip identifier
	strLineString = Word;
	std::string strDateString = strLineString.substr(searchString.length());

	// decompose date
	std::string strDay = strDateString.substr(0, 2);
	std::string strMonth = strDateString.substr(3, 2);
	std::string strYear = strDateString.substr(6, 4);

	unsigned int year = atoi(strYear.c_str());
	unsigned int month = atoi(strMonth.c_str());
	unsigned int day = atoi(strDay.c_str());

	m_fileStartDateAndTime.day = day;
	m_fileStartDateAndTime.month = month;
	m_fileStartDateAndTime.year = year;

	searchString = _T("Time=");
	SearchItem(strSectionString, searchString, fp);
	if (fscanf(fp, f.c_str(), Word) < 1) return false;

	// strip identifier
	strLineString = Word;
	std::string strTimeString = strLineString.substr(searchString.length());

	std::string strHour = strTimeString.substr(0, 2);
	std::string strMin = strTimeString.substr(3, 2);
	std::string strSec = strTimeString.substr(6, 2);

	unsigned int hour = atoi(strHour.c_str());
	unsigned int min = atoi(strMin.c_str());
	double		 sec = atof(strSec.c_str());

	m_fileStartDateAndTime.hours = hour;
	m_fileStartDateAndTime.minutes = min;
	m_fileStartDateAndTime.seconds = sec;

	// fill event list
	fillEventList(fp);

	// free pointers and close file
	free(Word);

	fclose(fp);

	getUnipolarAndBipolarChannels();

	// header is valid
	return true;
}


// helper needed to parse .txt file
bool IO_CoherenceBinSignalImporter::SearchItem(const std::string &Identifier, FILE* fp)
{
	// check file pointer
	if (fp == NULL)
		return false;

	char    Word[ScanLength];
	long	pos;

	// get length of search string
	int nStringLength = Identifier.length();

	fseek(fp, 0, SEEK_SET);
	while (((pos = ftell(fp)) || 1)) {
		char * Line = new char[ScanLength];
		bool readOk = fgets(Line, ScanLength, fp);
		
		if (!readOk)
			break;

		if ((Line[0] == REM) || ((Line[0] == '/') && (Line[1] == '/'))) 
			continue;

		if (sscanf(Line, "%s", Word) < 0) 
			continue;

		if (!strncmp(Word, Identifier.c_str(), nStringLength)) {
			fseek(fp, pos, SEEK_SET);      // push last line back to buffer
			return true;
		}

		delete[] Line;
	}

	return false;
}

// helper needed to parse .txt file
bool IO_CoherenceBinSignalImporter::SearchItem(const std::string &SectionIdentifier, const std::string &Identifier, FILE* fp)
{
	// check file pointer
	if (fp == NULL)
		return false;

	char    Word[ScanLength];
	long	pos;

	// get length of search and section string
	int nStringLength = Identifier.length();
	int nSectionStringLength = SectionIdentifier.length();

	fseek(fp, 0, SEEK_SET);
	bool bSectionFound = false;
	while (((pos = ftell(fp)) || 1)) {
		char * Line = new char[ScanLength];
		bool readOk = (fgets(Line, ScanLength, fp));

		if (!readOk)
			break;

		if ((Line[0] == REM) || ((Line[0] == '/') && (Line[1] == '/'))) continue;
		if (sscanf(Line, "%s", Word) < 0) continue;
		if (!strncmp(Word, SectionIdentifier.c_str(), nSectionStringLength))
		{
			bSectionFound = true; // section Identifier found
		}
		if (!strncmp(Word, Identifier.c_str(), nStringLength) && bSectionFound)
		{
			fseek(fp, pos, SEEK_SET);      // push last line back to buffer
			return true;
		}

		delete[] Line;
	}
	return false;
}

bool IO_CoherenceBinSignalImporter::fillEventList(FILE* fp){
	return true;
}

bool IO_CoherenceBinSignalImporter::getUnipolarAndBipolarChannels() {

	std::vector<std::string> tenTwentyChannels = { "C3", "P3", "C4", "P4", "F3", "C3", "F4", "C4", "F7", "Sp1", "SP1", "F8", "Sp2", "SP2", "Fp1", "FP1", "F3", "F7", "Fp2", "FP2", "F4", "F8", "P3", "O1", "P4", "O2", "T3", "T1", "T2", "T4", "T3", "T5", "T4", "T6", "T5", "O1", "T6", "O2", "Pz", "Cz", "PZ", "CZ", "FZ", "Fz" };
	std::vector<std::string> tenTwentyMontages = { "C3-P3", "C4-P4", "F3-C3", "F4-C4", "F7-Sp1", "F8-Sp2", "Fp1-F3", "Fp1-F7", "Fp2-F4", "Fp2-F8", "P3-O1", "P4-O2", "Sp1-T3", "Sp2-T4", "T3-T5", "T4-T6", "T5-O1", "T6-O2" };
	bool useTenTwentySystem = m_useScalpEEG;

	
	int nrChanns = m_nNumberTotalChannels;
	//Get Unipolar Channel Names
	for (unsigned int n = 0; n < nrChanns; n++) {

		std::string strLabel = m_Labels[n].first;
		std::string electrodeName, contactNrString;

		//Get contact number from the channel label
		std::string numbers[10] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
		std::string::size_type minIdx = 1000000, maxIdx = 0, length = 0;
		for (int nrIdx = 0; nrIdx < 10; nrIdx++) {
			std::string::size_type fi = strLabel.find(numbers[nrIdx]);
			std::string::size_type si = strLabel.rfind(numbers[nrIdx]);

			if (fi != std::string::npos) {
				if (fi < minIdx) {
					minIdx = fi;
				}
			}

			if (si != std::string::npos) {
				if (si > maxIdx) {
					maxIdx = si;
				}
			}
		}

		int contactNr;
		if (minIdx != 1000000) {
			length = (maxIdx - minIdx) + 1;
			electrodeName = strLabel.substr(0, minIdx);
			contactNrString = strLabel.substr(minIdx, length);
			std::stringstream ss(contactNrString);
			ss >> contactNr;
		}
		else {
			electrodeName = strLabel;
			contactNr = 0;
		}

		ContactNames contact;
		contact.contactGlobalIdx = n;
		contact.contactNr = contactNr;
		contact.electrodeName = electrodeName;
		contact.contactName = strLabel;

		bool channelNotSelected = true;
		// if there are any selected channels, skip channels which are not selected
		if (!m_selectedUnipolarChannels.empty()) {
			std::string edfChannName = contact.contactName;
			for (int i = 0; i < m_selectedUnipolarChannels.size(); ++i) {
				std::string selectedChannName = m_selectedUnipolarChannels[i];
				selectedChannName.erase(std::remove_if(selectedChannName.begin(), selectedChannName.end(), ::isspace), selectedChannName.end());
				edfChannName.erase(std::remove_if(edfChannName.begin(), edfChannName.end(), ::isspace), edfChannName.end());
				selectedChannName.erase(std::remove(selectedChannName.begin(), selectedChannName.end(), '-'), selectedChannName.end());
				edfChannName.erase(std::remove(edfChannName.begin(), edfChannName.end(), '-'), edfChannName.end());
				selectedChannName.erase(std::remove(selectedChannName.begin(), selectedChannName.end(), '_'), selectedChannName.end());
				edfChannName.erase(std::remove(edfChannName.begin(), edfChannName.end(), '_'), edfChannName.end());
				selectedChannName.erase(std::remove(selectedChannName.begin(), selectedChannName.end(), '#'), selectedChannName.end());
				edfChannName.erase(std::remove(edfChannName.begin(), edfChannName.end(), '#'), edfChannName.end());

				int compareResult = selectedChannName.compare(edfChannName);
				if (compareResult == 0) {
					channelNotSelected = false;
					break;
				}
			}
		}
		else {
			channelNotSelected = false;
		}
		std::string channNameStr = contact.contactName;
		bool ignoreTenTwentyChannels = (!useTenTwentySystem && (std::find(tenTwentyChannels.begin(), tenTwentyChannels.end(), channNameStr) != tenTwentyChannels.end()));
		bool ignoreChannel = (channNameStr.find("EKG") != std::string::npos) || (channNameStr.find("ekg") != std::string::npos) || (channNameStr.find("ECG") != std::string::npos) || (channNameStr.find("ecg") != std::string::npos);
		ignoreChannel = ignoreChannel || (channNameStr.find("igger") != std::string::npos) || (channNameStr.find("IGGER") != std::string::npos);
		ignoreChannel = ignoreChannel || (channNameStr.find("EMG") != std::string::npos) || (channNameStr.find("emg") != std::string::npos) || (channNameStr.find("EOG") != std::string::npos) || (channNameStr.find("eog") != std::string::npos);
		if (channelNotSelected || ignoreChannel || ignoreTenTwentyChannels) {
			continue;
		}

		m_unipolarContacts.push_back(contact);
	}

	int montageNr = 0;

	//read Montages' Names
	if (useTenTwentySystem == true) {
		for (int channIdx = 0; channIdx < m_unipolarContacts.size() - 1; ++channIdx) {
			MontageNames montage;
			montage.firstElectrodeName = m_unipolarContacts[channIdx].electrodeName;
			montage.firstContactNr = m_unipolarContacts[channIdx].contactNr;
			montage.firstContactGlobalIdx = m_unipolarContacts[channIdx].contactGlobalIdx;
			bool validTenTwentyMontage = false;
			for (int channIdxTwo = 0; channIdxTwo < m_unipolarContacts.size() - 1; ++channIdxTwo) {

				if (channIdx == channIdxTwo) { continue; }

				montage.secondElectrodeName = m_unipolarContacts[channIdxTwo].electrodeName;
				montage.secondContactNr = m_unipolarContacts[channIdxTwo].contactNr;
				montage.secondContactGlobalIdx = m_unipolarContacts[channIdxTwo].contactGlobalIdx;
				montage.montageName = montage.firstElectrodeName + std::to_string(montage.firstContactNr) + "-" + montage.secondElectrodeName + std::to_string(montage.secondContactNr);

				if (std::find(tenTwentyMontages.begin(), tenTwentyMontages.end(), montage.montageName) != tenTwentyMontages.end()) {
					validTenTwentyMontage = true;
					break;
				}
			}
			montage.montageMOSSDET_Nr = montageNr;

			std::string channNameStr = montage.montageName;
			bool ignoreChannel = (channNameStr.find("EKG") != std::string::npos) || (channNameStr.find("ekg") != std::string::npos) || (channNameStr.find("ECG") != std::string::npos) || (channNameStr.find("ecg") != std::string::npos);
			ignoreChannel = ignoreChannel || (channNameStr.find("igger") != std::string::npos) || (channNameStr.find("IGGER") != std::string::npos);
			ignoreChannel = ignoreChannel || (channNameStr.find("EMG") != std::string::npos) || (channNameStr.find("emg") != std::string::npos) || (channNameStr.find("EOG") != std::string::npos) || (channNameStr.find("eog") != std::string::npos);
			if (!validTenTwentyMontage) {
				continue;
			}
			m_montages.push_back(montage);
			montageNr++;
		}
	}
	else {
		for (int channIdx = 0; channIdx < m_unipolarContacts.size() - 1; ++channIdx) {
			MontageNames montage;
			montage.firstElectrodeName = m_unipolarContacts[channIdx].electrodeName;
			montage.firstContactNr = m_unipolarContacts[channIdx].contactNr;
			montage.firstContactGlobalIdx = m_unipolarContacts[channIdx].contactGlobalIdx;

			montage.secondElectrodeName = m_unipolarContacts[channIdx + 1].electrodeName;
			montage.secondContactNr = m_unipolarContacts[channIdx + 1].contactNr;
			montage.secondContactGlobalIdx = m_unipolarContacts[channIdx + 1].contactGlobalIdx;

			montage.montageName = m_unipolarContacts[channIdx].electrodeName + std::to_string(m_unipolarContacts[channIdx].contactNr) + "-" + m_unipolarContacts[channIdx + 1].electrodeName + std::to_string(m_unipolarContacts[channIdx + 1].contactNr);

			montage.montageMOSSDET_Nr = montageNr;

			bool channelNotSelected = true;
			if (!m_selectedBipolarChannels.empty()) {
				std::string edfChannName = montage.montageName;
				for (int i = 0; i < m_selectedBipolarChannels.size(); ++i) {
					std::string selectedChannName = m_selectedBipolarChannels[i];
					selectedChannName.erase(std::remove_if(selectedChannName.begin(), selectedChannName.end(), ::isspace), selectedChannName.end());
					edfChannName.erase(std::remove_if(edfChannName.begin(), edfChannName.end(), ::isspace), edfChannName.end());
					selectedChannName.erase(std::remove(selectedChannName.begin(), selectedChannName.end(), '-'), selectedChannName.end());
					edfChannName.erase(std::remove(edfChannName.begin(), edfChannName.end(), '-'), edfChannName.end());
					selectedChannName.erase(std::remove(selectedChannName.begin(), selectedChannName.end(), '_'), selectedChannName.end());
					edfChannName.erase(std::remove(edfChannName.begin(), edfChannName.end(), '_'), edfChannName.end());
					selectedChannName.erase(std::remove(selectedChannName.begin(), selectedChannName.end(), '#'), selectedChannName.end());
					edfChannName.erase(std::remove(edfChannName.begin(), edfChannName.end(), '#'), edfChannName.end());
					int compareResult = selectedChannName.compare(edfChannName);
					if (compareResult == 0) {
						channelNotSelected = false;
						break;
					}
				}
			}
			else {
				channelNotSelected = false;
			}

			std::string channNameStr = montage.montageName;
			bool ignoreChannel = (channNameStr.find("EKG") != std::string::npos) || (channNameStr.find("ekg") != std::string::npos) || (channNameStr.find("ECG") != std::string::npos) || (channNameStr.find("ecg") != std::string::npos);
			ignoreChannel = ignoreChannel || (channNameStr.find("igger") != std::string::npos) || (channNameStr.find("IGGER") != std::string::npos);
			ignoreChannel = ignoreChannel || (channNameStr.find("EMG") != std::string::npos) || (channNameStr.find("emg") != std::string::npos) || (channNameStr.find("EOG") != std::string::npos) || (channNameStr.find("eog") != std::string::npos);
			if (channelNotSelected || ignoreChannel) {
				continue;
			}

			if (montage.firstElectrodeName.compare(montage.secondElectrodeName) == 0) {
				m_montages.push_back(montage);
				montageNr++;
			}

		}
	}

	return true;
}

double IO_CoherenceBinSignalImporter::getSamplingRate() {
	return m_dSamplingRate;
}

bool IO_CoherenceBinSignalImporter::setSamplingRate(double dSamplingRate) {
	m_dSamplingRate = dSamplingRate;

	if (m_dSamplingRate < 0) {
		m_bValidImporter = false;
		return false;
	}

	return true;
}

// sample count
unsigned long IO_CoherenceBinSignalImporter::getSampleCount() {
	return (unsigned long)m_lSampleCount;
}

bool IO_CoherenceBinSignalImporter::setSampleCount(_int64 lSampleCount) {
	m_lSampleCount = lSampleCount;

	if (m_lSampleCount == 0) {
		m_bValidImporter = false;
		return false;
	}

	return true;
}

std::string IO_CoherenceBinSignalImporter::getPatientNameAndPath(void) {
	for (int i = m_strFilename.length() - 1; i >= 0; i--) {
		if (m_strFilename[i] == 92) {
			m_patientName = m_strFilename.substr(i + 1, m_strFilename.length() - i);
			m_patientPath = m_strFilename.substr(0, i + 1);
			break;
		}
	}

	m_patientName.pop_back(); m_patientName.pop_back(); m_patientName.pop_back(); m_patientName.pop_back();

	return m_patientName;
}