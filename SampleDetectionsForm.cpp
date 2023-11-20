#include "SampleDetectionsForm.h"
#include "Verification.h"

System::Void MOSDET::SampleDetectionsForm::detectionFilesList_Click(System::Object^  sender, System::EventArgs^  e) {

	OpenFileDialog^ openFileDialog = gcnew OpenFileDialog();
	openFileDialog->Filter = "txt Files (*.txt)|*.txt";
	openFileDialog->Title = "Select a detections .txt File";
	openFileDialog->Multiselect = true;

	if (openFileDialog->ShowDialog() == System::Windows::Forms::DialogResult::OK) {
		openFileDialog->FileName;
		m_strInputFileName = openFileDialog->FileName;
		m_strInputFileNames = openFileDialog->FileNames;
		m_strInputFileNames->Sort(m_strInputFileNames);
	}

	for each (System::String^ file in m_strInputFileNames) {
		this->detectionFilesList->Text += file;
		this->detectionFilesList->Text += "\r\n";
	}
	this->Refresh();
	fillSampleNumberTextBoxes();
}

System::Void MOSDET::SampleDetectionsForm::CI_UpDown_ValueChanged(System::Object^  sender, System::EventArgs^  e) {
	if (this->CI_UpDown->Value >= 90) {
		this->CI_UpDown->Increment = 1;
	}
	else if(this->CI_UpDown->Value > 85  && this->CI_UpDown->Value < 90){
		this->CI_UpDown->Value = 85;
		this->CI_UpDown->Increment = 5;
		this->CI_UpDown->Refresh();
	}

	if (this->CI_UpDown->Value < 75) {										//70
		m_CI = 70;
		m_ZS = 1.0364;
	}
	else if (this->CI_UpDown->Value >= 75 && this->CI_UpDown->Value < 80) {						//75
		m_CI = 75;
		m_ZS = 1.1503;
	}
	else if (this->CI_UpDown->Value >= 80 && this->CI_UpDown->Value < 85) {						//80
		m_CI = 80;
		m_ZS = 1.2816;
	}
	else if (this->CI_UpDown->Value >= 85 && this->CI_UpDown->Value < 90) {						//85
		m_CI = 85;
		m_ZS = 1.4395;
	}
	else if (this->CI_UpDown->Value == 90) {									//90
		m_CI = 90;
		m_ZS = 1.6449;
	}
	else if (this->CI_UpDown->Value == 91) {
		m_CI = 91;
		m_ZS = 1.6954;
	}
	else if (this->CI_UpDown->Value == 92) {
		m_CI = 92;
		m_ZS = 1.7507;
	}
	else if (this->CI_UpDown->Value == 93) {
		m_CI = 93;
		m_ZS = 1.8119;
	}
	else if (this->CI_UpDown->Value == 94) {
		m_CI = 94;
		m_ZS = 1.8808;
	}
	else if (this->CI_UpDown->Value == 95) {
		m_CI = 95;
		m_ZS = 1.96;
	}
	else if (this->CI_UpDown->Value == 96) {
		m_CI = 96;
		m_ZS = 2.0537;
	}
	else if (this->CI_UpDown->Value == 97) {
		m_CI = 97;
		m_ZS = 2.1701;
	}
	else if (this->CI_UpDown->Value == 98) {
		m_CI = 98;
		m_ZS = 2.3263;
	}
	else if (this->CI_UpDown->Value >= 99) {
		m_CI = 99;
		m_ZS = 2.5758;
	}

	fillSampleNumberTextBoxes();
}

System::Void MOSDET::SampleDetectionsForm::ME_UpDown_ValueChanged(System::Object^  sender, System::EventArgs^  e) {
	m_ME = System::Convert::ToInt16(ME_UpDown->Value) / 100.0;
	fillSampleNumberTextBoxes();
}

System::Void MOSDET::SampleDetectionsForm::RunButton_Click(System::Object^  sender, System::EventArgs^  e) {
	fillSampleNumberTextBoxes();
	if (m_strInputFileNames != nullptr) {
		for each (System::String^ file in m_strInputFileNames) {
			if ((file->Contains(".txt") || file->Contains(".TXT"))) {
				// check existence of measurement file name
				if (System::String::IsNullOrEmpty(file))
					return;
				Verification verifyDetections(file, m_strOutputDirectory, m_nrSamples, m_CI, m_ME*100);
				verifyDetections.getRandomSamples();
				verifyDetections.generateVerificationFile();
			}
		}
	}

}

System::Void MOSDET::SampleDetectionsForm::fillSampleNumberTextBoxes() {
	m_nrSamples = ((m_ZS* m_ZS) * (0.5)*(1 - 0.5)) / (m_ME*m_ME);
	this->TruePosTextBox->Text = System::Convert::ToString(m_nrSamples / 2);
	this->TrueNegsTextBox->Text = System::Convert::ToString(m_nrSamples / 2);
	this->TruePosTextBox->Refresh();
	this->TrueNegsTextBox->Refresh();

}