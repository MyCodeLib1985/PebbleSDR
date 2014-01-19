#ifndef PRODUCERCONSUMER_H
#define PRODUCERCONSUMER_H
//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "perform.h"
#include <QtCore>
#include <QThread>
#include "../pebblelib/device_interfaces.h"

class SDRProducerWorker;
class SDRConsumerWorker;

class ProducerConsumer : public QObject
{
    Q_OBJECT
public:
    ProducerConsumer();

    void Initialize(DeviceInterface *_device, int _numDataBufs, int _producerBufferSize);

    bool Start(bool _producer = true, bool _consumer = true);
    bool Stop();

    bool IsRunning();

    quint16 GetBufferSize() {return producerBufferSize;}

    bool IsFreeBufferAvailable();
    unsigned char *AcquireFreeBuffer(quint16 _timeout = 0);
    void ReleaseFreeBuffer();
    void PutbackFreeBuffer(); //Release a buffer without incrementing semaphors, used in error conditions

    unsigned char *AcquireFilledBuffer(quint16 _timeout = 0);
    void ReleaseFilledBuffer();
    void PutbackFilledBuffer();

    //Informational
    bool IsFreeBufferOverflow() {return freeBufferOverflow;}
    bool IsFilledBufferOverflow() {return filledBufferOverflow;}
    quint16 GetNumFreeBufs() {return semNumFreeBuffers->available();}
    quint16 GetNumFilledBufs() {return semNumFilledBuffers->available();}

private:
    DeviceInterface *device;
    //Producer/Consumer buffer management
    int numDataBufs; //Producer/Consumer buffers
    unsigned char **producerBuffer; //Array of buffers
    int producerBufferSize;
    int nextProducerDataBuf;
    int nextConsumerDataBuf;
    bool freeBufferOverflow;
    bool filledBufferOverflow;
    /*
      NumFreeBuffers starts at NUMDATABUFS and is decremented (acquire()) everytime the producer thread has new data.
      If it ever gets to zero, it will block forever and program will hang until consumer thread catches up.
      NumFreeBuffers is incremented (release()) in consumer thread when a buffer has been processed and can be reused.
    */
    QSemaphore *semNumFreeBuffers; //Init to NUMDATABUFS
    QSemaphore *semNumFilledBuffers;

    bool isThreadRunning;
    QThread *producerWorkerThread;
    QThread *consumerWorkerThread;
    SDRProducerWorker *producerWorker;
    SDRConsumerWorker *consumerWorker;

    bool producerThreadIsRunning;
    bool consumerThreadIsRunning;

};

//Alternative thread model using Worker objects and no QThread subclass
class SDRProducerWorker: public QObject
{
    Q_OBJECT
public:
    SDRProducerWorker(DeviceInterface * s);
    public slots:
    void Start();
    void Stopped();
    bool IsRunning() {return isRunning;}
signals:
    void finished();
private:
    DeviceInterface *sdr;
    bool isRunning;
    Perform perform;
};
class SDRConsumerWorker: public QObject
{
    Q_OBJECT
public:
    SDRConsumerWorker(DeviceInterface * s);
public slots:
    void Start();
    void Stopped();
    bool IsRunning() {return isRunning;}
signals:
    void finished();
  private:
    DeviceInterface *sdr;
    bool isRunning;
    Perform perform;
};

//Replacement for windows Sleep() function
class Sleeper : public QThread
{
public:
    static void usleep(unsigned long usecs);
    static void msleep(unsigned long msecs);
    static void sleep(unsigned long secs);
};


#endif // PRODUCERCONSUMER_H
