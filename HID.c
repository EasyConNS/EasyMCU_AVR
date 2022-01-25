#include "HID.h"

volatile uint8_t echo_ms = 0; // echo counter
USB_JoystickReport_Input_t next_report;

void ZeroEcho(void)
{
  echo_ms = 0;
}
// Reset report to default.
void ResetReport(void)
{
  memset(&next_report, 0, sizeof(USB_JoystickReport_Input_t));
  next_report.LX = STICK_CENTER;
  next_report.LY = STICK_CENTER;
  next_report.RX = STICK_CENTER;
  next_report.RY = STICK_CENTER;
  next_report.HAT = HAT_CENTER;
}
void SetButtons(const uint16_t Button) {next_report.Button = Button;}
void PressButtons(const uint16_t Button) {next_report.Button |= Button;}
void ReleaseButtons(const uint16_t Button) {next_report.Button &= ~(Button);}
void SetHATSwitch(const uint8_t HAT) {next_report.HAT = HAT;}
void SetLeftStick(const uint8_t LX, const uint8_t LY)
{
  next_report.LX = LX; next_report.LY = LY;
}
void SetRightStick(const uint8_t RX, const uint8_t RY)
{
  next_report.RX = RX; next_report.RY = RY;
}

void HIDInit(void)
{
  ResetReport();
  USB_Init();
}

void HIDTick(void)
{
	// decrement echo counter
	if (echo_ms != 0)
		echo_ms--;
}

void Report_Task(void);
void HIDTask(void)
{
  // We need to run our task to process and deliver data for our IN and OUT endpoints.
  Report_Task();
  // We also need to run the main USB management task.
  USB_USBTask();
}

// Fired to indicate that the device is enumerating.
void EVENT_USB_Device_Connect(void)
{
  // We can indicate that we're enumerating here (via status LEDs, sound, etc.).

  Device_Connected();
}

// Fired to indicate that the device is no longer connected to a host.
void EVENT_USB_Device_Disconnect(void)
{
  // We can indicate that our device is not ready (via status LEDs, sound, etc.).
}

// Fired when the host set the current configuration of the USB device after enumeration.
void EVENT_USB_Device_ConfigurationChanged(void)
{
  bool ConfigSuccess = true;

  // We setup the HID report endpoints.
  ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_OUT_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);
  ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_IN_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);

  // We can read ConfigSuccess to indicate a success or failure at this point.
}

// Process control requests sent to the device from the USB host.
void EVENT_USB_Device_ControlRequest(void)
{
  // We can handle two control requests: a GetReport and a SetReport.
  switch (USB_ControlRequest.bRequest)
  {
  // GetReport is a request for data from the device.
  case HID_REQ_GetReport:
    if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
    {
      //
    }

    break;
  case HID_REQ_SetReport:
    if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
    {
      //
    }

    break;
  }
  // Not used here, it looks like we don't receive control request from the Switch.
}

// Prepare the next report for the host.
inline void GetNextReport(USB_JoystickReport_Input_t *const ReportData)
{
	memcpy(ReportData, &next_report, sizeof(USB_JoystickReport_Input_t));
}

// Process and deliver data from IN and OUT endpoints.
void Report_Task(void)
{
  // If the device isn't connected and properly configured, we can't do anything here.
  if (USB_DeviceState != DEVICE_STATE_Configured)
    return;

  // We'll start with the OUT endpoint.
  Endpoint_SelectEndpoint(JOYSTICK_OUT_EPADDR);
  // We'll check to see if we received something on the OUT endpoint.
  if (Endpoint_IsOUTReceived())
  {
    // If we did, and the packet has data, we'll react to it.
    if (false && Endpoint_IsReadWriteAllowed())
    {
      // We'll create a place to store our data received from the host.
      USB_JoystickReport_Output_t JoystickOutputData;
      // We'll then take in that data, setting it up in our storage.
      while (Endpoint_Read_Stream_LE(&JoystickOutputData, sizeof(JoystickOutputData), NULL) != ENDPOINT_RWSTREAM_NoError)
        ;
      // At this point, we can react to this data.
      // However, since we're not doing anything with this data, we abandon it.
    }
    // Regardless of whether we reacted to the data, we acknowledge an OUT packet on this endpoint.
    Endpoint_ClearOUT();
  }

// [Optimized] Only send data when changed.
  if (echo_ms == 0)
  {
//////////////////////////////////////////
    // We'll then move on to the IN endpoint.
    Endpoint_SelectEndpoint(JOYSTICK_IN_EPADDR);
    // We first check to see if the host is ready to accept data.
    if (Endpoint_IsINReady())
    {
      // We'll create an empty report.
      USB_JoystickReport_Input_t JoystickInputData;
      // We'll then populate this report with what we want to send to the host.
      GetNextReport(&JoystickInputData);
      // Once populated, we can output this data to the host. We do this by first writing the data to the control stream.

      /* another codes impl
    while(Endpoint_Write_Stream_LE(&JoystickInputData, sizeof(JoystickInputData), NULL) != ENDPOINT_RWSTREAM_NoError);
    // We then send an IN packet on this endpoint.
    Endpoint_ClearIN();
*/
      if (Endpoint_Write_Stream_LE(&JoystickInputData, sizeof(JoystickInputData), NULL) == ENDPOINT_RWSTREAM_NoError)
      {
        // We then send an IN packet on this endpoint.
        Endpoint_ClearIN();

        Echo_Report();
        // set interval
        echo_ms = ECHO_INTERVAL;
      }
    }
// echo_ms end
  }
////////////////////////////////////////////
}