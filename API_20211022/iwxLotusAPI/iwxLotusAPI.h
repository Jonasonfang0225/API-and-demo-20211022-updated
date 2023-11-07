// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the IWXLOTUSAPI_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// IWX_LOTUS_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#pragma once
#include <stdio.h>
#include "iwxlotus_constants.h"
#include "iwxLotus_acq.h"
#include "iwxLotus_control.h"
#include "iwxLotus_stim.h"
#include "LotusStimHelperClasses.h"

#ifdef __cplusplus
extern "C" {
#endif


#ifdef IWXDAQ_EXPORTS
#define IWXDAQ_API __declspec(dllexport)
#else
#define IWXDAQ_API __declspec(dllimport)
#endif



typedef void(__stdcall* Control_change_callback)(uint8_t* p_param);
/****************************************************************************
   SetControlParamChangeCallback
   About:      Callback function to to handle Control device notifications to the software.
   p_param is an array of size NUM_CTRL_PARAM
   if(p_param[SW_Stim1_5_RIGHT] )  wcout << "Control Stim 5 Right Switch\n ";
	if (p_param[SW_Stim1_5_LEFT])  wcout << "Control Stim 5 Left Switch\n ";
	if (p_param[SW_Stim1_5_ON_OFF])  wcout << "Control Stim 5 On Off Switch\n ";
	if (p_param[SW_Stim2_6_RIGHT] )  wcout << "Control Stim 6 Right Switch\n ";
	if (p_param[SW_Stim2_6_LEFT] )  wcout << "Control Stim 6 Left Switch\n ";
	if (p_param[SW_Stim2_6_ON_OFF] )  wcout << "Control Stim 6 Onn-Off Switch\n ";
	if (p_param[SW_VIDEO_ON_OFF] )  wcout << "Control Video Switch\n ";
	if (p_param[SW_SCREENSHOT] )  wcout << "Control Screenshot Switch\n ";
	
	The change inthe Volume and the Stim1 and Stim2 amplitude  will be reported.  Say the user turns the volume knob positive by 3 clicks
	The value reported back will be 103, ie corresponding to a positive change of 3.
	The software can use this information to calculate the required change.
	This way the software, can keep track of what the amplitude should be,
	and not have to tell the control board about it.
	It give the software the freedom to set the step size of each change, ie the user may want each click to be worth 1mA or 0.1mA or 5mA
	
	if (p_param[Volume_AMP] != 100) {
		wcout << "Control Volume Amp :"; wcout << ((int)p_param[Volume_AMP] - 100); wcout << "\n ";
	}
	if (p_param[STIM1_AMP] != 100) {
		wcout << "Control STIM1 Amp :"; wcout << ((int)p_param[STIM1_AMP] - 100); wcout << "\n ";
	}
	if (p_param[STIM2_AMP] != 100) {
		wcout << "Control STIM2 Amp :"; wcout << ((int)p_param[STIM2_AMP] - 100); wcout << "\n ";
	}
   Outputs:    none

   Returns:
***************************************************************************/
IWX_LOTUS_API void __stdcall SetControlParamChangeCallback(Control_change_callback callback);

typedef void(__stdcall* LotusAPI_error_callback)(int p_param);
/****************************************************************************
   SetLotusErrorCallback
   About:      Callback function to report an error in USB communications. 
   This function should be called after the Allhardware has been detected. 
   This function is passed to the class handling the hardware USB communication, 
   and is only passed to the hardware that is detected. 
   when there is an error in communications the Error Callback function is called.

   LotusAPI_error_callback callback function should be defined as
   void __stdcall ErrorNotification(uint8_t p_param)
   p_param corresponds to the error code. 
   p_param = CONTROL_HARDWARE_NOT_FOUND  if there is a error communication with the Control USB device;
   p_param = ERR_COMM_HARDWARE  if there is a error communication with the Acquisition or Stimulation USB device;
   Outputs:    none

   Returns:    
***************************************************************************/
IWX_LOTUS_API void __stdcall SetLotusErrorCallback(LotusAPI_error_callback callback);

/**********************************************************************
COMMON FUNCTIONS
***********************************************************************/

/****************************************************************************
   OpenDevice
   About:      Opens Control, acq and stim Devices

   if device as already opened then that will be used

   Inputs:     wchar_t* log_file       log_file is used to keep a log.

   Outputs:    none

   Returns:    zero if All acquisition, stimulation and control device is created
				the device notfound is returned as a bit value
               ie:  BIT0 corresponds to ACQuisition device, BIT1 corresponds to STIM device, BIT2 corresponds to control device,


****************************************************************************/
IWX_LOTUS_API int OpenDevices(const wchar_t* log_file);

/****************************************************************************
   SetDemoMode
   About:      Set the API in demo mode, so that data from a file can be played back.

   Inputs:     wchar_t* acq_data_file       Data file containing acquisition data,should be in comma separated format.
               wchar_t* stim_data_file       Data file containing stimulation data,should be in comma separated format.

   Outputs:    none

   Returns:    true

****************************************************************************/
IWX_LOTUS_API int SetDemoMode(const wchar_t* acq_data_file, const wchar_t* stim_data_file);

/****************************************************************************
   SetDemoModeWithArray
   About:      Set the API in demo mode, so that data from a  array  can be played back.

   Inputs:    const float* acq_data : pointer to data for acquisition playback of size num_acq_point
              const float* stim_data pointer to data for stimulation playback of size num_stim_points
              stim_data  can be null if there is no stimulation data for feedback.
              there is no situation where acq_data is NULL and num_acq_point is zero

   Outputs:    none

   Returns:    true

****************************************************************************/
IWX_LOTUS_API int SetDemoModeWithArray(const float* acq_data, int num_acq_point, const float* stim_data, int num_stim_points);

/****************************************************************************
   FindHardware
   About:      Find Lotus Hardware, This function should be called before finding the Acq, Stim or Control Hardware.
                This should be the first command called since all other commands depend on devices being present.

   Inputs:    

   Outputs:   

   Returns:    the device notfound is returned as a bit value
               ie:  BIT0 corresponds to ACQuisition device, BIT1 corresponds to STIM device, BIT2 corresponds to control device,.

****************************************************************************/
IWX_LOTUS_API int FindHardware();

/****************************************************************************
   CloseDevices
   About:      Closes all devices

   Inputs:     Nothing

   Outputs:    Nothing

   Returns:    Nothing 

****************************************************************************/
IWX_LOTUS_API void CloseDevices();
/****************************************************************************
IsAcqAndStimConnected
Check if the acquisition and stimulation devices are connected.  This should only be called when recording is off
Inputs:

Outputs: bit wise info if the device is disconnected. 
if Acquisition module is disconnected bit0 will be set
if stim module is disconnected bit 1 will be set
zero if both modules are connected.

Returns:   0 if Success   Error code if there is an error shows which device is not connected.
			ie:  BIT0 corresponds to ACQuisition device, BIT1 corresponds to STIM device

****************************************************************************/
IWX_LOTUS_API int IsAcqAndStimConnected();


/****************************************************************************
StartImpedanceMeasurement
//  Currently unused 
About:      Start impedance measurement.  This is needed before reading the impedance values.
The software will have to wait until this measurement is completed before reading the values.

Inputs:

Outputs:

Returns:   0 if Success   Error code if there is an error;

****************************************************************************/
IWX_LOTUS_API int StartImpedanceMeasurement();

// Acquisition


/****************************************************************************
   StartAcq
   About:      Start acquisition

   Inputs:    

   Outputs:    Nothing

   Returns:    0 if Success  Error code if there is an error;

****************************************************************************/
IWX_LOTUS_API int StartAcq();

/****************************************************************************
   StopAcq
   About:      Stop Acquisition

   Inputs:     Nothing

   Outputs:    Nothing

   Returns:    0 if Success
			   Error code if there is an error;
****************************************************************************/
IWX_LOTUS_API int StopAcq();

/****************************************************************************
   ReadDataFromAcquisitionDevice
   About:      Reads data from the acquisition device. 

   Inputs:     float* data : array to hold the data read from the device.
               int data_size : maximum number of samples per channel that can be recorded. 
			   This is the maximum amount of data that the function can read per channel. 
			   The function will return all the data that has been recorded up to that point, 
			   The size of the data array has to be atleast equal to data_size*numberofchannelsrecorded. 
			   for example if you want to record 16 channels, and data_size = 2000, then the data array has to be at least 2000*16 = 32000 
			   num_samples_acquired_per_channel should be 0. 
			   If it is not zero then the program will wait until that much data is recorded before it will return.

   Outputs:    int num_samples_acquired_per_channel : The actual Number of samples of data acquired per channel

   Returns:    0 if Success
			   -3  if no data is available
			   if there is a USB communication problem, then the callback function will be used

****************************************************************************/
IWX_LOTUS_API int ReadDataFromAcquisitionDevice(int& num_samples_acquired_per_channel, float* data, int data_size);

/****************************************************************************
   ReadDataFromStimulatorDevice
   About:      Reads data from the stimulator device.

   Inputs:     float* data : array to hold the data read from the device. 
               int data_size : size of data array

   Outputs:    int num_samples_acquired_per_channel : The actual Number of samples of data acquired per channel and returned 

   Returns:    0 if Success
               -3  if no data is available
			   if there is a USB communication problem, then the callback function will be used


****************************************************************************/
IWX_LOTUS_API int ReadDataFromStimulatorDevice(int& num_samples_acquired_per_channel, float* data, int data_size);


/****************************************************************************
GetErrorMessage
About:      Gets the Error Message.

Inputs:    int err_code : error code 

Outputs:    

Returns:    const pointer to string with the error message

****************************************************************************/
IWX_LOTUS_API const wchar_t* GetErrorMessage(int err_code);

/****************************************************************************
CheckStimulationParameters
About:      Check the stimulation parameters to make sure they are within bounds

Inputs:    CStimulationTrain& stim_param : stimulation parameters to check;


Outputs:

Returns:    int err_code : error code

****************************************************************************/
IWX_LOTUS_API int CheckStimulationParameters(const CStimulationTrain& stim_param);


/**********************************************************************
UTILITY FUNCTIONS
***********************************************************************/

/****************************************************************************
FilterData

About:      Filter waveform data

Inputs:     const float* input_array:  the array of data to filter.
int sizeofarray:  size of the input and output array.
int sampling_speed: sampling speed for the data.
float lowcutoff: low cutoff for the filter : for a low pass filter set the value to 0;
float highcutoff: high cutoff for the filter: for a high pass filter set the value to the sampling speed
int order:  Order of the filter, needs to be an odd number

Outputs:    float* output_array:  the ourput array has to be same size as the input array.

Returns:    0 if Success
Error code if there is an error;

****************************************************************************/
IWX_LOTUS_API int FilterData(const float* input_array, float* output_array, int sizeofarray, int sampling_speed, float lowcutoff, float highcutoff, int order);

/****************************************************************************
NotchFilterData

About:      Notch filter waveform data

Inputs:     float* input_array:  the array of data to filter.
int sizeofarray:  size of the input and output array.
int sampling_speed: sampling speed for the data.
int notchfreq: notch frequency, 50 or 60;
bool second_harmonic : remove second harmonics
bool third_harmonics: remove the third harmonics

Outputs:    float* output_array:  the ourput array has to be same size as the input array.

Returns:    0 if Success
Error code if there is an error;

****************************************************************************/
IWX_LOTUS_API int NotchFilterData(const float* input_array, float* output_array, int sizeofarray, int sampling_speed, int notchfreq, bool second_harmonic, bool third_harmonic);

/****************************************************************************
OnlineNotchFilterSetup
About:      Setup Online Notch Filter
Inputs:    
int sampling_speed: sampling speed for the data.
int notchfreq: notch frequency, 50 or 60;
bool second_harmonic : set to true if you want the second harmonic to be removed as well 
Outputs:    
Returns:    0 if Success
Error code if there is an error;

****************************************************************************/
IWX_LOTUS_API int OnlineNotchFilterSetup( int sampling_speed, int notchfreq, bool second_harmonic);

/****************************************************************************
OnlineNotchFilterData
About:      perform the notch filter on the input data
Inputs:
int ch_index: channel index
float data: input data
Outputs:
Returns:    float filtered data
Error code if there is an error;

****************************************************************************/
IWX_LOTUS_API float OnlineNotchFilterData(int ch_index, float  data);

/****************************************************************************
OnlinebandPassFilterSetup
About:      Setup Online Bandpass Filter
OnlinebandPassFilterSetup will change the filter order to match the High and Low Pass freq requested.
for eg. if you set the filter order to 1001, and ask for a 10Hz HP, at a sampling speed of 16000, the filter order will be changed to 1601 since that is the min order required
		if you set it to 2001 then the order will not be changed since it is greater than 1601

Do not use the LOTUS_EMG_100_10kHz or LOTUS_EMG_10_1kHz  with the Online Bandpass filter, since that will affect your frequency response.
Inputs:
int ch_index: index of the filter to setup, if ch_index = -1, all filters are setup 
int sampling_speed: sampling speed for the data.
int lowcutoff:  Highpass filter value 
int highcutoff: Lowpass Filter value
int order of the filter: 
Outputs: order of the filter: if the provided order of the filter is not enough, the API will change the order of the filter.
Returns:    the time offset of the filter in sample points
Error code if there is an error;

****************************************************************************/
IWX_LOTUS_API int OnlinebandPassFilterSetup(int ch_index, int sampling_speed, float lowcutoff, float highcutoff, int& order);

/****************************************************************************
OnlineBandpassFilterData
About:      perform the bandpass filter on the input data
Inputs:
int ch_index: channel index
float data: input data
Outputs:
Returns:    float filtered data
Error code if there is an error;

****************************************************************************/
IWX_LOTUS_API float OnlineBandpassFilterData(int ch_index, float  data);


#ifdef __cplusplus
}
#endif