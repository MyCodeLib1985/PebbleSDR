#pragma once
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"

#include <QMainWindow>
#include <QtCore/QVariant>
#include <QMutex>
#include "Devices/softrock.h"
#include "audio.h"
#include "soundcard.h"
#include "cpx.h"
#include "demod.h"
#include "mixer.h"
#include "smeterwidget.h"
#include "receiverwidget.h"
#include "settings.h"
#include "sdr.h"
#include "presets.h"
#include "signalprocessing.h"
#include "noiseblanker.h"
#include "noisefilter.h"
#include "signalstrength.h"
#include "signalspectrum.h"
#include "firfilter.h"
#include "agc.h"
#include "iqbalance.h"
#include "Decoders/morse/morse.h"
#include "filters/fir.h"
#include "filters/fractresampler.h"
#include "devices/wavfile.h"

#include "Devices/sdr_iq.h"

class Receiver:public QObject
{
	Q_OBJECT

public:
	Receiver(ReceiverWidget *rw, QMainWindow *main);
	~Receiver(void);
	bool On();
	bool Off();
	void Close(); //Shutdown, just before destructor
	bool Power(bool on);
	//Called with a requested frequency and the last frequency
	//Calculates next higher or lower (actual), sets it and return to widget for display
	double SetFrequency(double fRequested, double fCurrent);
	void SetMode(DEMODMODE m);
	void SetGain(int g);
	void SetAgcGainTop(int g);
	void SetSquelch(int s);
	void SetMixer(int f);
	void SetFilter(int lo, int hi);
	void SetAnfEnabled(bool b);
	void SetNbEnabled(bool b);
	void SetNb2Enabled(bool b);
	void SetAgcMode(AGC::AGCMODE m);
	void SetLpfEnabled(bool b);
	void SetMute(bool b);
	void ShowPresets();
    Presets *GetPresets() {return presets;}
    bool GetPowerOn() {return powerOn;}

    void SetDataSelection(ReceiverWidget::DATA_SELECTION d);
	Settings * GetSettings() {return settings;}
	void ProcessBlock(CPX *in, CPX *out, int frameCount);
	void ProcessBlockTimeDomain(CPX *in, CPX *out, int frameCount);
	void ProcessBlockFreqDomain(CPX *in, CPX *out, int frameCount);
	SignalStrength *GetSignalStrength() {return signalStrength;}
	SignalSpectrum *GetSignalSpectrum() {return signalSpectrum;}
	IQBalance *GetIQBalance(){return iqBalance;}

    ReceiverWidget *receiverWidget;

    Morse *getMorse() {return morse;}

    public slots:
		void Restart();
        void RecToggled(bool on);
        void SdrOptionsPressed();
        void CloseSdrOptions();

private:
    FFTfftw *fft;
	int fftSize;
	bool useFreqDomainChain;

	bool mute;
	QMutex mutex;
	bool powerOn;
	Settings *settings;
	Presets *presets;
	//SoftRock *softRock;
	QMainWindow *mainWindow;
	SDR *sdr;
	Audio *audioOutput;
	Demod *demod;
	Mixer *mixer;
	NoiseBlanker *noiseBlanker;
	NoiseFilter *noiseFilter;
	SignalStrength *signalStrength;
	SignalSpectrum *signalSpectrum;
	AGC *agc;
	IQBalance *iqBalance;
    Morse *morse;
    ReceiverWidget::DATA_SELECTION dataSelection;
    bool isRecording;
    QString recordingFileName;
    QString recordingPath;
    WavFile recordingFile;

	double frequency; //Current LO frequency (not mixed)
    double mixerFrequency;
	int sampleRate;
	int framesPerBuffer; //#samples in each callback
	//sample rate and buffer size after down sampling step
    int downSample1Frames;
    int downSample2Frames;
    FIRFilter *downSampleFilter;

    //Trying cuteSdr downsample code
    CFractResampler fractResampler; //To get to final audio rate
    CDownConvert downConvert1; //Get to reasonable rate for demod and following
    CDownConvert downConvertWfm1; //Special to get to 300k
    int audioOutRate;
    int downSample1Rate;
    int downSampleWfm1Rate;
    CPX *workingBuf;



    //Use CuteSDR decimation filters for downsampling
    CDecimateBy2 *decimate1;
    int decimate2SampleRate;
    CDecimateBy2 *decmiate2;
    int decimate3SampleRate;
    CDecimateBy2 *decimate3;
    int decimate4SampleRate;
    CDecimateBy2 *decimate4;

	float gain;
	double sdrGain;
	int squelch;

	FIRFilter *bpFilter; //Current BandPass filter
	FIRFilter *lpFilter;
	FIRFilter *usbFilter;
	FIRFilter *lsbFilter;
	FIRFilter *amFilter;

	SDR_IQThread *sdrThread;

};