#include "AbstractReader.h"



AbstractReader::AbstractReader(std::string strFilename, std::vector<std::string > selectedUnipolarChanns, std::vector<std::string > selectedBipolarChanns, bool useScalpEEG){
	// read header and fill necesarry members
	m_strFilename = strFilename;
	m_selectedUnipolarChannels = selectedUnipolarChanns;
	m_selectedBipolarChannels = selectedBipolarChanns;
	m_useScalpEEG = useScalpEEG;
}


AbstractReader::~AbstractReader(){
}