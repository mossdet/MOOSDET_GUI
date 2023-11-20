#pragma once

#include <map>


namespace MOSDET {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for SampleDetectionsForm
	/// </summary>
	public ref class SampleDetectionsForm : public System::Windows::Forms::Form
	{
	public:
		SampleDetectionsForm(System::String^ strOutputDirectory)
		{
			InitializeComponent();
			m_strOutputDirectory = strOutputDirectory;
			m_CI = 95;
			m_ME = 0.025;
			m_ZS = 2.5758;
			fillSampleNumberTextBoxes();
			this->ME_UpDown->Increment = Decimal(0.25);
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~SampleDetectionsForm()
		{
			if (components)
			{
				delete components;
			}
		}
	private: System::Windows::Forms::Label^  CI_label;
	private: System::Windows::Forms::Label^  ME_label;
	protected:




		
	System::String^ m_strInputFileName;
	array<String^>^ m_strInputFileNames;
	System::String^ m_strOutputDirectory;
	private: int m_CI;
	private: double m_ME, m_ZS;
	private: int m_nrSamples;

	private: System::Windows::Forms::Label^  DetectionFilesLabel;
	public:
	public:


	private: System::Windows::Forms::TextBox^  detectionFilesList;
	private: System::Windows::Forms::Button^  RunButton;
	private: System::Windows::Forms::NumericUpDown^  CI_UpDown;
	private: System::Windows::Forms::NumericUpDown^  ME_UpDown;
	private: System::Windows::Forms::Label^  SampleSizeLabel;
	private: System::Windows::Forms::TextBox^  TruePosTextBox;
	private: System::Windows::Forms::TextBox^  TrueNegsTextBox;
	private: System::Windows::Forms::Label^  TruePositivesLabel;
	private: System::Windows::Forms::Label^  TrueNegativeLabel;
	private: System::Windows::Forms::Label^  label1;



	protected:

	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container ^components;

#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
			this->DetectionFilesLabel = (gcnew System::Windows::Forms::Label());
			this->detectionFilesList = (gcnew System::Windows::Forms::TextBox());
			this->CI_label = (gcnew System::Windows::Forms::Label());
			this->ME_label = (gcnew System::Windows::Forms::Label());
			this->RunButton = (gcnew System::Windows::Forms::Button());
			this->CI_UpDown = (gcnew System::Windows::Forms::NumericUpDown());
			this->ME_UpDown = (gcnew System::Windows::Forms::NumericUpDown());
			this->SampleSizeLabel = (gcnew System::Windows::Forms::Label());
			this->TruePosTextBox = (gcnew System::Windows::Forms::TextBox());
			this->TrueNegsTextBox = (gcnew System::Windows::Forms::TextBox());
			this->TruePositivesLabel = (gcnew System::Windows::Forms::Label());
			this->TrueNegativeLabel = (gcnew System::Windows::Forms::Label());
			this->label1 = (gcnew System::Windows::Forms::Label());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->CI_UpDown))->BeginInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->ME_UpDown))->BeginInit();
			this->SuspendLayout();
			// 
			// DetectionFilesLabel
			// 
			this->DetectionFilesLabel->AutoSize = true;
			this->DetectionFilesLabel->Location = System::Drawing::Point(3, 27);
			this->DetectionFilesLabel->Name = L"DetectionFilesLabel";
			this->DetectionFilesLabel->Size = System::Drawing::Size(77, 13);
			this->DetectionFilesLabel->TabIndex = 5;
			this->DetectionFilesLabel->Text = L"Detection Files";
			// 
			// detectionFilesList
			// 
			this->detectionFilesList->Location = System::Drawing::Point(91, 27);
			this->detectionFilesList->Name = L"detectionFilesList";
			this->detectionFilesList->Size = System::Drawing::Size(485, 20);
			this->detectionFilesList->TabIndex = 6;
			this->detectionFilesList->Click += gcnew System::EventHandler(this, &SampleDetectionsForm::detectionFilesList_Click);
			// 
			// CI_label
			// 
			this->CI_label->AutoSize = true;
			this->CI_label->Location = System::Drawing::Point(12, 88);
			this->CI_label->Name = L"CI_label";
			this->CI_label->Size = System::Drawing::Size(116, 13);
			this->CI_label->TabIndex = 0;
			this->CI_label->Text = L"Confidence Interval (%)";
			// 
			// ME_label
			// 
			this->ME_label->AutoSize = true;
			this->ME_label->Location = System::Drawing::Point(12, 134);
			this->ME_label->Name = L"ME_label";
			this->ME_label->Size = System::Drawing::Size(93, 13);
			this->ME_label->TabIndex = 1;
			this->ME_label->Text = L"Margin of Error (%)";
			// 
			// RunButton
			// 
			this->RunButton->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 8.25F, System::Drawing::FontStyle::Bold, System::Drawing::GraphicsUnit::Point,
				static_cast<System::Byte>(0)));
			this->RunButton->Location = System::Drawing::Point(479, 232);
			this->RunButton->Name = L"RunButton";
			this->RunButton->Size = System::Drawing::Size(110, 45);
			this->RunButton->TabIndex = 7;
			this->RunButton->Text = L"Generate Verification Files";
			this->RunButton->TextAlign = System::Drawing::ContentAlignment::MiddleLeft;
			this->RunButton->UseVisualStyleBackColor = true;
			this->RunButton->Click += gcnew System::EventHandler(this, &SampleDetectionsForm::RunButton_Click);
			// 
			// CI_UpDown
			// 
			this->CI_UpDown->Increment = System::Decimal(gcnew cli::array< System::Int32 >(4) { 5, 0, 0, 0 });
			this->CI_UpDown->Location = System::Drawing::Point(134, 86);
			this->CI_UpDown->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 99, 0, 0, 0 });
			this->CI_UpDown->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 70, 0, 0, 0 });
			this->CI_UpDown->Name = L"CI_UpDown";
			this->CI_UpDown->Size = System::Drawing::Size(52, 20);
			this->CI_UpDown->TabIndex = 8;
			this->CI_UpDown->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) { 99, 0, 0, 0 });
			this->CI_UpDown->ValueChanged += gcnew System::EventHandler(this, &SampleDetectionsForm::CI_UpDown_ValueChanged);
			// 
			// ME_UpDown
			// 
			this->ME_UpDown->DecimalPlaces = 2;
			this->ME_UpDown->Location = System::Drawing::Point(131, 128);
			this->ME_UpDown->Maximum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 20, 0, 0, 0 });
			this->ME_UpDown->Minimum = System::Decimal(gcnew cli::array< System::Int32 >(4) { 1, 0, 0, 0 });
			this->ME_UpDown->Name = L"ME_UpDown";
			this->ME_UpDown->Size = System::Drawing::Size(55, 20);
			this->ME_UpDown->TabIndex = 9;
			this->ME_UpDown->Value = System::Decimal(gcnew cli::array< System::Int32 >(4) { 5, 0, 0, 0 });
			this->ME_UpDown->ValueChanged += gcnew System::EventHandler(this, &SampleDetectionsForm::ME_UpDown_ValueChanged);
			// 
			// SampleSizeLabel
			// 
			this->SampleSizeLabel->AutoSize = true;
			this->SampleSizeLabel->Location = System::Drawing::Point(15, 170);
			this->SampleSizeLabel->Name = L"SampleSizeLabel";
			this->SampleSizeLabel->Size = System::Drawing::Size(68, 13);
			this->SampleSizeLabel->TabIndex = 10;
			this->SampleSizeLabel->Text = L"Sample Size:";
			// 
			// TruePosTextBox
			// 
			this->TruePosTextBox->Location = System::Drawing::Point(60, 188);
			this->TruePosTextBox->Name = L"TruePosTextBox";
			this->TruePosTextBox->Size = System::Drawing::Size(68, 20);
			this->TruePosTextBox->TabIndex = 11;
			// 
			// TrueNegsTextBox
			// 
			this->TrueNegsTextBox->Location = System::Drawing::Point(60, 212);
			this->TrueNegsTextBox->Name = L"TrueNegsTextBox";
			this->TrueNegsTextBox->Size = System::Drawing::Size(68, 20);
			this->TrueNegsTextBox->TabIndex = 12;
			// 
			// TruePositivesLabel
			// 
			this->TruePositivesLabel->AutoSize = true;
			this->TruePositivesLabel->Location = System::Drawing::Point(137, 191);
			this->TruePositivesLabel->Name = L"TruePositivesLabel";
			this->TruePositivesLabel->Size = System::Drawing::Size(78, 13);
			this->TruePositivesLabel->TabIndex = 13;
			this->TruePositivesLabel->Text = L"True Positives*";
			// 
			// TrueNegativeLabel
			// 
			this->TrueNegativeLabel->AutoSize = true;
			this->TrueNegativeLabel->Location = System::Drawing::Point(137, 215);
			this->TrueNegativeLabel->Name = L"TrueNegativeLabel";
			this->TrueNegativeLabel->Size = System::Drawing::Size(84, 13);
			this->TrueNegativeLabel->TabIndex = 14;
			this->TrueNegativeLabel->Text = L"True Negatives*";
			// 
			// label1
			// 
			this->label1->AutoSize = true;
			this->label1->Font = (gcnew System::Drawing::Font(L"Microsoft Sans Serif", 8.25F, static_cast<System::Drawing::FontStyle>((System::Drawing::FontStyle::Bold | System::Drawing::FontStyle::Italic)),
				System::Drawing::GraphicsUnit::Point, static_cast<System::Byte>(0)));
			this->label1->Location = System::Drawing::Point(3, 264);
			this->label1->Name = L"label1";
			this->label1->Size = System::Drawing::Size(355, 13);
			this->label1->TabIndex = 15;
			this->label1->Text = L"*The Art and Science of Learning from Data, Chapter 8, 2006";
			// 
			// SampleDetectionsForm
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(601, 286);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->TrueNegativeLabel);
			this->Controls->Add(this->TruePositivesLabel);
			this->Controls->Add(this->TrueNegsTextBox);
			this->Controls->Add(this->TruePosTextBox);
			this->Controls->Add(this->SampleSizeLabel);
			this->Controls->Add(this->ME_UpDown);
			this->Controls->Add(this->CI_UpDown);
			this->Controls->Add(this->RunButton);
			this->Controls->Add(this->detectionFilesList);
			this->Controls->Add(this->DetectionFilesLabel);
			this->Controls->Add(this->ME_label);
			this->Controls->Add(this->CI_label);
			this->Name = L"SampleDetectionsForm";
			this->Text = L"Verify Detections using randomized samples";
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->CI_UpDown))->EndInit();
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->ME_UpDown))->EndInit();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion


	private: System::Void RunButton_Click(System::Object^  sender, System::EventArgs^  e);
	private: System::Void detectionFilesList_Click(System::Object^  sender, System::EventArgs^  e);
	private: System::Void CI_UpDown_ValueChanged(System::Object^  sender, System::EventArgs^  e);
	private: System::Void ME_UpDown_ValueChanged(System::Object^  sender, System::EventArgs^  e);
	private: System::Void fillSampleNumberTextBoxes();

};
}
