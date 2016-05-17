#ifndef MOVINGAVGFILTER_H
#define MOVINGAVGFILTER_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "cpx.h"

/*
	Simple moving average for doubles (templatize if needed for other types)
	See http://www.analog.com/media/en/technical-documentation/dsp-book/dsp_book_Ch15.pdf
	Could be extended to support weighted moving average if needed
	Moving average is a simple low pass FIR filter that is the fastest possible FIR filter algorithm
	Useful for on/off keying like CW or RTTY where there is a sharp transition on the rising signal
	The size of the filter is based on the estimated rise time of the signal, ie 10ms for CW
	m_filterLen = sampleRate * msRiseTime -> 8000sps * .010 = 80
*/
class MovingAvgFilter
{
public:
	MovingAvgFilter(quint32 filterLen);
	~MovingAvgFilter();

	//Puts new sample into delay buffer and returns new moving average
	double newSample(double sample);
	//Clears moving average and starts over
	void reset();

private:
	bool m_primed; //Flag for special handling of first samples
	quint32 m_filterLen; //Number of samples to average
	double *m_delayBuf; //Ring buffer
	quint32 m_ringIndex; //Points to oldest sample in buffer
	double m_delayBufSum; //Sum of all the entries in the delay buffer
	double m_movingAvg; //Last average returned
};

#endif // MOVINGAVGFILTER_H
