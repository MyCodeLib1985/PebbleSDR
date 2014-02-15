#include "deviceinterfacebase.h"

DeviceInterfaceBase::DeviceInterfaceBase()
{
	connected = false;
	//These are common settings for every device, variables are defined in DeviceInterface
	startupType = DEFAULTFREQ;
	userFrequency = 10000000;
	inputDeviceName = "NA";
	outputDeviceName = "NA";
	sampleRate = 48000;
	iqGain = 1.0;
	iqOrder = IQ;
	iqBalanceGain = 1.0;
	iqBalancePhase = 0;
	iqBalanceEnable = false;
	lastFreq = 10000000;
	lastDemodMode = dmAM;
	lastSpectrumMode = 0;
	deviceNumber = 0;
	startupFrequency = 10000000;
	lowFrequency = 150000;
	highFrequency = 30000000;
	startupDemodMode = dmAM;

}

DeviceInterfaceBase::~DeviceInterfaceBase()
{

}

bool DeviceInterfaceBase::Initialize(cbProcessIQData _callback, quint16 _framesPerBuffer)
{
	Q_UNUSED(_callback);
	Q_UNUSED(_framesPerBuffer);
	return true;
}

bool DeviceInterfaceBase::Connect()
{
	return true;
}

bool DeviceInterfaceBase::Disconnect()
{
	return true;
}

void DeviceInterfaceBase::Start()
{
	return;
}

void DeviceInterfaceBase::Stop()
{
	return;
}

void DeviceInterfaceBase::SetupOptionUi(QWidget *parent)
{
	Q_UNUSED(parent);
	return;
}

QVariant DeviceInterfaceBase::Get(STANDARD_KEYS _key, quint16 _option) {
	Q_UNUSED(_option);
	switch (_key) {
		case PluginName:
			break;
		case PluginDescription:
			break;
		case PluginNumDevices:
			return 1;
			break;
		case DeviceName:
			break;
		case DeviceDescription:
			break;
		case DeviceNumber:
			return deviceNumber;
			break;
		case DeviceType:
			return AUDIO_IQ;
			break;
		case DeviceSampleRate:
			return sampleRate;
			break;
		case DeviceSampleRates:
			//We shouldn't know this, depends on audio device connected to receiver
			return QStringList()<<"48000"<<"96000"<<"192000";
			break;
		case DeviceFrequency:
			return deviceFrequency;
			break;
		case InputDeviceName:
			return inputDeviceName;
			break;
		case OutputDeviceName:
			return outputDeviceName;
			break;
		case HighFrequency:
			return highFrequency;
		case LowFrequency:
			return lowFrequency;
		case FrequencyCorrection:
			return 0;
			break;
		case IQGain:
			return iqGain;
			break;
		case StartupType:
			return startupType;
			break;
		case StartupDemodMode:
			return startupDemodMode;
		case StartupSpectrumMode:
			break;
		case StartupFrequency:
			return startupFrequency;
		case LastDemodMode:
			return lastDemodMode;
			break;
		case LastSpectrumMode:
			return lastSpectrumMode;
			break;
		case LastFrequency:
			//If freq is outside of mode we are in return default
			if (lastFreq > Get(DeviceInterface::HighFrequency).toDouble() || lastFreq < Get(DeviceInterface::LowFrequency).toDouble())
				return Get(DeviceInterface::StartupFrequency).toDouble();
			else
				return lastFreq;
			break;
		case UserDemodMode:
			break;
		case UserSpectrumMode:
			break;
		case UserFrequency:
			break;
		case IQOrder:
			return iqOrder;
			break;
		case IQBalanceEnabled:
			return iqBalanceEnable;
			break;
		case IQBalanceGain:
			return iqBalanceGain;
			break;
		case IQBalancePhase:
			return iqBalancePhase;
			break;
		default:
			break;
	}
	return QVariant();
}

bool DeviceInterfaceBase::Set(QString _key, QVariant _value) {
	Q_UNUSED(_key);
	Q_UNUSED(_value);
	return false;
}

bool DeviceInterfaceBase::Set(STANDARD_KEYS _key, QVariant _value, quint16 _option) {
	Q_UNUSED(_value);
	Q_UNUSED(_option);
	switch (_key) {
		case PluginName:
			Q_UNREACHABLE(); //Read only key
			break;
		case PluginDescription:
			Q_UNREACHABLE();
			break;
		case PluginNumDevices:
			Q_UNREACHABLE();
			break;
		case DeviceName:
			Q_UNREACHABLE();
			break;
		case DeviceDescription:
			Q_UNREACHABLE();
			break;
		case DeviceNumber:
			deviceNumber = _value.toInt();
			break;
		case DeviceType:
			Q_UNREACHABLE();
			break;
		case DeviceSampleRate:
			sampleRate = _value.toInt();
			break;
		case DeviceSampleRates:
			Q_UNREACHABLE();
			break;
		case DeviceFrequency:
			Q_UNREACHABLE(); //Must be handled by device
			break;
		case InputDeviceName:
			inputDeviceName = _value.toString();
			break;
		case OutputDeviceName:
			outputDeviceName = _value.toString();
			break;
		case HighFrequency:
			Q_UNREACHABLE();
			break;
		case LowFrequency:
			Q_UNREACHABLE();
			break;
		case FrequencyCorrection:
			break;
		case IQGain:
			iqGain = _value.toDouble();
			break;
		case StartupType:
			startupType = (STARTUP_TYPE)_value.toInt();
			break;
		case StartupDemodMode:
			Q_UNREACHABLE();
			break;
		case StartupSpectrumMode:
			Q_UNREACHABLE();
			break;
		case StartupFrequency:
			Q_UNREACHABLE();
			break;
		case LastDemodMode:
			lastDemodMode = _value.toInt();
			break;
		case LastSpectrumMode:
			lastSpectrumMode = _value.toInt();
			break;
		case LastFrequency:
			lastFreq = _value.toDouble();
			break;
		case UserDemodMode:
			Q_UNREACHABLE(); //Future
			break;
		case UserSpectrumMode:
			Q_UNREACHABLE(); //Future
			break;
		case UserFrequency:
			userFrequency = _value.toDouble();
			break;
		case IQOrder:
			iqOrder = (IQORDER)_value.toInt();
			break;
		case IQBalanceEnabled:
			iqBalanceEnable = _value.toBool();
			break;
		case IQBalanceGain:
			iqBalanceGain = _value.toDouble();
			break;
		case IQBalancePhase:
			iqBalancePhase = _value.toDouble();
			break;
		default:
			break;
	}
	return false;
}

QVariant DeviceInterfaceBase::Get(QString _key, quint16 _option)
{
	Q_UNUSED(_key);
	Q_UNUSED(_option);
	return QVariant();
}

//Settings shared by all devices
//Set defaults first, then call ReadSettings to handle initial ini file creation
void DeviceInterfaceBase::ReadSettings()
{
	//These are common settings for every device, variables are defined in DeviceInterface
	startupType = (STARTUP_TYPE)qSettings->value("StartupType", startupType).toInt();
	userFrequency = qSettings->value("StartupFreq", userFrequency).toDouble();
	inputDeviceName = qSettings->value("InputDeviceName", inputDeviceName).toString();
	outputDeviceName = qSettings->value("OutputDeviceName", outputDeviceName).toString();
	sampleRate = qSettings->value("SampleRate", sampleRate).toInt();
	iqGain = qSettings->value("IQGain",iqGain).toDouble();
	iqOrder = (IQORDER)qSettings->value("IQOrder", iqOrder).toInt();
	iqBalanceGain = qSettings->value("IQBalanceGain",iqBalanceGain).toDouble();
	iqBalancePhase = qSettings->value("IQBalancePhase",iqBalancePhase).toDouble();
	iqBalanceEnable = qSettings->value("IQBalanceEnable",iqBalanceEnable).toBool();
	lastFreq = qSettings->value("LastFreq", lastFreq).toDouble();
	lastDemodMode = qSettings->value("LastDemodMode",lastDemodMode).toInt();
	lastSpectrumMode = qSettings->value("LastSpectrumMode",lastSpectrumMode).toInt();
	deviceNumber = qSettings->value("DeviceNumber",deviceNumber).toInt();
	startupFrequency = qSettings->value("StartupFrequency",startupFrequency).toDouble();
	lowFrequency = qSettings->value("LowFrequency",lowFrequency).toDouble();
	highFrequency = qSettings->value("HighFrequency",highFrequency).toDouble();
	startupDemodMode = qSettings->value("StartupDemodMode",startupDemodMode).toInt();
}

void DeviceInterfaceBase::WriteSettings()
{
	qSettings->setValue("StartupType",startupType);
	qSettings->setValue("StartupFreq",userFrequency);
	qSettings->setValue("InputDeviceName", inputDeviceName);
	qSettings->setValue("OutputDeviceName", outputDeviceName);
	qSettings->setValue("SampleRate",sampleRate);
	qSettings->setValue("IQGain",iqGain);
	qSettings->setValue("IQOrder", iqOrder);
	qSettings->setValue("IQBalanceGain", iqBalanceGain);
	qSettings->setValue("IQBalancePhase", iqBalancePhase);
	qSettings->setValue("IQBalanceEnable", iqBalanceEnable);
	qSettings->setValue("LastFreq",lastFreq);
	qSettings->setValue("LastDemodMode",lastDemodMode);
	qSettings->setValue("LastSpectrumMode",lastSpectrumMode);
	qSettings->setValue("DeviceNumber",deviceNumber);
	qSettings->setValue("StartupFrequency",startupFrequency);
	qSettings->setValue("LowFrequency",lowFrequency);
	qSettings->setValue("HighFrequency",highFrequency);
	qSettings->setValue("StartupDemodMode",startupDemodMode);
}


//Should be a common function
void DeviceInterfaceBase::InitSettings(QString fname)
{
	//Use ini files to avoid any registry problems or install/uninstall
	//Scope::UserScope puts file C:\Users\...\AppData\Roaming\N1DDY
	//Scope::SystemScope puts file c:\ProgramData\n1ddy

	QString path = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
		//Pebble.app/contents/macos = 25
		path.chop(25);
#endif
	qSettings = new QSettings(path + "/PebbleData/" + fname +".ini",QSettings::IniFormat);

}