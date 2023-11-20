#pragma once

namespace MOSDET {

	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;

	/// <summary>
	/// Summary for MyForm2
	/// </summary>
	public ref class MyForm2 : public System::Windows::Forms::Form
	{
	public:
		MyForm2(int fileType, System::String^ strInputFileName, array<String^>^ strInputFileNames, bool unipolarChannels, bool bipolarChannels, std::vector<std::string> *selectedUnipolarChanels, 
			std::vector<std::string> *selectedBipolarChanels, bool *useChannsFileBoolPtr, System::Windows::Forms::TextBox^ selectedChannsTextBox, bool useScalpEEG)
		{
			m_fileType = fileType;
			m_strInputFileName = strInputFileName;
			m_strInputFileNames = strInputFileNames;
			m_unipolar = unipolarChannels;
			m_bipolar = bipolarChannels;
			m_selectedUnipolarChannels = selectedUnipolarChanels;
			m_selectedBipolarChannels = selectedBipolarChanels;

			m_useChannsFileBoolPtr = useChannsFileBoolPtr;

			m_selectedChannsTextBox = selectedChannsTextBox;

			m_useScalpEEG = useScalpEEG;

			readChannels(m_unipolar, m_bipolar);
			fillChannNamesArray();
			InitializeComponent();
			//
			//TODO: Add the constructor code here
			//
		}

	protected:
		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		~MyForm2()
		{
			if (components)
			{
				delete components;
			}
		}

		// Designer variables
	protected:
		int m_fileType = 0;
		System::String^ m_strInputFileName;
		array<String^>^ m_strInputFileNames;
		bool m_unipolar, m_bipolar, m_useScalpEEG;
	private:std::vector<std::string> *m_unipolarChanels = new std::vector<std::string>;
	private:std::vector<std::string> *m_bipolarChanels = new std::vector<std::string>;
	private:std::vector<std::string> *m_selectedUnipolarChannels;
	private:std::vector<std::string> *m_selectedBipolarChannels;

	private: bool *m_useChannsFileBoolPtr;
	private: cli::array< System::Windows::Forms::ListViewItem^  >^ m_channsArray;




	private: System::Windows::Forms::Button^  okButton;
	private: System::Windows::Forms::Button^  useFileButton;
	private: System::Windows::Forms::ListView^  listView1;
	public: System::Windows::Forms::TextBox^  m_selectedChannsTextBox;

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
			System::Windows::Forms::ListViewItem^  listViewItem1 = (gcnew System::Windows::Forms::ListViewItem(L"Ch1"));
			this->okButton = (gcnew System::Windows::Forms::Button());
			this->listView1 = (gcnew System::Windows::Forms::ListView());
			this->useFileButton = (gcnew System::Windows::Forms::Button());
			this->SuspendLayout();
			// 
			// okButton
			// 
			this->okButton->Location = System::Drawing::Point(1416, 12);
			this->okButton->Name = L"okButton";
			this->okButton->Size = System::Drawing::Size(75, 23);
			this->okButton->TabIndex = 3;
			this->okButton->Text = L"OK";
			this->okButton->UseVisualStyleBackColor = true;
			this->okButton->Click += gcnew System::EventHandler(this, &MyForm2::okButton_Click);
			// 
			// useFileButton
			// 
			this->useFileButton->Location = System::Drawing::Point(1417, 42);
			this->useFileButton->Name = L"UseFile";
			this->useFileButton->Size = System::Drawing::Size(75, 23);
			this->useFileButton->TabIndex = 4;
			this->useFileButton->Text = L"Use File";
			this->useFileButton->UseVisualStyleBackColor = true;
			this->useFileButton->Click += gcnew System::EventHandler(this, &MyForm2::useFileButton_Click);
			// 
			// listView1
			// 
			this->listView1->Activation = System::Windows::Forms::ItemActivation::OneClick;
			this->listView1->Alignment = System::Windows::Forms::ListViewAlignment::Left;
			this->listView1->CheckBoxes = true;
			this->listView1->Cursor = System::Windows::Forms::Cursors::Hand;
			this->listView1->GridLines = false;
			this->listView1->HideSelection = false;
			this->listView1->HoverSelection = true;
			this->listView1->AutoArrange = true;
			listViewItem1->StateImageIndex = 0;
			//this->listView1->Items->AddRange(gcnew cli::array< System::Windows::Forms::ListViewItem^  >(1) { listViewItem1 });
			this->listView1->Items->AddRange(m_channsArray);
			this->listView1->LabelWrap = false;
			this->listView1->Location = System::Drawing::Point(12, 3);
			this->listView1->Name = L"listView1";
			this->listView1->Size = System::Drawing::Size(1398, 604);
			this->listView1->TabIndex = 1;
			this->listView1->UseCompatibleStateImageBehavior = false;
			this->listView1->KeyDown += gcnew KeyEventHandler(this, &MyForm2::listView1_KeyDown);
			// 
			// MyForm2
			// 
			this->AutoScaleDimensions = System::Drawing::SizeF(6, 13);
			this->AutoScaleMode = System::Windows::Forms::AutoScaleMode::Font;
			this->ClientSize = System::Drawing::Size(1503, 619);
			this->Controls->Add(this->useFileButton);
			this->Controls->Add(this->listView1);
			this->Controls->Add(this->okButton);
			this->Name = L"MyForm2";
			this->Text = L"Channel Selection";
			this->ResumeLayout(false);

		}
#pragma endregion
	//this->listView1->Items->AddRange(gcnew cli::array< System::Windows::Forms::ListViewItem^  >(1) { listViewItem1 });
	//this->listView1->Items->AddRange(m_channsArray);

	private: bool readChannels(bool unipolarChannels, bool bipolarChannels);
	private: void fillChannNamesArray(void);
	private: System::Void okButton_Click(System::Object^  sender, System::EventArgs^  e);
	private: System::Void useFileButton_Click(System::Object^  sender, System::EventArgs^  e);
	private: System::Void listView1_KeyDown(System::Object^  sender, System::Windows::Forms::KeyEventArgs^ e);
	};
}
