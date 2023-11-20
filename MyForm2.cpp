#include <chrono>
#include <random>
#include <fstream>
#include <direct.h>
#include <windows.h>

#include "MyForm2.h"
#include "IO_EDFPlus_SignalImporter_c.h"
#include "BinaryReader.h"
#include "IO_CoherenceBinSignalImporter.h"
#include "BrandtBinaryReader.h"
#include "AbstractReader.h"


// Get all channels in selected file
bool MOSDET::MyForm2::readChannels(bool unipolarChannels, bool bipolarChannels) {

	int montageNr = 0;

	unsigned patientIdx = 1;
	for each (System::String^ file in m_strInputFileNames) {
		// check existence of measurement file name
		if (System::String::IsNullOrEmpty(file))
			return false;

		std::vector<std::string > selectedChanns;
		std::vector<ContactNames> unipolarContactNames;
		std::vector<MontageNames> montageLabels;
		std::string strFileName = (char*)(void*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(file);

		m_selectedUnipolarChannels->clear();
		m_selectedBipolarChannels->clear();

		AbstractReader* pReader = NULL;
		if (m_fileType == 1) {
			pReader = new BinaryReader(strFileName, *m_selectedUnipolarChannels, *m_selectedBipolarChannels, m_useScalpEEG);	// selected channels vectors are empty
		}
		else if (m_fileType == 2) {
			pReader = new IO_EDFPlus_SignalImporter_c(strFileName, *m_selectedUnipolarChannels, *m_selectedBipolarChannels, m_useScalpEEG);	// selected channels vectors are empty
		} 
		else if (m_fileType == 3) {
			size_t sampleType = 0;
			pReader = new IO_CoherenceBinSignalImporter(strFileName, sampleType, *m_selectedUnipolarChannels, *m_selectedBipolarChannels, m_useScalpEEG);	// selected channels vectors are empty
		}
		else if (m_fileType == 4) {
			pReader = new BrandtBinaryReader(strFileName, *m_selectedUnipolarChannels, *m_selectedBipolarChannels, m_useScalpEEG);	// selected channels vectors are empty
		}

		if (unipolarChannels) {
			pReader->readUnipolarLabels(unipolarContactNames);
			for (unsigned patContactIdx = 0; patContactIdx < unipolarContactNames.size(); ++patContactIdx) {
				std::string patContactName = unipolarContactNames[patContactIdx].contactName;
				if (std::find(m_unipolarChanels->begin(), m_unipolarChanels->end(), patContactName) == m_unipolarChanels->end()) {
					m_unipolarChanels->push_back(patContactName);
				}
			}
		}

		if (bipolarChannels) {
			pReader->readMontageLabels(montageLabels);
			for (unsigned patMontageIdx = 0; patMontageIdx < montageLabels.size(); ++patMontageIdx) {
				std::string patMontageName = montageLabels[patMontageIdx].montageName;
				if (std::find(m_bipolarChanels->begin(), m_bipolarChanels->end(), patMontageName) == m_bipolarChanels->end()) {
					m_bipolarChanels->push_back(patMontageName);
				}
			}
		}

		patientIdx++;
	}

	return true;
}

// Get all channels in selected file
void MOSDET::MyForm2::fillChannNamesArray(void) {

	if (m_unipolar) {
		int arraySize = m_unipolarChanels->size();
		m_channsArray = gcnew cli::array< System::Windows::Forms::ListViewItem^  > (arraySize);
		for (int i = 0; i < arraySize; i++) {
			System::String ^chanName = gcnew System::String(m_unipolarChanels->at(i).c_str());
			m_channsArray[i] = (gcnew System::Windows::Forms::ListViewItem(chanName));
		}
	}
	if (m_bipolar) {
		int arraySize = m_bipolarChanels->size();
		m_channsArray = gcnew cli::array< System::Windows::Forms::ListViewItem^  >(arraySize);
		for (int i = 0; i < arraySize; i++) {
			System::String ^chanName = gcnew System::String(m_bipolarChanels->at(i).c_str());
			m_channsArray[i] = (gcnew System::Windows::Forms::ListViewItem(chanName));
		}
	}
	bool stop = true;
}

//Select all when pressing ctrl + A
System::Void MOSDET::MyForm2::listView1_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^ e) {
	if (e->KeyCode == Keys::A && e->Control)
	{
		this->listView1->MultiSelect = true;
		System::Collections::IEnumerator^ myEnum = this->listView1->Items->GetEnumerator();
		while (myEnum->MoveNext()) {
			safe_cast<ListViewItem^>(myEnum->Current)->Selected = true;
		}
	}
	
	if (e->KeyCode == Keys::Enter)
	{
		this->listView1->Refresh();
		System::Collections::IEnumerator^ myEnum = this->listView1->CheckedItems->GetEnumerator();
		while (myEnum->MoveNext()) {
			bool checked = safe_cast<ListViewItem^>(myEnum->Current)->Checked;
			std::string channName = (char*)(void*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(safe_cast<ListViewItem^>(myEnum->Current)->Text);
			if (m_unipolar) {
				m_selectedUnipolarChannels->push_back(channName);
			}
			else {
				m_selectedBipolarChannels->push_back(channName);
			}
		}

		int tabCnt = 0;
		for (int i = 0; i < m_selectedUnipolarChannels->size(); ++i) {
			System::String ^chann = gcnew System::String(m_selectedUnipolarChannels->at(i).c_str());
			this->m_selectedChannsTextBox->Text += chann;
			tabCnt++;
			if (tabCnt > 6) {
				tabCnt = 0;
				this->m_selectedChannsTextBox->Text += "\r\n";
			}
			else {
				this->m_selectedChannsTextBox->Text += "\r\t\r\t";
			}
		}

		tabCnt = 0;
		for (int i = 0; i < m_selectedBipolarChannels->size(); ++i) {
			System::String ^chann = gcnew System::String(m_selectedBipolarChannels->at(i).c_str());
			this->m_selectedChannsTextBox->Text += chann;
			tabCnt++;
			if (tabCnt > 6) {
				tabCnt = 0;
				this->m_selectedChannsTextBox->Text += "\r\n";
			}
			else {
				this->m_selectedChannsTextBox->Text += "\r\t\r\t";
			}
		}

		this->Close();
	}
}

//Ok button
System::Void MOSDET::MyForm2::okButton_Click(System::Object^  sender, System::EventArgs^  e) {
	this->listView1->Refresh();
	System::Collections::IEnumerator^ myEnum = this->listView1->CheckedItems->GetEnumerator();
	while (myEnum->MoveNext()) {
		bool checked = safe_cast<ListViewItem^>(myEnum->Current)->Checked;
			std::string channName = (char*)(void*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(safe_cast<ListViewItem^>(myEnum->Current)->Text);
			if (m_unipolar) {
				m_selectedUnipolarChannels->push_back(channName);
			}
			else {
				m_selectedBipolarChannels->push_back(channName);
			}
	}

	int tabCnt = 0;

	for (int i = 0; i < m_selectedUnipolarChannels->size(); ++i) {
		System::String ^chann = gcnew System::String(m_selectedUnipolarChannels->at(i).c_str());
		this->m_selectedChannsTextBox->Text += chann;
		tabCnt++;
		if (tabCnt > 6) {
			tabCnt = 0;
			this->m_selectedChannsTextBox->Text += "\r\n";
		}
		else {
			this->m_selectedChannsTextBox->Text += "\r\t\r\t";
		}
	}

	tabCnt = 0;
	for (int i = 0; i < m_selectedBipolarChannels->size(); ++i) {
		System::String ^chann = gcnew System::String(m_selectedBipolarChannels->at(i).c_str());
		this->m_selectedChannsTextBox->Text += chann;
		tabCnt++;
		if (tabCnt > 6) {
			tabCnt = 0;
			this->m_selectedChannsTextBox->Text += "\r\n";
		}
		else {
			this->m_selectedChannsTextBox->Text += "\r\t\r\t";
		}
	}

	this->Close();
}

//Ok button
System::Void MOSDET::MyForm2::useFileButton_Click(System::Object^  sender, System::EventArgs^  e) {
	m_selectedUnipolarChannels->clear();
	m_selectedBipolarChannels->clear();
	*m_useChannsFileBoolPtr = true;
	this->m_selectedChannsTextBox->Text = "Use Files\r\n";

	for each (System::String^ file in m_strInputFileNames) {
		std::string patName = (char*)(void*)System::Runtime::InteropServices::Marshal::StringToHGlobalAnsi(file);
		patName.pop_back(); patName.pop_back(); patName.pop_back(); patName.pop_back();
		patName += "_SelectedChannels.txt";
		//patName += "_SelectedChannels_Ripples.txt";
		//patName += "_SelectedChannels_FR_Channs.txt";
		
		System::String^ channFile = gcnew System::String(patName.c_str());
		this->m_selectedChannsTextBox->Text += channFile + "\r\n";
	}

	this->Close();
}

