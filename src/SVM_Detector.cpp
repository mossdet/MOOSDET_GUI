#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>    // std::min_element, std::max_element, std::find
#include <math.h>       /* sqrt */
#include <numeric>
#include <ctime>
#include <direct.h>
#include <bitset>

#include <dlib/svm.h>

#include "DataTypes.h"
#include "SVM_Detector.h"
#include "SignalProcessing.h"
#include "HFO_EventsHandler.h"

#include <omp.h>

using namespace dlib;
typedef dlib::matrix<double, 0, 1> sample_type;
typedef radial_basis_kernel<sample_type> kernel_type;
typedef decision_function<kernel_type> dec_funct_type;
typedef normalized_function<dec_funct_type> funct_type;
typedef probabilistic_decision_function<kernel_type> probabilistic_funct_type;
typedef normalized_function<probabilistic_funct_type> pfunct_type;

SVM_Detector::SVM_Detector(std::string eoiName, std::string patientName, std::string functionName, std::string trainingDataFileName, 
	std::vector<unsigned> selectedFeatures, std::string outputPath) {

	// Classifier parameters
	m_trainingDataFileName = trainingDataFileName;
	m_selectedFeats = selectedFeatures;
	m_gamma = 0;
	m_nu = 0;
	m_probThreshold = 0;
	m_epochLength = 0;
	m_savedFunctionName = functionName;
	m_eoiName = eoiName;

	// Characterized Signal parameters
	m_patientName = patientName;
	m_filename;
	m_inputEvents;
	m_nrEvents = 0;
	m_samplingRate = 0;
	m_signalDuration = 0;
	m_avgMarkDuration = 0;
	m_expertAgreement = 0;

	// Detections Results
	m_nrNoOscills_EOI = 0;
	m_nrShort_EOI = 0;
	m_nrLong_EOI = 0;
	m_nrNoHistoryCompliant_EOI = 0;
	m_avgDetectionDuration = 0;
	m_chSpecifificDetecEpochs;
	m_detectedEOI;

	m_svmPerformance.falseNegatives = 0;
	m_svmPerformance.falsePositives = 0;
	m_svmPerformance.trueNegatives = 0;
	m_svmPerformance.truePositives = 0;

	//Initialize Folders
	m_outputPath = outputPath + "/MOSSDET_Output";
	m_patPath = outputPath + "/" + m_patientName;
	m_patDetectionFilesPath = m_outputPath + "/" + m_patientName + "/DetectionFiles";
	_mkdir(m_outputPath.c_str());
	//_mkdir(m_patPath.c_str());
	_mkdir(m_patDetectionFilesPath.c_str());
}

SVM_Detector::~SVM_Detector() {
}

bool SVM_Detector::detectEpochs(double chNr) {

	unsigned numTestingEvs = m_inputEvents.size();

	bool rippleDetection = m_eoiName.find("ast") == std::string::npos;

	funct_type learned_function;

	unsigned nrSVs = 0;
	deserialize(m_savedFunctionName) >> learned_function;
	nrSVs = learned_function.function.basis_vectors.size();

	// Read samples and learn teh mean and standard deviation in order to provide a normalizer to the SVM
	std::vector<sample_type> samples;
	vector_normalizer<sample_type> normalizer;
	unsigned nrSelectedFeatures = m_selectedFeats.size();
	for (unsigned evIdx = 0; evIdx < numTestingEvs; ++evIdx) {
		sample_type samp;
		samp.set_size(nrSelectedFeatures);
		unsigned numTrainingFeatures = m_inputEvents[evIdx].inputVals.size();
		unsigned nrReadFeatures = 0;
		for (unsigned featIdx = 0; featIdx < numTrainingFeatures; ++featIdx) {
			if (std::find(m_selectedFeats.begin(), m_selectedFeats.end(), featIdx) != m_selectedFeats.end()) {
				double val = m_inputEvents[evIdx].inputVals[featIdx];
				if (isnan(val) || isinf(val)) {
					val = 0;
				}

				samp(nrReadFeatures) = val;
				nrReadFeatures++;
			}
		}
		samples.push_back(samp);
	}
	normalizer.train(samples);		//here the mean and standardr deviation of the provided samples are learned

	m_epochLength = 0;
	m_chSpecifificDetecEpochs[chNr].clear();
	m_chSpecifificDetecEpochs[chNr].resize(numTestingEvs);
	#pragma omp parallel for
	for (long evIdx = 0; evIdx < numTestingEvs; ++evIdx) {
		funct_type svm_binaryClassifier;
		svm_binaryClassifier.normalizer = normalizer;
		svm_binaryClassifier.function = learned_function.function;
		double result = svm_binaryClassifier(samples[evIdx]);
		bool epochVal = result >= m_probThreshold;
		m_chSpecifificDetecEpochs[chNr][evIdx] = (epochVal);
		if (evIdx > 0) {
			m_epochLength += (m_inputEvents[evIdx].time - m_inputEvents[evIdx - 1].time) * 2;
		}
	}
	m_epochLength /= (numTestingEvs-1);

	return true;
}

bool SVM_Detector::getEEG_EventsFromDetectedEpochs(unsigned sharedChs, std::string channName) {

	double detectionOverlap = 0;

	unsigned nrChs = m_chSpecifificDetecEpochs.size();
	unsigned nrEpochs = m_chSpecifificDetecEpochs[0].size();

	unsigned nrShortEOIs = 0;
	unsigned nrLongEOIs = 0;


	double eventMaxAmplitude = 0;
	double eventMaxPower = 0.0;
	double eventMaxSpectralPeak = 0.0;
	std::vector<double> backgroundAmplitude;
	std::vector<double> backgroundPower;

	//Ripple values for min/max duration and feature indices
	double minEOI_Duration = 0.0;// m_epochLength
	double maxEOI_Duration = 0.2;

	int amplitudeFeatIdx = 40;
	int powerFeatIdx = 43;
	int spectrPeakFeatIdx = 6;

	bool fastRipple = m_eoiName.find("ast") != std::string::npos;
	bool spike = m_eoiName.find("ike") != std::string::npos;
	bool spindle = m_eoiName.find("indle") != std::string::npos;

	if (fastRipple) {
		minEOI_Duration = 0.0;// 0.035;
		maxEOI_Duration = 0.2;

		amplitudeFeatIdx = 54;
		powerFeatIdx = 57;
		spectrPeakFeatIdx = 10;
	}

	if (spike) {
		minEOI_Duration = 0.040;// 0.035;0.06;
		maxEOI_Duration = 0.35;

		amplitudeFeatIdx = 26;
		powerFeatIdx = 29;
		spectrPeakFeatIdx = 2;
	}

	if (spindle) {
		minEOI_Duration = 0.45;// 0.035;
		maxEOI_Duration = 3.5;

		amplitudeFeatIdx = 31;
		powerFeatIdx = 35;
		spectrPeakFeatIdx = 45;		// for spindles this is nr. ZeroCrossings
	}

	//For Marcel Detection
	//maxEOI_Duration = 60 * 60 * 24 * 365;

	bool eoiEpoch = false, eoiStart = false;

	unsigned channel = 0;
	double startTime = 0, endTime = 0, duration = 0;
	long startEvIdx = 0, endEvIdx = 0;

	//writeFeaturesAndOutput(channName);

	for (unsigned evIdx = 1; evIdx < nrEpochs; ++evIdx) {

		eoiEpoch = m_chSpecifificDetecEpochs[0][evIdx];

		// get background feature averages
		if (!eoiEpoch && !eoiStart) {
			backgroundAmplitude.push_back(m_inputEvents[evIdx].inputVals[amplitudeFeatIdx]);
			backgroundPower.push_back(m_inputEvents[evIdx].inputVals[powerFeatIdx]);

			if (backgroundAmplitude.size() *  m_epochLength > 1) {		//refresh vectors when tehy are larger than one minute
				backgroundAmplitude.erase(backgroundAmplitude.begin());
				backgroundPower.erase(backgroundPower.begin());
			}
		}

		// Event Start
		if (eoiEpoch && !eoiStart) {
			startTime = m_inputEvents.at(evIdx).time - m_epochLength;
			eoiStart = true;
			startEvIdx = evIdx;
		}

		// Event has started, characterize it
		if (eoiEpoch && eoiStart) {
			eventMaxAmplitude = m_inputEvents[evIdx].inputVals[amplitudeFeatIdx] > eventMaxAmplitude ? m_inputEvents[evIdx].inputVals[amplitudeFeatIdx] : eventMaxAmplitude;
			eventMaxPower = m_inputEvents[evIdx].inputVals[powerFeatIdx] > eventMaxPower ? m_inputEvents[evIdx].inputVals[powerFeatIdx] : eventMaxPower;
			eventMaxSpectralPeak = m_inputEvents[evIdx].inputVals[spectrPeakFeatIdx] > eventMaxSpectralPeak ? m_inputEvents[evIdx].inputVals[spectrPeakFeatIdx] : eventMaxSpectralPeak;
		}


		// Event end
		if (!eoiEpoch && eoiStart) {
			eoiStart = false;
			channel = m_inputEvents.at(evIdx).channel;
			endTime = m_inputEvents.at(evIdx).time - m_epochLength;
			duration = endTime - startTime;
			endEvIdx = evIdx;

			EOI_Event newDetection;
			newDetection.channelName = channName;
			newDetection.description = m_eoiName;
			newDetection.channelNr = channel;
			newDetection.startTime = startTime;
			newDetection.endTime = endTime;
			newDetection.duration = duration;
			//detection features
			newDetection.amplitude = eventMaxAmplitude;
			newDetection.power = eventMaxPower;
			newDetection.spectralPeak = eventMaxSpectralPeak;
			//Background fatures
			if (!backgroundAmplitude.empty()) {
				newDetection.backgroundAmplitude = std::accumulate(backgroundAmplitude.begin(), backgroundAmplitude.end(), 0.0) / backgroundAmplitude.size();
				newDetection.backgroundPower = std::accumulate(backgroundPower.begin(), backgroundPower.end(), 0.0) / backgroundPower.size();
				SignalProcessing varianceCalc;
				varianceCalc.setVarianceData(backgroundPower);
				double backgroundVar = varianceCalc.runVariance(0, backgroundPower.size());
				newDetection.backgroundStdDev = sqrt(backgroundVar);
			}

			//reset event features
			eventMaxAmplitude = 0;
			eventMaxPower = 0;
			eventMaxSpectralPeak = 0;

			// discard detections because of min or max duration
			if (duration < minEOI_Duration) {
				m_nrShort_EOI++;
				continue;
			}
			if (duration > maxEOI_Duration) {
				//if (duration > 7)
				m_nrLong_EOI++;
				endTime = startTime + maxEOI_Duration;
				duration = maxEOI_Duration;
				continue;
			}

			// save detection
			unsigned nrEOIs = m_detectedEOI.size();
			double minInterDetectDistance = m_epochLength;
			if (spike) {
				minInterDetectDistance = 0.25;
			}
			if (spindle) {
				minInterDetectDistance = 0.45;
			}

			if (nrEOIs > 0) {
				if (newDetection.description.compare(m_detectedEOI[nrEOIs - 1].description) == 0) {
					if (startTime - m_detectedEOI[nrEOIs - 1].endTime < minInterDetectDistance) {//Join detections
						if (endTime - m_detectedEOI[nrEOIs - 1].startTime > maxEOI_Duration) {// Truncate joined detectios which are too long
							m_detectedEOI[nrEOIs - 1].endTime = m_detectedEOI[nrEOIs - 1].startTime + maxEOI_Duration;
						}
						else {
							m_detectedEOI[nrEOIs - 1].endTime = endTime;
						}
						m_detectedEOI[nrEOIs - 1].amplitude = m_detectedEOI[nrEOIs - 1].amplitude > newDetection.amplitude ? m_detectedEOI[nrEOIs - 1].amplitude : newDetection.amplitude; // choose highest amplitude from teh two devents to join
						m_detectedEOI[nrEOIs - 1].power = m_detectedEOI[nrEOIs - 1].power > newDetection.power ? m_detectedEOI[nrEOIs - 1].power : newDetection.power; // choose highest power from teh two devents to join
						m_detectedEOI[nrEOIs - 1].spectralPeak = m_detectedEOI[nrEOIs - 1].spectralPeak > newDetection.spectralPeak ? m_detectedEOI[nrEOIs - 1].spectralPeak : newDetection.spectralPeak; // choose highest spectral peak from teh two devents to join
						m_detectedEOI[nrEOIs - 1].backgroundAmplitude = m_detectedEOI[nrEOIs - 1].backgroundAmplitude;
						m_detectedEOI[nrEOIs - 1].backgroundPower = m_detectedEOI[nrEOIs - 1].backgroundPower;
						m_detectedEOI[nrEOIs - 1].backgroundStdDev = m_detectedEOI[nrEOIs - 1].backgroundStdDev;
					}
					else {
						m_detectedEOI.push_back(newDetection);
					}
				}
				else {
					m_detectedEOI.push_back(newDetection);
				}
			}
			else {
				m_detectedEOI.push_back(newDetection);
			}
		}
	}


	return true;
}

bool SVM_Detector::fileExists(const char *fileName) {
	std::ifstream infile(fileName);
	return infile.good();
}


void SVM_Detector::readEOI_VisualAnnotations(std::string channelName, int idNumber) {

	double epochLength = 0; m_inputEvents[1].time - m_inputEvents[0].time;
	double firstReadEpchTime = m_inputEvents[0].time - 1.5 * epochLength > 0 ? m_inputEvents[0].time - 1.5 * epochLength : 0;
	double lastReadEpchTime = m_inputEvents[m_inputEvents.size() - 1].time + 1.5 * epochLength;

	std::string fn = m_patientPath + m_patientName + "_allAnnotations.txt";

	std::ifstream eoiAnnotations;
	eoiAnnotations.open(fn.c_str());
	eoiAnnotations.precision(32);
	std::string firstLine;
	std::getline(eoiAnnotations, firstLine);
	unsigned nrAnnotations = 0;

	std::string chosenExpert = "none"; "Expert1"; "Expert2"; "none"; "KK";"JJ";

	while (!eoiAnnotations.eof()) {
		std::string line;
		std::getline(eoiAnnotations, line);
		std::stringstream ss(line);

		double sampleStartAndEnd;
		int refChannNr;

		EOI_Event eoiMark;
		/*ss >> eoiMark.channelName;
		ss >> eoiMark.channelNr;
		ss >> eoiMark.description;
		ss >> refChannNr;
		ss >> sampelStartAndEnd;
		ss >> sampelStartAndEnd;
		ss >> eoiMark.startTime;
		ss >> eoiMark.endTime;*/

		ss >> eoiMark.description;
		ss >> eoiMark.channelName;
		ss >> eoiMark.channelNr;
		ss >> eoiMark.lowNeighborChannelName;
		ss >> eoiMark.lowNeighborChannelNr;
		ss >> eoiMark.highNeighborChannelName;
		ss >> eoiMark.lowNeighborChannelNr;
		ss >> sampleStartAndEnd;
		ss >> sampleStartAndEnd;
		ss >> eoiMark.startTime;
		ss >> eoiMark.endTime;


		if (m_useConfidenceInterval) {
			double duration = eoiMark.endTime - eoiMark.startTime;
			double center = eoiMark.startTime + duration / 2.0;
			eoiMark.startTime = center - 0.05;
			eoiMark.endTime = center + 0.05;
		}

		eoiMark.duration = eoiMark.endTime - eoiMark.startTime;

		//if (eoiMark.duration <= 0.10) {
			if (eoiMark.startTime > firstReadEpchTime && eoiMark.endTime <= lastReadEpchTime && (eoiMark.description.find(chosenExpert) == 0 || chosenExpert.find("none")==0) /*&& eoiMark.duration < 0.2*/) {
			
				std::string label = eoiMark.description;
				bool foundFastRipple = label.find("ast") != std::string::npos;
				bool foundRipple = ((label.find("ipple") != std::string::npos) || (label.find("HFO") != std::string::npos)) && !foundFastRipple;
				bool foundRippleAndFastRipple = false;
				bool foundSpike = label.find("ike") != std::string::npos;
				bool foundSpindle = label.find("indle") != std::string::npos;

				if (label.find("ipple") != std::string::npos) {
					label.erase(label.find("ipple"), 5);
					foundRippleAndFastRipple = label.find("ipple") != std::string::npos;
					if (foundRippleAndFastRipple) {
						foundRipple = false;
						foundFastRipple = false;
					}
				}

				bool markOfInterest = false;
				if (m_eoiName.find("HFO") != std::string::npos) {
					markOfInterest = foundRipple || foundFastRipple || foundRippleAndFastRipple;
				}
				else if (m_eoiName.find("ast") != std::string::npos) {
					markOfInterest = foundRippleAndFastRipple || foundFastRipple;
				}
				else if (m_eoiName.find("ipple") != std::string::npos) {
					markOfInterest = foundRipple || foundRippleAndFastRipple;
				}

				if (m_eoiName.find("ike") != std::string::npos) {
					markOfInterest = foundSpike;
				}

				if (m_eoiName.find("indle") != std::string::npos) {
					markOfInterest = foundSpindle;
				}

				if (markOfInterest) {
					if (!eoiMark.channelName.empty() && eoiMark.startTime != 0 && eoiMark.endTime != 0) {
					
						channelName.erase(std::remove_if(channelName.begin(), channelName.end(), ::isspace), channelName.end());
						eoiMark.channelName.erase(std::remove_if(eoiMark.channelName.begin(), eoiMark.channelName.end(), ::isspace), eoiMark.channelName.end());

						channelName.erase(std::remove(channelName.begin(), channelName.end(), '-'), channelName.end());
						eoiMark.channelName.erase(std::remove(eoiMark.channelName.begin(), eoiMark.channelName.end(), '-'), eoiMark.channelName.end());

						channelName.erase(std::remove(channelName.begin(), channelName.end(), '_'), channelName.end());
						eoiMark.channelName.erase(std::remove(eoiMark.channelName.begin(), eoiMark.channelName.end(), '_'), eoiMark.channelName.end());

						channelName.erase(std::remove(channelName.begin(), channelName.end(), '#'), channelName.end());
						eoiMark.channelName.erase(std::remove(eoiMark.channelName.begin(), eoiMark.channelName.end(), '#'), eoiMark.channelName.end());


						int compareResult = channelName.compare(eoiMark.channelName);
						
						std::string secondChannelName = channelName;
						secondChannelName.erase(secondChannelName.begin(), secondChannelName.begin() + 3);
						int compareResultTwo = secondChannelName.compare(eoiMark.channelName);

						if (compareResult == 0 || compareResultTwo == 0) {

							m_visualEOI.push_back(eoiMark);
							nrAnnotations++;

							if (m_expOneName.empty()) {
								m_expOneName = eoiMark.description.substr(0, 2);
							}
						
							if (m_expTwoName.empty()) {
								if (!m_expOneName.empty() && m_expOneName.compare(eoiMark.description.substr(0, 2))!=0) {
									m_expTwoName = eoiMark.description.substr(0, 2);
								}
							}

						}
					}
				}

			}

		//}

	}

	eoiAnnotations.close();
}

void SVM_Detector::readEOI_VisualSpikes(std::string channelName, int idNumber) {

	double epochLength = 0; m_inputEvents[1].time - m_inputEvents[0].time;
	double firstReadEpchTime = m_inputEvents[0].time - 1.5 * epochLength > 0 ? m_inputEvents[0].time - 1.5 * epochLength : 0;
	double lastReadEpchTime = m_inputEvents[m_inputEvents.size() - 1].time + 1.5 * epochLength;

	std::string fn = m_patientPath + m_patientName + "_allAnnotations.txt";

	std::ifstream eoiAnnotations;
	eoiAnnotations.open(fn.c_str());
	eoiAnnotations.precision(32);
	std::string firstLine;
	std::getline(eoiAnnotations, firstLine);

	std::string chosenExpert = "none"; "Expert1"; "Expert2"; "none"; "KK"; "JJ";

	while (!eoiAnnotations.eof()) {
		std::string line;
		std::getline(eoiAnnotations, line);
		std::stringstream ss(line);

		long sampleStartAndEnd;
		int refChannNr;

		EOI_Event eoiMark;
		/*ss >> eoiMark.channelName;
		ss >> eoiMark.channelNr;
		ss >> eoiMark.description;
		ss >> refChannNr;
		ss >> sampelStartAndEnd;
		ss >> sampelStartAndEnd;
		ss >> eoiMark.startTime;
		ss >> eoiMark.endTime;*/

		ss >> eoiMark.description;
		ss >> eoiMark.channelName;
		ss >> eoiMark.channelNr;
		ss >> eoiMark.lowNeighborChannelName;
		ss >> eoiMark.lowNeighborChannelNr;
		ss >> eoiMark.highNeighborChannelName;
		ss >> eoiMark.lowNeighborChannelNr;
		ss >> sampleStartAndEnd;
		ss >> sampleStartAndEnd;
		ss >> eoiMark.startTime;
		ss >> eoiMark.endTime;

		eoiMark.duration = eoiMark.endTime - eoiMark.startTime;

		if (eoiMark.startTime > firstReadEpchTime && eoiMark.endTime <= lastReadEpchTime && (eoiMark.description.find(chosenExpert) == 0 || chosenExpert.find("none") == 0) /*&& eoiMark.duration < 0.2*/) {

			std::string label = eoiMark.description;
			bool foundSpike = label.find("pike") != std::string::npos;

			if (foundSpike) {
				if (!eoiMark.channelName.empty() && eoiMark.startTime != 0 && eoiMark.endTime != 0) {

					channelName.erase(std::remove_if(channelName.begin(), channelName.end(), ::isspace), channelName.end());
					eoiMark.channelName.erase(std::remove_if(eoiMark.channelName.begin(), eoiMark.channelName.end(), ::isspace), eoiMark.channelName.end());

					channelName.erase(std::remove(channelName.begin(), channelName.end(), '-'), channelName.end());
					eoiMark.channelName.erase(std::remove(eoiMark.channelName.begin(), eoiMark.channelName.end(), '-'), eoiMark.channelName.end());

					channelName.erase(std::remove(channelName.begin(), channelName.end(), '_'), channelName.end());
					eoiMark.channelName.erase(std::remove(eoiMark.channelName.begin(), eoiMark.channelName.end(), '_'), eoiMark.channelName.end());

					channelName.erase(std::remove(channelName.begin(), channelName.end(), '#'), channelName.end());
					eoiMark.channelName.erase(std::remove(eoiMark.channelName.begin(), eoiMark.channelName.end(), '#'), eoiMark.channelName.end());

					int compareResult = channelName.compare(eoiMark.channelName);
					if (compareResult == 0) {
							double spikeDuration = eoiMark.endTime - eoiMark.startTime;
							double spikeCenter = eoiMark.startTime + spikeDuration / 2.0;
							eoiMark.startTime = spikeCenter - 0.05;
							eoiMark.endTime = spikeCenter + 0.05;
							m_visualSpikes.push_back(eoiMark);
					}
				}
			}
		}
	}

	eoiAnnotations.close();

	getSpikesWith_WithoutHFOs();
}

bool SVM_Detector::getSpikesWith_WithoutHFOs() {

	double minSharedPercent = 0.01;

	double nrSpikes = m_visualSpikes.size();
	double nrMarks = m_visualEOI.size();

	for (long spkIdx = 0; spkIdx < nrSpikes; spkIdx++) {

		double spikeStart = m_visualSpikes[spkIdx].startTime;
		double spikeEnd = m_visualSpikes[spkIdx].endTime;
		double spikeDuration = spikeEnd - spikeStart;

		bool overlap = false;
		bool validOnsetDiff = false;
		bool match = false;

		//#pragma omp parallel for
		for (long markIdx = 0; markIdx < nrMarks; markIdx++) {

			double markStart = m_visualEOI[markIdx].startTime;
			double markEnd = m_visualEOI[markIdx].endTime;
			double markDuration = markEnd - markStart;

			double minShared = markDuration < spikeDuration ? minSharedPercent * (markDuration) : minSharedPercent * (spikeDuration);
			double shared = 0;

			if (spikeStart <= markStart && spikeEnd >= markStart) {		//A
				if (spikeEnd <= markEnd)
					shared = spikeEnd - markStart;
				else
					shared = markEnd - markStart;
			}
			else if (spikeStart >= markStart && spikeStart <= markEnd) {	//B
				if (spikeEnd >= markEnd)
					shared = markEnd - spikeStart;
				else
					shared = spikeEnd - spikeStart;
			}

			if (shared >= minShared) {
				match = true;
				break;
			}
		}

		if (match) {
			m_visualSpikesWithHFO.push_back(m_visualSpikes[spkIdx]);
		}
		else {
			m_visualSpikes_wo_HFO.push_back(m_visualSpikes[spkIdx]);
		}
	}

	m_nrSpikes_wo_HFO = 0;
	m_nrSpikes_wo_HFO = m_visualSpikes_wo_HFO.size();

	return true;
}

unsigned SVM_Detector::deleteRepeatedMarks(double maxOnsetDiff, double minSharedPercent) {

	unsigned redundantMarks = 0;
	std::vector<EOI_Event> visualEOI_Copy = m_visualEOI;
	m_visualEOI.clear();

	unsigned nrMarks = visualEOI_Copy.size();
	std::vector<bool> marksToDelete(nrMarks, false);

	for (unsigned fspi = 0; fspi < nrMarks; ++fspi) {

		if (marksToDelete[fspi])		//skip marks whcih were already selected for deletion
			continue;

		double firstMarkStart = visualEOI_Copy[fspi].startTime;
		double firstMarkEnd = visualEOI_Copy[fspi].endTime;
		std::string labelFirst = visualEOI_Copy[fspi].description;

		for (unsigned sspi = fspi + 1; sspi < nrMarks; ++sspi) {
			if (sspi != fspi) {

				bool overlap = false;

				double secondMarkStart = visualEOI_Copy[sspi].startTime;
				double secondMarkEnd = visualEOI_Copy[sspi].endTime;
				std::string labelSec = visualEOI_Copy[sspi].description;

				bool repeatedMark = (firstMarkStart == secondMarkStart) && (firstMarkEnd == secondMarkEnd) && (labelFirst.compare(labelSec) == 0);

				if (repeatedMark) {
					marksToDelete[sspi] = true;
				}
			}
		}
	}

	for (unsigned i = 0; i < nrMarks; ++i) {
		if (!marksToDelete[i]) {
			m_visualEOI.push_back(visualEOI_Copy[i]);
		}
	}

	return m_visualEOI.size();
}

void SVM_Detector::getExpertAgreement(double minSharedPercent) {
	
	long a = 0, b = 0, c = 0, d = 0;

	m_avgMarkDuration = 0;
	for (unsigned i = 0; i < m_visualEOI.size(); ++i) {

		//Expert One
		if (!m_expOneName.empty() && m_visualEOI[i].description.find(m_expOneName) != std::string::npos) {
				m_visualEventsExpOne.push_back(m_visualEOI[i]);
		}

		//Expert Two
		if (!m_expTwoName.empty() && m_visualEOI[i].description.find(m_expTwoName) != std::string::npos) {
			m_visualEventsExpTwo.push_back(m_visualEOI[i]);
		}

		m_avgMarkDuration += m_visualEOI[i].duration;
	}
	m_avgMarkDuration = m_visualEOI.size() > 0 ? m_avgMarkDuration / m_visualEOI.size() : 0;

	unsigned nrMarksExpOne = m_visualEventsExpOne.size();
	unsigned nrMarksExpTwo = m_visualEventsExpTwo.size();
	
	std::vector<bool> untouchedExpTwo(nrMarksExpTwo, true);

	for (unsigned oneIdx = 0; oneIdx < nrMarksExpOne; ++oneIdx) {
		
		bool expOneToched = false;

		double firstMarkStart = m_visualEventsExpOne[oneIdx].startTime;
		double firstMarkEnd = m_visualEventsExpOne[oneIdx].endTime;
		double firstMarkduration = firstMarkEnd - firstMarkStart;

		for (unsigned twoIdx = 0; twoIdx < nrMarksExpTwo; ++twoIdx) {
			
			bool expTwoToched = false;

			double secondMarkStart = m_visualEventsExpTwo[twoIdx].startTime;
			double secondMarkEnd = m_visualEventsExpTwo[twoIdx].endTime;
			double secondMarkduration = secondMarkEnd - secondMarkStart;


			double shared = 0;

			if (firstMarkStart >= secondMarkStart && firstMarkEnd <= secondMarkEnd) {			//first mark completely within second mark
				shared = firstMarkduration;
			}
			else if (firstMarkStart <= secondMarkStart && firstMarkEnd >= secondMarkEnd) {		//second mark completely within first mark
				shared = secondMarkduration;
			}
			else if (firstMarkStart >= secondMarkStart && firstMarkStart <= secondMarkEnd) {		//first mark beginning is within the second marking
				if (firstMarkEnd >= secondMarkEnd) {
					shared = secondMarkEnd - firstMarkStart;
				}
				else {
					shared = firstMarkEnd - firstMarkStart;
				}
			}
			else if (firstMarkStart <= secondMarkStart && firstMarkEnd >= secondMarkStart) {		//epoch end is within a marking
				if (firstMarkEnd <= secondMarkEnd) {
					shared = firstMarkEnd - secondMarkStart;
				}
				else {
					shared = secondMarkEnd - secondMarkStart;
				}
			}

			double minShared = firstMarkduration * minSharedPercent < secondMarkduration * minSharedPercent ? firstMarkduration * minSharedPercent : secondMarkduration * minSharedPercent;
			
			if (shared >= minShared) {
				a++;
				expOneToched = true;
				untouchedExpTwo[twoIdx] = false;
			}
		}

		if (!expOneToched) {
			b++;
		}
	}

	for (unsigned twoIdx = 0; twoIdx < nrMarksExpTwo; ++twoIdx) {
		c += untouchedExpTwo[twoIdx];
	}


	double totalEOIs = m_avgMarkDuration > 0 ? m_signalDuration / m_avgMarkDuration : 0;
	d = totalEOIs - a - b - c;

	double sum = a + b + c + d;
	if (sum > 0) {
		double p_o = (a + d) / sum;
		double marginalA = ((a + b)*(a + c)) / sum;
		double marginalB = ((c + d)*(b + d)) / sum;
		double p_e = (marginalA + marginalB) / sum;
		m_interExpertKappa = (p_o - p_e) / (1 - p_e);
	}
	else {
		m_interExpertKappa = 0;
	}

}

unsigned SVM_Detector::joinCoOccMarks(double maxOnsetDiff, double minSharedPercent, bool join) {

	unsigned redundantMarks = 0;

	if (join) {
		unsigned nrMarks = m_visualEOI.size();
		std::vector<bool> marksToDelete(nrMarks, false);

		for (unsigned fspi = 0; fspi < nrMarks; ++fspi) {

			if (marksToDelete[fspi])		//skip marks whcih were already selected for deletion
				continue;

			double firstMarkStart = m_visualEOI[fspi].startTime;
			double firstMarkEnd = m_visualEOI[fspi].endTime;
			double firstMarkduration = firstMarkEnd - firstMarkStart;
			double firstMarkCenter = firstMarkStart + firstMarkduration / 2;
			
			std::string firstExpertName = m_visualEOI[fspi].description;

			std::string labelFirst = m_visualEOI[fspi].description;
			bool fr_firstExp = labelFirst.find("ast") != std::string::npos;
			bool ripp_firstExp = ((labelFirst.find("ipple") != std::string::npos) || (labelFirst.find("HFO") != std::string::npos)) && !fr_firstExp;
			bool ripp_fr_firstExp = false;
			bool spindleFirstExp = labelFirst.find("indle") != std::string::npos;

			if (labelFirst.find("ipple") != std::string::npos) {
				labelFirst.erase(labelFirst.find("ipple"), 5);
				ripp_fr_firstExp = labelFirst.find("ipple") != std::string::npos;
				if (ripp_fr_firstExp) {
					ripp_firstExp = false;
					fr_firstExp = false;
				}
			}

			for (unsigned sspi = fspi + 1; sspi < nrMarks; ++sspi) {
				if (sspi != fspi) {
					bool overlap = false;

					double secondMarkStart = m_visualEOI[sspi].startTime;
					double secondMarkEnd = m_visualEOI[sspi].endTime;
					double secondMarkduration = secondMarkEnd - secondMarkStart;
					double secondMarkCenter = secondMarkStart + secondMarkduration / 2;

					std::string labelSec = m_visualEOI[sspi].description;
					bool fr_secExp = labelSec.find("ast") != std::string::npos;
					bool ripp_secExp = ((labelSec.find("ipple") != std::string::npos) || (labelSec.find("HFO") != std::string::npos)) && !fr_secExp;
					bool ripp_fr_secExp = false;
					bool spindleSecExp = labelFirst.find("indle") != std::string::npos;

					if (labelSec.find("ipple") != std::string::npos) {
						labelSec.erase(labelSec.find("ipple"), 5);
						ripp_fr_secExp = labelSec.find("ipple") != std::string::npos;
						if (ripp_fr_secExp) {
							ripp_secExp = false;
							fr_secExp = false;
						}
					}


					std::string secondExpertName = m_visualEOI[sspi].description;

					double diff = std::abs(secondMarkStart - firstMarkStart);
					bool differentExpert = firstExpertName.compare(secondExpertName) != 0;

					bool sameMarkedEvent = (ripp_firstExp == ripp_secExp) && (fr_firstExp == fr_secExp) && (ripp_fr_firstExp == ripp_fr_secExp) && (spindleFirstExp == spindleSecExp);

					double shared = 0;

					if (firstMarkStart >= secondMarkStart && firstMarkEnd <= secondMarkEnd) {			//first mark completely within second mark
						shared = firstMarkduration;
					}
					else if (firstMarkStart <= secondMarkStart && firstMarkEnd >= secondMarkEnd) {		//second mark completely within first mark
						shared = secondMarkduration;
					}
					else if (firstMarkStart >= secondMarkStart && firstMarkStart <= secondMarkEnd) {		//first mark beginning is within the second marking
						if (firstMarkEnd >= secondMarkEnd) {
							shared = secondMarkEnd - firstMarkStart;
						}
						else {
							shared = firstMarkEnd - firstMarkStart;
						}
					}
					else if (firstMarkStart <= secondMarkStart && firstMarkEnd >= secondMarkStart) {		//epoch end is within a marking
						if (firstMarkEnd <= secondMarkEnd) {
							shared = firstMarkEnd - secondMarkStart;
						}
						else {
							shared = secondMarkEnd - secondMarkStart;
						}
					}

					if (shared < 0)
						while (1);


					bool match = false;

					double sharedPercent = minSharedPercent;
					double minShared = firstMarkduration * sharedPercent < secondMarkduration * sharedPercent ? firstMarkduration * sharedPercent : secondMarkduration * sharedPercent;
					match = (shared >= minShared && differentExpert && sameMarkedEvent);

					if (match) {
						redundantMarks++;
						double start = (firstMarkStart + secondMarkStart) / 2;
						double end = (firstMarkEnd + secondMarkEnd) / 2;

						m_visualEOI[fspi].startTime = start;
						m_visualEOI[fspi].endTime = end;
						m_visualEOI[fspi].duration = end - start;

						marksToDelete[sspi] = true;
						//break;
					}
				}
			}
		}

		for (unsigned i = 0; i < nrMarks; ++i) {
			if (!marksToDelete[i]) {
				m_visualEOI_Joined.push_back(m_visualEOI[i]);
			}
		}
		m_expertAgreement = nrMarks > 0 ? 200 * redundantMarks / nrMarks : 0;

	}
	else {
		m_visualEOI_Joined = m_visualEOI;
	}

	return m_visualEOI_Joined.size();
}

unsigned SVM_Detector::joinCoOccDetections(double maxOnsetDiff, double minSharedPercent, bool join) {

	if (join) {
		std::vector<EOI_Event> joinedDetectedEOI;
		unsigned nrDetects = m_detectedEOI.size();
		std::vector<bool> detectsToDelete(nrDetects, false);

		for (unsigned fspi = 0; fspi < nrDetects; ++fspi) {

			if (detectsToDelete[fspi])		//skip marks whcih were already selected for deletion
				continue;

			double firstDetectStart = m_detectedEOI[fspi].startTime;
			double firstDetectEnd = m_detectedEOI[fspi].endTime;
			double firstDetectduration = firstDetectEnd - firstDetectStart;

			if (m_useConfidenceInterval) {
				double center = firstDetectStart + firstDetectduration / 2.0;
				firstDetectStart = center - 0.05;
				firstDetectEnd = center + 0.05;
			}

			for (unsigned sspi = fspi + 1; sspi < nrDetects; ++sspi) {
				if (sspi != fspi) {
					bool overlap = false;

					double secondDetectStart = m_detectedEOI[sspi].startTime;
					double secondDetectEnd = m_detectedEOI[sspi].endTime;
					double secondDetectDuration = secondDetectEnd - secondDetectStart;

					if (m_useConfidenceInterval) {
						double center = secondDetectStart + secondDetectDuration / 2.0;
						secondDetectStart = center - 0.05;
						secondDetectEnd = center + 0.05;
					}

					double diff = std::abs(secondDetectStart - firstDetectStart);
					double shared = 0;

					if (firstDetectStart >= secondDetectStart && firstDetectEnd <= secondDetectEnd) {			//first mark completely within second mark
						shared = firstDetectduration;
					}
					else if (firstDetectStart <= secondDetectStart && firstDetectEnd >= secondDetectEnd) {		//second mark completely within first mark
						shared = secondDetectDuration;
					}
					else if (firstDetectStart >= secondDetectStart && firstDetectStart <= secondDetectEnd) {		//first mark beginning is within the second marking
						if (firstDetectEnd >= secondDetectEnd)
							shared = secondDetectEnd - firstDetectStart;
						else
							shared = firstDetectEnd - firstDetectStart;
					}
					else if (firstDetectStart <= secondDetectStart && firstDetectEnd >= secondDetectStart) {		//epoch end is within a marking
						if (firstDetectEnd <= secondDetectEnd)
							shared = firstDetectEnd - secondDetectStart;
						else
							shared = secondDetectEnd - secondDetectStart;
					}

					if (shared < 0)
						while (1);

					bool match = false;

					double minShared = firstDetectduration * minSharedPercent < secondDetectDuration * minSharedPercent ? firstDetectduration * minSharedPercent : secondDetectDuration * minSharedPercent;

					if (shared >= minShared) {
						double start = firstDetectStart < secondDetectStart ? firstDetectStart : secondDetectStart;
						double end = firstDetectEnd > secondDetectEnd ? firstDetectEnd : secondDetectEnd;

						m_detectedEOI[fspi].startTime = start;
						m_detectedEOI[fspi].endTime = end;
						m_detectedEOI[fspi].duration = end - start;
						m_detectedEOI[fspi].description += "+" + m_detectedEOI[sspi].description;

						detectsToDelete[sspi] = true;
					}
				}
			}
		}

		for (unsigned i = 0; i < nrDetects; ++i) {
			if (!detectsToDelete[i]) {
				joinedDetectedEOI.push_back(m_detectedEOI[i]);
			}
		}
		m_detectedEOI.clear();
		m_detectedEOI = joinedDetectedEOI;
	}

	return m_detectedEOI.size();
}

bool SVM_Detector::generateDetectionsFile(double onsetMinDiff, double minSharedPercent) {

	std::string filename = m_patDetectionFilesPath + "/" + m_patientName + "_" + m_eoiName +"Detections.txt";

	double nrDetections = m_detectedEOI.size();
	std::vector<bool> touchedDetects(nrDetections, false);
	m_avgDetectionDuration = 0;

	std::ofstream detectsFile;
	if (!fileExists(filename.c_str())) {
		detectsFile.open(filename);
		//Header
		detectsFile << "SignalDuration:\t" << m_absoluteSignalLength << "\n";
		detectsFile << "Data\t" << "Description\t" << "ChannelName\t" << "ChannelNr.\t" << "StartTime(s)\t" << "EndTime(s)\t" << "Duration(s)\t" << "DayNr.\t" << "StartTime(day)\t" << "EndTime(day)\t" << "Match\n";
	}
	else {
		detectsFile.open(filename, std::ios::app);    // open file for appending
	}
	detectsFile.precision(16);

	for (long di = 0; di < nrDetections; di++) {

		/*std::string label = m_detectedEOI[di].description;
		bool detectFastRipple = label.find("ast") != std::string::npos;
		bool detectRipple = label.find("ipple") != std::string::npos && !detectFastRipple;
		bool detectRippleAndFastRipple = false;
		if (label.find("ipple") != std::string::npos) {
			label.erase(label.find("ipple"), 5);
			detectRippleAndFastRipple = label.find("ipple") != std::string::npos;
			if (detectRippleAndFastRipple) {
				detectRipple = false;
				detectFastRipple = false;
			}
		}*/

		double detectionStart = m_detectedEOI[di].startTime;
		double detectionEnd = m_detectedEOI[di].endTime;
		double detectionDuration = detectionEnd - detectionStart;

		unsigned dayStartHour = 0, dayStartMin = 0;
		double dayStartSec = 0;	

		double fileStartTimeInSeconds = m_fileStartDateAndTime.hours * 3600 + m_fileStartDateAndTime.minutes * 60 + m_fileStartDateAndTime.seconds + m_fileStartDateAndTime.milliSeconds/1000;
		dayStartHour = (m_detectedEOI[di].startTime + fileStartTimeInSeconds) / 3600;
		dayStartMin = fmod(m_detectedEOI[di].startTime + fileStartTimeInSeconds, (double)3600) / 60;
		dayStartSec = fmod(fmod(m_detectedEOI[di].startTime + fileStartTimeInSeconds, (double)3600), 60);

		unsigned dayEndHour = 0, dayEndMin = 0;
		double dayEndSec;
		dayEndHour = (m_detectedEOI[di].endTime + fileStartTimeInSeconds) / 3600;
		dayEndMin = fmod(m_detectedEOI[di].endTime + fileStartTimeInSeconds, (double)3600) / 60;
		dayEndSec = fmod(fmod(m_detectedEOI[di].endTime + fileStartTimeInSeconds, (double)3600), 60);

		int dayNr = (dayStartHour / 24);
		if (dayStartHour >= 24) {
			dayStartHour = dayStartHour - (24 * (dayStartHour / 24));
		}
		if (dayEndHour >= 24) {
			dayEndHour = dayEndHour - (24 * (dayEndHour / 24));
		}

		bool overlap = false;
		bool validOnsetDiff = false;
		bool match = false;

//#pragma omp parallel for
		for (long vmi = 0; vmi < m_visualEOI_Joined.size(); vmi++) {

			/*std::string mark = m_visualEOI_Joined[vmi].description;
			bool markFastRipple = mark.find("ast") != std::string::npos;
			bool markRipple = ((mark.find("ipple") != std::string::npos) || (mark.find("HFO") != std::string::npos)) && !markFastRipple;
			bool markRippleAndFastRipple = false;
			if (mark.find("ipple") != std::string::npos) {
				mark.erase(mark.find("ipple"), 5);
				markRippleAndFastRipple = mark.find("ipple") != std::string::npos;
				if (markRippleAndFastRipple) {
					markRipple = false;
					markFastRipple = false;
				}
			}*/

			double markingStart = m_visualEOI_Joined[vmi].startTime;
			double markingEnd = m_visualEOI_Joined[vmi].endTime;
			double markingDuration = markingEnd - markingStart;
			double markingCenter = markingStart + markingDuration / 2.0;

			double minShared = markingDuration < detectionDuration ? minSharedPercent * (markingDuration) : minSharedPercent * (detectionDuration);
			double shared = 0;

			if (m_useConfidenceInterval) {
				markingStart = markingCenter - 0.05;
				markingEnd = markingCenter + 0.05;
			}


			double onsetDiff = std::abs(markingStart - detectionStart);

			if (onsetDiff <= onsetMinDiff) {
				validOnsetDiff = true;
			}

			if (detectionStart <= markingStart && detectionEnd >= markingStart) {		//A
				if (detectionEnd <= markingEnd)
					shared = detectionEnd - markingStart;
				else
					shared = markingEnd - markingStart;
			}
			else if (detectionStart >= markingStart && detectionStart <= markingEnd) {	//B
				if (detectionEnd >= markingEnd)
					shared = markingEnd - detectionStart;
				else
					shared = detectionEnd - detectionStart;
			}

			if (shared >= minShared) {
				overlap = true;
			}

			if (overlap && validOnsetDiff) {
				match = true;
				break;
				//uncomment if a strict performance measurement is wanted
				/*if (markRipple && (detectRipple || detectRippleAndFastRipple)) {
					match = true;
					break;
				}
				if (markFastRipple && (detectFastRipple || detectRippleAndFastRipple)) {
					match = true;
					break;
				}
				if (markRippleAndFastRipple && detectRippleAndFastRipple) {
					match = true;
					break;
				}*/
			}
		}

		m_avgDetectionDuration += m_detectedEOI[di].duration;
		touchedDetects[di] = match;
			detectsFile << "d:\t" <<
			m_detectedEOI[di].description <<"\t" <<
			m_detectedEOI[di].channelName << "\t" <<
			m_detectedEOI[di].channelNr << "\t" <<
			m_detectedEOI[di].startTime << "\t" <<
			m_detectedEOI[di].endTime << "\t" <<
			m_detectedEOI[di].duration << "\t" <<
			dayNr << "\t" <<
			dayStartHour << ":" << dayStartMin << ":" << dayStartSec << "\t" <<
			dayEndHour << ":" << dayEndMin << ":" << dayEndSec << "\t" <<
			match << "\n";
			detectsFile.flush();

	}
	
	m_avgDetectionDuration = nrDetections > 0 ? m_avgDetectionDuration / nrDetections : 0;

	m_svmPerformance.truePositives = 0;
	for (long i = 0; i < touchedDetects.size(); i++) {
		m_svmPerformance.truePositives += touchedDetects[i];
	}

	m_svmPerformance.falsePositives = touchedDetects.size() - m_svmPerformance.truePositives;

	detectsFile.close();
	return true;
}

bool SVM_Detector::generateSmallDetectionsFile(double onsetMinDiff, double minSharedPercent) {

	std::string filename = m_patDetectionFilesPath + "/" + m_patientName + "_" + m_eoiName + "Detections.txt";

	double nrDetections = m_detectedEOI.size();
	std::vector<bool> touchedDetects(nrDetections, false);
	m_avgDetectionDuration = 0;

	std::ofstream detectsFile;
	if (!fileExists(filename.c_str())) {
		detectsFile.open(filename);
		//Header
		detectsFile << "Data\t" << "Description\t" << "ChannelName\t" << "StartTime(s)\t" << "EndTime(s)\n";
	}
	else {
		detectsFile.open(filename, std::ios::app);    // open file for appending
	}
	detectsFile.precision(16);

	for (long di = 0; di < nrDetections; di++) {

		detectsFile << "d\t" << m_detectedEOI[di].description << "\t" << m_detectedEOI[di].channelName << "\t" << m_detectedEOI[di].startTime << "\t" << m_detectedEOI[di].endTime << "\n";
		detectsFile.flush();
	}
	detectsFile.close();
	return true;
}

bool SVM_Detector::generateDetectionsAndFeaturesFile(double onsetMinDiff, double minSharedPercent) {

	int precVal = 16;
	std::string filename = m_patDetectionFilesPath + "/" + m_patientName + "_" + m_eoiName + "DetectionsAndFeatures.txt";

	double nrDetections = m_detectedEOI.size();
	std::vector<bool> touchedDetects(nrDetections, false);
	m_avgDetectionDuration = 0;

	std::ofstream detectsFile;
	if (!fileExists(filename.c_str())) {
		detectsFile.open(filename);
		//Header
		detectsFile << "Description\t" << "ChannelName\t" << "StartTime(s)\t" << "EndTime(s)\t" << "MaxEventAmplitude\t" << "MaxEventPower\t" << "MaxEventSpectralPeak (Hz)\t" << "AvgBackgroundAmplitude\t" << "AvgBackgroundPower\t" << "BackgroundStdDev\n";
	}
	else {
		detectsFile.open(filename, std::ios::app);    // open file for appending
	}
	detectsFile.precision(precVal);

	precVal = 32;
	for (long di = 0; di < nrDetections; di++) {
		std::string detecsFeatsStr = m_detectedEOI[di].description + "\t" + m_detectedEOI[di].channelName + "\t" + 
			to_string_with_precision(m_detectedEOI[di].startTime, precVal) + "\t" + to_string_with_precision(m_detectedEOI[di].endTime, precVal) + "\t" +
			to_string_with_precision(m_detectedEOI[di].amplitude, precVal) + "\t" + to_string_with_precision(m_detectedEOI[di].power, precVal) + "\t" + 
			to_string_with_precision(m_detectedEOI[di].spectralPeak, precVal) + "\t" + to_string_with_precision(m_detectedEOI[di].backgroundAmplitude, precVal) + "\t" + 
			to_string_with_precision(m_detectedEOI[di].backgroundPower, precVal) + "\t" + to_string_with_precision(m_detectedEOI[di].backgroundStdDev, precVal) + "\n";
		detectsFile << detecsFeatsStr;

		/*detectsFile << m_detectedEOI[di].description << "\t" << m_detectedEOI[di].channelName << "\t" << m_detectedEOI[di].startTime << "\t" << m_detectedEOI[di].endTime << "\t"
			<< m_detectedEOI[di].amplitude << "\t" << m_detectedEOI[di].power << "\t" << m_detectedEOI[di].spectralPeak << "\t"
			<< m_detectedEOI[di].backgroundAmplitude << "\t" << m_detectedEOI[di].backgroundPower << "\t" << m_detectedEOI[di].backgroundStdDev << "\n";*/
		detectsFile.flush();
	}
	detectsFile.close();
	return true;
}

bool SVM_Detector::generateMarksFile(double onsetMinDiff, double minSharedPercent) {

	std::string filename = m_patDetectionFilesPath + "/" + m_patientName + "_" + m_eoiName + "Marks.txt";

	double nrMarks = m_visualEOI.size();

	std::ofstream marksFile;
	if (!fileExists(filename.c_str())) {
		marksFile.open(filename);
		//Header
		marksFile << "SignalDuration:\t" << m_absoluteSignalLength << "\n";
		marksFile << "Data\t" << "Description\t" << "ChannelName\t" << "Channel\t" << "StartTime(s)\t" << "EndTime(s)\t" << "StartTime(day)\t" << "RealEndTime(day)\t" << "Duration(s)\t" << "Match\n";
	}
	else {
		marksFile.open(filename, std::ios::app);    // open file for appending
	}


	for (long vmi = 0; vmi < nrMarks; vmi++) {

		double markStart = m_visualEOI[vmi].startTime;
		double markEnd = m_visualEOI[vmi].endTime;
		double markDuration = markEnd - markStart;

		unsigned dayStartHour = 0, dayStartMin = 0;
		double dayStartSec = 0;
		dayStartHour = (markStart + 0) / 3600;
		dayStartMin = fmod(markStart + 0, (double)3600) / 60;
		dayStartSec = fmod(fmod(markStart + 0, (double)3600), 60);

		unsigned dayEndHour = 0, dayEndMin = 0;
		double dayEndSec;
		dayEndHour = (markEnd + 0) / 3600;
		dayEndMin = fmod(markEnd + 0, (double)3600) / 60;
		dayEndSec = fmod(fmod(markEnd + 0, (double)3600), 60);

		bool overlap = false;
		bool validOnsetDiff = false;
		bool match = false;

#pragma omp parallel for
		for (long di = 0; di < m_detectedEOI.size(); di++) {

			double detectionStart = m_detectedEOI[di].startTime;
			double detectionEnd = m_detectedEOI[di].endTime;
			double detectionDuration = detectionEnd - detectionStart;
			double minShared = markDuration < detectionDuration ? minSharedPercent * (markDuration) : minSharedPercent * (detectionDuration);
			double shared = 0;

			double onsetDiff = std::abs(markStart - detectionStart);

			if (onsetDiff <= onsetMinDiff) {
				validOnsetDiff = true;
			}

			if (detectionStart <= markStart && detectionEnd >= markStart) {		//A
				if (detectionEnd <= markEnd)
					shared = detectionEnd - markStart;
				else
					shared = markEnd - markStart;
			}
			else if (detectionStart >= markStart && detectionStart <= markEnd) {	//B
				if (detectionEnd >= markEnd)
					shared = markEnd - detectionStart;
				else
					shared = detectionEnd - detectionStart;
			}

			if (shared >= minShared) {
				overlap = true;
			}

			if (overlap && validOnsetDiff) {
				match = true;
				break;
			}

		}

		marksFile << "d:\t" <<
			m_visualEOI[vmi].description << "\t" <<
			m_visualEOI[vmi].channelName << "\t" <<
			m_visualEOI[vmi].channelNr << "\t" <<
			m_visualEOI[vmi].startTime << "\t" <<
			m_visualEOI[vmi].endTime << "\t" <<
			dayStartHour << ":" << dayStartMin << ":" << dayStartSec << "\t" <<
			dayEndHour << ":" << dayEndMin << ":" << dayEndSec << "\t" <<
			m_visualEOI[vmi].duration << "\t" <<
			match << "\n";
	}

	marksFile.close();
	return true;
}


bool SVM_Detector::generateJoinedMarksFile(double onsetMinDiff, double minSharedPercent) {

	std::string filename = m_patDetectionFilesPath + "/" + m_patientName + "_" + m_eoiName + "JoinedMarks.txt";

	double nrMarks = m_visualEOI_Joined.size();
	std::vector<bool> untouchedMarks(nrMarks, true);
	m_avgMarkDuration = 0;

	std::ofstream marksFile;
	if (!fileExists(filename.c_str())) {
		marksFile.open(filename);
		//Header
		marksFile << "SignalDuration:\t" << m_signalDuration << "\n";
		marksFile << "Data\t" << "Description\t" << "ChannelName\t" << "Channel\t" << "StartTime(s)\t" << "EndTime(s)\t" << "StartTime(day)\t" << "RealEndTime(day)\t" << "Duration(s)\t" << "Match\n";
	}
	else {
		marksFile.open(filename, std::ios::app);    // open file for appending
	}


	for (long vmi = 0; vmi < nrMarks; vmi++) {

		/*std::string mark = m_visualEOI_Joined[vmi].description;
		bool markFastRipple = mark.find("ast") != std::string::npos;
		bool markRipple = ((mark.find("ipple") != std::string::npos) || (mark.find("HFO") != std::string::npos)) && !markFastRipple;
		bool markRippleAndFastRipple = false;
		if (mark.find("ipple") != std::string::npos) {
			mark.erase(mark.find("ipple"), 5);
			markRippleAndFastRipple = mark.find("ipple") != std::string::npos;
			if (markRippleAndFastRipple) {
				markRipple = false;
				markFastRipple = false;
			}
		}*/

		double markStart = m_visualEOI_Joined[vmi].startTime;
		double markEnd = m_visualEOI_Joined[vmi].endTime;
		double markDuration = markEnd - markStart;
		double markCenter = markStart + markDuration / 2.0;

		if (m_useConfidenceInterval) {
			markStart = markCenter - 0.05;
			markEnd = markCenter + 0.05;
		}

		unsigned dayStartHour = 0, dayStartMin = 0;
		double dayStartSec = 0;
		dayStartHour = (markStart + 0) / 3600;
		dayStartMin = fmod(markStart + 0, (double)3600) / 60;
		dayStartSec = fmod(fmod(markStart + 0, (double)3600), 60);

		unsigned dayEndHour = 0, dayEndMin = 0;
		double dayEndSec;
		dayEndHour = (markEnd + 0) / 3600;
		dayEndMin = fmod(markEnd + 0, (double)3600) / 60;
		dayEndSec = fmod(fmod(markEnd + 0, (double)3600), 60);

		bool overlap = false;
		bool validOnsetDiff = false;
		bool match = false;
//#pragma omp parallel for
		for (long di = 0; di < m_detectedEOI.size(); di++) {

			/*std::string label = m_detectedEOI[di].description;
			bool detectFastRipple = label.find("ast") != std::string::npos;
			bool detectRipple = label.find("ipple") != std::string::npos && !detectFastRipple;
			bool detectRippleAndFastRipple = false;
			if (label.find("ipple") != std::string::npos) {
				label.erase(label.find("ipple"), 5);
				detectRippleAndFastRipple = label.find("ipple") != std::string::npos;
				if (detectRippleAndFastRipple) {
					detectRipple = false;
					detectFastRipple = false;
				}
			}*/


			double detectionStart = m_detectedEOI[di].startTime;
			double detectionEnd = m_detectedEOI[di].endTime;
			double detectionDuration = detectionEnd - detectionStart;
			double minShared = markDuration < detectionDuration ? minSharedPercent * (markDuration) : minSharedPercent * (detectionDuration);
			double shared = 0;

			double onsetDiff = std::abs(markStart - detectionStart);

			if (onsetDiff <= onsetMinDiff) {
				validOnsetDiff = true;
			}

			if (detectionStart <= markStart && detectionEnd >= markStart) {		//A
				if (detectionEnd <= markEnd) {
					shared = detectionEnd - markStart;
				}
				else {
					shared = markEnd - markStart;
				}
			}
			else if (detectionStart >= markStart && detectionStart <= markEnd) {	//B
				if (detectionEnd >= markEnd) {
					shared = markEnd - detectionStart;
				}
				else {
					shared = detectionEnd - detectionStart;
				}
			}

			if (shared >= minShared) {
				overlap = true;
			}

			if (overlap && validOnsetDiff) {
				match = true;
				break;
				//uncomment if a strict performance measurement is wanted
				/*if (markRipple && (detectRipple || detectRippleAndFastRipple)) {
				match = true;
				break;
				}
				if (markFastRipple && (detectFastRipple || detectRippleAndFastRipple)) {
				match = true;
				break;
				}
				if (markRippleAndFastRipple && detectRippleAndFastRipple) {
				match = true;
				break;
				}*/
			}
		}

		m_avgMarkDuration += m_visualEOI_Joined[vmi].duration;

		untouchedMarks[vmi] = !match;

		marksFile << "d:\t" <<
			m_visualEOI_Joined[vmi].description << "\t" <<
			m_visualEOI_Joined[vmi].channelName << "\t" <<
			m_visualEOI_Joined[vmi].channelNr << "\t" <<
			m_visualEOI_Joined[vmi].startTime << "\t" <<
			m_visualEOI_Joined[vmi].endTime << "\t" <<
			dayStartHour << ":" << dayStartMin << ":" << dayStartSec << "\t" <<
			dayEndHour << ":" << dayEndMin << ":" << dayEndSec << "\t" <<
			m_visualEOI_Joined[vmi].duration << "\t" <<
			match << "\n";
	}

	m_avgMarkDuration = nrMarks  > 0? m_avgMarkDuration /= nrMarks : 0;

	for (unsigned i = 0; i < untouchedMarks.size(); ++i) {
		m_svmPerformance.falseNegatives += untouchedMarks[i];
	}
	

	marksFile.close();
	return true;
}

bool SVM_Detector::getFastTansientHFOs(double onsetMinDiff, double minSharedPercent) {

	std::string filename = m_patDetectionFilesPath + "/" + m_patientName + "_" + m_eoiName + "Spikes.txt";

	double nrIsolSpikes = m_visualSpikes_wo_HFO.size();
	double nrDetections = m_detectedEOI.size();

	std::vector<bool> touchedDetects(nrDetections, false);
	m_nrSpikesCausingFalseHFO = 0;

	std::ofstream spikesFile;
	if (!fileExists(filename.c_str())) {
		spikesFile.open(filename);
		//Header
		spikesFile << "SignalDuration:\t" << m_absoluteSignalLength << "\n";
		spikesFile << "Data\t" << "Description\t" << "ChannelName\t" << "Channel\t" << "StartTime(s)\t" << "EndTime(s)\t" << "Duration(s)\t" << "Match\n";
	}
	else {
		spikesFile.open(filename, std::ios::app);    // open file for appending
	}

	for (long isolSpkIdx = 0; isolSpkIdx < nrIsolSpikes; isolSpkIdx++) {

		double spikeStart = m_visualSpikes_wo_HFO[isolSpkIdx].startTime;
		double spikeEnd = m_visualSpikes_wo_HFO[isolSpkIdx].endTime;
		double spikeDuration = spikeEnd - spikeStart;

		bool overlap = false;
		bool validOnsetDiff = false;
		bool match = false;

		//#pragma omp parallel for
		for (long detIdx = 0; detIdx < nrDetections; detIdx++) {

			double detStart = m_detectedEOI[detIdx].startTime;
			double detEnd = m_detectedEOI[detIdx].endTime;
			double detDuration = detEnd - detStart;
			double detCenter = detStart + detDuration / 2.0;

			double minShared = detDuration < spikeDuration ? minSharedPercent * (detDuration) : minSharedPercent * (spikeDuration);
			double shared = 0;

			if (spikeStart <= detStart && spikeEnd >= detStart) {		//A
				if (spikeEnd <= detEnd)
					shared = spikeEnd - detStart;
				else
					shared = detEnd - detStart;
			}
			else if (spikeStart >= detStart && spikeStart <= detEnd) {	//B
				if (spikeEnd >= detEnd)
					shared = detEnd - spikeStart;
				else
					shared = spikeEnd - spikeStart;
			}

			if (shared >= minShared) {
				match = true;
				touchedDetects[detIdx] = match;
			}
		}

		m_nrSpikesCausingFalseHFO += match;

		spikesFile << "d:\t" <<
			m_visualSpikes_wo_HFO[isolSpkIdx].description << "\t" <<
			m_visualSpikes_wo_HFO[isolSpkIdx].channelName << "\t" <<
			m_visualSpikes_wo_HFO[isolSpkIdx].channelNr << "\t" <<
			m_visualSpikes_wo_HFO[isolSpkIdx].startTime << "\t" <<
			m_visualSpikes_wo_HFO[isolSpkIdx].endTime << "\t" <<
			m_visualSpikes_wo_HFO[isolSpkIdx].duration << "\t" <<
			match << "\n";

	}

	m_artefactualDetections = 0;
	m_nrSpikes_wo_HFO = 0;
	for (long i = 0; i < touchedDetects.size(); i++) {
		m_artefactualDetections += touchedDetects[i];
	}
	spikesFile.close();

	m_nrSpikes_wo_HFO = m_visualSpikes_wo_HFO.size();

	return true;
}

double SVM_Detector::getMultiPatientPerformance(double maxOnsetDiff, double minSharedPercent, std::string patientName,
	std::string eegChannName, bool useWindowConfInterv) {

	double nrDetections = m_detectedEOI.size();
	double totalEOIs = m_avgDetectionDuration > 0 ? m_signalDuration / m_avgDetectionDuration : m_signalDuration / m_epochLength;

	//std::string filename = m_patDetectionFilesPath + "/" + m_patientName + "_" + m_eoiName + "Performance.xls";
	std::string filename = m_outputPath + "/" + m_eoiName + "Performance.xls";

	double sensitivity = 0, precision = 0, F_One = 0, F_two = 0, specificity = 0, expertAgreement = 0, 
		kappa = 0, mattCC = 0, weightedKappa = 0, accuracy = 0, negativePredictiveValue = 0;

	if (nrDetections > 0) {
		m_svmPerformance.trueNegatives = totalEOIs - m_svmPerformance.falsePositives - m_svmPerformance.truePositives - m_svmPerformance.falseNegatives;

		getMetrics(sensitivity, precision, specificity, accuracy, negativePredictiveValue, kappa, weightedKappa, mattCC);
		F_One = (precision > 0 && sensitivity > 0) ? 2 * (precision * sensitivity) / (precision + sensitivity) : 0;
		F_two = (precision > 0 && sensitivity > 0) ? 5 * (precision * sensitivity) / (4 * precision + sensitivity) : 0;
	}

	int nrSelectedFeats = m_selectedFeats.size();
	std::string selectedFeats;
	for (int i = 0; i < nrSelectedFeats; ++i) {
		selectedFeats += std::to_string(m_selectedFeats[i]) + ", ";
	}

	mattCC *= 100;

	double PercentArtefactualHFOs = m_detectedEOI.size() > 0 ? 100.0 * ((double)m_artefactualDetections / (double)m_detectedEOI.size()) : 0;
	double PercentSpikesAsHFOs = m_nrSpikes_wo_HFO > 0 ? 100.0 * ((double)m_nrSpikesCausingFalseHFO / (double)m_nrSpikes_wo_HFO) : 0;
	m_nrSpikes = m_visualSpikes.size();

	std::string confInterval = useWindowConfInterv ? "WindowCI" : "Share" + std::to_string((int)(minSharedPercent*100.0)) + "%";

	time_t t = time(0);   // get time now
	struct tm * now = localtime(&t);

	std::ofstream performanceFile;
	std::ifstream infile(filename);
	if (!infile.good()) {
		//remove(filename.c_str());
		performanceFile.open(filename);
		//performanceFile << (now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' << now->tm_mday << ", " << now->tm_hour << ":" << now->tm_min << ":" << now->tm_sec << std::endl;
		performanceFile << "Date\t" << "Time\t" 
			<< "PatientName\t" << "EEG_Channel\t"
			<< "SamplingRate\t" << "Length(s)\t"
			<< "Threshold\t" << "WindowCI\t" << "MaxOnsetDiff\t" << "MinSharedTime\t" 
			
			<< "TP\t" << "FP\t" << "TN\t" << "FN\t"
			<< "Sensitivity\t" << "Specificity\t" << "Precision\t" << "F1\t" << "F2\t" << "Kappa\t" << "WeightedKappa\t" << "MatthewsCC\t" 
			<< "avgDetectionDuration\t" << "avgMarkDuration\t" << "ExpertAgreement\t" << "InterExpertKappa\t"
			<< "Markings\t" << "Detections\t"
			<< "Nr.Spikes\t" << "Nr.Spikes w/o HFO\t" << "Nr.SpikesDetectedAsHFO\t" << "Nr. Artefactual Detections\t" << "PercentSpikesAsHFOs\t" << "PercentArtefactualHFOs\t"
			<< "Function\t" << "nrShortEOIs\t" << "m_nrNoOscills_EOI\t" << "nrLongEOIs\t" << "nrNoHistoryCompliant_EOI\t" << "nrSelectedFeats\n";
	}
	else {
		performanceFile.open(filename, std::ios::app);    // open file for appending
	}

	performanceFile << (now->tm_year + 1900) << '-' << (now->tm_mon + 1) << '-' << now->tm_mday << "\t";
	performanceFile << now->tm_hour << ":" << now->tm_min << ":" << now->tm_sec << "\t";
	performanceFile << patientName << "\t" << eegChannName << "\t";
	performanceFile << m_samplingRate << "\t" << m_signalDuration << "\t";
	performanceFile << m_probThreshold << "\t" << confInterval << "\t" << maxOnsetDiff << "\t" << minSharedPercent << "\t";
	performanceFile << m_svmPerformance.truePositives << "\t" << m_svmPerformance.falsePositives << "\t" << m_svmPerformance.trueNegatives << "\t" << m_svmPerformance.falseNegatives << "\t";
	performanceFile << sensitivity << "\t" << specificity << "\t" << precision << "\t" << F_One << "\t" << F_two << "\t" << kappa << "\t" << weightedKappa << "\t" << mattCC << "\t";
	performanceFile << m_avgDetectionDuration << "\t" << m_avgMarkDuration << "\t" << m_expertAgreement << "\t" << m_interExpertKappa << "\t";
	performanceFile << m_visualEOI_Joined.size() << "\t" << m_detectedEOI.size() << "\t";
	performanceFile << m_nrSpikes << "\t" << m_nrSpikes_wo_HFO << "\t" << m_nrSpikesCausingFalseHFO << "\t" << m_artefactualDetections << "\t" << PercentSpikesAsHFOs << "\t" << PercentArtefactualHFOs << "\t";
	performanceFile << m_savedFunctionName << "\t" << m_nrShort_EOI << "\t" << m_nrNoOscills_EOI << "\t" << m_nrLong_EOI << "\t" << m_nrNoHistoryCompliant_EOI << "\t" << selectedFeats;
	performanceFile << "\n";
	performanceFile.close();

	m_nrShort_EOI = 0;
	m_nrLong_EOI = 0;
	m_nrNoOscills_EOI = 0;
	m_nrNoHistoryCompliant_EOI = 0;

	return mattCC;
}

bool SVM_Detector::getMetrics(double &sensitivity, double &precision, double &specificity, double &accuracy, double &negativePredictiveValue, double &kappa, double &weightedKappa, double& mattCC) {

	//sensitivity = m_visualEOI_Joined.size() > 0 ? 100 * (1 - m_svmPerformance.falseNegatives / m_visualEOI_Joined.size()) : 0;
	sensitivity = m_visualEOI_Joined.size() > 0 ? 100 * (m_svmPerformance.truePositives / (m_svmPerformance.truePositives + m_svmPerformance.falseNegatives)) : 0;

	precision = (m_svmPerformance.truePositives + m_svmPerformance.falsePositives) > 0 ? 100 * (m_svmPerformance.truePositives / (m_svmPerformance.truePositives + m_svmPerformance.falsePositives)) : 0;

	specificity = (m_svmPerformance.falsePositives + m_svmPerformance.trueNegatives) > 0 ? 100.0 * (m_svmPerformance.trueNegatives / (m_svmPerformance.falsePositives + m_svmPerformance.trueNegatives)) : 0;

	accuracy = (m_svmPerformance.truePositives + m_svmPerformance.falsePositives + m_svmPerformance.falseNegatives + m_svmPerformance.trueNegatives) > 0 ? (m_svmPerformance.truePositives + m_svmPerformance.trueNegatives) / (m_svmPerformance.truePositives + m_svmPerformance.falsePositives + m_svmPerformance.falseNegatives + m_svmPerformance.trueNegatives) : 0;

	negativePredictiveValue = (m_svmPerformance.trueNegatives + m_svmPerformance.falseNegatives) > 0 ? m_svmPerformance.trueNegatives / (m_svmPerformance.trueNegatives + m_svmPerformance.falseNegatives) : 0;

	//Kappa
	double a = 0, b = 0, c = 0, d = 0;

	a = m_svmPerformance.truePositives;
	b = m_svmPerformance.falseNegatives;
	c = m_svmPerformance.falsePositives;
	d = m_svmPerformance.trueNegatives;

	double sum = a + b + c + d;

	double p_o = (a + d) / sum;
	double marginalA = ((a + b)*(a + c)) / sum;
	double marginalB = ((c + d)*(b + d)) / sum;
	double p_e = (marginalA + marginalB) / sum;
	kappa = (p_o - p_e) / (1 - p_e);

	//Weighted Kappa
	a = m_svmPerformance.truePositives * 10;
	b = m_svmPerformance.falseNegatives * 10;
	c = m_svmPerformance.falsePositives;
	d = m_svmPerformance.trueNegatives;

	sum = a + b + c + d;
	p_o = (a + d) / sum;
	marginalA = ((a + b)*(a + c)) / sum;
	marginalB = ((c + d)*(b + d)) / sum;
	p_e = (marginalA + marginalB) / sum;

	weightedKappa = (p_o - p_e) / (1 - p_e);

	// Matthews Correlation Coefficient
	double matthewsCC_num = (m_svmPerformance.truePositives * m_svmPerformance.trueNegatives - m_svmPerformance.falsePositives * m_svmPerformance.falseNegatives);
	double matthewsCC_denom = sqrt((m_svmPerformance.truePositives + m_svmPerformance.falsePositives) * (m_svmPerformance.truePositives + m_svmPerformance.falseNegatives) * (m_svmPerformance.trueNegatives + m_svmPerformance.falsePositives) * (m_svmPerformance.trueNegatives + m_svmPerformance.falseNegatives));

	mattCC = matthewsCC_denom > 0 ? matthewsCC_num / matthewsCC_denom : 0;

	return true;
}

void SVM_Detector::spikeDetection(std::vector<bool> &allSpikesEpochVal, std::vector<bool> &spikeHFO_EpochVal, std::vector<bool> &onlySpike_EpochVal) {
	
	unsigned numTestingEvs = m_inputEvents.size();

	funct_type learned_functionAllSpikes;
	std::string allSpikesFuncName = "DecisionFunctions/AllSpikes_UD_NPR1_PFSA80_5e-05_0.03125_2-14, 19_3_1_reduced.dat";
	double allSpikes_Thrheshold = 0.0;
	//std::string allSpikesFuncName = "DecisionFunctions/allSpikes_Clinical_NPR1_PFSA80_1e-05_0.78125_2-14, 12_52_55_reduced.dat";
	//double allSpikes_Thrheshold = 0.0;

	deserialize(allSpikesFuncName) >> learned_functionAllSpikes;
	std::vector<int> selFeatsAllSpikes = { 3, 6, 7, 9, 16, 20, 24 };
	std::vector<sample_type> samplesAllSpikes;
	vector_normalizer<sample_type> normalizerAllSpikes;
	unsigned nrSelectedFeaturesAllSpikes = selFeatsAllSpikes.size();
	for (unsigned evIdx = 0; evIdx < numTestingEvs; ++evIdx) {
		sample_type samp;
		samp.set_size(nrSelectedFeaturesAllSpikes);
		unsigned numTrainingFeatures = m_inputEvents[evIdx].inputVals.size();
		unsigned nrReadFeatures = 0;
		for (unsigned featIdx = 0; featIdx < numTrainingFeatures; ++featIdx) {
			if (std::find(selFeatsAllSpikes.begin(), selFeatsAllSpikes.end(), featIdx) != selFeatsAllSpikes.end()) {
				samp(nrReadFeatures) = m_inputEvents[evIdx].inputVals[featIdx];
				nrReadFeatures++;
			}
			//assert(NUM_SELECTED_FEATURES == samp.size());
		}
		samplesAllSpikes.push_back(samp);
	}
	normalizerAllSpikes.train(samplesAllSpikes);

	funct_type learned_functionSpikeHFO;
	std::string spikeHFO_FuncName = "DecisionFunctions/SpikeHFO_UD_NPR4_PFSA20_1e-05_0.00125_2-14, 21_33_4_reduced.dat";
	deserialize(spikeHFO_FuncName) >> learned_functionSpikeHFO;
	std::vector<int> selFeatsSpikeHFO = { 0, 1, 2, 3, 4, 6, 7, 9, 15, 16, 17, 19, 20, 22, 24, 28, 29, 30, 32, 33, 35, 38, 39, 40, 41, 42, 45, 46, 52 };
	double spikeHFO_Thrheshold = 0.1;
	std::vector<sample_type> samplesSpikeHFO;
	vector_normalizer<sample_type> normalizerSpikeHFO;
	unsigned nrSelectedFeaturesSpikeHFO = selFeatsSpikeHFO.size();
	for (unsigned evIdx = 0; evIdx < numTestingEvs; ++evIdx) {
		sample_type samp;
		samp.set_size(nrSelectedFeaturesSpikeHFO);
		unsigned numTrainingFeatures = m_inputEvents[evIdx].inputVals.size();
		unsigned nrReadFeatures = 0;
		for (unsigned featIdx = 0; featIdx < numTrainingFeatures; ++featIdx) {
			if (std::find(selFeatsSpikeHFO.begin(), selFeatsSpikeHFO.end(), featIdx) != selFeatsSpikeHFO.end()) {
				samp(nrReadFeatures) = m_inputEvents[evIdx].inputVals[featIdx];
				nrReadFeatures++;
			}
			//assert(NUM_SELECTED_FEATURES == samp.size());
		}
		samplesSpikeHFO.push_back(samp);
	}
	normalizerSpikeHFO.train(samplesSpikeHFO);

	funct_type learned_functionOnlySpike;
	std::string onlySpike_FuncName = "DecisionFunctions/onlySpike_Clinical_NPR1_PFSA50_1e-05_0.15625_2-14, 13_38_50_reduced.dat";
	deserialize(onlySpike_FuncName) >> learned_functionOnlySpike;
	std::vector<int> selFeatsOnlySpike = { 3, 4, 6, 7, 8, 9, 11, 13, 14, 16, 20, 21, 22, 24, 26, 27, 39 };
	double onlySpike_Thrheshold = 0.2;
	std::vector<sample_type> samplesOnlySpike;
	vector_normalizer<sample_type> normalizerOnlySpike;
	unsigned nrSelectedFeaturesOnlySpike = selFeatsOnlySpike.size();
	for (unsigned evIdx = 0; evIdx < numTestingEvs; ++evIdx) {
		sample_type samp;
		samp.set_size(nrSelectedFeaturesOnlySpike);
		unsigned numTrainingFeatures = m_inputEvents[evIdx].inputVals.size();
		unsigned nrReadFeatures = 0;
		for (unsigned featIdx = 0; featIdx < numTrainingFeatures; ++featIdx) {
			if (std::find(selFeatsOnlySpike.begin(), selFeatsOnlySpike.end(), featIdx) != selFeatsOnlySpike.end()) {
				samp(nrReadFeatures) = m_inputEvents[evIdx].inputVals[featIdx];
				nrReadFeatures++;
			}
			//assert(NUM_SELECTED_FEATURES == samp.size());
		}
		samplesOnlySpike.push_back(samp);
	}
	normalizerOnlySpike.train(samplesOnlySpike);

	for (long evIdx = 0; evIdx < numTestingEvs; ++evIdx) {
		funct_type allSpikesClassifier, spikeHFO_Classifier, onlySpikeClassifier;

		allSpikesClassifier.normalizer = normalizerAllSpikes;
		allSpikesClassifier.function = learned_functionAllSpikes.function;
		allSpikesEpochVal[evIdx] = allSpikesClassifier(samplesAllSpikes[evIdx]) >= allSpikes_Thrheshold ? true : false;

		spikeHFO_Classifier.normalizer = normalizerSpikeHFO;
		spikeHFO_Classifier.function = learned_functionSpikeHFO.function;
		spikeHFO_EpochVal[evIdx] = spikeHFO_Classifier(samplesSpikeHFO[evIdx]) >= spikeHFO_Thrheshold ? true : false;

		onlySpikeClassifier.normalizer = normalizerOnlySpike;
		onlySpikeClassifier.function = learned_functionOnlySpike.function;
		onlySpike_EpochVal[evIdx] = onlySpikeClassifier(samplesOnlySpike[evIdx]) >= onlySpike_Thrheshold ? true : false;
	}

}

bool SVM_Detector::getSupportVectors() {		//use only with HFO detector

	funct_type learned_function;
	deserialize(m_savedFunctionName) >> learned_function;

	int nrSVs = learned_function.function.basis_vectors.size();
	unsigned nrSelectedFeatures = m_selectedFeats.size();

	std::vector<sample_type> supportVectors;
	for (int svMatIdx = 0; svMatIdx < nrSVs; ++svMatIdx) {
		sample_type vector;
		vector.set_size(nrSelectedFeatures);
		vector = learned_function.function.basis_vectors(svMatIdx);
		supportVectors.push_back(vector);
	}

	std::string directoryName = m_outputPath + "/SupportVectors";
	mkdir(directoryName.c_str());
	std::string filename = directoryName + "/" + m_eoiName + "_SupportVectors.xls";
	remove(filename.c_str());

	std::ofstream vectorsFile;
	vectorsFile.open(filename);

	//Header
	vectorsFile << "SV_Nr\t";
	EOI_Features eoiFeatures;
	for (int selFeatIdx = 0; selFeatIdx < nrSelectedFeatures; selFeatIdx++) {
		vectorsFile << eoiFeatures.featureNames[m_selectedFeats[selFeatIdx]] << "\t";
	}
	vectorsFile << "\n";

	for (int svIdx = 0; svIdx < nrSVs; svIdx++) {
		vectorsFile << svIdx  <<"\t";
		for (int selFeatIdx = 0; selFeatIdx < nrSelectedFeatures; selFeatIdx++) {
			vectorsFile << supportVectors[svIdx](selFeatIdx) << "\t";
		}
		vectorsFile << "\n";
	}

	vectorsFile.close();

	return true;
}


bool SVM_Detector::getEvents(std::vector<EOI_Event> &readEvents) {
	readEvents = m_detectedEOI;
	return true;
}


bool SVM_Detector::writeFeaturesAndOutput(std::string channName) {

	//Write header
	EOI_Features signalFeatures;
	std::string filename = m_patDetectionFilesPath + "/" + m_patientName + "_" + m_eoiName + "_FeaturesAndOutput.txt";
	std::ofstream featuresAndOutput;
	if (!fileExists(filename.c_str())) {
		featuresAndOutput.open(filename);
		//Header
		featuresAndOutput << "Channel" << "\t" << "StartTime" << "\t";
		for (int i = 0; i < signalFeatures.featureNames.size(); i++) {
			featuresAndOutput << signalFeatures.featureNames[i] << "\t";
		}
		featuresAndOutput << "Output" << "\n";
	}
	else {
		featuresAndOutput.open(filename, std::ios::app);    // open file for appending
	}

	//Write body
	unsigned nrEpochs = m_chSpecifificDetecEpochs[0].size();
	for (unsigned evIdx = 1; evIdx < nrEpochs; ++evIdx) {
		double startTime = m_inputEvents.at(evIdx).time - m_epochLength;
		bool eoiEpoch = m_chSpecifificDetecEpochs[0][evIdx];

		featuresAndOutput << channName << "\t" << startTime << "\t";
		unsigned numFeatures = signalFeatures.features.size();
		for (int i = 0; i < numFeatures; i++) {
			double val = m_inputEvents[evIdx].inputVals[i];
			if (isnan(val) || isinf(val)) {
				val = 0;
			}
			featuresAndOutput << val << "\t";
		}
		featuresAndOutput << eoiEpoch << "\n";
	}
	featuresAndOutput.close();

	return 1;
}


std::string SVM_Detector::to_string_with_precision(float val, int precVal){
	std::ostringstream out;
	out.precision(precVal);
	out << std::fixed << val;
	return out.str();
}