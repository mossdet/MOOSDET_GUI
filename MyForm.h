#pragma once

#include <string>
#include <vector>

#include "SignalProcessing.h"
#include "Tests.h"

#include <Windows.h>

using namespace System;
using namespace System::Collections;
using namespace System::Collections::Generic;
using namespace System::Text;
using namespace System::Diagnostics;

namespace MOSDET {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
	using namespace System::Runtime::InteropServices;

	public ref class MyForm : public System::Windows::Forms::Form {
	public:
		MyForm(void) {

			m_montageNr = 0;
			m_unipolar = true;
			m_bipolar = false;
			m_startTime = 0.0;
			m_endTime = 31536000.0;//1 year

			m_ProcessorGroup = 0;
			m_ProcessingPercent = 90;
			m_readMinutesEpochLenghth = 120;

			ExePath();
			InitializeComponent();

			this->textBox3->Text = m_strOutputDirectory;

			this->textBox1->ScrollBars = ScrollBars::Vertical;
			this->textBox2->ScrollBars = ScrollBars::Vertical;

			//setLowLevelProcessing();

			//Set NUMA Node
			GROUP_AFFINITY  GroupAffinity;
			PGROUP_AFFINITY PreviousGroupAffinity;
			BOOL success = GetThreadGroupAffinity(GetCurrentThread(), &GroupAffinity);
			GroupAffinity.Group = m_ProcessorGroup;
			success = SetThreadGroupAffinity(GetCurrentThread(), &GroupAffinity, PreviousGroupAffinity);
			//success = SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);// REALTIME_PRIORITY_CLASS NORMAL_PRIORITY_CLASS HIGH_PRIORITY_CLASS

			/*WORD numberOfProcessorGroups = GetMaximumProcessorGroupCount();
			for (int i = 1; i <= numberOfProcessorGroups; ++i) {
				std::string str = std::to_string(i);
				m_processorNumbers->Add(gcnew String(str.c_str()));
			}
			this->CPU_DropDown->Items->AddRange(m_processorNumbers->ToArray());*/
			m_processorNumbers->Add(gcnew String("1"));
			m_processorNumbers->Add(gcnew String("2"));
			this->CPU_DropDown->Items->AddRange(m_processorNumbers->ToArray());

			//characterizeFilter((char*)(void*)Marshal::StringToHGlobalAnsi(m_strOutputDirectory), 0, SignalProcessing::getOrder());
		}

	protected:
		/// Clean up any resources being used.
		~MyForm() {
			if (components) {
				delete components;
			}
		}

	protected:
		System::String^ m_strOutputDirectory;
		System::String^ m_strInputFileName;
		array<String^>^ m_strInputFileNames;
	private:std::vector<std::string> *m_selectedUnipolarChannels = new std::vector<std::string>;
	private:std::vector<std::string> *m_selectedBipolarChannels = new std::vector<std::string>;
			bool m_unipolar, m_bipolar;
			Double m_startTime;
			Double m_endTime;
			int m_fileType = 0;

			ArrayList^ m_processorNumbers = gcnew ArrayList();
			int m_ProcessorGroup;
			int m_ProcessingPercent;
			int m_readMinutesEpochLenghth;

			int m_montageNr;
			bool *m_useChannsFileBoolPtr = new bool(false);

	private: System::Windows::Forms::PictureBox^  pictureBox1;
	private: System::Windows::Forms::MenuStrip^  menuStrip1;
	private: System::Windows::Forms::ToolStripMenuItem^  fileToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  selectInputFilesToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  outputToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  selectOutputPathToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  detectToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  detectHFOsToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  detectSleepSpindlesToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  detectRipplesToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  detectFastRipplesToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  selectUnipolarChannelsToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  selectBipolarChannelsToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  generateEDFWithSelectedMontagesToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  detectSpikesToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  verifyToolStripMenuItem;
	private: System::Windows::Forms::ToolStripMenuItem^  verifyDetectionsToolStripMenuItem;

	private: System::Windows::Forms::Label^  label1;
	private: System::Windows::Forms::Label^  label2;
	private: System::Windows::Forms::Label^  label3;
	private: System::Windows::Forms::Label^  label4;
	private: System::Windows::Forms::Label^  label5;
	private: System::Windows::Forms::TextBox^  textBox1;
	public: System::Windows::Forms::TextBox^  textBox2;
	private: System::Windows::Forms::TextBox^  textBox3;
	private: System::Windows::Forms::TextBox^  textBox4;
	private: System::Windows::Forms::TextBox^  textBox5;
	private: System::Windows::Forms::CheckBox^  getPerformanceCheckBox1;
	private: System::Windows::Forms::Button^  button1;
	private: System::Windows::Forms::TextBox^  runStatus;
	private: System::Windows::Forms::ComboBox^  CPU_DropDown;
	private: System::Windows::Forms::Label^  label6;
	private: System::Windows::Forms::Label^  label7;
	private: System::Windows::Forms::TextBox^  PercentCPU;
	private: System::Windows::Forms::TextBox^  textBox6;
	private: System::Windows::Forms::Label^  label8;
	private: System::Windows::Forms::CheckBox^  scalpEEG;







	protected:
	private:
		System::ComponentModel::Container ^components;
#pragma region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void) {
			System::ComponentModel::ComponentResourceManager^  resources = (gcnew System::ComponentModel::ComponentResourceManager(MyForm::typeid));
			this->pictureBox1 = (gcnew System::Windows::Forms::PictureBox());
			this->menuStrip1 = (gcnew System::Windows::Forms::MenuStrip());
			this->fileToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->selectInputFilesToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->selectUnipolarChannelsToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->selectBipolarChannelsToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->outputToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->selectOutputPathToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->generateEDFWithSelectedMontagesToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->detectToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->detectHFOsToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->detectRipplesToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->detectFastRipplesToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->detectSpikesToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->detectSleepSpindlesToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->verifyToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->verifyDetectionsToolStripMenuItem = (gcnew System::Windows::Forms::ToolStripMenuItem());
			this->label1 = (gcnew System::Windows::Forms::Label());
			this->label2 = (gcnew System::Windows::Forms::Label());
			this->label3 = (gcnew System::Windows::Forms::Label());
			this->textBox1 = (gcnew System::Windows::Forms::TextBox());
			this->textBox2 = (gcnew System::Windows::Forms::TextBox());
			this->textBox3 = (gcnew System::Windows::Forms::TextBox());
			this->label4 = (gcnew System::Windows::Forms::Label());
			this->label5 = (gcnew System::Windows::Forms::Label());
			this->textBox4 = (gcnew System::Windows::Forms::TextBox());
			this->textBox5 = (gcnew System::Windows::Forms::TextBox());
			this->getPerformanceCheckBox1 = (gcnew System::Windows::Forms::CheckBox());
			this->button1 = (gcnew System::Windows::Forms::Button());
			this->runStatus = (gcnew System::Windows::Forms::TextBox());
			this->CPU_DropDown = (gcnew System::Windows::Forms::ComboBox());
			this->label6 = (gcnew System::Windows::Forms::Label());
			this->label7 = (gcnew System::Windows::Forms::Label());
			this->PercentCPU = (gcnew System::Windows::Forms::TextBox());
			this->textBox6 = (gcnew System::Windows::Forms::TextBox());
			this->label8 = (gcnew System::Windows::Forms::Label());
			this->scalpEEG = (gcnew System::Windows::Forms::CheckBox());
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox1))->BeginInit();
			this->menuStrip1->SuspendLayout();
			this->SuspendLayout();
			// 
			// pictureBox1
			// 
			resources->ApplyResources(this->pictureBox1, L"pictureBox1");
			this->pictureBox1->Name = L"pictureBox1";
			this->pictureBox1->TabStop = false;
			// 
			// menuStrip1
			// 
			this->menuStrip1->Items->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(4) {
				this->fileToolStripMenuItem,
					this->outputToolStripMenuItem, this->detectToolStripMenuItem, this->verifyToolStripMenuItem
			});
			resources->ApplyResources(this->menuStrip1, L"menuStrip1");
			this->menuStrip1->Name = L"menuStrip1";
			// 
			// fileToolStripMenuItem
			// 
			this->fileToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(3) {
				this->selectInputFilesToolStripMenuItem,
					this->selectUnipolarChannelsToolStripMenuItem, this->selectBipolarChannelsToolStripMenuItem
			});
			this->fileToolStripMenuItem->Name = L"fileToolStripMenuItem";
			resources->ApplyResources(this->fileToolStripMenuItem, L"fileToolStripMenuItem");
			// 
			// selectInputFilesToolStripMenuItem
			// 
			this->selectInputFilesToolStripMenuItem->Name = L"selectInputFilesToolStripMenuItem";
			resources->ApplyResources(this->selectInputFilesToolStripMenuItem, L"selectInputFilesToolStripMenuItem");
			this->selectInputFilesToolStripMenuItem->Click += gcnew System::EventHandler(this, &MyForm::selectInputFilesToolStripMenuItem_Click);
			// 
			// selectUnipolarChannelsToolStripMenuItem
			// 
			this->selectUnipolarChannelsToolStripMenuItem->Name = L"selectUnipolarChannelsToolStripMenuItem";
			resources->ApplyResources(this->selectUnipolarChannelsToolStripMenuItem, L"selectUnipolarChannelsToolStripMenuItem");
			this->selectUnipolarChannelsToolStripMenuItem->Click += gcnew System::EventHandler(this, &MyForm::selectUnipolarChannelsToolStripMenuItem_Click);
			// 
			// selectBipolarChannelsToolStripMenuItem
			// 
			this->selectBipolarChannelsToolStripMenuItem->Name = L"selectBipolarChannelsToolStripMenuItem";
			resources->ApplyResources(this->selectBipolarChannelsToolStripMenuItem, L"selectBipolarChannelsToolStripMenuItem");
			this->selectBipolarChannelsToolStripMenuItem->Click += gcnew System::EventHandler(this, &MyForm::selectBipolarChannelsToolStripMenuItem_Click);
			// 
			// outputToolStripMenuItem
			// 
			this->outputToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(2) {
				this->selectOutputPathToolStripMenuItem,
					this->generateEDFWithSelectedMontagesToolStripMenuItem
			});
			this->outputToolStripMenuItem->Name = L"outputToolStripMenuItem";
			resources->ApplyResources(this->outputToolStripMenuItem, L"outputToolStripMenuItem");
			// 
			// selectOutputPathToolStripMenuItem
			// 
			this->selectOutputPathToolStripMenuItem->Name = L"selectOutputPathToolStripMenuItem";
			resources->ApplyResources(this->selectOutputPathToolStripMenuItem, L"selectOutputPathToolStripMenuItem");
			this->selectOutputPathToolStripMenuItem->Click += gcnew System::EventHandler(this, &MyForm::selectOutputPathToolStripMenuItem_Click);
			// 
			// generateEDFWithSelectedMontagesToolStripMenuItem
			// 
			this->generateEDFWithSelectedMontagesToolStripMenuItem->Name = L"generateEDFWithSelectedMontagesToolStripMenuItem";
			resources->ApplyResources(this->generateEDFWithSelectedMontagesToolStripMenuItem, L"generateEDFWithSelectedMontagesToolStripMenuItem");
			this->generateEDFWithSelectedMontagesToolStripMenuItem->Click += gcnew System::EventHandler(this, &MyForm::generateMontageFilesToolStripMenuItem_Click);
			// 
			// detectToolStripMenuItem
			// 
			this->detectToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(5) {
				this->detectHFOsToolStripMenuItem,
					this->detectRipplesToolStripMenuItem, this->detectFastRipplesToolStripMenuItem, this->detectSpikesToolStripMenuItem, this->detectSleepSpindlesToolStripMenuItem
			});
			this->detectToolStripMenuItem->Name = L"detectToolStripMenuItem";
			resources->ApplyResources(this->detectToolStripMenuItem, L"detectToolStripMenuItem");
			// 
			// detectHFOsToolStripMenuItem
			// 
			this->detectHFOsToolStripMenuItem->Name = L"detectHFOsToolStripMenuItem";
			resources->ApplyResources(this->detectHFOsToolStripMenuItem, L"detectHFOsToolStripMenuItem");
			this->detectHFOsToolStripMenuItem->Click += gcnew System::EventHandler(this, &MyForm::detectHFOsToolStripMenuItem_Click);
			// 
			// detectRipplesToolStripMenuItem
			// 
			this->detectRipplesToolStripMenuItem->Name = L"detectRipplesToolStripMenuItem";
			resources->ApplyResources(this->detectRipplesToolStripMenuItem, L"detectRipplesToolStripMenuItem");
			this->detectRipplesToolStripMenuItem->Click += gcnew System::EventHandler(this, &MyForm::detectRipplesToolStripMenuItem_Click);
			// 
			// detectFastRipplesToolStripMenuItem
			// 
			this->detectFastRipplesToolStripMenuItem->Name = L"detectFastRipplesToolStripMenuItem";
			resources->ApplyResources(this->detectFastRipplesToolStripMenuItem, L"detectFastRipplesToolStripMenuItem");
			this->detectFastRipplesToolStripMenuItem->Click += gcnew System::EventHandler(this, &MyForm::detectFastRipplesToolStripMenuItem_Click);
			// 
			// detectSpikesToolStripMenuItem
			// 
			this->detectSpikesToolStripMenuItem->Name = L"detectSpikesToolStripMenuItem";
			resources->ApplyResources(this->detectSpikesToolStripMenuItem, L"detectSpikesToolStripMenuItem");
			this->detectSpikesToolStripMenuItem->Click += gcnew System::EventHandler(this, &MyForm::detectSpikesToolStripMenuItem_Click);
			// 
			// detectSleepSpindlesToolStripMenuItem
			// 
			this->detectSleepSpindlesToolStripMenuItem->Name = L"detectSleepSpindlesToolStripMenuItem";
			resources->ApplyResources(this->detectSleepSpindlesToolStripMenuItem, L"detectSleepSpindlesToolStripMenuItem");
			this->detectSleepSpindlesToolStripMenuItem->Click += gcnew System::EventHandler(this, &MyForm::detectSleepSpindlesToolStripMenuItem_Click);
			// 
			// verifyToolStripMenuItem
			// 
			this->verifyToolStripMenuItem->DropDownItems->AddRange(gcnew cli::array< System::Windows::Forms::ToolStripItem^  >(1) { this->verifyDetectionsToolStripMenuItem });
			this->verifyToolStripMenuItem->Name = L"verifyToolStripMenuItem";
			resources->ApplyResources(this->verifyToolStripMenuItem, L"verifyToolStripMenuItem");
			// 
			// verifyDetectionsToolStripMenuItem
			// 
			this->verifyDetectionsToolStripMenuItem->Name = L"verifyDetectionsToolStripMenuItem";
			resources->ApplyResources(this->verifyDetectionsToolStripMenuItem, L"verifyDetectionsToolStripMenuItem");
			this->verifyDetectionsToolStripMenuItem->Click += gcnew System::EventHandler(this, &MyForm::verifyDetectionsToolStripMenuItem_Click_1);
			// 
			// label1
			// 
			resources->ApplyResources(this->label1, L"label1");
			this->label1->Name = L"label1";
			// 
			// label2
			// 
			resources->ApplyResources(this->label2, L"label2");
			this->label2->Name = L"label2";
			// 
			// label3
			// 
			resources->ApplyResources(this->label3, L"label3");
			this->label3->Name = L"label3";
			// 
			// textBox1
			// 
			this->textBox1->BackColor = System::Drawing::SystemColors::ControlDark;
			this->textBox1->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
			resources->ApplyResources(this->textBox1, L"textBox1");
			this->textBox1->Name = L"textBox1";
			// 
			// textBox2
			// 
			this->textBox2->BackColor = System::Drawing::SystemColors::ControlDark;
			this->textBox2->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
			resources->ApplyResources(this->textBox2, L"textBox2");
			this->textBox2->Name = L"textBox2";
			// 
			// textBox3
			// 
			this->textBox3->BackColor = System::Drawing::SystemColors::ControlDark;
			this->textBox3->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
			resources->ApplyResources(this->textBox3, L"textBox3");
			this->textBox3->Name = L"textBox3";
			// 
			// label4
			// 
			resources->ApplyResources(this->label4, L"label4");
			this->label4->Name = L"label4";
			// 
			// label5
			// 
			resources->ApplyResources(this->label5, L"label5");
			this->label5->Name = L"label5";
			// 
			// textBox4
			// 
			this->textBox4->BackColor = System::Drawing::SystemColors::ControlDark;
			this->textBox4->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
			resources->ApplyResources(this->textBox4, L"textBox4");
			this->textBox4->Name = L"textBox4";
			this->textBox4->TextChanged += gcnew System::EventHandler(this, &MyForm::textBox4_TextChanged);
			// 
			// textBox5
			// 
			this->textBox5->BackColor = System::Drawing::SystemColors::ControlDark;
			this->textBox5->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
			resources->ApplyResources(this->textBox5, L"textBox5");
			this->textBox5->Name = L"textBox5";
			this->textBox5->TextChanged += gcnew System::EventHandler(this, &MyForm::textBox5_TextChanged);
			// 
			// getPerformanceCheckBox1
			// 
			resources->ApplyResources(this->getPerformanceCheckBox1, L"getPerformanceCheckBox1");
			this->getPerformanceCheckBox1->Name = L"getPerformanceCheckBox1";
			this->getPerformanceCheckBox1->UseVisualStyleBackColor = true;
			// 
			// button1
			// 
			this->button1->BackColor = System::Drawing::SystemColors::ControlDark;
			this->button1->FlatAppearance->BorderColor = System::Drawing::SystemColors::ControlDark;
			this->button1->FlatAppearance->BorderSize = 0;
			resources->ApplyResources(this->button1, L"button1");
			this->button1->Name = L"button1";
			this->button1->UseVisualStyleBackColor = false;
			this->button1->Click += gcnew System::EventHandler(this, &MyForm::restart_Click);
			// 
			// runStatus
			// 
			this->runStatus->BackColor = System::Drawing::SystemColors::ControlDark;
			this->runStatus->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
			resources->ApplyResources(this->runStatus, L"runStatus");
			this->runStatus->Name = L"runStatus";
			// 
			// CPU_DropDown
			// 
			this->CPU_DropDown->BackColor = System::Drawing::SystemColors::ControlDark;
			resources->ApplyResources(this->CPU_DropDown, L"CPU_DropDown");
			this->CPU_DropDown->FormattingEnabled = true;
			this->CPU_DropDown->Name = L"CPU_DropDown";
			this->CPU_DropDown->Tag = L"CPU_DropDown";
			this->CPU_DropDown->SelectedIndexChanged += gcnew System::EventHandler(this, &MyForm::CPU_DropDown_SelectedIndexChanged);
			// 
			// label6
			// 
			resources->ApplyResources(this->label6, L"label6");
			this->label6->Name = L"label6";
			// 
			// label7
			// 
			resources->ApplyResources(this->label7, L"label7");
			this->label7->Name = L"label7";
			// 
			// PercentCPU
			// 
			this->PercentCPU->BackColor = System::Drawing::SystemColors::ControlDark;
			this->PercentCPU->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
			resources->ApplyResources(this->PercentCPU, L"PercentCPU");
			this->PercentCPU->Name = L"PercentCPU";
			this->PercentCPU->TextChanged += gcnew System::EventHandler(this, &MyForm::PercentCPU_TextChanged);
			// 
			// textBox6
			// 
			this->textBox6->BackColor = System::Drawing::SystemColors::ControlDark;
			this->textBox6->BorderStyle = System::Windows::Forms::BorderStyle::FixedSingle;
			resources->ApplyResources(this->textBox6, L"textBox6");
			this->textBox6->Name = L"textBox6";
			this->textBox6->TextChanged += gcnew System::EventHandler(this, &MyForm::textBox6_TextChanged);
			// 
			// label8
			// 
			resources->ApplyResources(this->label8, L"label8");
			this->label8->Name = L"label8";
			// 
			// scalpEEG
			// 
			resources->ApplyResources(this->scalpEEG, L"scalpEEG");
			this->scalpEEG->Name = L"scalpEEG";
			this->scalpEEG->UseVisualStyleBackColor = true;
			// 
			// MyForm
			// 
			resources->ApplyResources(this, L"$this");
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->BackColor = System::Drawing::Color::LightSlateGray;
			this->Controls->Add(this->scalpEEG);
			this->Controls->Add(this->textBox6);
			this->Controls->Add(this->label8);
			this->Controls->Add(this->PercentCPU);
			this->Controls->Add(this->label7);
			this->Controls->Add(this->label6);
			this->Controls->Add(this->CPU_DropDown);
			this->Controls->Add(this->runStatus);
			this->Controls->Add(this->button1);
			this->Controls->Add(this->getPerformanceCheckBox1);
			this->Controls->Add(this->label1);
			this->Controls->Add(this->label2);
			this->Controls->Add(this->label3);
			this->Controls->Add(this->label4);
			this->Controls->Add(this->label5);
			this->Controls->Add(this->textBox1);
			this->Controls->Add(this->textBox2);
			this->Controls->Add(this->textBox3);
			this->Controls->Add(this->textBox4);
			this->Controls->Add(this->textBox5);
			this->Controls->Add(this->pictureBox1);
			this->Controls->Add(this->menuStrip1);
			this->MainMenuStrip = this->menuStrip1;
			this->Name = L"MyForm";
			(cli::safe_cast<System::ComponentModel::ISupportInitialize^>(this->pictureBox1))->EndInit();
			this->menuStrip1->ResumeLayout(false);
			this->menuStrip1->PerformLayout();
			this->ResumeLayout(false);
			this->PerformLayout();

		}
#pragma endregion

	private: System::Void selectInputFilesToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	private: System::Void selectUnipolarChannelsToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	private: System::Void selectBipolarChannelsToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	private: System::Void selectOutputPathToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	private: System::Void detectHFOsToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	private: System::Void detectRipplesToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	private: System::Void detectFastRipplesToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	private: System::Void detectSleepSpindlesToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	private: System::Void detectSpikesToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	private: System::Void generateMontageFilesToolStripMenuItem_Click(System::Object^  sender, System::EventArgs^  e);
	private: System::Void verifyDetectionsToolStripMenuItem_Click_1(System::Object^  sender, System::EventArgs^  e);
	private: System::Void textBox4_TextChanged(System::Object^  sender, System::EventArgs^  e);
	private: System::Void textBox5_TextChanged(System::Object^  sender, System::EventArgs^  e);
	private: System::String^ ExePath();
	private: System::Double executionTimeLog(System::String^ strInputFileName, int loopNr, double samplingRate, double analysisDuration, int nrChanns, double durationMs);
	private: System::Void restart_Click(System::Object^  sender, System::EventArgs^  e);
	private: System::Void CPU_DropDown_SelectedIndexChanged(System::Object^  sender, System::EventArgs^  e);
	private: System::Void PercentCPU_TextChanged(System::Object^  sender, System::EventArgs^  e);
	private: System::Void textBox6_TextChanged(System::Object^  sender, System::EventArgs^  e);
	public: System::Void setLowLevelProcessing();

};
}

