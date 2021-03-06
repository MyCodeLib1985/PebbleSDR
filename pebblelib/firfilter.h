#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include <QMutex>
#include "fft.h"
#include "delayline.h"
#include "windowfunction.h"

class FIRFilter: public QObject
{
public:
	enum FILTERTYPE {LOWPASS, HIGHPASS, BANDPASS, BANDSTOP};
	FIRFilter(int sr, int ns, bool useFFT=false, int taps=64, int delay=0);
	~FIRFilter(void);
	//Apply the filter, results in out
	CPX * ProcessBlock(CPX *in);
	void Convolution(FFT *fft);

	//Create coefficients for filter
	void setEnabled(bool b);
	//Mutually exclusive, filter can only be one of the below types
	void SetLowPass(float cutoff, WindowFunction::WINDOWTYPE w = WindowFunction::HAMMING);
	void SetHighPass(float cutoff, WindowFunction::WINDOWTYPE w = WindowFunction::HAMMING);
	void SetBandPass(float lo, float hi, WindowFunction::WINDOWTYPE w = WindowFunction::BLACKMANHARRIS);
	void SetBandPass2(float lo, float hi, WindowFunction::WINDOWTYPE w = WindowFunction::BLACKMAN); //Alternate algorithm
	void SetBandStop(float lo, float hi, WindowFunction::WINDOWTYPE w = WindowFunction::BLACKMANHARRIS); //FFT version only
	//Allows us to load predefined coefficients for FFT FIR
	void SetLoadable(float * coeff);

	//Angular frequency in radians/sec w = 2*Pi*frequency //This lower case w appears in many formulas
	//fHz is normalized for sample rate, ie fHz = f/fs = 10000/48000
	static inline float fAng(float fHz) {return TWOPI * fHz;}
	//Angular frequency in degrees/sec
	static inline float fDeg(float fHz) {return 360 * fHz;}

	CPX Sinc(float Fc, int i);
	CPX Sinc2(float Fc, int i); //Alternate algorithm
	void SpectralInversion();
	void SpectralReversal();



private:
	int numSamples;
	int numSamplesX2;
	int sampleRate;
	//Each process block returns it's output to a module specific buffer
	//So mixer has it's own output buffer, noise blanker has its own, etc.
	//This is not absolutely necessary, but makes it easier to test, debug, and add new blocks
	CPX *out;

	QMutex mutex;
	bool useFFT;
	int numTaps; //Size of filter coefficient array
	int M; //numTaps -1
	int delay;
	float lpCutoff;
	float hpCutoff;
	bool enabled;
	DelayLine * delayLine;
	//Filter coefficients
	CPX *taps; 

	//FFT versions
	FFT *fftFIR;
	FFT *fftSamples;

	void FFTSetBandPass(float lo, float hi,WindowFunction::WINDOWTYPE wtype);
	void FFTSetLowPass(float cutoff, WindowFunction::WINDOWTYPE wtype);
	void FFTSetHighPass(float cutoff, WindowFunction::WINDOWTYPE wtype); //NOT WORKING
	void FFTSetBandStop(float lo, float hi, WindowFunction::WINDOWTYPE wtype); //NOT TESTED

	//Converts taps[] to fftTaps[]
	void MakeFFTTaps();


};
