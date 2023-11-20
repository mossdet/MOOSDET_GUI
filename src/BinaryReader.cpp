#include <fstream>
#include <sstream>
#include <algorithm>

#include "BinaryReader.h"


BinaryReader::BinaryReader(const std::string strFilename, std::vector<std::string > selectedUnipolarChanns, std::vector<std::string > selectedBipolarChanns, bool useScalpEEG):
	AbstractReader::AbstractReader(strFilename, selectedUnipolarChanns, selectedBipolarChanns, useScalpEEG) {
	// read header and fill necesarry members
	m_strFilename = strFilename;
	m_selectedUnipolarChannels = selectedUnipolarChanns;
	m_selectedBipolarChannels = selectedBipolarChanns;

	getPatientNameAndPath();

	m_bValidImporter = readHeader();
}

BinaryReader::~BinaryReader() {
}

// read the date
bool BinaryReader::ReadSamples(long long lStartSample, long long lNumberSamples, matrixStd& signalSamples)
{
	// check for valid importer
	if (!isValidImporter())
		return false;

	// check requested samples against available samples
	if (lStartSample + lNumberSamples > getSampleCount())
		lNumberSamples = getSampleCount() - lStartSample;

	if (signalSamples.empty()) {				//First read, load all selected channels to memory

		unsigned int nSignals = m_unipolarContacts.size();
		signalSamples.resize(nSignals);

		for (int signalNr = 0; signalNr < nSignals; ++signalNr) {

			std::string chName = m_unipolarContacts[signalNr].contactName;
			std::string filename = m_patientPath + m_patientName + "_EEGs\\" + chName + ".bin";

			std::ifstream is(filename, std::ifstream::binary);
			if (is) {
				// get length of file:
				std::fstream::pos_type start = is.tellg();
				is.seekg(0, is.end);
				std::fstream::pos_type end = is.tellg();
				size_t samplesInFile = ((size_t)(end - start)) / ((size_t)m_sizeOfDouble);
				is.seekg(0, is.beg);
				if (samplesInFile != getSampleCount() || samplesInFile < lNumberSamples)
					return false;

				int lengthToRead = m_sizeOfDouble * lNumberSamples;
				is.seekg(lStartSample * m_sizeOfDouble);

				char * buffer = new char[lNumberSamples * m_sizeOfDouble];
				is.read(buffer, lengthToRead);
				is.close();

				double* doubleBuff = (double*)buffer;

				double dScale = -1;// 1E-6;
				signalSamples[signalNr].resize(lNumberSamples);
#pragma omp parallel for
				for (long i = 0; i < lNumberSamples; ++i) {
					double val = doubleBuff[i] * dScale;
					signalSamples[signalNr][i] = val;
				}
				// ...buffer contains the entire file...
				delete[] buffer;
				delete[] doubleBuff;

			}
		}
	}

	return true;
}

bool BinaryReader::ReadReformattedSamples(long long lStartSample, long long lNumberSamples, matrixStd& reformattedSamples)
{
	// check for valid importer
	if (!isValidImporter())
		return false;

	// check requested samples against available samples
	if (lStartSample + lNumberSamples > getSampleCount())
		lNumberSamples = getSampleCount() - lStartSample;

	if (reformattedSamples.empty()) {				//First read, load all selected channels to memory

		unsigned int nSignals = m_montages.size();
		reformattedSamples.resize(nSignals);

		for (int signalNr = 0; signalNr < nSignals; ++signalNr) {

			std::string chName = m_montages[signalNr].montageName;
			std::string filename = m_patientPath  + m_patientName + "_EEGs\\"+ chName + ".bin";

			auto is = std::fstream(filename, std::ios::in | std::ios::binary);
			//std::ifstream is(filename, std::ifstream::binary);

			if (is) {
				// get length of file:
				std::fstream::pos_type start = is.tellg();
				is.seekg(0, is.end);
				std::fstream::pos_type end = is.tellg();
				size_t samplesInFile = ((size_t)(end - start)) / ((size_t)m_sizeOfDouble);
				is.seekg(0, is.beg);
				m_lSampleCount;
				if (samplesInFile != getSampleCount() || samplesInFile < lNumberSamples)
					return false;

				int lengthToRead = m_sizeOfDouble * lNumberSamples;
				is.seekg(lStartSample * m_sizeOfDouble);

				char * buffer = new char[lNumberSamples * m_sizeOfDouble];
				is.read(buffer, lengthToRead);
				is.close();

				double* doubleBuff = (double*)buffer;

				double dScale = -1;// 1E-6;
				reformattedSamples[signalNr].resize(lNumberSamples);
#pragma omp parallel for
				for (long i = 0; i < lNumberSamples; ++i) {
					double val = doubleBuff[i] * dScale;
					reformattedSamples[signalNr][i] = val;
				}
				// ...buffer contains the entire file...
				delete[] doubleBuff;

			}
		}
	}

	return true;
}

// read header
bool BinaryReader::readHeader() {

	std::ifstream headerFile;
	headerFile.open(getFileName().c_str());
	std::string firstLine;
	//Surname	GivenName	Day	Month	Year	Hour	Min	Sec	SR	NrSamples	DoubleSize
	std::getline(headerFile, firstLine);

	std::string surName, givenName;
	unsigned int day = 0, month = 0, year = 0, hours = 0, minutes = 0;
	double seconds = 0;
	double samplingRate = 0;
	long long conversionStartSample = 0;
	long long sampleCount = 0;
	int sizeOfDouble = 0;

	while (!headerFile.eof()) {
		std::string line;
		std::getline(headerFile, line);
		std::stringstream ss(line);

		ss >> surName;
		ss >> givenName;

		ss >> day;
		ss >> month;
		ss >> year;

		ss >> hours;
		ss >> minutes;
		ss >> seconds;

		ss >> samplingRate;
		ss >> conversionStartSample;
		ss >> sampleCount;
		ss >> sizeOfDouble;
	}
	headerFile.close();

	m_fileStartDateAndTime.day = day;
	m_fileStartDateAndTime.month = month;
	m_fileStartDateAndTime.year = year;
	m_fileStartDateAndTime.hours = hours;
	m_fileStartDateAndTime.minutes = minutes;
	m_fileStartDateAndTime.seconds = seconds;

	if (samplingRate == 0 || sampleCount == 0 || sizeOfDouble == 0)
		return false;

	if (!setSampleCount(sampleCount))
		return false;

	if (!setSamplingRate(samplingRate))
		return false;

	setSizeOfDouble(sizeOfDouble);

	std::vector<std::string> tenTwentyChannels = { "C3", "P3", "C4", "P4", "F3", "C3", "F4", "C4", "F7", "Sp1", "SP1", "F8", "Sp2", "SP2", "Fp1", "FP1", "F3", "F7", "Fp2", "FP2", "F4", "F8", "P3", "O1", "P4", "O2", "T3", "T1", "T2", "T4", "T3", "T5", "T4", "T6", "T5", "O1", "T6", "O2", "Pz", "Cz", "PZ", "CZ" , "FZ", "Fz" };
	std::vector<std::string> tenTwentyMontages = { "C3-P3", "C4-P4", "F3-C3", "F4-C4", "F7-Sp1", "F8-Sp2", "Fp1-F3", "Fp1-F7", "Fp2-F4", "Fp2-F8", "P3-O1", "P4-O2", "Sp1-T3", "Sp2-T4", "T3-T5", "T4-T6", "T5-O1", "T6-O2" };
	bool useTenTwentySystem = m_useScalpEEG;

	//Get Unipolar channel labels
	std::ifstream unipolarChannsFile;
	std::string unipolarChannelsFN = m_patientPath + m_patientName + "_chanelLabels.txt";
	unipolarChannsFile.open(unipolarChannelsFN.c_str());
	std::string firstLineUnip;
	std::getline(unipolarChannsFile, firstLineUnip);

	std::vector<ContactNames> unipolarContacts;
	while (!unipolarChannsFile.eof()) {
		std::string line;
		std::getline(unipolarChannsFile, line);
		std::stringstream ss(line);

		//Read file with Unipolar contacts
		ContactNames contact;
		std::string empty;
		ss >> contact.contactGlobalIdx;
		ss >> contact.electrodeName;
		ss >> empty;

		contact.contactNr = contact.contactGlobalIdx;
		contact.contactName = contact.electrodeName;

		if (!contact.contactName.empty())
			unipolarContacts.push_back(contact);
	}
	unipolarChannsFile.close();

	for (int n = 0; n < unipolarContacts.size(); ++n) {
		ContactNames contact;
		contact.contactGlobalIdx = n;
		contact.contactNr = unipolarContacts[n].contactNr;
		contact.electrodeName = unipolarContacts[n].electrodeName;
		contact.contactName = unipolarContacts[n].contactName;

		bool channelNotSelected = true;
		// if there are any selected channels, skip channels which are not selected
		if (!m_selectedUnipolarChannels.empty()) {
			std::string tempChannName = contact.contactName;
			for (int i = 0; i < m_selectedUnipolarChannels.size(); ++i) {
				std::string selectedChannName = m_selectedUnipolarChannels[i];
				selectedChannName.erase(std::remove_if(selectedChannName.begin(), selectedChannName.end(), ::isspace), selectedChannName.end());
				tempChannName.erase(std::remove_if(tempChannName.begin(), tempChannName.end(), ::isspace), tempChannName.end());

				selectedChannName.erase(std::remove(selectedChannName.begin(), selectedChannName.end(), '-'), selectedChannName.end());
				tempChannName.erase(std::remove(tempChannName.begin(), tempChannName.end(), '-'), tempChannName.end());

				selectedChannName.erase(std::remove(selectedChannName.begin(), selectedChannName.end(), '_'), selectedChannName.end());
				tempChannName.erase(std::remove(tempChannName.begin(), tempChannName.end(), '_'), tempChannName.end());

				selectedChannName.erase(std::remove(selectedChannName.begin(), selectedChannName.end(), '#'), selectedChannName.end());
				tempChannName.erase(std::remove(tempChannName.begin(), tempChannName.end(), '#'), tempChannName.end());

				int compareResult = selectedChannName.compare(tempChannName);
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


	// Get Bipolar channel labels
	std::ifstream bipolarChannsFile;
	std::string bipolarChannelsFN = m_patientPath + m_patientName + "_montageChannels.txt";
	bipolarChannsFile.open(bipolarChannelsFN.c_str());
	std::string firstLineBip;
	std::getline(bipolarChannsFile, firstLineBip);

	std::vector<MontageNames> bipolarContacts;
	while (!bipolarChannsFile.eof()) {
		std::string line;
		std::getline(bipolarChannsFile, line);
		std::stringstream ss(line);

		//Read file with Bipolar contacts

		MontageNames montage;
		ss >> montage.montageName;
		ss >> montage.montageMOSSDET_Nr;

		if (!montage.montageName.empty())
			bipolarContacts.push_back(montage);
	}
	bipolarChannsFile.close();

	for (int n = 0; n < bipolarContacts.size(); ++n) {
		int montageNr = 0;
		bool channelNotSelected = true;
		// if there are any selected channels, skip channels which are not selected
		if (!m_selectedBipolarChannels.empty()) {
			std::string tempChannName = bipolarContacts[n].montageName;

			for (int i = 0; i < m_selectedBipolarChannels.size(); ++i) {
				std::string selectedChannName = m_selectedBipolarChannels[i];
				selectedChannName.erase(std::remove_if(selectedChannName.begin(), selectedChannName.end(), ::isspace), selectedChannName.end());
				tempChannName.erase(std::remove_if(tempChannName.begin(), tempChannName.end(), ::isspace), tempChannName.end());

				selectedChannName.erase(std::remove(selectedChannName.begin(), selectedChannName.end(), '-'), selectedChannName.end());
				tempChannName.erase(std::remove(tempChannName.begin(), tempChannName.end(), '-'), tempChannName.end());

				selectedChannName.erase(std::remove(selectedChannName.begin(), selectedChannName.end(), '_'), selectedChannName.end());
				tempChannName.erase(std::remove(tempChannName.begin(), tempChannName.end(), '_'), tempChannName.end());

				selectedChannName.erase(std::remove(selectedChannName.begin(), selectedChannName.end(), '#'), selectedChannName.end());
				tempChannName.erase(std::remove(tempChannName.begin(), tempChannName.end(), '#'), tempChannName.end());

				int compareResult = selectedChannName.compare(tempChannName);
				if (compareResult == 0) {
					channelNotSelected = false;
					break;
				}
			}
		}
		else {
			channelNotSelected = false;
		}

		std::string channNameStr = bipolarContacts[n].montageName;
		bool ignoreChannel = (channNameStr.find("EKG") != std::string::npos) || (channNameStr.find("ekg") != std::string::npos) || (channNameStr.find("ECG") != std::string::npos) || (channNameStr.find("ecg") != std::string::npos);
		ignoreChannel = ignoreChannel || (channNameStr.find("igger") != std::string::npos) || (channNameStr.find("IGGER") != std::string::npos);
		ignoreChannel = ignoreChannel || (channNameStr.find("EMG") != std::string::npos) || (channNameStr.find("emg") != std::string::npos) || (channNameStr.find("EOG") != std::string::npos) || (channNameStr.find("eog") != std::string::npos);
		if (channelNotSelected || ignoreChannel) {
			continue;
		}
		bipolarContacts[n].montageMOSSDET_Nr = montageNr;
		m_montages.push_back(bipolarContacts[n]);
		montageNr++;
	}

	return true;
}

// valid importer
bool BinaryReader::isValidImporter() {
	double samplingRate = getSamplingRate();
	unsigned sampleCount = getSampleCount();
	unsigned nrChs = getNumberUnipolarChannels();
	// check sampling rate, sample count and number channels
	if (getSamplingRate() <= 0.0 || getSampleCount() == 0 || getNumberUnipolarChannels() == 0)
		m_bValidImporter = false;

	return m_bValidImporter;
};

double BinaryReader::getSamplingRate() {
	return m_dSamplingRate;
}

bool BinaryReader::setSamplingRate(double dSamplingRate) {
	m_dSamplingRate = dSamplingRate;

	if (m_dSamplingRate < 0) {
		m_bValidImporter = false;
		return false;
	}

	return true;
}

// sample count
unsigned long BinaryReader::getSampleCount() {
	return (unsigned long)m_lSampleCount;
}

bool BinaryReader::setSampleCount(_int64 lSampleCount) {
	m_lSampleCount = lSampleCount;

	if (m_lSampleCount == 0) {
		m_bValidImporter = false;
		return false;
	}

	return true;
}

// number of channels

unsigned int BinaryReader::getNumberUnipolarChannels() {
	return (unsigned int)m_unipolarContacts.size();
}

unsigned int BinaryReader::getNumberReformattedMontages() {
	return (unsigned int)m_montages.size();
}

// file name
std::string BinaryReader::getFileName() {
	return m_strFilename;
}



bool BinaryReader::readUnipolarLabels(std::vector<ContactNames> &unipolarContactNames) {
	if (m_unipolarContacts.size() == 0)
		return false;

	for (int i = 0; i < m_unipolarContacts.size(); ++i)
		unipolarContactNames.push_back(m_unipolarContacts[i]);

	return true;
}

bool BinaryReader::readMontageLabels(std::vector<MontageNames> &montageLabels) {
	if (m_montages.size() == 0)
		return false;

	for (int i = 0; i < m_montages.size(); ++i)
		montageLabels.push_back(m_montages[i]);

	return true;
}


bool BinaryReader::WriteReformattedSamples(long long lStartTime, long long lEndTime)
{
	return true;
}

std::string BinaryReader::getPatientNameAndPath(void) {
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