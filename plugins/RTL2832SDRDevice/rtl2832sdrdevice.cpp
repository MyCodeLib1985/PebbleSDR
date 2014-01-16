//GPL license and attributions are in gpl.h and terms are included in this file by reference
#include "gpl.h"
#include "rtl2832sdrdevice.h"
#include "arpa/inet.h" //For ntohl()
/*
  rtl_tcp info
*/

RTL2832SDRDevice::RTL2832SDRDevice()
{
    InitSettings("rtl2832sdr");
    inBuffer = NULL;
    connected = false;
    running = false;
    optionUi = NULL;
    rtlTcpSocket = NULL;
}

RTL2832SDRDevice::~RTL2832SDRDevice()
{
    Stop();
    Disconnect();
    if (inBuffer != NULL)
        delete (inBuffer);
}

QString RTL2832SDRDevice::GetPluginName(int _devNum)
{
    switch (_devNum) {
        case RTL_USB:
            return "RTL2832 USB";
            break;
        case RTL_TCP:
            return "RTL2832 TCP";
            break;
        default:
            qDebug()<<"Error: Unknown RTL device";
            return "Error";
            break;

    }
}

QString RTL2832SDRDevice::GetPluginDescription(int _devNum)
{
    switch (_devNum) {
        case RTL_USB:
            return "RTL2832 USB Devices";
            break;
        case RTL_TCP:
            return "RTL2832 TCP Servers";
            break;
        default:
            qDebug()<<"Error: Unknown RTL device";
            return "Error";
            break;

    }
}

bool RTL2832SDRDevice::Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer)
{
    ProcessIQData = _callback;
    framesPerBuffer = _framesPerBuffer;
    inBuffer = new CPXBuf(framesPerBuffer);

    //crystalFreqHz = DEFAULT_CRYSTAL_FREQUENCY;
    //RTL2832 samples from 900001 to 3200000 sps (900ksps to 3.2msps)
    //1 msps seems to be optimal according to boards
    //We have to decimate from rtl rate to our rate for everything to work

    //Make rtlSample rate close to 1msps but with even decimate
    //Test with higher sps, seems to work better
    /*
      RTL2832 sample rate likes to be 2.048, 1.024 or an even quotient of 28.8mhz
      Other rates may cause intermittent sync problems
      2048000 is the default rate for DAB and FM
      3.20 (28.8 / 9)
      2.88 (28.8 / 10)
      2.40 (28.8 / 12)
      2.048 (works)
      1.92 (28.8 / 15)
      1.80 (28.8 / 16)
      1.60 (28.8 / 18)
      1.44 (28.8 / 20) even dec at 48 and 96k
      1.20 (28.8 / 24)
      1.152 (28.8 / 25) 192k * 6 - This is our best rate convert to 192k effective rate
      1.024 (works)
       .96 (28.8 / 30)
    */
    /*
    range += osmosdr::range_t( 250000 ); // known to work
      range += osmosdr::range_t( 1000000 ); // known to work
      range += osmosdr::range_t( 1024000 ); // known to work
      range += osmosdr::range_t( 1800000 ); // known to work
      range += osmosdr::range_t( 1920000 ); // known to work
      range += osmosdr::range_t( 2000000 ); // known to work
      range += osmosdr::range_t( 2048000 ); // known to work
      range += osmosdr::range_t( 2400000 ); // known to work
    //  range += osmosdr::range_t( 2600000 ); // may work
    //  range += osmosdr::range_t( 2800000 ); // may work
    //  range += osmosdr::range_t( 3000000 ); // may work
    //  range += osmosdr::range_t( 3200000 ); // max rate
    */
    //Different sampleRates for different RTL rates
    //rtlSampleRate = 2.048e6; //We can keep up with Spectrum
    //rtlSampleRate = 1.024e6;

    //sampleRate must <= to rtlSampleRate, find closest /2 match
    sampleRate = (sampleRate <= rtlSampleRate) ? sampleRate : sampleRate/2;

    rtlDecimate = rtlSampleRate / sampleRate; //Must be even number, convert to lookup table
    /*
    //Find whole number decimate rate less than 2048000
    rtlDecimate = 1;
    rtlSampleRate = 0;
    quint32 tempRtlSampleRate = 0;
    while (tempRtlSampleRate <= 1800000) {
        rtlSampleRate = tempRtlSampleRate;
        tempRtlSampleRate = sampleRate * ++rtlDecimate;
    }
*/
    rtlFrequency = 162400000;

    sampleGain = .005; //Matched with rtlGain

    haveDongleInfo = false; //Look for first bytes
    tcpDongleInfo.magic[0] = 0x0;
    tcpDongleInfo.magic[1] = 0x0;
    tcpDongleInfo.magic[2] = 0x0;
    tcpDongleInfo.magic[3] = 0x0;

    tcpDongleInfo.tunerType = RTLSDR_TUNER_UNKNOWN;
    tcpDongleInfo.tunerGainCount = 0;

    running = false;
    connected = false;

    //1 byte per I + 1 byte per Q
    //This is set so we always get framesPerBuffer samples after decimating to lower sampleRate
    readBufferSize = framesPerBuffer * rtlDecimate * 2;
    numProducerBuffers = 50;
    producerConsumer.Initialize(this, numProducerBuffers, readBufferSize);
    readBufferIndex = 0;

    if (deviceNumber == RTL_TCP) {
        //Start this immediately, before connect, so we don't miss any data
        producerConsumer.Start(false,true); //We don't need producer
        if (rtlTcpSocket == NULL) {
            rtlTcpSocket = new QTcpSocket();

            //QTcpSocket can have strange behavior if shared between threads
            //We use the signals emitted by QTcpSocket to get new data, instead of polling in a separate thread
            //As a result, there is no producer thread in the TCP case.
            //Set up state change connections
            connect(rtlTcpSocket,&QTcpSocket::connected, this, &RTL2832SDRDevice::TCPSocketConnected);
            connect(rtlTcpSocket,&QTcpSocket::disconnected, this, &RTL2832SDRDevice::TCPSocketDisconnected);
            connect(rtlTcpSocket,SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(TCPSocketError(QAbstractSocket::SocketError)));
            connect(rtlTcpSocket,&QTcpSocket::readyRead, this, &RTL2832SDRDevice::TCPSocketNewData);
            //test
            connect(this,&RTL2832SDRDevice::reset,this,&RTL2832SDRDevice::Reset);
        }
    } else if (deviceNumber == RTL_USB) {
        //Start this immediately, before connect, so we don't miss any data
        producerConsumer.Start(true,true); //We don't need producer

    }

    return true;
}

//Restart everything with current values, same as power off then power on
//Should be part of DeviceInterface?
void RTL2832SDRDevice::Reset()
{
    Stop();
    Disconnect();
    Initialize(ProcessIQData,framesPerBuffer);
    Connect();
    Start();
}

bool RTL2832SDRDevice::Connect()
{
    if (deviceNumber == RTL_USB) {
        int device_count;
        int dev_index = 0; //Assume we only have 1 RTL2832 device for now
        //dev = NULL;
        device_count = rtlsdr_get_device_count();
        if (device_count == 0) {
            qDebug("No supported devices found.");
            return false;
        }

        //for (int i = 0; i < device_count; i++)
        //    qDebug("%s",rtlsdr_get_device_name(i));

        //rtlsdr_get_device_name(dev_index));

        if (rtlsdr_open(&dev, dev_index) < 0) {
            qDebug("Failed to open rtlsdr device #%d", dev_index);
            return false;
        }
        connected = true;
        return true;
    } else if (deviceNumber == RTL_TCP) {

        //The socket buffer is infinite by default, it will keep growing if we're not able to fetch data fast enough
        //So we set it to a fixed size, which is the maximum amount of data that can be fetched in one read call
        //We can try differnt buffer sizes here to keep up with higher sample rates
        //If too large, we may have problems processing all the data before the next batch comes in.
        //Setting to 1 framesPerBuffer seems to work best
        //If problems, try setting to some multiple that less than numProducerBuffers
        rtlTcpSocket->setReadBufferSize(readBufferSize);

        //rtl_tcp server starts to dump data as soon as there is a connection, there is no start/stop command
        rtlTcpSocket->connectToHost(rtlServerIP,rtlServerPort,QTcpSocket::ReadWrite);
        if (!rtlTcpSocket->waitForConnected(3000)) {
            //Socket was closed or error
            qDebug()<<"RTL Server error "<<rtlTcpSocket->errorString();
            return false;
        }
        qDebug()<<"RTL Server connected";

        connected = true;
        return true;
    }
    return false;
}

void RTL2832SDRDevice::TCPSocketError(QAbstractSocket::SocketError socketError)
{
    qDebug()<<"TCP Socket Error "<<socketError;
}

void RTL2832SDRDevice::TCPSocketConnected()
{
    //Nothing to do, we waiting for connection in Connect()
    qDebug()<<"TCP connected signal received";
}

void RTL2832SDRDevice::TCPSocketDisconnected()
{
    //Get get this signal in 2 cases
    // -When we turn off device
    // -When device is disconnected due to network or other failure
    //If device is running, try to reset
    if (running) {
        emit Reset();
    }
    qDebug()<<"TCP disconnected signal received";
}

void RTL2832SDRDevice::TCPSocketNewData()
{
    //Handle case where we may be shutting down and ProducerConsumer is not running
    if (!running || !producerConsumer.IsRunning())
        return;

    rtlTcpSocketMutex.lock();

    qint64 bytesRead;
    //We have to process all the data we have because we limit buffer size
    //We only get one readRead() signal for each block of data that's ready

    qint64 bytesAvailable = rtlTcpSocket->bytesAvailable();
    if (bytesAvailable == 0) {
        rtlTcpSocketMutex.unlock();
        return;
    }

    //qDebug()<<"Bytes available "<<bytesAvailable;

    //The first bytes we get from rtl_tcp are dongle information
    //This comes immediately after connect, before we start running
    if (!haveDongleInfo && bytesAvailable >= (qint64)sizeof(DongleInfo)) {
        rtlTunerType = RTLSDR_TUNER_UNKNOWN; //Default if we don't get valid data
        bytesRead = rtlTcpSocket->peek((char *)&tcpDongleInfo,sizeof(DongleInfo));
        haveDongleInfo = true; //We only try once for this after we read some bytes

        if (bytesRead == sizeof(DongleInfo)) {
            //If we got valid data, then use internally
            if (tcpDongleInfo.magic[0] == 'R' &&
                    tcpDongleInfo.magic[1] == 'T' &&
                    tcpDongleInfo.magic[2] == 'L' &&
                    tcpDongleInfo.magic[3] == '0') {
                //We have valid data
                rtlTunerType = (RTLSDR_TUNERS)ntohl(tcpDongleInfo.tunerType);
                rtlTunerGainCount = ntohl(tcpDongleInfo.tunerGainCount);
                qDebug()<<"Dongle type "<<rtlTunerType;
                qDebug()<<"Dongle gain count "<< rtlTunerGainCount;
            }
        }

    }

    char *bufPtr;
    quint16 bytesToFillBuffer;
    while (bytesAvailable) {
        if (readBufferIndex == 0) {
            //Starting a new buffer
            //We wait for a free buffer for 100ms before giving up.  May need to adjust this
            if (!producerConsumer.AcquireFreeBuffer(100)) {
                rtlTcpSocketMutex.unlock();
                //We're sol in this situation because we won't get another readReady() signal
                emit reset(); //Start over
                return;
            }
            //qDebug()<<"Acquired free buffer";
        }
        bufPtr = producerConsumer.GetProducerBufferAsChar();
        bytesToFillBuffer = readBufferSize - readBufferIndex;
        bytesToFillBuffer = (bytesToFillBuffer <= bytesAvailable) ? bytesToFillBuffer : bytesAvailable;
        bytesRead = rtlTcpSocket->read(bufPtr + readBufferIndex, bytesToFillBuffer);
        bytesAvailable -= bytesRead;
        readBufferIndex += bytesRead;
        readBufferIndex %= readBufferSize;
        if (readBufferIndex == 0) {
            producerConsumer.NextProducerBuffer();
            producerConsumer.ReleaseFilledBuffer();
            //qDebug()<<"Released filled buffer";
        }

    }
    //qDebug()<<"Read index "<<readBufferIndex;
    rtlTcpSocketMutex.unlock();

}

bool RTL2832SDRDevice::Disconnect()
{
    if (!connected)
        return false;

    if (deviceNumber == RTL_USB) {
        rtlsdr_close(dev);
    } else if (deviceNumber == RTL_TCP) {
        rtlTcpSocket->disconnectFromHost();
        //Wait for disconnect
        //if (!rtlTcpSocket->waitForDisconnected(500)) {
        //    qDebug()<<"rtlTcpSocket didn't disconnect within timeout";
        //}
    }
    connected = false;
    dev = NULL;
    return true;
}

void RTL2832SDRDevice::Start()
{
    //Handles both USB and TCP
    SetRtlGain(rtlGainMode, rtlGain);
    SetRtlFrequencyCorrection(rtlFreqencyCorrection);

    if (deviceNumber == RTL_USB) {
        rtlTunerType = RTLSDR_TUNER_UNKNOWN; //Default if we don't get valid data
        rtlTunerType = (RTLSDR_TUNERS) rtlsdr_get_tuner_type(dev);

        /* Set the sample rate */
        if (rtlsdr_set_sample_rate(dev, rtlSampleRate) < 0) {
            qDebug("WARNING: Failed to set sample rate.");
            return;
        }

        //Center freq is set in SetFrequency()

        /* Reset endpoint before we start reading from it (mandatory) */
        if (rtlsdr_reset_buffer(dev) < 0) {
            qDebug("WARNING: Failed to reset buffers.");
            return;
        }        
        running = true; //Must be after reset
    } else if (deviceNumber == RTL_TCP) {
        running = true;
        SendTcpCmd(TCP_SET_SAMPLERATE,rtlSampleRate);
    }

    return;
}

//Stop is called by Reciever first, then Disconnect
//Stop incoming samples
void RTL2832SDRDevice::Stop()
{
    running = false;

    if (deviceNumber == RTL_USB) {

    } else if (deviceNumber == RTL_TCP) {

    }
    producerConsumer.Stop();
}

bool RTL2832SDRDevice::SetRtlFrequencyCorrection(qint16 _correction)
{
    if (!connected)
        return false;

    if (deviceNumber == RTL_USB) {
        int r = rtlsdr_set_freq_correction(dev,_correction);
        if (r<0) {
            qDebug()<<"Frequency correction failed";
            return false;
        }

    } else if (deviceNumber == RTL_TCP) {
        SendTcpCmd(TCP_SET_FREQ_CORRECTION,_correction);
    }
    return false;
}

bool RTL2832SDRDevice::SetRtlGain(quint16 _mode, quint16 _gain)
{
    if (!connected)
        return false;

    if (deviceNumber == RTL_USB) {
        int r = rtlsdr_set_tuner_gain_mode(dev, _mode);
        if (r < 0) {
            qDebug("WARNING: Failed to enable gain mode.");
            return false;
        }
        if (rtlGainMode == GAIN_MODE_MANUAL){
            /* Set the tuner gain */
            r = rtlsdr_set_tuner_gain(dev, _gain);
            if (r < 0) {
                qDebug("WARNING: Failed to set tuner gain.");
                return false;
            }
            //int actual = rtlsdr_get_tuner_gain(dev);
            //qDebug()<<"Actual RTL gain set to "<<actual;
        }
        return true;
    } else if (deviceNumber == RTL_TCP) {
        SendTcpCmd(TCP_SET_GAIN_MODE, _mode);
        if (rtlGainMode == GAIN_MODE_MANUAL) {
            SendTcpCmd(TCP_SET_TUNER_GAIN, _gain);
            //When do we need to set IF Gain?
            //SendTcpCmd(TCP_SET_IF_GAIN, _gain);
        }
    }
    return false;
}

double RTL2832SDRDevice::SetFrequency(double fRequested,double fCurrent)
{
    if (!connected)
        return false;

    if (deviceNumber == RTL_USB) {
        /* Set the frequency */
        if (rtlsdr_set_center_freq(dev, fRequested) < 0) {
            qDebug("WARNING: Failed to set center freq.");
            rtlFrequency = fCurrent;
        } else {
            rtlFrequency = fRequested;
        }
        lastFreq = rtlFrequency;
        return rtlFrequency;

    } else if (deviceNumber == RTL_TCP) {
        lastFreq = fRequested;
        if (SendTcpCmd(TCP_SET_FREQ,fRequested))
            return fRequested;
        else
            return fCurrent;
    }
    return fCurrent;
}

void RTL2832SDRDevice::ShowOptions()
{

}

void RTL2832SDRDevice::ReadSettings()
{
    QSettings * qs = GetQSettings();
    //Valid gain values (in tenths of a dB) for the E4000 tuner:
    //-10, 15, 40, 65, 90, 115, 140, 165, 190,
    //215, 240, 290, 340, 420, 430, 450, 470, 490
    //0 for automatic gain
    rtlGain = qs->value("RtlGain",15).toInt();
    rtlServerIP = QHostAddress(qs->value("IPAddr","127.0.0.1").toString());
    rtlServerPort = qs->value("Port","1234").toInt();
    rtlSampleRate = qs->value("RtlSampleRate",2048000).toUInt();
    rtlGainMode = qs->value("RtlGainMode",GAIN_MODE_AUTO).toUInt();
    rtlFreqencyCorrection = qs->value("RtlFrequencyCorrection",0).toInt();

}

void RTL2832SDRDevice::WriteSettings()
{
    QSettings *qs = GetQSettings();
    qs->setValue("RtlGain",rtlGain);
    qs->setValue("IPAddr",rtlServerIP.toString());
    qs->setValue("Port",rtlServerPort);
    qs->setValue("RtlSampleRate",rtlSampleRate);
    qs->setValue("RtlGainMode",rtlGainMode);
    qs->setValue("RtlFrequencyCorrection",rtlFreqencyCorrection);
    qs->sync();

}

double RTL2832SDRDevice::GetStartupFrequency()
{
    return 162450000;
}

int RTL2832SDRDevice::GetStartupMode()
{
    return dmFMN;
}

/*
Elonics E4000	 52 - 2200 MHz with a gap from 1100 MHz to 1250 MHz (varies)
Rafael Micro R820T	24 - 1766 MHz
Fitipower FC0013	22 - 1100 MHz (FC0013B/C, FC0013G has a separate L-band input, which is unconnected on most sticks)
Fitipower FC0012	22 - 948.6 MHz
FCI FC2580	146 - 308 MHz and 438 - 924 MHz (gap in between)
*/
double RTL2832SDRDevice::GetHighLimit()
{
    switch (rtlTunerType) {
        case RTLSDR_TUNER_E4000:
            return 2200000000;
        case RTLSDR_TUNER_FC0012:
            return 948600000;
        case RTLSDR_TUNER_FC0013:
            return 1100000000;
        case RTLSDR_TUNER_FC2580:
            return 146000000;
        case RTLSDR_TUNER_R820T:
            return 1766000000;
        case RTLSDR_TUNER_R828D:
            return 1766000000;
        case RTLSDR_TUNER_UNKNOWN:
        default:
            return 1700000000;
    }
}

double RTL2832SDRDevice::GetLowLimit()
{
    switch (rtlTunerType) {
        case RTLSDR_TUNER_E4000:
            return 52000000;
        case RTLSDR_TUNER_FC0012:
            return 22000000;
        case RTLSDR_TUNER_FC0013:
            return 22000000;
        case RTLSDR_TUNER_FC2580:
            return 146000000;
        case RTLSDR_TUNER_R820T:
            return 24000000;
        case RTLSDR_TUNER_R828D:
            return 24000000;
        case RTLSDR_TUNER_UNKNOWN:
        default:
            return 64000000;
    }
}

double RTL2832SDRDevice::GetGain()
{
    return 1;
}

QString RTL2832SDRDevice::GetDeviceName()
{
    switch (rtlTunerType) {
        case RTLSDR_TUNER_E4000:
            return "RTL2832-E4000";
        case RTLSDR_TUNER_FC0012:
            return "RTL2832-FC0012";
        case RTLSDR_TUNER_FC0013:
            return "RTL2832-FC0013";
        case RTLSDR_TUNER_FC2580:
            return "RTL2832-FC2580";
        case RTLSDR_TUNER_R820T:
            return "RTL2832-R820T";
        case RTLSDR_TUNER_R828D:
            return "RTL2832-R820D";
        case RTLSDR_TUNER_UNKNOWN:
        default:
            return "RTL2832-Unknown";
    }
}

int RTL2832SDRDevice::GetSampleRate()
{
    return sampleRate;
}

int *RTL2832SDRDevice::GetSampleRates(int &len)
{
    len = 6;
    //Ugly, but couldn't find easy way to init with {1,2,3} array initializer
    //These are all multiples of 2 to support easy decimation
    sampleRates[0] = 64000;
    sampleRates[1] = 128000;
    sampleRates[2] = 256000;
    sampleRates[3] = 512000;
    sampleRates[4] = 1024000;
    sampleRates[5] = 2048000;
    return sampleRates;

}

bool RTL2832SDRDevice::UsesAudioInput()
{
    return false;
}

bool RTL2832SDRDevice::GetTestBenchChecked()
{
    return isTestBenchChecked;
}

void RTL2832SDRDevice::SetupOptionUi(QWidget *parent)
{
    ReadSettings();

    //This sets up the sdr specific widget
    if (optionUi != NULL)
        delete optionUi;
    optionUi = new Ui::RTL2832UI();
    optionUi->setupUi(parent);
    parent->setVisible(true);

    if (deviceNumber == RTL_TCP) {
        optionUi->tcpFrame->setVisible(true);
        //Read options file and display
        optionUi->ipAddress->setText(rtlServerIP.toString());
        //Set mask after we set text, otherwise won't parse correctly
        //optionUi->ipAddress->setInputMask("000.000.000.000;0");
        optionUi->port->setText(QString::number(rtlServerPort));
        //optionUi->port->setInputMask("00000;_");

        //connect(o->pushButton,SIGNAL(clicked(bool)),this,SLOT(Test2(bool)));
        //Debugging tip using qApp as known slot, this works
        //connect(o->pushButton,SIGNAL(clicked(bool)),qApp,SLOT(aboutQt()));
        //New QT5 syntax using pointer to member functions
        //connect(o->pushButton,&QPushButton::clicked, this, &SDR_IP::Test2);
        //Lambda function example: needs compiler flag -std=c++0x
        //connect(o->pushButton,&QPushButton::clicked,[=](bool b){qDebug()<<"test";});
        connect(optionUi->ipAddress,&QLineEdit::editingFinished,this,&RTL2832SDRDevice::IPAddressChanged);
        connect(optionUi->port,&QLineEdit::editingFinished,this,&RTL2832SDRDevice::IPPortChanged);
    } else {
        optionUi->tcpFrame->setVisible(false);
    }
    //Common UI
    //Even though we can support additional rates, need to keep in sync with Pebble sampleRate options
    //So we keep decimate by 2 logic intact
    //optionUi->sampleRateSelector->addItem("250 ksps",(quint32)250000);
    optionUi->sampleRateSelector->addItem("1024 msps",(quint32)1024000);
    //optionUi->sampleRateSelector->addItem("1920 msps",(quint32)1920000);
    optionUi->sampleRateSelector->addItem("2048 msps",(quint32)2048000);
    //optionUi->sampleRateSelector->addItem("2400 msps",(quint32)2400000);
    int cur = optionUi->sampleRateSelector->findData(rtlSampleRate);
    optionUi->sampleRateSelector->setCurrentIndex(cur);
    connect(optionUi->sampleRateSelector,SIGNAL(currentIndexChanged(int)),this,SLOT(SampleRateChanged(int)));

    if (rtlGainMode == GAIN_MODE_AUTO)
        optionUi->autoGainButton->setChecked(true);
    else
        optionUi->manualGainButton->setChecked(true);

    //RTL IF Gain valid values for E4000, may not be valid for other devices, but we don't know acutal device in TCP mode
    QStringList ifGainItems = { "-10", "15", "40", "65", "90", "115", "140", "165", "190", "215", "240", "290", "340", "420", "430", "450", "470", "490"};
    optionUi->manualGainSelector->addItems(ifGainItems);
    cur = optionUi->manualGainSelector->findText(QString::number(rtlGain));
    optionUi->manualGainSelector->setCurrentIndex(cur);
    connect(optionUi->manualGainSelector,SIGNAL(currentIndexChanged(int)),this,SLOT(GainChanged(int)));

    connect(optionUi->autoGainButton,&QRadioButton::toggled,this,&RTL2832SDRDevice::GainModeChanged);
    connect(optionUi->autoGainButton,&QRadioButton::toggled,this,&RTL2832SDRDevice::GainModeChanged);

    optionUi->frequencyCorrectionEdit->setValue(rtlFreqencyCorrection);
    connect(optionUi->frequencyCorrectionEdit,SIGNAL(valueChanged(int)),this,SLOT(FreqCorrectionChanged(int)));

}

void RTL2832SDRDevice::IPAddressChanged()
{
    //qDebug()<<optionUi->ipAddress->text();
    rtlServerIP = QHostAddress(optionUi->ipAddress->text());
    WriteSettings();
}

void RTL2832SDRDevice::IPPortChanged()
{
    rtlServerPort = optionUi->port->text().toInt();
    WriteSettings();
}

void RTL2832SDRDevice::SampleRateChanged(int _index)
{
    int cur = optionUi->sampleRateSelector->currentIndex();
    rtlSampleRate = optionUi->sampleRateSelector->itemData(cur).toUInt();
    WriteSettings();
}

void RTL2832SDRDevice::GainModeChanged(bool _clicked)
{
    if (optionUi->autoGainButton->isChecked()) {
        rtlGainMode = GAIN_MODE_AUTO;
        optionUi->manualGainSelector->setEnabled(false);

    } else if (optionUi->manualGainButton->isChecked()) {
        rtlGainMode = GAIN_MODE_MANUAL;
        optionUi->manualGainSelector->setEnabled(true);

    }
    //Make real time changes
    SetRtlGain(rtlGainMode,rtlGain);
    WriteSettings();
}

void RTL2832SDRDevice::GainChanged(int _selection)
{
    rtlGain = optionUi->manualGainSelector->currentText().toUShort();
    //Make real time changes
    SetRtlGain(rtlGainMode,rtlGain);
    WriteSettings();
}

void RTL2832SDRDevice::FreqCorrectionChanged(int _correction)
{
    rtlFreqencyCorrection = _correction;
    SetRtlFrequencyCorrection(rtlFreqencyCorrection);
    WriteSettings();
}

//!!!Not currently called, but should be called when option dialog closes
void RTL2832SDRDevice::WriteOptionUi()
{
    rtlServerIP = QHostAddress(optionUi->ipAddress->text());
    rtlServerPort = optionUi->port->text().toInt();
    WriteSettings();
}

bool RTL2832SDRDevice::SendTcpCmd(quint8 _cmd, quint32 _data)
{
    rtlTcpSocketMutex.lock();
    RTL_CMD cmd;
    cmd.cmd = _cmd;
    //ntohl() is what rtl-tcp uses to convert to network byte order
    //So we use inverse htonl (hardware to network long)
    cmd.data = htonl(_data);
    quint16 bytesWritten = rtlTcpSocket->write((const char *)&cmd, sizeof(cmd));
    //QTcpSocket is always buffered, no choice.
    //So we need to either call an explicit flush() or waitForBytesWritten() to make sure data gets sent
    //Not sure which one is better or if it makes a difference
    rtlTcpSocket->flush();

    //Can't use waitForBytesWritten because it runs event loop while waiting which can trigger signals
    //The readyRead() signal handler also locks the mutex, so we get in a deadlock state
    //rtlTcpSocket->waitForBytesWritten(1000);

    bool res = (bytesWritten == sizeof(cmd));
    rtlTcpSocketMutex.unlock();
    return res;
}


void RTL2832SDRDevice::StopProducerThread(){}
void RTL2832SDRDevice::RunProducerThread()
{    
    if (!connected)
        return;

    int bytesRead;

    //TCP doesn't use producerThread
    if (deviceNumber == RTL_USB) {
        if (!running)
            return;

        if (!producerConsumer.AcquireFreeBuffer())
            return;

        //Insert code to put data from device in buffer
        if (rtlsdr_read_sync(dev, producerConsumer.GetProducerBuffer(), readBufferSize, &bytesRead) < 0) {
            qDebug("Sync transfer error");
            producerConsumer.ReleaseFreeBuffer(); //Put back buffer for next try
            return;
        }

        if (bytesRead < readBufferSize) {
            qDebug("Under read");
            producerConsumer.ReleaseFreeBuffer(); //Put back buffer for next try
            return;
        }
        producerConsumer.NextProducerBuffer();
        producerConsumer.ReleaseFilledBuffer();
    }
}

void RTL2832SDRDevice::StopConsumerThread()
{
    //Problem - RunConsumerThread may be in process when we're asked to stopp
    //We have to wait for it to complete, then return.  Bad dependency - should not have tight connection like this
}

void RTL2832SDRDevice::RunConsumerThread()
{
    if (!connected)
        return;

    if (!running)
        return;

    //Wait for data to be available from producer
    if (!producerConsumer.AcquireFilledBuffer())
        return;

    double fpSampleRe;
    double fpSampleIm;


    //RTL I/Q samples are 8bit unsigned 0-256
    int bufInc = rtlDecimate * 2;
    int decMult = 1; //1=8bit, 2=9bit, 3=10bit for rtlDecimate = 6 and sampleRate = 192k
    int decNorm = 128 * decMult;
    double decAvg = rtlDecimate / decMult; //Double the range = 1 bit of sample size
    for (int i=0,j=0; i<framesPerBuffer; i++, j+=bufInc) {
#if 1
        //We are significantly oversampling, so we can use decimation to increase dynamic range
        //http://www.atmel.com/Images/doc8003.pdf Each bit of resolution requires 4 bits of oversampling
        //With atmel method, we add the oversampled data, and right shift by N
        //THis is NOT the same as just averaging the samples, which is a LP filter function
        //                      OverSampled     Right Shift (sames as /2)
        // 8 bit resolution     N/A             N/A
        // 9 bit resolution     4x              1   /2
        //10 bit resolution     16x             2   /4
        //
        //Then scale by 2^n where n is # extra bits of resolution to get back to original signal level


        //See http://www.actel.com/documents/Improve_ADC_WP.pdf as one example
        //We take N (rtlDecimate) samples and create one result
        fpSampleRe = fpSampleIm = 0.0;
        for (int k = 0; k < bufInc; k+=2) {
            //I/Q reversed from normal devices, correct here
            fpSampleIm += producerConsumer.GetConsumerBufferDataAsDouble(j + k);
            fpSampleRe += producerConsumer.GetConsumerBufferDataAsDouble(j + k + 1);
        }
        //If we average, we get a better sample
        //But if we average with a smaller number, we increase range of samples
        //Instead of 8bit 0-255, we get 9bit 0-511
        //Testing assuming rtlDecmiate = 6
        fpSampleRe = fpSampleRe / decAvg; //Effectively increasing dynamic range
        fpSampleRe -= decNorm;
        fpSampleRe /= decNorm;
        fpSampleRe *= sampleGain;
        fpSampleIm = fpSampleIm / decAvg;
        fpSampleIm -= decNorm;
        fpSampleIm /= decNorm;
        fpSampleIm *= sampleGain;
#else
        //Oringal simple decimation - no increase in dynamic range
        //Every nth sample from producer buffer
        fpSampleRe = (double)producerBuffer[nextConsumerDataBuf][j];
        fpSampleRe -= 127.0;
        fpSampleRe /= 127.0;
        fpSampleRe *= sampleGain;

        fpSampleIm = (double)producerBuffer[nextConsumerDataBuf][j+1];
        fpSampleIm -= 127.0;
        fpSampleIm /= 127.0;
        fpSampleIm *= sampleGain;
#endif
        inBuffer->Re(i) = fpSampleRe;
        inBuffer->Im(i) = fpSampleIm;
    }

    //We're done with databuf, so we can release before we call ProcessBlock
    //Update lastDataBuf & release dataBuf
    producerConsumer.NextConsumerBuffer();
    producerConsumer.ReleaseFreeBuffer();

    ProcessIQData(inBuffer->Ptr(),framesPerBuffer);

}

QSettings *RTL2832SDRDevice::GetQSettings()
{
    //REturn settings file for each deviceNumber
    if (deviceNumber == RTL_USB)
        return usbSettings;
    else if (deviceNumber == RTL_TCP)
        return tcpSettings;
    else
        return NULL;
}

//These need to be moved to a DeviceInterface implementation class, equivalent to SDR
//Must be called from derived class constructor to work correctly
void RTL2832SDRDevice::InitSettings(QString fname)
{
    //Use ini files to avoid any registry problems or install/uninstall
    //Scope::UserScope puts file C:\Users\...\AppData\Roaming\N1DDY
    //Scope::SystemScope puts file c:\ProgramData\n1ddy

    QString path = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
        //Pebble.app/contents/macos = 25
        path.chop(25);
#endif
    qSettings = NULL; //Not useing, trap access
    //qSettings = new QSettings(path + "/PebbleData/" + fname +".ini",QSettings::IniFormat);
    usbSettings = new QSettings(path + "/PebbleData/" + fname +"_usb.ini",QSettings::IniFormat);
    tcpSettings = new QSettings(path + "/PebbleData/" + fname +"_tcp.ini",QSettings::IniFormat);

}