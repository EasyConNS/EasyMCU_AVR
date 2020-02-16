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

// constants
#define VERSION 0x42
#define ECHO 1
#define SERIAL_BUFFER_SIZE 20
#define KEYCODE_MAX 33
#define REGISTER_OFFSET 60
#define STACK_OFFSET 80
#define CALLSTACK_OFFSET 120
#define FORSTACK_OFFSET 180
#define DIRECTION_OFFSET 300
#define INS_OFFSET 370

// serial protocal control bytes and replies
#define CMD_READY 0xA5
#define CMD_DEBUG 0x80
#define CMD_HELLO 0x81
#define CMD_FLASH 0x82
#define CMD_SCRIPTSTART 0x83
#define CMD_SCRIPTSTOP 0x84
#define CMD_VERSION 0x85
#define REPLY_ERROR 0x00
#define REPLY_ACK 0xFF
#define REPLY_BUSY 0xFE
#define REPLY_HELLO 0x80
#define REPLY_FLASHSTART 0x81
#define REPLY_FLASHEND 0x82
#define REPLY_SCRIPTACK 0x83

// indexed variables and inline functions
#define Max(a, b) ((a > b) ? (a) : (b))
#define Min(a, b) ((a < b) ? (a) : (b))
#define SERIAL_BUFFER(i) mem[(i)]
#define KEY(keycode) mem[SERIAL_BUFFER_SIZE+(keycode)]
#define REGL(i) mem[REGISTER_OFFSET+((i)<<1)]
#define REGH(i) mem[REGISTER_OFFSET+((i)<<1)+1]
#define STACK(i,p) mem[STACK_OFFSET+((i)<<1)+p]
#define CALLSTACK(i,p) mem[CALLSTACK_OFFSET+((i)<<1)+p]
#define DX(i) mem[DIRECTION_OFFSET+((i)<<1)]
#define DY(i) mem[DIRECTION_OFFSET+((i)<<1)+1]
#define FOR(i,p) mem[FORSTACK_OFFSET+(i)*12+(p)]
#define SETWAIT(time) script_nexttime=timer_ms+(time)
#define RESETAFTER(keycode,n) KEY(keycode)=n
#define JUMP(low, high) script_addr = (uint8_t*)((low) | ((high) << 8))
#define E(val) _e_set = 1, e_val = (val)
#define E_SET ((_e_set_t = _e_set),(_e_set = 0),_e_set_t)

// single-byte variables
#define _ins0 mem[INS_OFFSET]
#define _ins1 mem[INS_OFFSET+1]
#define _ins2 mem[INS_OFFSET+2]
#define _ins3 mem[INS_OFFSET+3]
#define _code mem[INS_OFFSET+4]
#define _keycode mem[INS_OFFSET+5]
#define _lr mem[INS_OFFSET+6]
#define _direction mem[INS_OFFSET+7]
#define _addrL mem[INS_OFFSET+8]
#define _addrH mem[INS_OFFSET+9]
#define _stackindex mem[INS_OFFSET+10]
#define _callstackindex mem[INS_OFFSET+11]
#define _forstackindex mem[INS_OFFSET+12]
#define _report_echo mem[INS_OFFSET+13]
#define _e_set mem[INS_OFFSET+14]
#define _e_set_t mem[INS_OFFSET+15]
#define _script_running mem[INS_OFFSET+16]
#define _ri0 mem[INS_OFFSET+17]
#define _ri1 mem[INS_OFFSET+18]
#define _v mem[INS_OFFSET+19]

// All variables other than mem must have default values. Otherwise EasyCon cannot locate the data segment.
uint8_t mem[400] = {0xFF, 0xFF, VERSION};   // preallocated memory for all purposes, as well as static instruction carrier
size_t serial_buffer_length = 0;                      // current length of serial buffer
bool serial_command_ready = false;             // CMD_READY acknowledged, ready to receive command byte
uint8_t* flash_addr = 0;                                // start location for EEPROM flashing
uint16_t flash_index = 0;                               // current buffer index
uint16_t flash_count = 0;                               // number of bytes expected for this time
uint8_t* script_addr = 0;                               // address of next instruction
uint8_t* script_eof = 0;                                 // address of EOF
uint32_t script_nexttime = 0;                         // run next instruction after timer_ms reaches this
uint16_t tail_wait = 0;                                   // insert an extra wait before next instruction (used by compressed instruction)
int16_t e_val = 0;                                         // external argument for next instruction (used for dynamic for-loop, wait etc.)
uint32_t timer_elapsed = 0;                          // previous execution time

// Main entry point.
int main(void)
{
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
        // Process local script instructions.
        Script_Task();
    }
}

// Configures hardware and peripherals, such as the USB peripherals.
void SetupHardware(void)
{
    // We need to disable watchdog if enabled by bootloader/fuses.
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    // We need to disable clock division before initializing the USB hardware.
    clock_prescale_set(clock_div_1);
    
    // Initialize timer interrupt
    TIMSK0 |= (1 << TOIE0);
    sei(); 
    //enable interrupts
    TCCR0B |= (1 << CS01) | (1 << CS00);
    // set prescaler to 64 and start the timer
    
    // We can then initialize our hardware and peripherals, including the USB stack.
    
    // The USB stack should be initialized last.
    USB_Init();
    
    // Initialize serial port.
    Serial_Init(9600, false);
    
    // Initialize script.
    Script_Init();
    
    // Initialize report.
    ResetReport();
}

volatile uint32_t timer_ms = 0;
ISR (TIMER0_OVF_vect) // timer0 overflow interrupt
{
    TCNT0 += 6;
    // add 6 to the register (our work around)
    timer_ms++;
}

// Fired to indicate that the device is enumerating.
void EVENT_USB_Device_Connect(void)
{
    // We can indicate that we're enumerating here (via status LEDs, sound, etc.).
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

    // Not used here, it looks like we don't receive control request from the Switch.
}
USB_JoystickReport_Input_t next_report;

// Process and deliver data from IN and OUT endpoints.
void HID_Task(void)
{
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
    
    // [Optimized] Only send data when changed.
    //if (_report_echo || (_script_running && timer_ms + 5 < script_nexttime))
    if (_report_echo)
    {
        // We'll then move on to the IN endpoint.
        Endpoint_SelectEndpoint(JOYSTICK_IN_EPADDR);
        // We first check to see if the host is ready to accept data.
        if (Endpoint_IsINReady())
        {
            // Once populated, we can output this data to the host. We do this by first writing the data to the control stream.
            while(Endpoint_Write_Stream_LE(&next_report, sizeof(next_report), NULL) != ENDPOINT_RWSTREAM_NoError);
            // We then send an IN packet on this endpoint.
            Endpoint_ClearIN();
        }
        _report_echo--;
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
    _report_echo = ECHO;
}

// Process data from serial port.
void Serial_Task(void)
{
    // read all incoming bytes
    while (true)
    {
        // read next byte
        int16_t byte = Serial_ReceiveByte();
        if (byte < 0)
            break;
        if (flash_index < flash_count)
        {
            // flashing
            SERIAL_BUFFER(flash_index) = byte;
            flash_index++;
            if (flash_index == flash_count)
            {
                // all bytes received
                for (flash_index = 0; flash_index < flash_count; flash_index++, flash_addr++)
                    eeprom_write_byte(flash_addr, SERIAL_BUFFER(flash_index));
                Serial_SendByte(REPLY_FLASHEND);
            }
        }
        else
        {
            // regular
            SERIAL_BUFFER(serial_buffer_length++) = byte;
            // check control byte
            if ((byte & 0x80) != 0)
            {
                if (serial_buffer_length == 1 && !serial_command_ready && byte == CMD_READY)
                {
                    // comand ready
                    serial_command_ready = true;
                }
                else if (serial_buffer_length == 8)
                {
                    // report data
                    if (_script_running)
                    {
                        // script running, send BUSY
                        Serial_SendByte(REPLY_BUSY);
                    }
                    else
                    {
                        //memset(&next_report, 0, sizeof(USB_JoystickReport_Input_t));
                        next_report.Button = (SERIAL_BUFFER(0) << 9) | (SERIAL_BUFFER(1) << 2) | (SERIAL_BUFFER(2) >> 5);
                        next_report.HAT = (uint8_t)((SERIAL_BUFFER(2) << 3) | (SERIAL_BUFFER(3) >> 4));
                        next_report.LX = (uint8_t)((SERIAL_BUFFER(3) << 4) | (SERIAL_BUFFER(4) >> 3));
                        next_report.LY = (uint8_t)((SERIAL_BUFFER(4) << 5) | (SERIAL_BUFFER(5) >> 2));
                        next_report.RX = (uint8_t)((SERIAL_BUFFER(5) << 6) | (SERIAL_BUFFER(6) >> 1));
                        next_report.RY = (uint8_t)((SERIAL_BUFFER(6) << 7) | (SERIAL_BUFFER(7) & 0x7f));
                        // set flag
                        _report_echo = ECHO;
                        // send ACK
                        Serial_SendByte(REPLY_ACK);
                    }
                    //Serial_SendByte(next_report.Button >> 8);
                    //Serial_SendByte(next_report.Button);
                    //Serial_SendByte(next_report.HAT);
                    //Serial_SendByte(next_report.LX);
                    //Serial_SendByte(next_report.LY);
                    //Serial_SendByte(next_report.RX);
                    //Serial_SendByte(next_report.RY);
                    serial_command_ready = false;
                }
                else if (serial_command_ready)
                {
                    serial_command_ready = false;
                    // command
                    switch (byte)
                    {
                        case CMD_DEBUG:
                            ;uint32_t n;
                            // instruction count
                            //uint8_t* count = (uint8_t*)(eeprom_read_byte((uint8_t*)0) | (eeprom_read_byte((uint8_t*)1) << 8));
                            //for (uint8_t* i = 0; i < count; i++)
                                //Serial_SendByte(eeprom_read_byte(i));
                            
                            // current loop variable
                            for (int i = 0; i < 4; i++)
                                Serial_SendByte(FOR(_forstackindex - 1, i));
                            
                            // time elapsed
                            n = timer_elapsed;
                            for (int i = 0; i < 4; i++)
                            {
                                Serial_SendByte(n & 0xFF);
                                n >>= 8;
                            }
                            
                            // PC
                            n = (uint16_t)script_addr;
                            for (int i = 0; i < 2; i++)
                            {
                                Serial_SendByte(n & 0xFF);
                                n >>= 8;
                            }
                            break;
                        case CMD_VERSION:
                            Serial_SendByte(VERSION);
                            break;
                        case CMD_READY:
                            serial_command_ready = true;
                            break;
                        case CMD_HELLO:
                            Serial_SendByte(REPLY_HELLO);
                            break;
                        case CMD_FLASH:
                            if (serial_buffer_length != 5)
                            {
                                Serial_SendByte(REPLY_ERROR);
                                break;
                            }
                            Script_Stop();
                            flash_addr = (uint8_t*)(SERIAL_BUFFER(0) | (SERIAL_BUFFER(1) << 7));
                            flash_count = (SERIAL_BUFFER(2) | (SERIAL_BUFFER(3) << 7));
                            flash_index = 0;
                            Serial_SendByte(REPLY_FLASHSTART);
                            break;
                        case CMD_SCRIPTSTART:
                            Script_Start();
                            Serial_SendByte(REPLY_SCRIPTACK);
                            break;
                        case CMD_SCRIPTSTOP:
                            Script_Stop();
                            Serial_SendByte(REPLY_SCRIPTACK);
                            break;
                        default:
                            // error
                            Serial_SendByte(REPLY_ERROR);
                            break;
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
            // overflow protection
            if (serial_buffer_length >= SERIAL_BUFFER_SIZE)
                serial_buffer_length = 0;
        }
    }
}

// Initialize script.
void Script_Init(void)
{
    if (mem[0] != 0xFF || mem[1] != 0xFF)
    {
        // flash instructions from firmware
        int len = mem[0] | ((mem[1] & 0b01111111) << 8);
        for (int i = 0; i < len; i++)
            eeprom_write_byte((uint8_t*)i, mem[i]);
    }
    // calculate direction presets
    for (int i = 0; i < 16; i++)
    {
        // part 1
        int x = (Min(i, 8)) << 5;
        int y = (Max(i, 8) - 8) << 5;
        x = Min(x, 255);
        y = Min(y, 255);
        DX(i) = x;
        DY(i) = y;
    }
    for (int i = 16; i < 32; i++)
    {
        // part 2
        int x = (24 - Min(i, 24)) << 5;
        int y = (32 - Max(i, 24)) << 5;
        x = Min(x, 255);
        y = Min(y, 255);
        DX(i) = x;
        DY(i) = y;
    }
    // start script if highest bit is 0
    if ((eeprom_read_byte((uint8_t*)1) >> 7) == 0)
        Script_Start();
}

// Run script.
void Script_Start(void)
{
    _script_running = 1;
    script_addr = (uint8_t*)2;
    uint16_t eof = eeprom_read_byte((uint8_t*)0) | (eeprom_read_byte((uint8_t*)1) << 8);
    if (eof == 0xFFFF)
        eof = 0;
    script_eof = (uint8_t*)(eof & 0x7FFF);
    script_nexttime = 0;
    timer_ms = 0;
    // reset variables
    for (int i = 0; i <= KEYCODE_MAX; i++)
        KEY(i) = 0;
    _stackindex = 0;
    _forstackindex = 0;
    tail_wait = 0;
    _e_set = false;
}

// Stop script.
void Script_Stop(void)
{
    _script_running = 0;
    timer_elapsed = timer_ms;
}

// Process script instructions.
void Script_Task(void)
{
    while (true)
    {
        if (false && timer_ms >= 1000)
        {
            // RX debugging
            Serial_SendByte(0x01);
            timer_ms -= 1000;
        }
        // status check
        if (!_script_running)
            return;
        // time check
        if (timer_ms < script_nexttime)
            return;
        // release keys
        for (int i = 0; i <= KEYCODE_MAX; i++)
        {
            if (KEY(i) != 0)
            {
                KEY(i)--;
                if (KEY(i) == 0)
                {
                    if (i == 32)
                    {
                        // LS
                        next_report.LX = STICK_CENTER;
                        next_report.LY = STICK_CENTER;
                        _report_echo = ECHO;
                    }
                    else if (i == 33)
                    {
                        // RS
                        next_report.RX = STICK_CENTER;
                        next_report.RY = STICK_CENTER;
                        _report_echo = ECHO;
                    }
                    else if ((i & 0x10) == 0)
                    {
                        // Button
                        next_report.Button &= ~(1 << i);
                        _report_echo = ECHO;
                    }
                    else
                    {
                        // HAT
                        next_report.HAT = HAT_CENTER;
                        _report_echo = ECHO;
                    }
                }
            }
        }
        if (tail_wait != 0)
        {
            // wait after compressed instruction
            SETWAIT(tail_wait);
            tail_wait = 0;
            return;
        }
        if (script_addr >= script_eof)
        {
            // reaches EOF, end script
            Script_Stop();
            return;
        }
        _addrL = (uint16_t)script_addr & 0xFF;
        _addrH = (uint16_t)script_addr >> 8;
        _ins0 = eeprom_read_byte(script_addr++);
        _ins1 = eeprom_read_byte(script_addr++);
        if (_ins0 & 0b10000000)
        {
            // key/stick actions
            if ((_ins0 & 0b01000000) == 0)
            {
                // key action
                _keycode = (_ins0 >> 1) & 0b11111;
                // modify report
                if ((_keycode & 0x10) == 0)
                {
                    // Button
                    next_report.Button |= 1 << _keycode;
                    _report_echo = ECHO;
                }
                else
                {
                    // HAT
                    next_report.HAT = _keycode & 0xF;
                    _report_echo = ECHO;
                }
                // post effect
                if ((_ins0 & 0b00000001) == 0)
                {
                    // standard
                    uint32_t time = _ins1;
                    // unscale
                    time *= 10;
                    SETWAIT(time);
                    RESETAFTER(_keycode, 1);
                }
                else if ((_ins1 & 0b10000000) == 0)
                {
                    // compressed
                    tail_wait = _ins1 & 0b01111111;
                    // unscale
                    tail_wait *= 50;
                    SETWAIT(50);
                    RESETAFTER(_keycode, 1);
                }
                else
                {
                    // hold
                    uint32_t n = _ins1 & 0b01111111;
                    RESETAFTER(_keycode, n);
                }
            }
            else
            {
                // stick action
                _lr = (_ins0 >> 5) & 1;
                _keycode = 32 | _lr;
                _direction = _ins0 & 0b11111;
                // modify report
                if (_lr)
                {
                    // RS
                    next_report.RX = DX(_direction);
                    next_report.RY = DY(_direction);
                    _report_echo = ECHO;
                }
                else
                {
                    // LS
                    next_report.LX = DX(_direction);
                    next_report.LY = DY(_direction);
                    _report_echo = ECHO;
                }
                // post effect
                if ((_ins1 & 0b10000000) == 0)
                {
                    // standard
                    uint32_t time = _ins1 & 0b01111111;
                    // unscale
                    time *= 50;
                    SETWAIT(time);
                    RESETAFTER(_keycode, 1);
                }
                else
                {
                    // hold
                    uint32_t n = _ins1 & 0b01111111;
                    RESETAFTER(_keycode, n);
                }
            }
        }
        else
        {
            // flow control
            _code = (_ins0 >> 3) & 0b1111;
            uint32_t time;
            uint32_t count;
            int16_t rd;
            int16_t rs;
            switch (_code)
            {
                case 0b0000:
                    if ((_ins0 & 0b100) == 0)
                    {
                        // return
                        _ri0 = ((_ins0 & 0b11) << 1) | (_ins1 >> 7);
                        _v = _ins1 & 0b01111111;
                        if (_callstackindex)
                        {
                            // sub function
                            // assign return value
                            REGL(_ri0) = _v;
                            REGH(_ri0) = 0;
                            // pop return address
                            JUMP(CALLSTACK(_callstackindex - 1, 0), CALLSTACK(_callstackindex - 1, 1));
                            _callstackindex--;
                            break;
                        }
                        else
                        {
                            // main function
                            Script_Stop();
                            break;
                        }
                    }
                    else
                    {
                        // serial print
                        rd = ((_ins0 & 0b1) << 7) | _ins1;
                        if ((_ins0 & 0b10) == 0)
                        {
                            Serial_SendByte(rd);
                            Serial_SendByte(rd >> 8);
                        }
                        else
                        {
                            Serial_SendByte(mem[rd]);
                            Serial_SendByte(mem[rd] >> 8);
                        }
                        break;
                    }
                    break;
                case 0b0001:
                    // wait
                    time = 0;
                    if ((_ins0 & 0b100) == 0)
                    {
                        // standard
                        time |= _ins0 & 0b11;
                        time <<= 8;
                        time |= _ins1;
                        // unscale
                        time *= 10;
                    }
                    else if ((_ins0 & 0b10) == 0)
                    {
                        // extended
                        _ins2 = eeprom_read_byte(script_addr++);
                        _ins3 = eeprom_read_byte(script_addr++);
                        time |= _ins0 & 0b1;
                        time <<= 8;
                        time |= _ins1;
                        time <<= 8;
                        time |= _ins2;
                        time <<= 8;
                        time |= _ins3;
                        // unscale
                        time *= 10;
                    }
                    else
                    {
                        // high precision
                        time |= _ins0 & 0b1;
                        time <<= 8;
                        time |= _ins1;
                    }
                    SETWAIT(time);
                    break;
                case 0b0010:
                    // for loop
                    if (_forstackindex == 0 || FOR(_forstackindex - 1, 8) != _addrL || FOR(_forstackindex - 1, 9) != _addrH)
                    {
                        // loop : init
                        _forstackindex++;
                        // store loop variable
                        for (int i = 0; i < 4; i++)
                            FOR(_forstackindex - 1, i) = 0;
                        // store for address
                        FOR(_forstackindex - 1, 8) = _addrL;
                        FOR(_forstackindex - 1, 9) = _addrH;
                        // store next address
                        FOR(_forstackindex - 1, 10) = _ins1;
                        FOR(_forstackindex - 1, 11) = _ins0 & 0b111;
                        // obtain loop count from e_val
                        if (E_SET)
                        {
                            if (e_val <= 0)
                            {
                                // Mode 1 : break
                                E(1);
                            }
                            else
                            {
                                // overwrite loop count
                                for (int i = 0; i < 4; i++)
                                {
                                    FOR(_forstackindex - 1, i) = e_val & 0xFF;
                                    e_val >>= 8;
                                }
                                break;
                            }
                        }
                        else
                        {
                            // Mode 0 : init
                            E(0);
                        }
                        // jump to next (for further initialization)
                        JUMP(FOR(_forstackindex - 1, 10), FOR(_forstackindex - 1, 11));
                        break;
                    }
                    break;
                case 0b0011:
                    // next
                    if (_ins0 & 0b100)
                    {
                        // extended
                        _ins2 = eeprom_read_byte(script_addr++);
                        _ins3 = eeprom_read_byte(script_addr++);
                    }
                    if (E_SET)
                    {
                        if (e_val == 1)
                        {
                            // Mode 1 : break
                            _forstackindex--;
                            break;
                        }
                        else if (e_val == 0)
                        {
                            // Mode 0 : init
                            if ((_ins0 & 0b100) == 0)
                            {
                                // small number
                                count = _ins0 & 0b11;
                                count <<= 8;
                                count |= _ins1;
                            }
                            else
                            {
                                // large number
                                count = _ins0 & 0b11;
                                count <<= 8;
                                count |= _ins1;
                                count <<= 8;
                                count |= _ins2;
                                count <<= 8;
                                count |= _ins3;
                            }
                            // store loop count
                            for (int i = 0; i < 4; i++)
                            {
                                FOR(_forstackindex - 1, i + 4) = count & 0xFF;
                                count >>= 8;
                            }
                        }
                    }
                    else
                    {
                        // normal loop step
                        // load loop count
                        count = 0;
                        for (int i = 0; i < 4; i++)
                        {
                            count |= FOR(_forstackindex - 1, 8 - 1 - i);
                            if (i + 1 < 4)
                                count <<= 8;
                        }
                        if (count > 0)
                        {
                            // load loop variable
                            time = 0;
                            for (int i = 0; i < 4; i++)
                            {
                                time |= FOR(_forstackindex - 1, 4 - 1 - i);
                                if (i + 1 < 4)
                                    time <<= 8;
                            }
                            time++;
                            if (time >= count)
                            {
                                // break
                                _forstackindex--;
                                break;
                            }
                            // write loop count
                            for (int i = 0; i < 4; i++)
                            {
                                FOR(_forstackindex - 1, i) = time & 0xFF;
                                time >>= 8;
                            }
                        }
                    }
                    // jump back
                    JUMP(FOR(_forstackindex - 1, 8), FOR(_forstackindex - 1, 9));
                    break;
            }
        }
    }
}
