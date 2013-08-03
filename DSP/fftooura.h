#ifndef FFTOOURA_H
#define FFTOOURA_H

#include "fft.h"

class FFTOoura : public FFT
{
public:
    FFTOoura();
    ~FFTOoura();

    void FFTParams( qint32 size, bool invert, double dBCompensation, double sampleRate);
    void FFTForward(CPX * in, CPX * out, int size);
    void FFTMagnForward(CPX * in,int size,double baseline,double correction,double *fbr);
    void FFTInverse(CPX * in, CPX * out, int size);

protected:
    //Complex DFT isgn indicates forward or inverse
    void cdft(int n, int isgn, double *a, int *ip, double *w);

    //Other versions not used in Pebble
    void rdft(int n, int isgn, double *a, int *ip, double *w); //Real DFT
    void ddct(int n, int isgn, double *a, int *ip, double *w); //Discrete Cosine Transform
    void ddst(int n, int isgn, double *a, int *ip, double *w); //Discrete Sine Transform
    void dfct(int n, double *a, double *t, int *ip, double *w);  //Real Symmetric DFT
    void dfst(int n, double *a, double *t, int *ip, double *w); //Real Anti-symmetric DFT

private:
    double *offtSinCosTable;
    int *offtWorkArea;
};

#endif // FFTOOURA_H
