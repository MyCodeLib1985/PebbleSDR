#ifndef BANDPASSFILTER_H
#define BANDPASSFILTER_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "global.h"
#include "processstep.h"
#include "firfilter.h"
#include "fastfir.h"


class BandPassFilter : public ProcessStep
{
public:
	BandPassFilter(quint32 _sampleRate, quint32 _bufferSize);
	~BandPassFilter(void);

	void setBandPass(float _low, float _high);
	CPX *process(CPX *in, quint32 _numSamples);
	float lowFreq(){return m_lowFreq;}
	float highFreq(){return m_highFreq;}

private:
	FIRFilter *m_bpFilter1; //Old implementation
	CFastFIR *m_bpFilter2;
	bool m_useFastFIR;
	float m_lowFreq;
	float m_highFreq;
};

#endif // BANDPASSFILTER_H
