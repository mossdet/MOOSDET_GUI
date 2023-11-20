#include <fstream>
#include <sstream>
#include <algorithm>

#include "BrandtBinaryReader.h"


BrandtBinaryReader::BrandtBinaryReader(const std::string strFilename, std::vector<std::string > selectedUnipolarChanns, std::vector<std::string > selectedBipolarChanns, bool useScalpEEG):
	AbstractReader::AbstractReader(strFilename, selectedUnipolarChanns, selectedBipolarChanns, useScalpEEG) {
	// read header and fill necesarry members
	m_strFilename = strFilename;
	m_selectedUnipolarChannels = selectedUnipolarChanns;
	m_selectedBipolarChannels = selectedBipolarChanns;

	getPatientNameAndPath();

	m_bValidImporter = readHeader();
}

BrandtBinaryReader::~BrandtBinaryReader() {
}

// read the date
bool BrandtBinaryReader::ReadSamples(long long lStartSample, long long lNumberSamples, matrixStd& signalSamples)
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

			// Get Filename
			char buffer[200];
			int i = m_unipolarContacts[signalNr].contactGlobalIdx;
			int n = sprintf(buffer, "EEGData-00.int32.%03d", i);
			std::string chFN(buffer);
			std::string channelDataPath = m_patientPath + m_patientName + "\\" + chFN;

			// Read Data
			std::size_t dataSizeBytes = 4;
			int32_t * signalBuffA = new int32_t[lNumberSamples * dataSizeBytes]; // BrandtBinary Files save data with int32 spacing 
			FILE * pFile = fopen(channelDataPath.c_str(), "r");
			fseek(pFile, 0, SEEK_END);
			std::size_t totalFileSize = ftell(pFile);
			rewind(pFile);
			if (pFile != NULL) {
				std::size_t result = fread(signalBuffA, dataSizeBytes, totalFileSize, pFile);
				fclose(pFile);
			}
			else {
				return false;
			}

			signalSamples[signalNr].resize(lNumberSamples);
			//Save Data
#pragma omp parallel for
			for (long long si = 0; si < lNumberSamples; ++si) {
				double val = (double)signalBuffA[si] * (double)7.960022E-2;
				signalSamples[signalNr][si] = val;
			}
			delete[] signalBuffA;
		}
	}

	return true;
}

bool BrandtBinaryReader::ReadReformattedSamples(long long lStartSample, long long lNumberSamples, matrixStd& reformattedSamples)
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

			// Get Filename
			char fnBufferA[200];
			char fnBufferB[200];

			int contactIdxA = m_montages[signalNr].firstContactGlobalIdx;
			int contactIdxB = m_montages[signalNr].secondContactGlobalIdx;

			int nA = sprintf(fnBufferA, "EEGData-00.int32.%03d", contactIdxA);
			std::string chFN_A(fnBufferA);
			std::string channelDataPathA = m_patientPath + m_patientName + "\\" + chFN_A;

			int nB = sprintf(fnBufferB, "EEGData-00.int32.%03d", contactIdxB);
			std::string chFN_B(fnBufferB);
			std::string channelDataPathB = m_patientPath + m_patientName + "\\" + chFN_B;

			/*std::size_t dataSizeBytes = 4;
			std::ifstream is(channelDataPathA.c_str(), std::ifstream::binary);
			if (!is)
				return false;
			// get length of file:
			std::fstream::pos_type start = is.tellg();
			is.seekg(0, is.end);
			std::fstream::pos_type end = is.tellg();
			size_t samplesInFile = ((size_t)(end - start)) / ((size_t)dataSizeBytes);
			is.seekg(0, is.beg);
			if (samplesInFile != getSampleCount() || samplesInFile < lNumberSamples)
				return false;

			long long lengthToRead = dataSizeBytes * lNumberSamples;
			is.seekg(lStartSample * dataSizeBytes);

			char * signalBuffA = new char[lNumberSamples * dataSizeBytes];
			is.read(signalBuffA, lengthToRead);
			is.close();

			std::ifstream isB(channelDataPathB.c_str(), std::ifstream::binary);
			if (!isB)
				return false;
			// get length of file:
			start = isB.tellg();
			isB.seekg(0, isB.end);
			end = isB.tellg();
			samplesInFile = ((size_t)(end - start)) / ((size_t)dataSizeBytes);
			isB.seekg(0, isB.beg);
			if (samplesInFile != getSampleCount() || samplesInFile < lNumberSamples)
				return false;

			lengthToRead = dataSizeBytes * lNumberSamples;
			isB.seekg(lStartSample * dataSizeBytes);

			char * signalBuffB = new char[lNumberSamples * dataSizeBytes];
			isB.read(signalBuffB, lengthToRead);
			isB.close();

			int32_t* int32BuffA = (int32_t*)signalBuffA;
			int32_t* int32BuffB = (int32_t*)signalBuffB;*/

			// Read Data First Contact
			std::size_t dataSizeBytes = 4;
			int32_t * signalBuffA = new int32_t[lNumberSamples*dataSizeBytes]; // BrandtBinary Files save data with int32 spacing 
			FILE * pFileA = fopen(channelDataPathA.c_str(), "r");
			fseek(pFileA, 0, SEEK_END);
			std::size_t totalFileSizeA = ftell(pFileA);
			rewind(pFileA);
			if (pFileA != NULL) {
				std::size_t result = fread(signalBuffA, dataSizeBytes, totalFileSizeA, pFileA);
				fclose(pFileA);
			}
			else {
				return false;
			}

			// Read Data Second Contact
			int32_t * signalBuffB = new int32_t[lNumberSamples*dataSizeBytes]; // BrandtBinary Files save data with int32 spacing 
			FILE * pFileB = fopen(channelDataPathB.c_str(), "r");
			fseek(pFileB, 0, SEEK_END);
			std::size_t totalFileSizeB = ftell(pFileB);
			rewind(pFileB);
			if (pFileB != NULL) {
				std::size_t result = fread(signalBuffB, dataSizeBytes, totalFileSizeB, pFileB);
				fclose(pFileB);
			}
			else {
				return false;
			}
			//int32 * signalBuffBInt32 = (int32_t)signalBuffB;

			reformattedSamples[signalNr].resize(lNumberSamples);
			//Save Data
#pragma omp parallel for
			for (long long si = 0; si < lNumberSamples; ++si) {
				double valA = (double)signalBuffA[si] * (double)7.960022E-2;
				double valB = (double)signalBuffB[si] * (double)7.960022E-2;
				double val = valA - valB;
				reformattedSamples[signalNr][si] = val;
			}
			delete[] signalBuffA;
			delete[] signalBuffB;
		}
	}

	return true;
}

// read header
bool BrandtBinaryReader::readHeader() {

	//Get Sample Count
	std::string guideFN = m_patientPath + m_patientName + ".bin";
	std::ifstream guideFile;
	guideFile.open(guideFN.c_str());
	long long sampleCount = 0;
	while (!guideFile.eof()) {
		std::string line;
		std::getline(guideFile, line);
		std::stringstream ss(line);

		if (!line.empty()) {
			std::string txt;
			ss >> txt;
			ss >> sampleCount;
			break;
		}
	}
	guideFile.close();

	if (!setSampleCount(sampleCount))
		return false;

	//Get Sampling Rate
	std::string headerFN = m_patientPath + m_patientName + "\\cnv-info.txt";
	std::ifstream headerFile;
	headerFile.open(headerFN.c_str());
	double samplingRate = 0;
	int lineIdx = 1;
	while (!headerFile.eof()) {
		std::string line;
		std::getline(headerFile, line);

		if (lineIdx == 3) {
			int stringLength = line.size();
			std::size_t found = line.find("=");
			std::string  srString = line.substr(found+1, stringLength);
			samplingRate = atoi(srString.c_str());
			break;
		}

		lineIdx++;
	}
	headerFile.close();

	if (!setSamplingRate(samplingRate))
		return false;

	std::vector<std::string> tenTwentyChannels = { "C3", "P3", "C4", "P4", "F3", "C3", "F4", "C4", "F7", "Sp1", "SP1", "F8", "Sp2", "SP2", "Fp1", "FP1", "F3", "F7", "Fp2", "FP2", "F4", "F8", "P3", "O1", "P4", "O2", "T3", "T1", "T2", "T4", "T3", "T5", "T4", "T6", "T5", "O1", "T6", "O2", "Pz", "Cz", "PZ", "CZ" , "FZ", "Fz" };
	bool useTenTwentySystem = m_useScalpEEG;


	//Read channel names
	std::string channelsInfoFN = m_patientPath + m_patientName + "\\channel-info.txt";
	std::ifstream chInfoFile;
	chInfoFile.open(channelsInfoFN.c_str());
	std::vector<ContactNames> unipolarContacts;
	int contactGlobalIdx = 1;
	while (!chInfoFile.eof()) {
		std::string line;
		std::getline(chInfoFile, line);
		line.erase(std::remove(line.begin(), line.end(), ' '), line.end());
		std::stringstream ss(line);

		//Read file with Unipolar contacts
		std::string channNrStr, contactName;


		ss >> channNrStr;
		ss >> contactName;

		if (!channNrStr.empty() && !contactName.empty()) {

			int contactNr = 0;
			std::string electrodeName;

			std::vector<std::size_t> nrIndicesVec;
			std::size_t minNrIdx = 0;

			std::string numbers[10] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
			for (int ni = 0; ni < 10; ++ni) {
				std::size_t foundNr = contactName.find(numbers[ni].c_str());
				if (foundNr != std::string::npos) {
					nrIndicesVec.push_back(foundNr);
				}
			}

			std::size_t minNivi = 100 * 100;
			for (int nivi = 0; nivi < nrIndicesVec.size(); ++nivi) {
				std::size_t val = nrIndicesVec[nivi];
				if (val < minNivi)
					minNivi = val;
			}

			electrodeName = contactName;
			if (!nrIndicesVec.empty()) {
				minNrIdx = minNivi;
				electrodeName = contactName.substr(0, minNrIdx);
				std::string ctctNrStr = contactName.substr(minNrIdx, contactName.size() - 1);
				contactNr = atoi(ctctNrStr.c_str());
			}


			ContactNames contact;
			contact.contactGlobalIdx = contactGlobalIdx;
			contact.contactNr = contactNr;
			contact.electrodeName = electrodeName;
			contact.contactName = contactName;
			contactGlobalIdx++;

			if (!contact.electrodeName.empty()) {
				unipolarContacts.push_back(contact);
			}
		}
	}
	chInfoFile.close();

	for (int n = 0; n < unipolarContacts.size(); ++n) {
		ContactNames contact;
		contact.contactGlobalIdx = unipolarContacts[n].contactGlobalIdx;
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

	int montageNr = 0;
	//read Montages' Names
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
			std::string mtgName = montage.montageName;
			for (int i = 0; i < m_selectedBipolarChannels.size(); ++i) {
				std::string selectedChannName = m_selectedBipolarChannels[i];
				selectedChannName.erase(std::remove_if(selectedChannName.begin(), selectedChannName.end(), ::isspace), selectedChannName.end());
				mtgName.erase(std::remove_if(mtgName.begin(), mtgName.end(), ::isspace), mtgName.end());

				selectedChannName.erase(std::remove(selectedChannName.begin(), selectedChannName.end(), '-'), selectedChannName.end());
				mtgName.erase(std::remove(mtgName.begin(), mtgName.end(), '-'), mtgName.end());

				selectedChannName.erase(std::remove(selectedChannName.begin(), selectedChannName.end(), '_'), selectedChannName.end());
				mtgName.erase(std::remove(mtgName.begin(), mtgName.end(), '_'), mtgName.end());

				selectedChannName.erase(std::remove(selectedChannName.begin(), selectedChannName.end(), '#'), selectedChannName.end());
				mtgName.erase(std::remove(mtgName.begin(), mtgName.end(), '#'), mtgName.end());

				int compareResult = selectedChannName.compare(mtgName);
				if (compareResult == 0) {
					channelNotSelected = false;
					break;
				}
			}
		}
		else {
			channelNotSelected = false;
		}

		if (channelNotSelected) {
			continue;
		}

		if (montage.firstElectrodeName.compare(montage.secondElectrodeName) == 0) {
			m_montages.push_back(montage);
			montageNr++;
		}

	}

	return true;
}

// valid importer
bool BrandtBinaryReader::isValidImporter() {
	double samplingRate = getSamplingRate();
	unsigned sampleCount = getSampleCount();
	unsigned nrChs = getNumberUnipolarChannels();
	// check sampling rate, sample count and number channels
	if (getSamplingRate() <= 0.0 || getSampleCount() == 0 || getNumberUnipolarChannels() == 0)
		m_bValidImporter = false;

	return m_bValidImporter;
};

double BrandtBinaryReader::getSamplingRate() {
	return m_dSamplingRate;
}

bool BrandtBinaryReader::setSamplingRate(double dSamplingRate) {
	m_dSamplingRate = dSamplingRate;

	if (m_dSamplingRate <= 0) {
		m_bValidImporter = false;
		return false;
	}

	return true;
}

// sample count
unsigned long BrandtBinaryReader::getSampleCount() {
	return (unsigned long)m_lSampleCount;
}

bool BrandtBinaryReader::setSampleCount(_int64 lSampleCount) {
	m_lSampleCount = lSampleCount;

	if (m_lSampleCount == 0) {
		m_bValidImporter = false;
		return false;
	}

	return true;
}

// number of channels

unsigned int BrandtBinaryReader::getNumberUnipolarChannels() {
	return (unsigned int)m_unipolarContacts.size();
}

unsigned int BrandtBinaryReader::getNumberReformattedMontages() {
	return (unsigned int)m_montages.size();
}

// file name
std::string BrandtBinaryReader::getFileName() {
	return m_strFilename;
}



bool BrandtBinaryReader::readUnipolarLabels(std::vector<ContactNames> &unipolarContactNames) {
	if (m_unipolarContacts.size() == 0)
		return false;

	for (int i = 0; i < m_unipolarContacts.size(); ++i)
		unipolarContactNames.push_back(m_unipolarContacts[i]);

	return true;
}

bool BrandtBinaryReader::readMontageLabels(std::vector<MontageNames> &montageLabels) {
	if (m_montages.size() == 0)
		return false;

	for (int i = 0; i < m_montages.size(); ++i)
		montageLabels.push_back(m_montages[i]);

	return true;
}


bool BrandtBinaryReader::WriteReformattedSamples(long long lStartTime, long long lEndTime)
{
	return true;
}

std::string BrandtBinaryReader::getPatientNameAndPath(void) {
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