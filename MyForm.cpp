#include <direct.h>
#include <chrono>
#include <fstream>

#include "MyForm.h"
#include "MyForm2.h"
#include "SampleDetectionsForm.h"

#include "HFO_Detector.h"
#include "SS_Detector.h"
#include "SignalProcessing.h"
#include "IO_EDFPlus_SignalImporter_c.h"
#include "BinaryReader.h"
#include "IO_CoherenceBinSignalImporter.h"
#include "BrandtBinaryReader.h"
#include "IO_ProfusionImporter_c.h"

#include <omp.h>

using namespace System;
using namespace System::Windows::Forms;

[STAThread]
int main(array<String^>^ args)
{
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false);

	MOSDET::MyForm form;
	form.Show();
	Application::Run(%form);
}

System::Void MOSDET::MyForm::selectInputFilesToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
	// Displays an OpenFileDialog so the user can select a measurement file.
	OpenFileDialog^ openFileDialog1 = gcnew OpenFileDialog();
	openFileDialog1->Filter = "Binary Raw (*.bin)|*.bin|EDF Files (*.edf)|*.edf|Coherence (*.bin)|*.bin|BrandtBinary (*.bin)|*.bin";
	openFileDialog1->Title = "Select File Format and File(s)";
	openFileDialog1->Multiselect = true;

	if (openFileDialog1->ShowDialog() == System::Windows::Forms::DialogResult::OK)
	{
		openFileDialog1->FileName;
		m_strInputFileName = openFileDialog1->FileName;
		m_strInputFileNames = openFileDialog1->FileNames;
		m_strInputFileNames->Sort(m_strInputFileNames);
		m_fileType = openFileDialog1->FilterIndex;

	}
	
	std::string selectedFiles;
	for each (System::String^ file in m_strInputFileNames) {
		this->textBox1->Text += file;
		this->textBox1->Text += "\r\n";
	}
	
	this->Refresh();
}

//Select Output Directory
System::Void MOSDET::MyForm::selectOutputPathToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
	bool stop = true;
	// Displays an OpenFileDialog so the user can select a source space file.
	FolderBrowserDialog^ folderBrowserDialog1 = gcnew FolderBrowserDialog();

	// If the user clicked OK in the dialog
	if (folderBrowserDialog1->ShowDialog() == System::Windows::Forms::DialogResult::OK)
	{
		m_strOutputDirectory = folderBrowserDialog1->SelectedPath;
	}
	
	this->textBox3->Text = m_strOutputDirectory;
	this->textBox3->Refresh();
}

//Detect Ripples and Fast-Ripples
System::Void MOSDET::MyForm::detectHFOsToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {

	//SignalProcessing::characterizeBandpassRippleFilter(80, 250);
	//SignalProcessing::characterizeBandpassFastRippleFilter(250, 500);
	//SignalProcessing::characterizeHighpassFilter(1);

	// check for eeg extension
	bool fileTypeOK = (m_strInputFileName->Contains(".edf") || m_strInputFileName->Contains(".EDF") || m_strInputFileName->Contains(".bin") || m_strInputFileName->Contains(".BIN"));	
	if (fileTypeOK) {

		unsigned patientIdx = 1;
		bool useScalpEEG = this->scalpEEG->Checked;
		bool compareWithVisualMarks = this->getPerformanceCheckBox1->Checked;
		for each (System::String^ file in m_strInputFileNames) {
			// check existence of measurement file name
			if (System::String::IsNullOrEmpty(file))
				return;

			auto start_time = std::chrono::high_resolution_clock::now();

			std::string eoiName = "HFO";
			HFO_Detector hfoRippleDetector(file, m_fileType, m_strOutputDirectory, patientIdx, m_bipolar, m_selectedUnipolarChannels, m_selectedBipolarChannels, eoiName, *m_useChannsFileBoolPtr, 
				m_startTime, m_endTime, compareWithVisualMarks, m_ProcessorGroup, m_ProcessingPercent, m_readMinutesEpochLenghth, useScalpEEG);
			hfoRippleDetector.readEEG();
			int output = hfoRippleDetector.characterizeEEG();

			if (output == 1) {
				this->runStatus->Text = "OK";
				this->runStatus->Refresh();
			}
			else if (output == 0) {
				this->runStatus->Text = "E#0";
				this->runStatus->Refresh();
			}
			else if(output == -1){
				this->runStatus->Text = "E#1";
				this->runStatus->Refresh();
			}
			else if (output == -2) {
				this->runStatus->Text = "E#2";
				this->runStatus->Refresh();
			}
			
			double samplingRate, analysisDuration;
			int nrChannels;
			hfoRippleDetector.getSignalParams(samplingRate, analysisDuration, nrChannels);

			double durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
			executionTimeLog(file, patientIdx, samplingRate, analysisDuration, nrChannels, durationMs);
			patientIdx++;

			InitializeComponent();

		}
	}
}


//Detect HFO-Ripples
System::Void MOSDET::MyForm::detectRipplesToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
	//SignalProcessing::characterizeBandpassRippleFilter(80, 250);
	//SignalProcessing::characterizeBandpassFastRippleFilter(250, 500);
	//SignalProcessing::characterizeHighpassFilter(1);

	// check for eeg extension
	bool fileTypeOK = (m_strInputFileName->Contains(".edf") || m_strInputFileName->Contains(".EDF"));

	if (fileTypeOK) {

		unsigned patientIdx = 1;
		bool useScalpEEG = this->scalpEEG->Checked;
		bool compareWithVisualMarks = this->getPerformanceCheckBox1->Checked;
		for each (System::String^ file in m_strInputFileNames) {
			// check existence of measurement file name
			if (System::String::IsNullOrEmpty(file))
				return;

			auto start_time = std::chrono::high_resolution_clock::now();

			std::string eoiName = "Ripples";
			HFO_Detector hfoRippleDetector(file, m_fileType, m_strOutputDirectory, patientIdx, m_bipolar, m_selectedUnipolarChannels, m_selectedBipolarChannels, eoiName, *m_useChannsFileBoolPtr,
				m_startTime, m_endTime, compareWithVisualMarks, m_ProcessorGroup, m_ProcessingPercent, m_readMinutesEpochLenghth, useScalpEEG);
			hfoRippleDetector.readEEG();
			hfoRippleDetector.characterizeEEG();

			double samplingRate, analysisDuration;
			int nrChannels;
			hfoRippleDetector.getSignalParams(samplingRate, analysisDuration, nrChannels);

			double durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
			executionTimeLog(file, patientIdx, samplingRate, analysisDuration, nrChannels, durationMs);
			patientIdx++;

		}
	}
}

//Detect HFO-Fast-Ripples
System::Void MOSDET::MyForm::detectFastRipplesToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
	//SignalProcessing::characterizeBandpassRippleFilter(80, 250);
	//SignalProcessing::characterizeBandpassFastRippleFilter(250, 500);
	//SignalProcessing::characterizeHighpassFilter(1);

	// check for eeg extension
	bool fileTypeOK = (m_strInputFileName->Contains(".edf") || m_strInputFileName->Contains(".EDF"));

	if (fileTypeOK) {

		unsigned patientIdx = 1;
		bool useScalpEEG = this->scalpEEG->Checked;
		bool compareWithVisualMarks = this->getPerformanceCheckBox1->Checked;
		for each (System::String^ file in m_strInputFileNames) {
			// check existence of measurement file name
			if (System::String::IsNullOrEmpty(file))
				return;

			auto start_time = std::chrono::high_resolution_clock::now();

			std::string eoiName = "FastRipples";
			HFO_Detector hfoRippleDetector(file, m_fileType, m_strOutputDirectory, patientIdx, m_bipolar, m_selectedUnipolarChannels, m_selectedBipolarChannels, eoiName, *m_useChannsFileBoolPtr, 
				m_startTime, m_endTime, compareWithVisualMarks, m_ProcessorGroup, m_ProcessingPercent, m_readMinutesEpochLenghth, useScalpEEG);
			hfoRippleDetector.readEEG();
			hfoRippleDetector.characterizeEEG();

			double samplingRate, analysisDuration;
			int nrChannels;
			hfoRippleDetector.getSignalParams(samplingRate, analysisDuration, nrChannels);

			double durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
			executionTimeLog(file, patientIdx, samplingRate, analysisDuration, nrChannels, durationMs);
			patientIdx++;

		}
	}
}

//Detect Sleep-Spindles
System::Void MOSDET::MyForm::detectSleepSpindlesToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
	// check for eeg extension
	bool fileTypeOK = (m_strInputFileName->Contains(".edf") || m_strInputFileName->Contains(".EDF") || m_strInputFileName->Contains(".bin") || m_strInputFileName->Contains(".BIN"));

	if (fileTypeOK) {

		unsigned patientIdx = 1;
		bool useScalpEEG = this->scalpEEG->Checked;
		bool compareWithVisualMarks = this->getPerformanceCheckBox1->Checked;
		for each (System::String^ file in m_strInputFileNames) {
			// check existence of measurement file name
			if (System::String::IsNullOrEmpty(file))
				return;

			auto start_time = std::chrono::high_resolution_clock::now();

			std::string eoiName = "SleepSpindles";
			SS_Detector spindleDetector(file, m_strOutputDirectory, patientIdx, m_bipolar, m_selectedUnipolarChannels, m_selectedBipolarChannels, eoiName, *m_useChannsFileBoolPtr, m_startTime, m_endTime, compareWithVisualMarks, useScalpEEG);
			bool readOK = spindleDetector.readEEG();

			double samplingRate, analysisDuration;
			int nrChannels;
			if (readOK) {
				spindleDetector.characterizeEEG();
				spindleDetector.getSignalParams(samplingRate, analysisDuration, nrChannels);
			}

			double durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
			executionTimeLog(file, patientIdx, samplingRate, analysisDuration, nrChannels, durationMs);

			patientIdx++;

		}
	}
}

//Detect Spikes
System::Void MOSDET::MyForm::detectSpikesToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
	// check for eeg extension
	bool fileTypeOK = (m_strInputFileName->Contains(".edf") || m_strInputFileName->Contains(".EDF"));

	if (fileTypeOK) {

		unsigned patientIdx = 1;
		bool useScalpEEG = this->scalpEEG->Checked;
		bool compareWithVisualMarks = this->getPerformanceCheckBox1->Checked;
		for each (System::String^ file in m_strInputFileNames) {
			// check existence of measurement file name
			if (System::String::IsNullOrEmpty(file))
				return;

			auto start_time = std::chrono::high_resolution_clock::now();

			std::string eoiName = "Spikes";
			HFO_Detector hfoRippleDetector(file, m_fileType, m_strOutputDirectory, patientIdx, m_bipolar, m_selectedUnipolarChannels, m_selectedBipolarChannels, eoiName, *m_useChannsFileBoolPtr, m_startTime,
				m_endTime, compareWithVisualMarks, m_ProcessorGroup, m_ProcessingPercent, m_readMinutesEpochLenghth, useScalpEEG);
			hfoRippleDetector.readEEG();
			hfoRippleDetector.characterizeEEG();

			double samplingRate, analysisDuration;
			int nrChannels;
			hfoRippleDetector.getSignalParams(samplingRate, analysisDuration, nrChannels);

			double durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
			executionTimeLog(file, patientIdx, samplingRate, analysisDuration, nrChannels, durationMs);
			patientIdx++;

		}
	}
}

//Generate .edf files with selected montages
System::Void MOSDET::MyForm::generateMontageFilesToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {

	unsigned patientIdx = 0;
	for (int i = m_strInputFileNames->GetLowerBound(0); i <= m_strInputFileNames->GetUpperBound(0); i++) {
		System::String^ file = m_strInputFileNames[i];
		// check existence of measurement file name
		if (System::String::IsNullOrEmpty(file)) {
			return;
		}
		bool useScalpEEG = this->scalpEEG->Checked;
		std::string  filename = (char*)(void*)Marshal::StringToHGlobalAnsi(file);
		AbstractReader* pReader = NULL;
		if (m_fileType == 1) {
			pReader = new BinaryReader(filename, *m_selectedUnipolarChannels, *m_selectedBipolarChannels, useScalpEEG);	// selected channels vectors are empty
		}
		else if (m_fileType == 2) {
			pReader = new IO_EDFPlus_SignalImporter_c(filename, *m_selectedUnipolarChannels, *m_selectedBipolarChannels, useScalpEEG);	// selected channels vectors are empty
		}
		else if (m_fileType == 3) {
			size_t sampleType = 0;
			pReader = new IO_CoherenceBinSignalImporter(filename, sampleType, *m_selectedUnipolarChannels, *m_selectedBipolarChannels, useScalpEEG);	// selected channels vectors are empty
		}
		else if (m_fileType == 4) {
			pReader = new BrandtBinaryReader(filename, *m_selectedUnipolarChannels, *m_selectedBipolarChannels, useScalpEEG);	// selected channels vectors are empty
		}
		else {
			return;
		}

		pReader->WriteReformattedSamples(m_startTime, m_endTime);
	}
}

System::Void MOSDET::MyForm::selectUnipolarChannelsToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
	m_unipolar = true;
	m_bipolar = false;
	bool useScalpEEG = this->scalpEEG->Checked;

	m_selectedUnipolarChannels->clear();
	m_selectedBipolarChannels->clear();

	this->textBox2->Clear();
	m_montageNr = 0;

	MyForm2 ^formTwo = gcnew MyForm2(m_fileType, m_strInputFileName, m_strInputFileNames, m_unipolar, m_bipolar, m_selectedUnipolarChannels, m_selectedBipolarChannels, m_useChannsFileBoolPtr, this->textBox2, useScalpEEG);
	formTwo->Show();
	bool stop = true;
}

System::Void MOSDET::MyForm::selectBipolarChannelsToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e) {
	m_unipolar = false;
	m_bipolar = true;
	bool useScalpEEG = this->scalpEEG->Checked;

	m_selectedUnipolarChannels->clear();
	m_selectedBipolarChannels->clear();

	this->textBox2->Clear();
	m_montageNr = 1;

	MyForm2 ^formTwo = gcnew MyForm2(m_fileType, m_strInputFileName, m_strInputFileNames, m_unipolar, m_bipolar, m_selectedUnipolarChannels, m_selectedBipolarChannels, m_useChannsFileBoolPtr, this->textBox2, useScalpEEG);
	formTwo->Show();
	bool stop = true;
}

System::Void MOSDET::MyForm::textBox4_TextChanged(System::Object^  sender, System::EventArgs^  e) {
	if (!String::IsNullOrEmpty(textBox4->Text)) {
		m_startTime = Double::Parse(textBox4->Text);
	}
}

System::Void MOSDET::MyForm::textBox5_TextChanged(System::Object^  sender, System::EventArgs^  e) {
	if (!String::IsNullOrEmpty(textBox5->Text)) {
		m_endTime = Double::Parse(textBox5->Text);
	}
}

System::String^ MOSDET::MyForm::ExePath() {
	char buff[FILENAME_MAX];
	getcwd(buff, FILENAME_MAX);
	std::string current_working_dir(buff);
	m_strOutputDirectory = gcnew System::String(buff);

	return m_strOutputDirectory;
}

System::Double MOSDET::MyForm::executionTimeLog(System::String^ strInputFileName, int loopNr, double samplingRate, double analysisDuration, int nrChanns, double durationMs) {
		
	std::string cpp_str_outputDirectory = (char*)(void*)Marshal::StringToHGlobalAnsi(m_strOutputDirectory);
	std::string eegFilename = (char*)(void*)Marshal::StringToHGlobalAnsi(strInputFileName);

	std::ofstream characterizationTimeLog;

	std::string outputPath = cpp_str_outputDirectory + "/MOSSDET_Output/";
	std::string filename = outputPath + "ExecutionTime.txt";
	mkdir(outputPath.c_str());

	std::ifstream infile(filename);
	bool fileExists = infile.good();

	if (!fileExists) {
		characterizationTimeLog.open(filename);
		characterizationTimeLog << "FileNr\t" << "File\t" << "SamplingRate (Hz)\t" << "SignalLength (s)\t" << "Nr. Channels\t" << "RunTime (ms) \n";
	}
	else {
		characterizationTimeLog.open(filename, std::ios::app);    // open file for appending
	}

	characterizationTimeLog << loopNr << "\t" << eegFilename << "\t" << samplingRate << "\t" << analysisDuration << "\t" << nrChanns << "\t" << durationMs << "\n";
	characterizationTimeLog.close();

	return durationMs;
}

System::Void MOSDET::MyForm::restart_Click(System::Object^  sender, System::EventArgs^  e) {
	Application::Restart();
}

System::Void MOSDET::MyForm::verifyDetectionsToolStripMenuItem_Click_1(System::Object^  sender, System::EventArgs^  e) {

	SampleDetectionsForm ^verifySamples = gcnew SampleDetectionsForm(m_strOutputDirectory);
	verifySamples->Show();
	bool stop = true;

}

System::Void MOSDET::MyForm::CPU_DropDown_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e) {
	
	this->CPU_DropDown->Tag = this->CPU_DropDown->SelectedItem;
	m_ProcessorGroup = this->CPU_DropDown->SelectedIndex;
}

System::Void MOSDET::MyForm::PercentCPU_TextChanged(System::Object^  sender, System::EventArgs^  e) {
	if (!String::IsNullOrEmpty(PercentCPU->Text)) {
		m_ProcessingPercent = System::Convert::ToInt16((this->PercentCPU->Text));
	}
}

System::Void MOSDET::MyForm::textBox6_TextChanged(System::Object^  sender, System::EventArgs^  e) {
	if (!String::IsNullOrEmpty(textBox6->Text)) {
		m_readMinutesEpochLenghth = System::Convert::ToInt16((this->textBox6->Text));
	}
}

System::Void MOSDET::MyForm::setLowLevelProcessing() {
	//HANDLE process = GetCurrentProcess();
	//Set NUMA Node
	GROUP_AFFINITY  GroupAffinity;
	PGROUP_AFFINITY PreviousGroupAffinity;
	BOOL success = GetThreadGroupAffinity(GetCurrentThread(), &GroupAffinity);
	GroupAffinity.Group = m_ProcessorGroup;
	success = SetThreadGroupAffinity(GetCurrentThread(), &GroupAffinity, PreviousGroupAffinity);
	//success = SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);// REALTIME_PRIORITY_CLASS NORMAL_PRIORITY_CLASS HIGH_PRIORITY_CLASS

	WORD numberOfProcessorGroups = GetMaximumProcessorGroupCount();
	for (int i = 1; i <= numberOfProcessorGroups; ++i) {
		std::string str = std::to_string(i);
		m_processorNumbers->Add(gcnew String(str.c_str()));
	}
	this->CPU_DropDown->Items->AddRange(m_processorNumbers->ToArray());
}