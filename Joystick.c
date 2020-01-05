/*
Nintendo Switch Fightstick - Proof-of-

Based on the LUFA library's Low-Level Joystick 
	(C) Dean 
Based on the HORI's Pokken Tournament Pro Pad 
	(C) 

This project implements a modified version of HORI's Pokken Tournament Pro 
USB descriptors to allow for the creation of custom controllers for 
Nintendo Switch. This also works to a limited degree on the PS3.

Since System Update v3.0.0, the Nintendo Switch recognizes the 
Tournament Pro Pad as a Pro Controller. Physical design limitations 
the Pokken Controller from functioning at the same level as the 
Controller. However, by default most of the descriptors are there, with 
exception of Home and Capture. Descriptor modification allows us to 
these buttons for our use.
*/

#include "Joystick.h"

// Main entry point.
int main(void) {
	// We'll start by performing hardware and peripheral setup.
	SetupHardware();
	// We'll then enable global interrupts for our use.
	GlobalInterruptEnable();
	// Once that's done, we'll enter an infinite loop.
	for (;;)
	{
		// We need to run our task to process and deliver data for our IN and OUT endpoints.
		HID_Task();
		// We also need to run the main USB management task.
		USB_USBTask();
		// Manage data from/to serial port.
		Serial_Task();
	}
}

// Configures hardware and peripherals, such as the USB peripherals.
void SetupHardware(void) {
	// We need to disable watchdog if enabled by bootloader/fuses.
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	// We need to disable clock division before initializing the USB hardware.
	clock_prescale_set(clock_div_1);
	// We can then initialize our hardware and peripherals, including the USB stack.
    
	// The USB stack should be initialized last.
	USB_Init();
    
    // Initialize serial port.
    Serial_Init(9600, false);
    
    // Initialize report.
    ResetReport();
}

// Fired to indicate that the device is enumerating.
void EVENT_USB_Device_Connect(void) {
	// We can indicate that we're enumerating here (via status LEDs, sound, etc.).
}

// Fired to indicate that the device is no longer connected to a host.
void EVENT_USB_Device_Disconnect(void) {
	// We can indicate that our device is not ready (via status LEDs, sound, etc.).
}

// Fired when the host set the current configuration of the USB device after enumeration.
void EVENT_USB_Device_ConfigurationChanged(void) {
	bool ConfigSuccess = true;

	// We setup the HID report endpoints.
	ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_OUT_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);
	ConfigSuccess &= Endpoint_ConfigureEndpoint(JOYSTICK_IN_EPADDR, EP_TYPE_INTERRUPT, JOYSTICK_EPSIZE, 1);

	// We can read ConfigSuccess to indicate a success or failure at this point.
}

// Process control requests sent to the device from the USB host.
void EVENT_USB_Device_ControlRequest(void) {
	// We can handle two control requests: a GetReport and a SetReport.

	// Not used here, it looks like we don't receive control request from the Switch.
}

const int16_t SERIAL_END_BYTE = 0x00FE;
const int16_t SERIAL_HELLO_BYTE = 0x00EF;
int16_t serial_buffer[100];
size_t serial_buffer_length = 0;
USB_JoystickReport_Input_t next_report;
bool report_changed = true;

// Process and deliver data from IN and OUT endpoints.
void HID_Task(void) {
	// If the device isn't connected and properly configured, we can't do anything here.
	if (USB_DeviceState != DEVICE_STATE_Configured)
		return;

    // [Optimized] We don't need to receive data at all.
    if (false)
    {
        // We'll start with the OUT endpoint.
        Endpoint_SelectEndpoint(JOYSTICK_OUT_EPADDR);
        // We'll check to see if we received something on the OUT endpoint.
        if (Endpoint_IsOUTReceived())
        {
            // If we did, and the packet has data, we'll react to it.
            if (Endpoint_IsReadWriteAllowed())
            {
                // We'll create a place to store our data received from the host.
                USB_JoystickReport_Output_t JoystickOutputData;
                // We'll then take in that data, setting it up in our storage.
                while(Endpoint_Read_Stream_LE(&JoystickOutputData, sizeof(JoystickOutputData), NULL) != ENDPOINT_RWSTREAM_NoError);
                // At this point, we can react to this data.

                // However, since we're not doing anything with this data, we abandon it.
            }
            // Regardless of whether we reacted to the data, we acknowledge an OUT packet on this endpoint.
            Endpoint_ClearOUT();
        }
    }
    
    // [Optimized] Only send data when status has changed.
    if (report_changed)
    {
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
            while(Endpoint_Write_Stream_LE(&JoystickInputData, sizeof(JoystickInputData), NULL) != ENDPOINT_RWSTREAM_NoError);
            // We then send an IN packet on this endpoint.
            Endpoint_ClearIN();
        }
        report_changed = false;
    }
}

// Process data from serial port.
void Serial_Task(void)
{
    // read all available bytes into buffer
    while (true)
    {
        int16_t byte = Serial_ReceiveByte();
        if (byte < 0)
            break;
        if (byte == SERIAL_END_BYTE)
        {
            if (serial_buffer_length == 7)
            {
                // report data
                //memset(&next_report, 0, sizeof(USB_JoystickReport_Input_t));
                next_report.Button = serial_buffer[0] | (serial_buffer[1] << 8);
                next_report.HAT = (uint8_t)serial_buffer[2];
                next_report.LX = (uint8_t)serial_buffer[3];
                next_report.LY = (uint8_t)serial_buffer[4];
                next_report.RX = (uint8_t)serial_buffer[5];
                next_report.RY = (uint8_t)serial_buffer[6];
                // set flag
                report_changed = true;
                // send ACK
                Serial_SendByte(serial_buffer_length);
            }
            else if (serial_buffer_length == 1)
            {
                if (serial_buffer[0] == SERIAL_HELLO_BYTE)
                {
                    // hello
                    Serial_SendByte(SERIAL_HELLO_BYTE);
                }
                else
                {
                    // error
                    Serial_SendByte(serial_buffer_length);
                }
            }
            else
            {
                // error
                Serial_SendByte(serial_buffer_length);
            }
            // clear buffer
            serial_buffer_length = 0;
        }
        else
            serial_buffer[serial_buffer_length++] = byte;
    }
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

// Prepare the next report for the host.
void GetNextReport(USB_JoystickReport_Input_t* const ReportData)
{
	memcpy(ReportData, &next_report, sizeof(USB_JoystickReport_Input_t));
}
