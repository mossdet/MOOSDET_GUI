#include <chrono>
#include <random>
#include <fstream>
#include <direct.h>
#include <windows.h>
#include <math.h>

#include <omp.h>

#include "SS_Detector.h"
#include "AbstractReader.h"
#include "IO_EDFPlus_SignalImporter_c.h"
#include "BinaryReader.h"
#include "SS_EventsHandler.h"
#include "SignalProcessing.h"
#include "CorrelationFeatureSelection.h"
#include "SVM_Detector.h"
#include "Plotter.h"

#define EPOCH				0.100//s
#define SS_LOW_FREQ			11
#define SS_HIGH_FREQ		16

#define RAW					1
#define NOTCHED				2
#define SPINDLE				3

using namespace System::Runtime::InteropServices;

SS_Detector::SS_Detector(System::String^ strInputFileName, System::String^ strDirName, int patIdx, int montage, std::vector<std::string> *selectedUnipolarChanels, std::vector<std::string> *selectedBipolarChanels, 
	std::string &eoiName, bool useChannsFileBool, System::Double startTime, System::Double endTime, bool compareDetectionsWithVisalMarks, bool useScalpEEG) {

	m_patIdx = patIdx;

	m_strFileName = (char*)(void*)Marshal::StringToHGlobalAnsi(strInputFileName);
	m_outputDirectory = (char*)(void*)Marshal::StringToHGlobalAnsi(strDirName);

	getPatientNameAndPath();

	m_montageNr = montage;
	m_selectedUnipolarChanels = *selectedUnipolarChanels;
	m_selectedBipolarChanels = *selectedBipolarChanels;
	m_useMontage = montage > 0;

	m_eoiName = eoiName;
	m_useChannsFileBool = useChannsFileBool;
	m_analysisStartTime = startTime;
	m_analysisEndTime = endTime;

	m_waveletParams->dStartFrequency = 1;
	m_waveletParams->dEndFrequency = 25;
	m_waveletParams->dDeltaFrequency = 1;
	m_waveletParams->waveletOscillations = 6;
	m_exePath = ExePath();

	m_firstMarkedSample = -1;
	m_lastMarkedSample = 0;

	m_compareDetectionsWithVisualMarks = compareDetectionsWithVisalMarks;

	m_useScalpEEG = useScalpEEG;

	int processorGroup = 0;
	int processingPercent = 75;

	//Parallel Computing
	double maxNumThreads = omp_get_max_threads();
	int numProcessors = omp_get_num_procs();

	maxNumThreads = maxNumThreads * ((double)processingPercent / 100.0);
	omp_set_num_threads((int)maxNumThreads);

	//HANDLE process = GetCurrentProcess();
	//Set NUMA Node
	/*GROUP_AFFINITY  GroupAffinity;
	PGROUP_AFFINITY PreviousGroupAffinity;
	BOOL success = GetThreadGroupAffinity(GetCurrentThread(), &GroupAffinity);
	GroupAffinity.Group = (WORD)processorGroup;
	success = SetThreadGroupAffinity(GetCurrentThread(), &GroupAffinity, PreviousGroupAffinity);*/

}

long SS_Detector::readEEG() {

	m_MASS_File = false;//m_patientName.find("PSG") != std::string::npos;

	if (m_useChannsFileBool) {
		readChannelsFromFile();
		//getSpecificStartAndEndTimes();
	}



	// check for eeg extension
	int fileType;
	if (m_strFileName.find(".edf") != std::string::npos || m_strFileName.find(".EDF") != std::string::npos) {
		fileType = 1;
	}
	else if (m_strFileName.find(".bin") != std::string::npos || m_strFileName.find(".BIN") != std::string::npos) {
		fileType = 2;
	}

	AbstractReader* pReader = NULL;
	if (fileType == 1) {
		pReader = new IO_EDFPlus_SignalImporter_c(m_strFileName, m_selectedUnipolarChanels, m_selectedBipolarChanels, m_useScalpEEG);	// selected channels vectors are empty
	}
	else if (fileType == 2) {
		pReader = new BinaryReader(m_strFileName, m_selectedUnipolarChanels, m_selectedBipolarChanels, m_useScalpEEG);	// selected channels vectors are empty
	}
	else {
		return 0;
	}

	pReader->readUnipolarLabels(m_unipolarLabels);
	pReader->readMontageLabels(m_montageLabels);

	m_samplingRate = pReader->getSamplingRate();
	if (m_MASS_File) {
		m_samplingRate = 256.016385;
	}

	m_nrChannels = m_useMontage ? m_montageLabels.size() : m_unipolarLabels.size();
	generate_EOI_ChannelsFile();

	if (getPerformance()) {
		readAndWriteEOI_VisualAnnotations();
	}

	m_fileAnalysisStartSample = (m_analysisStartTime * m_samplingRate) > 0 ? (m_analysisStartTime * m_samplingRate) : 0;
	m_fileAnalysisSamplesToRead = (m_analysisEndTime - m_analysisStartTime + 1) * m_samplingRate;// pReader->getSampleCount();

	if (m_useMontage) {
		bool ok = pReader->ReadReformattedSamples(m_fileAnalysisStartSample, m_fileAnalysisSamplesToRead, m_reformattedSignalSamples);
		if (!ok)
			return 0;
	}
	else {
		bool ok = pReader->ReadSamples(m_fileAnalysisStartSample, m_fileAnalysisSamplesToRead, m_reformattedSignalSamples);
		if (!ok)
			return 0;
	}

	m_totalSampleCount = m_reformattedSignalSamples.size() > 0 ? m_reformattedSignalSamples.back().size() : 0;

	m_fileStartDateAndTime = pReader->getDateAndTime();

	return m_totalSampleCount;
}

bool SS_Detector::characterizeEEG() {

	double epochLength = ceil(EPOCH * m_samplingRate);		//20ms Epoch Length

															// Initialize variables for the loop reading the whole EEG signal
	double normLength = 1 * 60 * 60.0 * 12;	//12 hours
	long totalFileSamples = m_totalSampleCount;
	long samplesToRead = normLength * m_samplingRate < m_totalSampleCount ? normLength * m_samplingRate : m_totalSampleCount;
	long firstSampleToRead = 0;
	long unreadSamples = m_totalSampleCount;
	int loopNr = 0;


	while (unreadSamples > 0) {

		SS_Features signalFeatures;
		signalFeatures.setMatrices(m_nrChannels);

		int waveletDelay = getWaveletCausedDelay();// +2 * epochLength; //TODO
		int filterDelay = 2 * getFilteringCausedDelay();// +2 * epochLength; //TODO
		int delay = waveletDelay >= filterDelay ? 1 * waveletDelay : 1 * filterDelay;

		unsigned rewind = 0;
		if (firstSampleToRead > delay) {
			rewind = delay;
		}
		unsigned overlap = 0;
		if (firstSampleToRead + samplesToRead + delay < m_totalSampleCount) {
			overlap = delay;
		}

		long cycleReadStart = firstSampleToRead - rewind;
		long cycleReadEnd = firstSampleToRead + samplesToRead + overlap;
		//Read cyccle samples
		matrixStd cycleReformattedSamples(m_reformattedSignalSamples.size());
#pragma omp parallel for
		for (int ch = 0; ch < m_reformattedSignalSamples.size(); ++ch) {
			for (long s = cycleReadStart; s < cycleReadEnd; ++s) {
				m_MASS_File;
				double val = m_reformattedSignalSamples[ch][s];
				if (m_MASS_File)
					val *= -1;
				cycleReformattedSamples[ch].push_back(val);
				//cycleReformattedSamples[ch].push_back(0);
			}
			//cycleReformattedSamples[ch][(cycleReadEnd - cycleReadStart)/2] = 1;
		}

		matrixStd rawSignal;
		//Filter selected channels
		matrixStd antiOffset_plus_notch, spindleSelctdChs;

		//Filter power line noise
		if (m_MASS_File) {
			matrixStd matA;
			SignalProcessing::applyHighPassFilterToSignal(cycleReformattedSamples, matA, 1, m_samplingRate);										// Filter offset and DC components
			SignalProcessing::applyBandStopFilterToSignal(matA, antiOffset_plus_notch, 59, 61, m_samplingRate); matA.clear();							//50
		}
		else {
			matrixStd matA;
			SignalProcessing::applyHighPassFilterToSignal(cycleReformattedSamples, matA, 1, m_samplingRate);										// Filter offset and DC components
			SignalProcessing::applyBandStopFilterToSignal(matA, antiOffset_plus_notch, 49, 51, m_samplingRate); matA.clear();							//50
		}
																																								// Filter in the frequency band of EOI
		SignalProcessing::spindleBP(cycleReformattedSamples, spindleSelctdChs, SS_LOW_FREQ, SS_HIGH_FREQ, m_samplingRate);

		//saveSelectedEEG_DataToFile(firstSampleToRead, rewind, overlap, cycleReformattedSamples, antiOffset_plus_notch, spindleSelctdChs, 0);
		//saveSelectedEEG_DataToFile(firstSampleToRead, rewind, overlap, cycleReformattedSamples, antiOffset_plus_notch, fastRippleSelectedChs, 0);
		/*for (int ch = 0; ch < m_reformattedSignalSamples.size(); ++ch) {
		plotSelectedEE_Channel(firstSampleToRead, rewind, overlap, cycleReformattedSamples, antiOffset_plus_notch, rippleSelctdChs, fastRippleSelectedChs, ch);
		}*/

		matrixStd waveletPowOutput;
		int firstChann = 0;
		int lastChann = cycleReformattedSamples.size() - 1;
		bool waveletError = getWaveletFrequencyeAnalysisAllSignal(cycleReformattedSamples, signalFeatures, rewind, overlap, firstSampleToRead, samplesToRead, firstChann, lastChann, epochLength);

#pragma omp parallel sections
		{
#pragma omp section
			{
				getFeatureAnalysisAllSignals(cycleReformattedSamples, signalFeatures, rewind, overlap, firstSampleToRead, samplesToRead, epochLength, RAW);
			}

#pragma omp section
			{
				getFeatureAnalysisAllSignals(antiOffset_plus_notch, signalFeatures, rewind, overlap, firstSampleToRead, samplesToRead, epochLength, NOTCHED);
			}
#pragma omp section
			{
				getFeatureAnalysisAllSignals(spindleSelctdChs, signalFeatures, rewind, overlap, firstSampleToRead, samplesToRead, epochLength, SPINDLE);
			}
		}


		detect(signalFeatures, firstSampleToRead); //TODO
		//generateEpochCharacterizationSpindles(signalFeatures);

		unreadSamples = totalFileSamples - (firstSampleToRead + samplesToRead);
		firstSampleToRead += samplesToRead;
		if (samplesToRead > unreadSamples) {
			samplesToRead = unreadSamples;
		}
	}

	return true;
}

bool SS_Detector::detect(SS_Features &signalFeatures, long firstLocalSampleToRead) {

	if (signalFeatures.m_time.empty())
		return false;

	int nrChanns = signalFeatures.features[0]->size();
	int epochSkip = firstLocalSampleToRead == 0 ? 100 : 0;
	double maxOnsetDiff = 1000;
	double minSharedPercent = 0.25;
	bool joinVisualMarks = true;
	bool useWindowConfInterv = false;
	
	std::string outputPath = m_outputDirectory + "/MOSSDET_Output";
	std::string patPath = m_outputDirectory + "/MOSSDET_Output/" + m_patientName;
	mkdir(outputPath.c_str());
	mkdir(patPath.c_str());

	
	for (int channIdx = 0; channIdx < nrChanns; ++channIdx) {

		if (getPerformance()) {
			if (m_markedSS[channIdx].empty()) {
				continue;
			}
		}

		std::string channelName = m_useMontage ? m_montageLabels[channIdx].montageName : m_unipolarLabels[channIdx].contactName;
		int idNumber = m_useMontage ? m_montageLabels[channIdx].montageMOSSDET_Nr : m_unipolarLabels[channIdx].contactGlobalIdx;

		//Fill training epochs
		long nrEpochs = signalFeatures.features[0]->at(channIdx).size();
		std::vector<EOI_Epoch>  inputEpochs(nrEpochs - epochSkip);
		for (int epIdx = epochSkip; epIdx < nrEpochs; ++epIdx) {
			inputEpochs[epIdx - epochSkip].channel = channIdx;
			inputEpochs[epIdx - epochSkip].time = signalFeatures.m_time[epIdx];

			int numFeats = signalFeatures.features.size();
			inputEpochs[epIdx - epochSkip].inputVals.resize(numFeats);

			for (int featIdx = 0; featIdx < numFeats; ++featIdx)
				inputEpochs[epIdx - epochSkip].inputVals[featIdx] = signalFeatures.features[featIdx]->at(channIdx).at(epIdx);
		}

		std::vector<unsigned> spindleSelectedFeatures{ 1, 13, 14, 22, 25, 27, 28, 31, 32, 34, 39, 43 };
		//std::string spindleFunctionName = m_exePath + "/decisionFunctions/reducedDecisionFunction_1e-05_0.78125_10-28, 21_49_13.dat";
		std::string spindleFunctionName = m_exePath + "/svm_spindle.dat";
		double spindleTh = 0;

		//Ripple Detection
		if ( m_eoiName.compare("SleepSpindles") == 0) {

			SVM_Detector svmSleepSpindle_Detector("Spindle", m_patientName, spindleFunctionName, "", spindleSelectedFeatures, m_outputDirectory);
			svmSleepSpindle_Detector.setEOI_Name("Spindle");
			svmSleepSpindle_Detector.setFunctionName(spindleFunctionName);
			svmSleepSpindle_Detector.setProbThreshold(0, spindleTh);
			svmSleepSpindle_Detector.setPatientPath(m_patientPath);

			svmSleepSpindle_Detector.initEpochVectors(1);
			svmSleepSpindle_Detector.setInputEvents(inputEpochs);
			svmSleepSpindle_Detector.setAbsoluteSignalLength(m_totalSampleCount / m_samplingRate);
			//Detect channel
			svmSleepSpindle_Detector.detectEpochs(0);
			svmSleepSpindle_Detector.getEEG_EventsFromDetectedEpochs(1, channelName);
			svmSleepSpindle_Detector.setSamplingRate(m_samplingRate);
			svmSleepSpindle_Detector.setCI(useWindowConfInterv);
			svmSleepSpindle_Detector.setFileStartDateAndTime(m_fileStartDateAndTime);

			if (getPerformance()) {
				svmSleepSpindle_Detector.readEOI_VisualAnnotations(channelName, idNumber);
				svmSleepSpindle_Detector.deleteRepeatedMarks(maxOnsetDiff, minSharedPercent);
				//svmSleepSpindle_Detector.getExpertAgreement(minSharedPercent);
				svmSleepSpindle_Detector.generateMarksFile(maxOnsetDiff, minSharedPercent);

				svmSleepSpindle_Detector.joinCoOccMarks(maxOnsetDiff, minSharedPercent, joinVisualMarks);
				svmSleepSpindle_Detector.generateJoinedMarksFile(maxOnsetDiff, minSharedPercent);
			}
			//svmSleepSpindle_Detector.generateDetectionsFile(maxOnsetDiff, minSharedPercent);
			svmSleepSpindle_Detector.generateDetectionsAndFeaturesFile(maxOnsetDiff, minSharedPercent);

			if (getPerformance()) {
				//if (svmSleepSpindle_Detector.isChannelAnnotated()) {
					svmSleepSpindle_Detector.getMultiPatientPerformance(maxOnsetDiff, minSharedPercent, m_patientName, channelName, useWindowConfInterv);
				//}
			}
		}

	}
	return true;
}

bool SS_Detector::getWaveletFrequencyeAnalysisAllSignal(matrixStd &rawSignal, SS_Features &epochFeatures, long rewind, long overlap, long firstSampleToRead, long &samplesToRead,
	int firstChannel, int lastChannel, long epochLength) {

	double startFrequency = m_waveletParams->dStartFrequency;
	double endFrequency = m_waveletParams->dEndFrequency;
	double deltaFrequency = m_waveletParams->dDeltaFrequency;
	int nrOscillations = m_waveletParams->waveletOscillations;

	long signalLength = rawSignal.back().size();
	long analysisStart = rewind;
	long analysisEnd = signalLength - overlap;

	//get data only from the channels to analyze
	int nrChannsToAnalyze = lastChannel - firstChannel + 1;

	matrixStd waveletPowOutput;
	unsigned nrComponents = SignalProcessing::morletWaveletTransform(rawSignal, m_samplingRate, startFrequency, endFrequency, deltaFrequency, nrOscillations, waveletPowOutput);
	SignalProcessing::getMatrixPower(waveletPowOutput);

	unsigned expectedComponents = (endFrequency - startFrequency) / deltaFrequency;
	if (nrComponents != expectedComponents)
		return false;

#pragma omp parallel for
	for (int channIdx = 0; channIdx < nrChannsToAnalyze; ++channIdx) {
		int epochAvgSampleIdx = 0;
		double sumSubSpindlePower = 0, sumSpindlePower = 0, sumSupraSpindlePower = 0, epochSpectralPeak = 0, epochMaxSpindleRangePower = 0;

		for (long sampIdx = analysisStart; sampIdx < analysisEnd; ++sampIdx) {
			for (unsigned int nc = 0; nc < nrComponents; nc++) {
				int freqBand = startFrequency + deltaFrequency * nc;

				double centerFrequency = startFrequency + nc * deltaFrequency;
				long kernelLenght = centerFrequency > 0 ? nrOscillations / centerFrequency * m_samplingRate : 0;
				double delay = kernelLenght / 2;

				int delayIdx = sampIdx + delay < signalLength ? sampIdx + delay : signalLength - 1;
				double freqPower = waveletPowOutput[channIdx * nrComponents + nc][delayIdx];

				if (!std::isfinite(freqPower) || std::isnan(freqPower) != 0) {
					freqPower = 0;
				}

				if (freqBand >= 0 && freqBand <= 10) {
					sumSubSpindlePower += freqPower;
				}
				else if (freqBand > 10 && freqBand <= 16) {
					sumSpindlePower += freqPower;

					if (freqPower > epochMaxSpindleRangePower) {
						epochSpectralPeak = freqBand;
						epochMaxSpindleRangePower = freqPower;
					}
				}
				else if (freqBand > 16) {
					sumSupraSpindlePower += freqPower;
				}
			}
			//writeVectorToFile(channIdx, ((double)(m_fileAnalysisStartSample + firstSampleToRead + sampIdx - (rewind)+1) / m_samplingRate), vectorA, vectorB);

			epochAvgSampleIdx++;
			if (epochAvgSampleIdx == epochLength) {
				double subSpindlePower = sumSubSpindlePower / epochLength;
				double spindlePower = sumSpindlePower / epochLength;
				double supraSpindlePower = sumSupraSpindlePower / epochLength;
				
				epochFeatures.m_subSpindle_PowWavelet[firstChannel + channIdx].push_back(subSpindlePower);
				epochFeatures.m_spindle_PowWavelet[firstChannel + channIdx].push_back(spindlePower);
				epochFeatures.m_supraSpindle_PowWavelet[firstChannel + channIdx].push_back(supraSpindlePower);

				epochFeatures.m_spectralPeak[firstChannel + channIdx].push_back(epochSpectralPeak);

				double time = ((double)(m_fileAnalysisStartSample + firstSampleToRead + sampIdx - (rewind)+1) / m_samplingRate);
				if (firstChannel + channIdx == 0) {
					epochFeatures.m_time.push_back(time);
				}
				epochFeatures.m_SS_Mark[channIdx].push_back(0);

				epochAvgSampleIdx = 0;
				sumSubSpindlePower = 0;
				sumSpindlePower = 0;
				sumSupraSpindlePower = 0;
				epochSpectralPeak = 0;
				epochMaxSpindleRangePower = 0;

				if (sampIdx + 1 < analysisEnd) {				//epochs share 50%
					sampIdx = sampIdx - epochLength / 2;
				}
				if (sampIdx + epochLength >= analysisEnd) {		//go back if not enough sampels for one full epoch
					sampIdx = analysisEnd - epochLength;
				}

			}
		}
	}

	return true;
}

bool SS_Detector::getFeatureAnalysisAllSignals(matrixStd &signal, SS_Features &epochFeatures, long rewind, long overlap, long firstSampleToRead, long &samplesToRead, long epochLength, int signalType) {

	long signalLength = signal[0].size();
	int nrSS_Channs = m_nrChannels;
	long analysisStart = rewind;
	long analysisEnd = signalLength - overlap;

	//#pragma omp parallel for
	for (int channIdx = 0; channIdx < nrSS_Channs; ++channIdx) {

		int epochAvgSampleIdx = 0;

		//Amplitude Features
		double maxAmpl = 0, amplVariance = 0, meanAmplitude = 0, lineLength = 0, teagerEnergy = 0;
		//Waveform Features
		double zeroCrossingsNr = 0, peaksRateNr = 0;
		// Hjorth Features
		double mobility = 0, complexity = 0;
		// Symmetry
		double symmetry = 0, assymetry = 0;
		double power = 0;

		double binarization = 0;

		//Helpers
		SignalProcessing varianceCalc;
		if (!varianceCalc.setVarianceData(signal[channIdx]))
			bool stop = true;

		double max = -1e9, min = 1e9;

		//Mean amplitude needds to be obtained one epoch in advance in order to calculate the zero crossings
		int meanAmpBfferSize = epochLength;
		for (long i = analysisStart; i < analysisStart + meanAmpBfferSize; ++i) {
			meanAmplitude += signal[channIdx][i];
		}
		meanAmplitude /= meanAmpBfferSize;

		for (long sampIdx = analysisStart; sampIdx < analysisEnd; ++sampIdx) {
			
			double val = signal[channIdx][sampIdx];

			if (sampIdx >= analysisStart + meanAmpBfferSize && sampIdx + 1 < signalLength) {
				meanAmplitude *= meanAmpBfferSize;
				meanAmplitude = (meanAmplitude + val - signal[channIdx][sampIdx - meanAmpBfferSize]) / meanAmpBfferSize;
			}

			//Max amplitude
			if (val > max)
				max = val;
			if (val < min)
				min = val;

			//Line Length
			if (sampIdx > 0) {
				lineLength += std::abs(signal[channIdx][sampIdx] - signal[channIdx][sampIdx - 1]);
			}

			if (sampIdx > 0 && sampIdx < signalLength - 1) {																			//Teager energy
				teagerEnergy += (val * val) - (signal[channIdx][sampIdx - 1] * signal[channIdx][sampIdx + 1]);
			}

			// Zero crossings
			if (sampIdx > 0) {
				if (signal[channIdx][sampIdx - 1] > meanAmplitude && val <= meanAmplitude) {
					zeroCrossingsNr++;
				}
				else if (signal[channIdx][sampIdx - 1] < meanAmplitude && val >= meanAmplitude) {
					zeroCrossingsNr++;
				}
			}

			// Peaks
			for (int avg = 10; avg >= 0; --avg) {										// reduce the length of the calculated average in those cases where the if condition returns a false
				if (sampIdx >= avg && sampIdx < analysisEnd - avg) {
					double beforeAvg = 0, afterAvg = 0;
					for (unsigned i = 1; i <= avg; ++i) {
						beforeAvg += signal[channIdx][sampIdx - i];
						afterAvg += signal[channIdx][sampIdx + i];
					}
					beforeAvg /= avg;
					afterAvg /= avg;

					if (val > beforeAvg && val > afterAvg)
						peaksRateNr++;
					if (val < beforeAvg && val < afterAvg)
						peaksRateNr++;
				}
				break;
			}

			power += val * val;

			/****************************************************************************************/
			epochAvgSampleIdx++;
			if (epochAvgSampleIdx == epochLength) {

				long epochSampStart = sampIdx - (epochLength - 1);
				long epochSampleEnd = sampIdx;

				maxAmpl = (max - min);
				amplVariance = varianceCalc.runVariance(epochSampStart, epochLength);
				lineLength;
				zeroCrossingsNr;
				teagerEnergy;
				peaksRateNr;

				// Hjorth Features
				std::vector<double> firstDerivative, secondDerivative, thirdDerivative;
				SignalProcessing::firstDerivative(signal[channIdx], firstDerivative, epochSampStart, epochLength, m_samplingRate);
				SignalProcessing::firstDerivative(firstDerivative, secondDerivative, 0, epochLength, m_samplingRate);
				// Mobility
				SignalProcessing varianceFD;
				if (!varianceFD.setVarianceData(firstDerivative))
					bool stop = true;
				double varValFD = varianceFD.runVariance(0, epochLength);
				mobility = amplVariance > 0 ? sqrt(varValFD / amplVariance) : 0;
				// Complexity
				SignalProcessing varianceSD;
				if (!varianceSD.setVarianceData(secondDerivative))
					bool stop = true;
				double varValSD = varianceSD.runVariance(0, epochLength);
				double mobilityFD = varValSD / varValFD;
				complexity = mobility > 0 ? mobilityFD / mobility : 0;
				if (isnan(complexity) || isinf(complexity)) {
					complexity = 0;
				}

				//Symmetry and Asymmetry
				double center = (epochSampStart + epochLength / 2);
				double numSymm = 0, nummAss = 0, maxSymm = -1 * 1000, maxAsymm = -1 * 1000;
				long i = center, j = center, n = 0;
				while (i < epochSampleEnd && j >= epochSampStart) {
					double symmetric = 0.5 * (signal[channIdx][i] + signal[channIdx][j]);
					double assymetric = 0.5 * (signal[channIdx][i] - signal[channIdx][j]);
					if (abs(symmetric) > maxSymm)
						maxSymm = abs(symmetric);
					if (abs(assymetric) > maxAsymm)
						maxAsymm = abs(assymetric);
					numSymm += symmetric * symmetric;
					nummAss += assymetric * assymetric;
					i++; j--; n++;
				}
				symmetry = (maxSymm*maxSymm * (2 * n + 1)) > 0 ? numSymm / (maxSymm*maxSymm * (2 * n + 1)) : 0;
				assymetry = (maxAsymm*maxAsymm * (2 * n + 1)) > 0 ? nummAss / (maxAsymm*maxAsymm * (2 * n + 1)) : 0;

				if (signalType == RAW) {
					epochFeatures.m_maxAmplRaw[channIdx].push_back(maxAmpl);
					epochFeatures.m_amplVarianceRaw[channIdx].push_back(amplVariance);
					epochFeatures.m_meanAmplitudeRaw[channIdx].push_back(meanAmplitude);
					epochFeatures.m_lineLengthRaw[channIdx].push_back(lineLength);
					epochFeatures.m_teagerEnergyRaw[channIdx].push_back(teagerEnergy);
					epochFeatures.m_zeroCrossingsNrRaw[channIdx].push_back(zeroCrossingsNr);
					epochFeatures.m_peaksRateNrRaw[channIdx].push_back(peaksRateNr);
					epochFeatures.m_mobilityRaw[channIdx].push_back(mobility);
					epochFeatures.m_complexityRaw[channIdx].push_back(complexity);
					epochFeatures.m_autocorrelationRaw[channIdx].push_back(symmetry);
					epochFeatures.m_symmetryRaw[channIdx].push_back(symmetry);
					epochFeatures.m_asymmetryRaw[channIdx].push_back(assymetry);
					epochFeatures.m_powerRaw[channIdx].push_back(power);
					epochFeatures.m_teagerEnergyAutocorrRaw[channIdx].push_back(teagerEnergy);
				}
				else if (signalType == NOTCHED) {
					epochFeatures.m_maxAmplNotchedDC[channIdx].push_back(maxAmpl);
					epochFeatures.m_amplVarianceNotchedDC[channIdx].push_back(amplVariance);
					epochFeatures.m_meanAmplitudeNotchedDC[channIdx].push_back(meanAmplitude);
					epochFeatures.m_lineLengthNotchedDC[channIdx].push_back(lineLength);
					epochFeatures.m_teagerEnergyNotchedDC[channIdx].push_back(teagerEnergy);
					epochFeatures.m_zeroCrossingsNrNotchedDC[channIdx].push_back(zeroCrossingsNr);
					epochFeatures.m_peaksRateNrNotchedDC[channIdx].push_back(peaksRateNr);
					epochFeatures.m_mobilityNotchedDC[channIdx].push_back(mobility);
					epochFeatures.m_complexityNotchedDC[channIdx].push_back(complexity);
					epochFeatures.m_autocorrelationNotchedDC[channIdx].push_back(symmetry);
					epochFeatures.m_symmetryNotchedDC[channIdx].push_back(symmetry);
					epochFeatures.m_asymmetryNotchedDC[channIdx].push_back(assymetry);
					epochFeatures.m_powerNotchedDC[channIdx].push_back(power);
					epochFeatures.m_teagerEnergyAutocorrNotchedDC[channIdx].push_back(teagerEnergy);
				}
				else if (signalType == SPINDLE) {
					epochFeatures.m_maxAmplBP[channIdx].push_back(maxAmpl);
					epochFeatures.m_amplVarianceBP[channIdx].push_back(amplVariance);
					epochFeatures.m_meanAmplitudeBP[channIdx].push_back(meanAmplitude);
					epochFeatures.m_lineLengthBP[channIdx].push_back(lineLength);
					epochFeatures.m_teagerEnergyBP[channIdx].push_back(teagerEnergy);
					epochFeatures.m_zeroCrossingsNrBP[channIdx].push_back(zeroCrossingsNr);
					epochFeatures.m_peaksRateNrBP[channIdx].push_back(peaksRateNr);
					epochFeatures.m_mobilityBP[channIdx].push_back(mobility);
					epochFeatures.m_complexityBP[channIdx].push_back(complexity);
					epochFeatures.m_autocorrelationBP[channIdx].push_back(symmetry);
					epochFeatures.m_symmetryBP[channIdx].push_back(symmetry);
					epochFeatures.m_asymmetryBP[channIdx].push_back(assymetry);
					epochFeatures.m_powerBP[channIdx].push_back(power);
					epochFeatures.m_teagerEnergyAutocorrBP[channIdx].push_back(teagerEnergy);
				}

				// clear feature variables
				maxAmpl = 0; amplVariance = 0; meanAmplitude = 0; lineLength = 0; teagerEnergy = 0;
				zeroCrossingsNr = 0; peaksRateNr = 0;
				mobility = 0; complexity = 0;
				symmetry = 0; assymetry = 0;
				power = 0;

				//clear helpers
				max = -1e9; min = 1e9;

				epochAvgSampleIdx = 0;

				if (sampIdx + 1 < analysisEnd) {				//epochs share 50%
					sampIdx = sampIdx - epochLength / 2;
				}
				if (sampIdx + epochLength >= analysisEnd) {		//go back if not enough sampels for one full epoch
					sampIdx = analysisEnd - epochLength;
				}
			}
		}
	}

	return true;
}

void SS_Detector::getSignalParams(double &samplingRate, double &analysisDuration, int &nrChannels) {
	samplingRate = m_samplingRate;
	analysisDuration = m_totalSampleCount / m_samplingRate;
	nrChannels = m_nrChannels;
}

std::string SS_Detector::getPatientNameAndPath() {
	for (int i = m_strFileName.length() - 1; i >= 0; i--) {
		if (m_strFileName[i] == 92) {
			m_patientName = m_strFileName.substr(i + 1, m_strFileName.length() - i);
			m_patientPath = m_strFileName.substr(0, i + 1);
			break;
		}
	}

	m_patientName.pop_back(); m_patientName.pop_back(); m_patientName.pop_back(); m_patientName.pop_back();

	return m_patientName;
}

std::string SS_Detector::ExePath() {
	char buff[FILENAME_MAX];
	getcwd(buff, FILENAME_MAX);
	std::string current_working_dir(buff);
	return current_working_dir;
}

int SS_Detector::getWaveletCausedDelay(void) {

	int maxDelay = 0;

	double startFrequency = m_waveletParams->dStartFrequency;
	double endFrequency = m_waveletParams->dEndFrequency;
	double deltaFrequency = m_waveletParams->dDeltaFrequency;
	int nrOscillations = m_waveletParams->waveletOscillations;

	int nrComponents = (endFrequency - startFrequency) / deltaFrequency;
	for (int componentIdx = 0; componentIdx < nrComponents; componentIdx++) {
		double centerFrequency = startFrequency + componentIdx * deltaFrequency;
		long kernelLenght = centerFrequency > 0 ? (nrOscillations / centerFrequency) * m_samplingRate : 0;
		double delay = kernelLenght / 2;
		if (delay > maxDelay)
			maxDelay = delay;
	}

	return maxDelay;
}

int SS_Detector::getFilteringCausedDelay(void) {

	return SignalProcessing::getOrder(); //we filter two times so teh delay is equal to the filter order
}

void SS_Detector::readChannelsFromFile(void) {

	m_useMontage ? m_selectedBipolarChanels.clear() : m_selectedUnipolarChanels.clear();
	std::string fn = m_patientPath + m_patientName + "_selectedChannels.txt";
	std::ifstream selectChanns;
	selectChanns.open(fn.c_str());

	std::string firstLine;
	std::getline(selectChanns, firstLine);

	while (!selectChanns.eof()) {
		std::string line;
		std::getline(selectChanns, line);
		std::stringstream ss(line);

		int channNr;
		std::string channName;
		ss >> channNr;
		ss >> channName;
		if (!channName.empty()) {
			m_useMontage ? m_selectedBipolarChanels.push_back(channName) : m_selectedUnipolarChanels.push_back(channName);
		}
	}

	selectChanns.close();
}

void SS_Detector::getSpecificStartAndEndTimes() {

	if (m_patientName.find("0_NM") != std::string::npos) {
		m_analysisStartTime = 24.00;
		m_analysisEndTime = 84;
	}
	else if (m_patientName.find("0_rksegment1_Segment_1") != std::string::npos) {
		m_analysisStartTime = 0;
		m_analysisEndTime = 61;
	}
	else if (m_patientName.find("0_rksegment2_Segment_1") != std::string::npos) {
		m_analysisStartTime = 0;
		m_analysisEndTime = 179;
	}
	else if (m_patientName.find("09_KA") != std::string::npos) {
		m_analysisStartTime = 0;
		m_analysisEndTime = 60;
	}
	else if (m_patientName.find("15_SchM_2000chan1_128_Segment_1") != std::string::npos) {
		m_analysisStartTime = 0;
		m_analysisEndTime = 62;
	}
	else if (m_patientName.find("17_StV") != std::string::npos) {
		m_analysisStartTime = 125;
		m_analysisEndTime = 187;
	}
	else if (m_patientName.find("20_WoT") != std::string::npos) {
		m_analysisStartTime = 68;
		m_analysisEndTime = 130;
	}
	else if ((m_patientName.find("_SCHLAF_") != std::string::npos) || (m_patientName.find("_SCHLAF1_") != std::string::npos)) {
		m_analysisStartTime = 0;
		m_analysisEndTime = 181;
	}
	else if (m_patientName.find("KUSSA") != std::string::npos) {
		m_analysisStartTime = 0;
		m_analysisEndTime = 300;
	}
	else if (m_patientName.find("KONV") != std::string::npos || m_patientName.find("FERTIGMARK") != std::string::npos) {
		m_analysisStartTime = 0;
		m_analysisEndTime = 62;
	}
	else {
		double samplingRate;
		long firstAnalysisSample = 0, lastAnalysisSample = 0;
		std::string line, varName;

		std::string fn = m_patientPath + m_patientName + "_AnalysisTimeFile.txt";
		std::ifstream analysisTimeFile;
		analysisTimeFile.open(fn.c_str());

		std::getline(analysisTimeFile, line);
		std::stringstream first(line);
		first >> varName;
		first >> samplingRate;

		std::getline(analysisTimeFile, line);
		std::stringstream second(line);
		second >> varName;
		second >> firstAnalysisSample;

		std::getline(analysisTimeFile, line);
		std::stringstream third(line);
		third >> varName;
		third >> lastAnalysisSample;

		m_analysisStartTime = firstAnalysisSample / samplingRate;
		m_analysisEndTime = lastAnalysisSample / samplingRate;

		analysisTimeFile.close();
	}

	/*if (m_patientName.find("-SNR") != std::string::npos) {
	m_analysisStartTime = 0;
	m_analysisEndTime = 120;
	}*/
}

void SS_Detector::generate_EOI_ChannelsFile(void) {
	std::ofstream eoiChannels;

	std::string outputPath = m_outputDirectory + "/MOSSDET_Output";
	std::string patPath = outputPath + "/" + m_patientName;
	std::string filename = patPath + "/" + m_patientName + "_ChannelsFile.txt";
	//std::string filename = patPath + "/" + m_patientName + "_AllChannels.txt";
	//std::string filename = patPath + "/" + m_patientName + "_EOI_ChannsFile.txt";

	mkdir(outputPath.c_str());
	mkdir(patPath.c_str());

	if (!fileExists(filename.c_str())) {
		eoiChannels.open(filename);
		eoiChannels << "Channel\t" << "Label\n";
	}
	else {
		eoiChannels.open(filename, std::ios::app);    // open file for appending
	}

	for (int channIdx = 0; channIdx < m_nrChannels; ++channIdx) {
		std::string channelName = m_useMontage ? m_montageLabels[channIdx].montageName : m_unipolarLabels[channIdx].contactName;
		channelName.erase(std::remove_if(channelName.begin(), channelName.end(), ::isspace), channelName.end());

		//int idNumber = m_useMontage ? m_montageLabels[channIdx].montageGolbalNr : m_unipolarLabels[channIdx].contactGlobalIdx;

		/*if (channelName.find("#") != std::string::npos) {
		char lastChar = channelName.back();
		if (lastChar != '-')
		channelName += "-";
		}*/

		eoiChannels << channIdx << "\t" << channelName << "\n";

	}

	eoiChannels.close();
}

bool SS_Detector::generateEpochCharacterizationSpindles(SS_Features &epochFeatures) {

	std::string outputPath = m_outputDirectory + "\\MOSSDET_Output";
	std::string patOutPath = outputPath + "\\" + m_patientName;
	std::string patTrainingFilesPath = outputPath + "\\" + m_patientName + "\\TrainingFiles";

	mkdir(outputPath.c_str());
	mkdir(patOutPath.c_str());
	mkdir(patTrainingFilesPath.c_str());

	epochFeatures.normalizeFeatures();/// Just for the paper

	std::vector<bool> chnnWithMarks(m_nrChannels);
	bool ripple = true;
	bool fastRipple = false;

#pragma omp parallel for
	for (int channIdx = 0; channIdx < m_nrChannels; ++channIdx) {
		std::string channelName = m_useMontage ? m_montageLabels[channIdx].montageName : m_unipolarLabels[channIdx].contactName;
		channelName.erase(std::remove_if(channelName.begin(), channelName.end(), ::isspace), channelName.end());
		int idNumber = m_useMontage ? m_montageLabels[channIdx].montageMOSSDET_Nr : m_unipolarLabels[channIdx].contactGlobalIdx;
		//std::string filename = patTrainingFilesPath + "\\" + m_patientName + "_" + channelName + "_epochCharacterization_Ripple.txt";
		std::string filename = patTrainingFilesPath + "\\" + m_patientName + "_" + channelName + "_epochCharacterization.txt";
		//std::string filename = patTrainingFilesPath + "\\" + channelName + ".txt";

		//filename.erase(std::remove_if(filename.begin(), filename.end(), ::isspace), filename.end());

		std::ofstream trainingFile;
		if (!fileExists(filename.c_str())) {
			trainingFile.open(filename);
			//Header
			trainingFile << "Data \t" << "CH \t" << "Time \t";
			for (unsigned i = 0; i < epochFeatures.featureNames.size(); ++i) {
				trainingFile << epochFeatures.featureNames[i] << "\t";
			}
			trainingFile << "Output \n";
		}
		else {
			trainingFile.open(filename, std::ios::app);    // open file for appending
		}

		trainingFile.precision(32);
		unsigned nrEpochs = epochFeatures.m_time.size();
		//Generate Excel readable File
		for (unsigned sampleIdx = 0; sampleIdx < nrEpochs; ++sampleIdx) {
			double sampleTime = epochFeatures.m_time[sampleIdx];
			unsigned currSample = epochFeatures.m_time[sampleIdx] * m_samplingRate;

			trainingFile << "d:\t" << channelName << "\t" << epochFeatures.m_time[sampleIdx] << "\t";

			unsigned nrFeats = epochFeatures.features.size();
			for (unsigned featIdx = 0; featIdx < nrFeats; ++featIdx) {
				unsigned featLength = epochFeatures.features[featIdx]->at(channIdx).size();
				if (featLength > 1 && sampleIdx < featLength) {
					trainingFile << epochFeatures.features[featIdx]->at(channIdx)[sampleIdx] << "\t";
				}
			}

			trainingFile << 0 << "\n";
			//trainingFile << epochFeatures.m_SS_Mark.at(channIdx)[sampleIdx] << "\n";
		}
		trainingFile.close();
	}
	return true;
}

bool SS_Detector::saveSelectedEEG_DataToFile(long firstSampleToRead, unsigned rewind, unsigned overlap, matrixStd& signal, matrixStd& notchedSignal, matrixStd& bpSignal, unsigned channelToSave) {

	long nrSamples = signal[0].size();
	int channIdx = channelToSave;// m_EOI_Channels->at(channelToSave);
	std::string channelName = m_useMontage ? m_montageLabels[channIdx].montageName : m_unipolarLabels[channIdx].contactName;

	int firOrder = SignalProcessing::getOrder();
	std::string outputPath = m_outputDirectory + "/MOSSDET_Output";
	std::string patPath = outputPath + "/" + m_patientName;
	std::string filename = patPath + "/" + m_patientName + "_" + channelName + "_Fir" + std::to_string(firOrder) + "_EEG_Data.dat";
	mkdir(outputPath.c_str());
	mkdir(patPath.c_str());
	//remove(filename.c_str());

	std::ofstream eegDataFile;
	if (!fileExists(filename.c_str())) {
		eegDataFile.open(filename);
		//Header
		eegDataFile << "Time\t" << "Raw\t" << "Notched+High-Pass\t" << "Band-Pass\n";
	}
	else {
		eegDataFile.open(filename, std::ios::app);    // open file for appending
	}

	int timeZeroCtr = 0;
	double reverseFactor = 1.0;
	for (long sampIdx = rewind; sampIdx < nrSamples - overlap; sampIdx++) {
		double time = (double)(m_fileAnalysisStartSample + firstSampleToRead + sampIdx - rewind) / m_samplingRate;
		double rawVal = signal[channelToSave][sampIdx] * reverseFactor;
		double notchedVal = notchedSignal[channelToSave][sampIdx] * reverseFactor;
		double bpVal = bpSignal[channelToSave][sampIdx] * reverseFactor;

		eegDataFile << time << "\t" << rawVal << "\t" << notchedVal << "\t" << bpVal << "\n";
		if (time == 0.0)
			timeZeroCtr++;
	}

	eegDataFile.close();

	return timeZeroCtr == 1;
}

void SS_Detector::readAndWriteEOI_VisualAnnotations() {

	std::vector<EOI_Event> readAllAnnotations;
	long nrAnnotations = 0;

	/*****************************************************************************Read all annotations******************************************************************************************/
	std::string readFilename = m_patientPath + m_patientName + "_allAnnotations.txt";
	//std::string readFilename = m_patientPath + m_patientName + "_SS_Annotations.txt";


	std::ifstream eoiAnnotations;
	eoiAnnotations.open(readFilename.c_str());
	eoiAnnotations.precision(32);
	std::string firstLine;
	std::getline(eoiAnnotations, firstLine);
	while (!eoiAnnotations.eof()) {
		std::string line;
		std::getline(eoiAnnotations, line);
		std::stringstream ss(line);

		double sampleStartAndEnd;
		double duration;
		int refChannNr;
		std::string refChannName;

		EOI_Event eoiMark;
		/*ss >> eoiMark.channelName;
		ss >> eoiMark.description;
		ss >> refChannName;
		ss >> sampleStartAndEnd;
		ss >> duration;
		eoiMark.description += refChannName;
		eoiMark.description.erase(std::remove_if(eoiMark.description.begin(), eoiMark.description.end(), ::isspace), eoiMark.description.end());
		eoiMark.startTime = sampleStartAndEnd / m_samplingRate;
		eoiMark.endTime = sampleStartAndEnd / m_samplingRate + duration / m_samplingRate;*/

		//Channel	ChannelNr	RefChannel	RefChannelNr	StartSample	EndSample	StartTime	EndTime

		/*ss >> eoiMark.channelName;
		ss >> eoiMark.channelNr;
		ss >> eoiMark.description;;
		ss >> refChannNr;
		ss >> sampleStartAndEnd;
		ss >> sampleStartAndEnd;
		ss >> eoiMark.startTime;
		ss >> eoiMark.endTime;*/

		ss >> eoiMark.description;
		ss >> eoiMark.channelName;
		ss >> eoiMark.channelNr;
		ss >> eoiMark.lowNeighborChannelName;
		ss >> eoiMark.lowNeighborChannelNr;
		ss >> eoiMark.highNeighborChannelName;
		ss >> eoiMark.highNeighborChannelNr;
		ss >> sampleStartAndEnd;
		ss >> sampleStartAndEnd;
		ss >> eoiMark.startTime;
		ss >> eoiMark.endTime;

		if (!eoiMark.channelName.empty() && eoiMark.startTime != 0 && eoiMark.endTime != 0) {
			if (getChannelNumber(eoiMark.channelName, eoiMark.channelNr)) {
				getChannelNumber(eoiMark.lowNeighborChannelName, eoiMark.lowNeighborChannelNr);
				getChannelNumber(eoiMark.highNeighborChannelName, eoiMark.highNeighborChannelNr);
				readAllAnnotations.push_back(eoiMark);
				nrAnnotations++;
			}
		}
	}

	eoiAnnotations.close();

	/*****************************************************************************Write all annotations******************************************************************************************/
	m_markedSS.resize(m_nrChannels);

	std::string outputPath = m_outputDirectory + "/MOSSDET_Output";
	std::string patPath = outputPath + "/" + m_patientName + "/";
	mkdir(outputPath.c_str());
	mkdir(patPath.c_str());
	std::string writeFilename = patPath + m_patientName + "_allAnnotations.txt";
	remove(writeFilename.c_str());

	std::ofstream eoiAnnotWriteFile;
	if (!fileExists(writeFilename.c_str())) {
		eoiAnnotWriteFile.open(writeFilename);
		//Header
		eoiAnnotWriteFile << "Mark\t" << "Channel\t" << "ChannelNr\t" << "LowerNeighborChName\t" << "LowerNeighborChNr\t" << "UpperNeighborChName\t" << "UpperNeighborChNr\t" << "StartSample\t" << "EndSample\t" << "StartTime\t" << "EndTime\n";
	}

	for (int annotIdx = 0; annotIdx < nrAnnotations; ++annotIdx) {
		long annotChannNr = readAllAnnotations[annotIdx].channelNr;
		m_markedSS[annotChannNr].push_back(readAllAnnotations[annotIdx]);

		eoiAnnotWriteFile << "SleepSpindle" << "\t"
			<< readAllAnnotations[annotIdx].channelName << "\t" << readAllAnnotations[annotIdx].channelNr << "\t";											//Mark Channel
		if (m_useMontage) {
			eoiAnnotWriteFile << "Montage" << "\t" << 0 << "\t"	<< "Montage"<< "\t" << 0 << "\t";
		}
		else {
			eoiAnnotWriteFile << readAllAnnotations[annotIdx].channelName << "\t" << readAllAnnotations[annotIdx].channelNr << "\t"											//Mark Channel
				<< readAllAnnotations[annotIdx].channelName << "\t" << readAllAnnotations[annotIdx].channelNr << "\t";											//Mark Channel
		}

		eoiAnnotWriteFile << (long)readAllAnnotations[annotIdx].startTime * m_samplingRate << "\t" << (long)readAllAnnotations[annotIdx].endTime * m_samplingRate << "\t"				// Start and end samples
			<< readAllAnnotations[annotIdx].startTime << "\t" << readAllAnnotations[annotIdx].endTime << "\n";												// Start and end times
	}
	eoiAnnotWriteFile.close();
}

bool SS_Detector::getChannelNumber(std::string markChannelName, unsigned int &channNr) {

	//#pragma omp parallel for
	for (int channIdx = 0; channIdx < m_nrChannels; ++channIdx) {

		std::string channelName = m_useMontage ? m_montageLabels[channIdx].montageName : m_unipolarLabels[channIdx].contactName;

		markChannelName.erase(std::remove_if(markChannelName.begin(), markChannelName.end(), ::isspace), markChannelName.end());
		channelName.erase(std::remove_if(channelName.begin(), channelName.end(), ::isspace), channelName.end());

		markChannelName.erase(std::remove(markChannelName.begin(), markChannelName.end(), '-'), markChannelName.end());
		channelName.erase(std::remove(channelName.begin(), channelName.end(), '-'), channelName.end());

		markChannelName.erase(std::remove(markChannelName.begin(), markChannelName.end(), '_'), markChannelName.end());
		channelName.erase(std::remove(channelName.begin(), channelName.end(), '_'), channelName.end());

		markChannelName.erase(std::remove(markChannelName.begin(), markChannelName.end(), '#'), markChannelName.end());
		channelName.erase(std::remove(channelName.begin(), channelName.end(), '#'), channelName.end());

		int compareResult = markChannelName.compare(channelName);

		channelName.erase(channelName.begin(), channelName.begin() + 3);
		int compareResultTwo = markChannelName.compare(channelName);

		if (compareResult == 0 || compareResultTwo == 0) {
			channNr = channIdx;
			return true;
		}

	}

	return false;
}