// iwxLotusAPI.cpp : Defines the exported functions for the DLL application.
//

#include "stdwx.h"
#include "iwxLotusAPI.h"
#include "LotusAcqDevice.h"
#include "LotusStimDevice.h"
#include "LotusControlDevice.h"


extern CLotusAcqDevice* p_acq_device;
extern CLotusStimDevice* p_stim_device;
extern CLotusControlDevice* p_ctrl_device;
wxString log_filename = wxEmptyString;
bool m_demo_mode = false;

Control_change_callback control_fn = NULL;
LotusAPI_error_callback error_fn = NULL;

#define MAX_NUM_CH 16
class  CiwxDAQ  {
public:
	CiwxDAQ(void);
	virtual ~CiwxDAQ();
	void SetDemoMode(const wchar_t* acq_data_file, const wchar_t* stim_data_file);
	void SetDemoMode(const float* acq_data, int num_acq_point, const float* stim_data, int num_stim_points);
	bool m_initialized;
	int m_last_error;
	CAcquisitionSettings m_acq_settings;
	int m_online_data_array_size;
	LOTUS_ACQUISITION_MODES m_mode;
	wxString m_acq_data_filename;
	wxString m_stim_data_filename;
	std::vector<float> m_acq_data;
	std::vector<float> m_stim_data;
	int m_demo_acq_read_index;
	int m_demo_stim_read_index;
	int m_num_acq_ch;
	int m_num_stim_ch;
	uint8_t p_control_param[NUM_CTRL_PARAM];
	CIIRBesselFilter m_notch_filter[MAX_NUM_CH];
	CIwxFilter m_bandpass_filter[MAX_NUM_CH];
	float m_acquisition_impedance_offset[MAX_NUM_CH];

//	typedef std::deque<float> data_vec_t;
};


// see iwxDAQ.h for the class definition
CiwxDAQ::CiwxDAQ()
{
	m_acq_data_filename = wxEmptyString;
	m_stim_data_filename = wxEmptyString;
	m_demo_acq_read_index = m_demo_stim_read_index = 0;
	m_num_acq_ch = 1;
	m_initialized = wxInitialize();
	return;
}
CiwxDAQ::~CiwxDAQ()
{
	if (m_initialized) wxUninitialize();
}

void CiwxDAQ::SetDemoMode(const wchar_t* acq_data_file, const wchar_t* stim_data_file)
{

	m_acq_data_filename = acq_data_file;
	m_stim_data_filename = stim_data_file;
	FIRSTLOGFILE(wxT("SetDemoMode: AcqFile:") + m_acq_data_filename + wxT(" & StimFile:") + m_stim_data_filename + wxT("\n"));
	m_stim_data.clear();
	m_acq_data.clear();
	m_demo_acq_read_index = m_demo_stim_read_index = 0;
	if(!m_acq_data_filename.IsEmpty()){
		wxFFileInputStream infile(m_acq_data_filename);
		MyTextInputStream txt_in(infile);
		wxString new_line;
		float temp_float;
		while (infile.IsOk()) {
			txt_in >> temp_float;
			m_acq_data.push_back(temp_float);
		}
	}
	if (!m_stim_data_filename.IsEmpty()) {
		wxFFileInputStream infile(m_stim_data_filename);
		MyTextInputStream txt_in(infile);
		wxString new_line;
		float temp_float;
		while (infile.IsOk()) {
			txt_in >> temp_float;
			m_stim_data.push_back(temp_float);
		}
	}
}

void CiwxDAQ::SetDemoMode(const float* acq_data, int num_acq_point, const float* stim_data, int num_stim_points) {
	FIRSTLOGFILE(wxT("SetDemoMode: array passed\n"));
	m_stim_data.clear();
	m_acq_data.clear();
	m_demo_acq_read_index = m_demo_stim_read_index = 0;
	if (acq_data && num_acq_point > 0) {
		for (int i = 0; i < num_acq_point; ++i) {
			m_acq_data.push_back(acq_data[i]);
		}
	}
	if (stim_data && num_stim_points > 0) {
		for (int i = 0; i < num_stim_points; ++i) {
			m_stim_data.push_back(stim_data[i]);
		}
	}
}


CiwxDAQ* p_dev = NULL;

int SetDemoMode(const wchar_t* acq_data_file, const wchar_t* stim_data_file)
{
	if (!p_dev)p_dev = new CiwxDAQ();
	p_dev->SetDemoMode(acq_data_file, stim_data_file);
	m_demo_mode = true;
	return SUCCESS;
}

int SetDemoModeWithArray(const float* acq_data, int num_acq_point, const float* stim_data, int num_stim_points) {
	if (!p_dev)p_dev = new CiwxDAQ();
	p_dev->SetDemoMode(acq_data, num_acq_point, stim_data, num_stim_points);
	m_demo_mode = true;
	return SUCCESS;
}

int OpenDevices(const wchar_t* log_file)
{
	if (!p_dev)p_dev = new CiwxDAQ();
	log_filename = log_file;
	FILE* m_log_file;
	if (!log_filename.IsEmpty() && (m_log_file = _tfopen(log_filename, FILE_WRITE)) != NULL) {
		_ftprintf(m_log_file, L"Lotus Log File\n"); 
		fclose(m_log_file);
	}; 
	return FindHardware();
}

void CloseDevices()
{
	FIRSTLOGFILE(wxT("Close Devices"));
	if (p_acq_device) {
		delete p_acq_device;
		p_acq_device = NULL;
	}
	if (p_stim_device) {
		SetStimHVPowerSupply(false);
		delete p_stim_device;
		p_stim_device = NULL;
	}
	if (p_ctrl_device) {
		// turn off all LED except power led on close device
		uint8_t p_control_parameters[NUM_CTRL_LED_PARAM];
		for (int j = 0; j < NUM_CTRL_LED_PARAM; ++j) {
			p_control_parameters[j] = 0;
		}
		p_ctrl_device->SetControlStimulatorParameters(p_control_parameters);

		p_ctrl_device->SetControlParamChangeCallback(NULL);
		Sleep(100);
		delete p_ctrl_device;
		p_ctrl_device = NULL;
	}
	if (p_dev) {
		delete p_dev;
		p_dev = NULL;
	}
	return ;
}

int FindHardware()
{
	FIRSTLOGFILE(wxT("Find Hardware\n"));
	if (!p_dev)p_dev = new CiwxDAQ();
	int errcode = 0;
	int index = 0;
	if (p_acq_device) {
		delete p_acq_device;
		p_acq_device = NULL;
	}
	if (p_stim_device) {
		delete p_stim_device;
		p_stim_device = NULL;
	}
	if (p_ctrl_device) {
		delete p_ctrl_device;
		p_ctrl_device = NULL;
	}
	if (m_demo_mode) {
		return SUCCESS; // we don't want to connect to the hardware. 
	}
	int return_val = 0x07;// no devices found;
	while (errcode == SUCCESS && errcode != ERR_NODEVICE) {
		CDevice device;
		errcode = ERR_NODEVICE;
		switch (device.FindDevice(index, false)) {
		case LOTUS_ACQ:
			index = device.GetDeviceIndex();
			p_acq_device = new CLotusAcqDevice(index);
			errcode = SUCCESS;
			return_val &= 0xFE; // mark acq device found
			break;
		case LOTUS_STIM:
			index = device.GetDeviceIndex();
			p_stim_device = new CLotusStimDevice(index);
			errcode = SUCCESS;
			return_val &= 0xFD; // mark stim device found
			break;
		case LOTUS_CTRL:
			index = device.GetDeviceIndex();
			p_ctrl_device = new CLotusControlDevice(index);
			errcode = SUCCESS;
			return_val &= 0xFB; // mark control device found
			break;
		}
		++index;
	}
	return return_val;
}

int IsAcqAndStimConnected() {
	int ret_val = 0x03;
	if (p_acq_device) {
		if( p_acq_device->IsDeviceConnected()) ret_val &= 0xFE;
		else {	
//			delete p_acq_device;
//			p_acq_device = NULL;
		}
	}

	if (p_stim_device) {
		if (p_stim_device->IsDeviceConnected()) ret_val &= 0xFD;
		else {
//			delete p_stim_device;
//			p_stim_device = NULL;
		}
	}
	return ret_val;
}

int FindAcquisitionHardware(wchar_t* manufacturer_id_buffer, wchar_t* sn_buffer, int size_buffer)
{
	if (p_acq_device && p_acq_device->m_device_connected) {
		wcscpy_s(manufacturer_id_buffer, size_buffer, L"NeuroInvent");
		wxString sn = p_acq_device->GetDeviceSerialNumber();
		const wxChar* myStringChars = sn.wc_str();
		for (int i = 0; i < sn.Len() && i < size_buffer; i++) {
			sn_buffer[i] = myStringChars[i];
		}
		sn_buffer[sn.Len()] = _T('\0');
		FIRSTLOGFILE(wxT("Acquisition Hardware Found: SN =") + sn +wxT("\n"));
		return SUCCESS;
	}
	else {
		wcscpy_s(manufacturer_id_buffer, size_buffer, L"None");
		wcscpy_s(sn_buffer, size_buffer, L"None");
		if (m_demo_mode) {
			wcscpy_s(manufacturer_id_buffer, size_buffer, L"Lotus Demo");
			wcscpy_s(sn_buffer, size_buffer, L"DemoSN");
			FIRSTLOGFILE(wxT("Acquisition Hardware Demo\n"));
			return SUCCESS;
		}
		FIRSTLOGFILE(wxT("Acquisition Hardware not found\n"));
		return ACQ_HARDWARE_NOT_FOUND;
	}
}

int FindControlHardware(wchar_t* manufacturer_id_buffer, wchar_t* sn_buffer, int size_buffer)
{
	if (p_ctrl_device) {
		wcscpy_s(manufacturer_id_buffer, size_buffer, L"NeuroInvent");
		wxString sn = (p_ctrl_device->GetDeviceSerialNumber()).wc_str();
		const wxChar* myStringChars = sn.wc_str();
		for (int i = 0; i < sn.Len() && i < size_buffer; i++) {
			sn_buffer[i] = myStringChars[i];
		}
		sn_buffer[sn.Len()] = _T('\0');
		FIRSTLOGFILE(wxT("Control Hardware Found =") + sn + wxT("\n"));
		return SUCCESS;
	}
	else {
		wcscpy_s(manufacturer_id_buffer, size_buffer, L"None");
		wcscpy_s(sn_buffer, size_buffer, L"None");
		if (m_demo_mode) {
			wcscpy_s(manufacturer_id_buffer, size_buffer, L"Lotus Demo");
			wcscpy_s(sn_buffer, size_buffer, L"DemoSN");
			FIRSTLOGFILE(wxT("Acquisition Hardware Demo\n"));
			return SUCCESS;
		}
		FIRSTLOGFILE(wxT("Control Hardware not found\n"));
		return CONTROL_HARDWARE_NOT_FOUND;
	}
}

int FindStimulationHardware(wchar_t* manufacturer_id_buffer, wchar_t* sn_buffer, int size_buffer)
{
	if (p_stim_device && p_stim_device->m_device_connected) {
		wcscpy_s(manufacturer_id_buffer, size_buffer, L"NeuroInvent");
		wxString sn = (p_stim_device->GetDeviceSerialNumber()).wc_str();
		const wxChar* myStringChars = sn.wc_str();
		for (int i = 0; i < sn.Len() && i < size_buffer; i++) {
			sn_buffer[i] = myStringChars[i];
		}
		sn_buffer[sn.Len()] = _T('\0');
		FIRSTLOGFILE(wxT("Stimulation Hardware Found =") + sn + wxT("\n"));
		return SUCCESS;
	}
	else {
		wcscpy_s(manufacturer_id_buffer, size_buffer, L"None");
		wcscpy_s(sn_buffer, size_buffer, L"None");
		if (m_demo_mode) {
			wcscpy_s(manufacturer_id_buffer, size_buffer, L"Lotus Demo");
			wcscpy_s(sn_buffer, size_buffer, L"DemoSN");
			return SUCCESS;
		}
		FIRSTLOGFILE(wxT("Stimulation Hardware not found\n"));
		return STIM_HARDWARE_NOT_FOUND;
	}
}

int SetAcquisitionParameters(int speed, LOTUS_ACQUISITION_MODES mode, unsigned int channels_to_record, unsigned int stim_channels_to_record, int online_data_array_size)
{
	FIRSTLOGFILE(wxString::Format(wxT("SetAcquisitionParameters: Speed = %d, mode=%d, channels_to_record = %x, stim_channels_to_record = %x, online_data_array_size = %d  \n"), speed, mode, channels_to_record, stim_channels_to_record, online_data_array_size));
	if (!p_dev)p_dev = new CiwxDAQ();
	int num_dev = 0;
	int return_val = IsAcqAndStimConnected(); 
	
	if (p_acq_device && p_acq_device->m_device_connected) {
		++num_dev;
		(p_acq_device)->SetFirstDevice(true);
		if (p_stim_device && p_stim_device->m_device_connected) {
			(p_stim_device)->SetFirstDevice(false);
			(p_stim_device)->SetLastDevice(true);
			(p_acq_device)->SetLastDevice(false);
		}
		p_acq_device->GetDeviceAcquisitionSettings(p_dev->m_acq_settings);
	}
	if (p_stim_device && p_stim_device->m_device_connected) {
		++num_dev;
		p_stim_device->GetDeviceAcquisitionSettings(p_dev->m_acq_settings);
	}

	p_dev->m_acq_settings.m_acquisition_speed = speed;
	p_dev->m_acq_settings.m_device_acq_settings.num_analog_in_channels = 16;
	if (p_stim_device && p_stim_device->m_device_connected)channels_to_record = channels_to_record + (stim_channels_to_record << 16);

	p_dev->m_acq_settings.m_channels_displayed = std::bitset<MAX_CHANNELS>(channels_to_record);
	p_dev->m_acq_settings.m_ch_used = std::bitset<MAX_NUM_AIN_CH_CURRENTLY>(channels_to_record);
	p_dev->m_acq_settings.m_dac_ch_used = 0;// std::bitset<MAX_NUM_DAC_CH_CURRENTLY>(stim_channels_to_record);
//	int online_data_array_size = 0x1FFFF;  // This is the size of the API buffer used. This can be changed if needed. 

	p_dev->m_online_data_array_size = online_data_array_size;
	p_dev->m_mode = mode;
	p_dev->m_num_acq_ch = p_dev->m_acq_settings.m_ch_used.count();
	p_dev->m_num_stim_ch = std::bitset<MAX_NUM_DAC_CH_CURRENTLY>(stim_channels_to_record).count();

	return_val = 0x3;
	if (p_acq_device && p_acq_device->m_device_connected) {
		return_val &= 0xFE; // mark acq device found
	}
	if (p_stim_device && p_stim_device->m_device_connected) {
		return_val &= 0xFD; // mark stim device found
	}

	return return_val;
}

int StartAcq()
{
	FIRSTLOGFILE(wxT("StartAcq\n"));
	int err = ACQ_HARDWARE_NOT_FOUND;
	int return_val = IsAcqAndStimConnected();
	// device setup
	int num_dev = 0;
	if (p_acq_device && p_acq_device->m_device_connected)++num_dev;
	if (p_stim_device && p_stim_device->m_device_connected)++num_dev;
	if (p_acq_device && p_acq_device->m_device_connected) {
		p_acq_device->DeviceSetup(p_dev->m_acq_settings, p_dev->m_online_data_array_size, num_dev);
	}

	return_val = 0x3;
	if (p_acq_device && p_acq_device->m_device_connected) {
		p_acq_device->SetAcquisitionMode(p_dev->m_mode);
		return_val &= 0xFE; // mark acq device found
	}
	if (p_stim_device && p_stim_device->m_device_connected) {
		p_stim_device->DeviceSetup(p_dev->m_acq_settings, p_dev->m_online_data_array_size, num_dev);
		return_val &= 0xFD; // mark stim device found
	}
	wxMilliSleep(100);
	// end device setup


	bool stim_ok = false;
	bool acq_ok = false;
	if (p_dev->m_num_stim_ch > 0) {
		if (p_stim_device && p_stim_device->m_device_connected) {
			if (err = p_stim_device->PreStartSetup(NULL) != SUCCESS) return err;
			stim_ok = true;
		}
		else {
			return STIM_HARDWARE_NOT_FOUND;
		}
	}
	if (p_dev->m_num_acq_ch > 0){
		if(p_acq_device){
			if (err = p_acq_device->PreStartSetup(NULL) != SUCCESS) return err;
			acq_ok = true;
		}
		else {
			return ACQ_HARDWARE_NOT_FOUND;
		}
	}
	if(stim_ok)err = p_stim_device->Start(0);
	if (err != SUCCESS) return err;
	if(acq_ok)err = p_acq_device->Start(0);
	if (err != SUCCESS) return err;
	if (m_demo_mode) {
		p_dev->m_demo_acq_read_index = p_dev->m_demo_stim_read_index = 0;
	}

	return err;
}

int StopAcq()
{
	FIRSTLOGFILE(wxT("StopAcq\n"));
	if (p_acq_device && p_acq_device->m_device_connected)p_acq_device->Stop();
	if (p_stim_device && p_stim_device->m_device_connected)p_stim_device->Stop();
	if (p_ctrl_device)p_ctrl_device->Stop();

	return SUCCESS;
}

int ReadDataFromAcquisitionDevice(int& num_samples_acquired_per_channel, float* data, int data_size)
{
	wxString trigger_string;
	long trigger_index;
	int record = 1;
	if (p_acq_device && p_acq_device->m_device_connected) {
		return p_acq_device->ReadData(data, data_size, num_samples_acquired_per_channel, trigger_index, record, trigger_string);
	}
	else {
		if (m_demo_mode && p_dev->m_num_acq_ch > 0) {
			int num_pts_available = p_dev->m_acq_data.size();
			if (num_pts_available > 0) {
				num_samples_acquired_per_channel = data_size / p_dev->m_num_acq_ch;
				for (int i = 0; i < data_size; ++i) {
					data[i] = p_dev->m_acq_data[p_dev->m_demo_acq_read_index];
					if (++p_dev->m_demo_acq_read_index >= num_pts_available)p_dev->m_demo_acq_read_index = 0;
				}
				return SUCCESS;
			}
		}
	}
	return ACQ_HARDWARE_NOT_FOUND;
}

int ReadDataFromStimulatorDevice(int& num_samples_acquired_per_channel, float* data, int data_size)
{
	wxString trigger_string;
	long trigger_index;
	int record = 1;
	if (p_stim_device && p_stim_device->m_device_connected) {
		return p_stim_device->ReadData(data, data_size, num_samples_acquired_per_channel, trigger_index, record, trigger_string);
	}
	else {
		if (m_demo_mode && p_dev->m_num_stim_ch > 0) {
			int num_pts_available = p_dev->m_stim_data.size();
			if (num_pts_available > 0) {
				num_samples_acquired_per_channel = data_size / p_dev->m_num_stim_ch;
				for (int i = 0; i < data_size; ++i) {
					data[i] = p_dev->m_stim_data[p_dev->m_demo_stim_read_index];
					if (++p_dev->m_demo_stim_read_index >= num_pts_available)p_dev->m_demo_stim_read_index = 0;
				}
				return SUCCESS;
			}
		}
		//p_dev->m_demo_acq_read_index = p_dev->m_demo_stim_read_index
	}
	return STIM_HARDWARE_NOT_FOUND;
}


int FilterData(const float* input_array, float* output_array, int sizeofarray, int sampling_speed, float lowcutoff, float highcutoff, int order)
{
	wxString msg = wxString::Format(wxT("FilterData: sampling_speed=%d, lowcutoff =%g, highcutoff = %g, order = %d\n"), sampling_speed, lowcutoff, highcutoff, order);
	FIRSTLOGFILE(msg);
	CIwxFilter filter;
	filter.SetParameters(sampling_speed, lowcutoff, highcutoff, order);
	return filter.FilterArrayData(input_array, output_array, sizeofarray);
}

int NotchFilterData(const float* input_array, float* output_array, int sizeofarray, int sampling_speed, int notchfreq, bool second_harmonic, bool third_harmonic)
{
	wxString msg = wxString::Format(wxT("NotchFilterData: sampling_speed=%d, notchfreq =%d\n"), sampling_speed, notchfreq);
	FIRSTLOGFILE(msg);
	CIwxFilter filter;
	return filter.NotchFilterData(input_array, output_array, sizeofarray, sampling_speed, notchfreq, second_harmonic, third_harmonic);
}

int OnlineNotchFilterSetup(int sampling_speed, int notchfreq, bool second_harmonic)
{
	wxString msg = wxString::Format(wxT("OnlineNotchFilterSetup: sampling_speed=%d, notchfreq =%d\n"), sampling_speed, notchfreq);
	FIRSTLOGFILE(msg);
	double ratio = (double)notchfreq / (double)sampling_speed;
	double ratio2 = 0;
	if (second_harmonic)ratio2 = ratio * 2;
	for (int i = 0; i < MAX_NUM_CH; ++i) {
		p_dev->m_notch_filter[i].SetNotchFilter(ratio, ratio2);
	}
	return SUCCESS;
}

float OnlineNotchFilterData(int ch_index, float  data)
{
	if (ch_index >= 0 && ch_index < MAX_NUM_CH)return p_dev->m_notch_filter[ch_index].ClockIn(data);
	else return data;
}



int StartImpedanceMeasurement()
{
	// Not Yet Implemented
	return SUCCESS;
}

const wchar_t* GetErrorMessage(int err_code) {
	
	if (err_code >= CONTROL_HARDWARE_NOT_FOUND && err_code < NUM_LOTUS_ERR) {
		return LotusErrorString[err_code - CONTROL_HARDWARE_NOT_FOUND];
	}
	else if (err_code > 0 && err_code < ERR_LAST_ERROR) {
		return CLabScribeConstants::GetError(err_code);
	}

	return L"Invalid Error Code";
}

int CheckStimulationParameters(const CStimulationTrain& stim_param) {
	if (stim_param.m_delay < 0) return STIM_DELAY_ERROR; //!< delay from the start of the recording until the first pulse.  
	if (stim_param.m_num_pulses < 0 || stim_param.m_num_pulses > 10) return STIM_NUM_PULSES_ERROR;  //!< number of pulses in the train   Range 0 - 10
	if (stim_param.m_pulse_width < 0.05 || stim_param.m_pulse_width > 1.0) return STIM_PULSE_WIDTH_ERROR;  //!< width of the the pulse in msec    Range: 50 usec to 250usec 
	if (stim_param.m_pulse_off_time < 0.05 || stim_param.m_pulse_off_time > 10000) return STIM_OFFTIME_ERROR;  //!< in msec, m_pulse_off_time + m_pulse_width = pulse Period : 
	if (stim_param.m_num_trains < 0 || stim_param.m_num_trains > 10000) return STIM_NUM_TRAINS_ERROR;   //!< set to 0 for continous trains  
	if (stim_param.m_intertrain_duration < 0.05 || stim_param.m_intertrain_duration > 10000) return STIM_INTERTRAIN_DURATION_ERROR;  //!< time in msec from the end of pulse ( including off time) to the begining of the first pulse in the next train.

	switch (stim_param.m_stim_mode) {
	case CV5: //!< Constant Voltage with 5V Range
		if (stim_param.m_pulse_amplitude < -5 || stim_param.m_pulse_amplitude > 5) return STIM_AMPLITUDE_ERROR;  //!< amplitude of the pulse  
		break;
	case CV20: //!< Constant Voltage with 20V Range
		if (stim_param.m_pulse_amplitude < -20 || stim_param.m_pulse_amplitude > 20) return STIM_AMPLITUDE_ERROR;  //!< amplitude of the pulse  
		break;
	case CC5://!< Constant Current with 5mA Range
		if (stim_param.m_pulse_amplitude < -5 || stim_param.m_pulse_amplitude > 5) return STIM_AMPLITUDE_ERROR;  //!< amplitude of the pulse  
		break;
	case CC20: //!< Constant Current with 20mA Range
		if (stim_param.m_pulse_amplitude < -20 || stim_param.m_pulse_amplitude > 20) return STIM_AMPLITUDE_ERROR;  //!< amplitude of the pulse  
		break;
	case CC50://!< Constant Current with 100mA Range
		if (stim_param.m_pulse_amplitude < -50 || stim_param.m_pulse_amplitude > 50) return STIM_AMPLITUDE_ERROR;  //!< amplitude of the pulse  
		break;
	default:
		return STIM_STIM_MODE_ERROR;   //!< whether the mode is constant voltage or constant current  
	}
	return SUCCESS;
}


IWX_LOTUS_API void __stdcall SetControlParamChangeCallback(Control_change_callback callback)
{
	control_fn = callback;
	if (p_ctrl_device) {
		p_ctrl_device->SetControlParamChangeCallback(control_fn);
	}
}

IWX_LOTUS_API void __stdcall SetLotusErrorCallback(LotusAPI_error_callback callback)
{
	error_fn = callback;
	if (p_acq_device) {
		p_acq_device->SetErrorFunctionCallback(error_fn);
	}
	if (p_stim_device) {
		p_stim_device->SetErrorFunctionCallback(error_fn);
	}
	if (p_ctrl_device) {
		p_ctrl_device->SetErrorFunctionCallback(error_fn);
	}
}

IWX_LOTUS_API int OnlinebandPassFilterSetup(int ch_index, int sampling_speed, float lowcutoff, float highcutoff, int& order)
{
	wxString msg = wxString::Format(wxT("OnlinebandPassFilterSetup: sampling_speed=%d, lowcutoff =%g, highcutoff = %g, order = %d\n"), sampling_speed, lowcutoff, highcutoff, order);
	FIRSTLOGFILE(msg);
	int high_ratio = sampling_speed / highcutoff + 1;
	int low_ratio = sampling_speed / lowcutoff + 1;
	if (order < high_ratio)order = high_ratio;
	if (order < low_ratio)order = low_ratio;

	if (ch_index >= 0 && ch_index < MAX_NUM_CH) {
		p_dev->m_bandpass_filter[ch_index].SetParameters(sampling_speed, lowcutoff, highcutoff, order);
		return order / 2;
	}
	else if (ch_index == -1) {
		for (int i = 0; i < MAX_NUM_CH; ++i) {
			p_dev->m_bandpass_filter[i].SetParameters(sampling_speed, lowcutoff, highcutoff, order);
		}
		return  order / 2;
	}
	else {
		return -1;
	}
}

IWX_LOTUS_API float OnlineBandpassFilterData(int ch_index, float  data)
{
	if (ch_index >= 0 && ch_index < MAX_NUM_CH)return p_dev->m_bandpass_filter[ch_index].FilterData(data);
	else return data;
}

IWX_LOTUS_API int SetStimulationParameters(unsigned int stim_ch, const CStimulationTrain& stim_param)
{
	// check stimulation parameters
	int iRet = CheckStimulationParameters(stim_param);
	if (iRet != SUCCESS) return iRet;

	if (p_stim_device && p_stim_device->m_device_connected) {
		iRet = p_stim_device->SetStimulationParameters(stim_ch, stim_param);
		p_stim_device->GetDacSettingsVector(p_dev->m_acq_settings.m_dac_ch_settings_vector);
		return iRet;
	}
	else {
		if (m_demo_mode) {

			return SUCCESS;
		}
	}
	return STIM_HARDWARE_NOT_FOUND;
}