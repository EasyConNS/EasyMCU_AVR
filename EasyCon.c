#include "EasyCon.h"

// global variables
uint8_t mem[MEM_SIZE] = {0xFF, 0xFF, VERSION}; // preallocated memory for all purposes, as well as static instruction carrier
size_t serial_buffer_length = 0;               // current length of serial buffer
bool serial_command_ready = false;             // CMD_READY acknowledged, ready to receive command byte
uint8_t *flash_addr = 0;                       // start location for EEPROM flashing
uint16_t flash_index = 0;                      // current buffer index
uint16_t flash_count = 0;                      // number of bytes expected for this time
uint8_t *script_addr = 0;                      // address of next instruction
uint8_t *script_eof = 0;                       // address of EOF
uint16_t tail_wait = 0;                        // insert an extra wait before next instruction (used by compressed instruction)
uint32_t timer_elapsed = 0;                    // previous execution time
bool auto_run = false;

// timers define
volatile uint32_t timer_ms = 0; // script timer
volatile uint32_t wait_ms = 0;  // waiting counter

// Initialize script. Load static script into EEPROM if exists.
void ScriptInit(void)
{
    if (mem[0] != 0xFF || mem[1] != 0xFF)
    {
        // flash instructions from firmware
        int len = mem[0] | ((mem[1] & 0b01111111) << 8);
        for (int i = 0; i < len; i++)
            if (EEP_Read_Byte((uint8_t *)i) != mem[i])
                EEP_Write_Byte((uint8_t *)i, mem[i]);
    }
    memset(mem, 0, sizeof(mem));

    // randomize
    _seed = EEP_Read_Word((uint16_t *)SEED_OFFSET) + 1;
    srand(_seed);
    EEP_Write_Word((uint16_t *)SEED_OFFSET, _seed);

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
    _report_echo = ECHO_TIMES;
    // only if highest bit is 0
    auto_run = (EEP_Read_Byte((uint8_t *)1) >> 7) == 0;
}

void ScriptTick(void)
{
    // decrement waiting counter
    if (wait_ms != 0 && (_report_echo == 0 || wait_ms > 1))
        wait_ms--;
}

void Decrement_Report_Echo(void)
{
    // decrement echo counter
    if (!_script_running || _report_echo > 0 || wait_ms < 2)
    {
        _report_echo = Max(0, _report_echo - 1);
    }
}

bool isScriptRunning(void)
{
    if(_script_running == 1)
    {
        return true;
    }
    else
    {
        return false;
    }
}

// Run script on startup.
void Script_AutoStart(void)
{
    if (auto_run)
        Script_Start();
}

// Run script.
void Script_Start(void)
{
    script_addr = (uint8_t *)2;
    uint16_t eof = EEP_Read_Byte((uint8_t *)0) | (EEP_Read_Byte((uint8_t *)1) << 8);
    if (eof == 0xFFFF)
        eof = 0;
    script_eof = (uint8_t *)(eof & 0x7FFF);
    // reset variables
    wait_ms = 0;
    ///////////////////////////
    ZeroEcho();
    ///////////////////////////
    timer_ms = 0;
    tail_wait = 0;
    memset(mem + VARSPACE_OFFSET, 0, sizeof(mem) - VARSPACE_OFFSET);
    _script_running = 1;
    _seed = EEP_Read_Word((uint16_t *)SEED_OFFSET);

    StartRunningLED();
}

// Stop script.
void Script_Stop(void)
{
    _script_running = 0;
    timer_elapsed = timer_ms;
    ////////////////////////////
    ResetReport();
    ///////////////////////////
    _report_echo = ECHO_TIMES;

    StopRunningLED();
}

// Process script instructions.
void ScriptTask(void)
{
    while (true)
    {
        // status check
        if (!_script_running)
            return;
        // timer check
        if (wait_ms > 0)
            return;
        // release keys
        BlinkLED();
        for (int i = 0; i <= KEYCODE_MAX; i++)
        {
            if (KEY(i) != 0)
            {
                KEY(i)
                --;
                if (KEY(i) == 0)
                {
                    if (i == 32)
                    {
                        // LS
                        SetLeftStick(STICK_CENTER, STICK_CENTER);
                        _report_echo = ECHO_TIMES;
                    }
                    else if (i == 33)
                    {
                        // RS
                        SetRightStick(STICK_CENTER, STICK_CENTER);
                        _report_echo = ECHO_TIMES;
                    }
                    else if ((i & 0x10) == 0)
                    {
                        // Button
                        ReleaseButtons(_BV(i));
                        _report_echo = ECHO_TIMES;
                    }
                    else
                    {
                        // HAT
                        SetHATSwitch(HAT_CENTER);
                        _report_echo = ECHO_TIMES;
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
        _addr = (uint16_t)script_addr;
        _ins0 = EEP_Read_Byte(script_addr++);
        _ins1 = EEP_Read_Byte(script_addr++);
        int32_t n;
        int16_t reg;
        if (_ins0 & 0b10000000)
        {
            // key/stick actions
            if ((_ins0 & 0b01000000) == 0)
            {
                // Instruction : Key
                _keycode = (_ins0 >> 1) & 0b11111;
                // modify report
                if ((_keycode & 0x10) == 0)
                {
                    // Button
                    PressButtons(_BV(_keycode));
                    _report_echo = ECHO_TIMES;
                }
                else
                {
                    // HAT
                    SetHATSwitch(_keycode & 0xF);
                    _report_echo = ECHO_TIMES;
                }
                // post effect
                if (E_SET)
                {
                    // pre-loaded duration
                    SETWAIT(REG(_e_val));
                    RESETAFTER(_keycode, 1);
                }
                else if ((_ins0 & 0b00000001) == 0)
                {
                    // standard
                    n = _ins1;
                    // unscale
                    n *= 10;
                    SETWAIT(n);
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
                    n = _ins1 & 0b01111111;
                    RESETAFTER(_keycode, n);
                }
            }
            else
            {
                // Instruction : Stick
                _lr = (_ins0 >> 5) & 1;
                _keycode = 32 | _lr;
                _direction = _ins0 & 0b11111;
                // modify report
                if (_lr)
                {
                    // RS
                    SetRightStick(DX(_direction), DY(_direction));
                    _report_echo = ECHO_TIMES;
                }
                else
                {
                    // LS
                    SetLeftStick( DX(_direction), DY(_direction));
                    _report_echo = ECHO_TIMES;
                }
                // post effect
                if (E_SET)
                {
                    // pre-loaded duration
                    SETWAIT(REG(_e_val));
                    RESETAFTER(_keycode, 1);
                }
                else if ((_ins1 & 0b10000000) == 0)
                {
                    // standard
                    n = _ins1 & 0b01111111;
                    // unscale
                    n *= 50;
                    SETWAIT(n);
                    RESETAFTER(_keycode, 1);
                }
                else
                {
                    // hold
                    n = _ins1 & 0b01111111;
                    RESETAFTER(_keycode, n);
                }
            }
        }
        else
        {
            // flow control
            switch ((_ins0 >> 3) & 0b1111)
            {
            case 0b0000:
                if ((_ins0 & 0b100) == 0)
                {
                    // empty
                }
                else
                {
                    // Instruction : SerialPrint
                    reg = _ins & ((1 << 9) - 1);
                    if ((_ins0 & 0b10) == 0)
                    {
                        Serial_Send(reg);
                        Serial_Send(reg >> 8);
                    }
                    else
                    {
                        Serial_Send(mem[reg]);
                        Serial_Send(mem[reg + 1]);
                    }
                    break;
                }
                break;
            case 0b0001:
                // Instruction : Wait
                if (E_SET)
                {
                    // pre-loaded duration
                    n = REG(_e_val);
                }
                else if ((_ins0 & 0b100) == 0)
                {
                    // standard
                    n = _ins & ((1 << 10) - 1);
                    // unscale
                    n *= 10;
                }
                else if ((_ins0 & 0b10) == 0)
                {
                    // extended
                    _ins2 = EEP_Read_Byte(script_addr++);
                    _ins3 = EEP_Read_Byte(script_addr++);
                    n = _insEx & ((1L << 25) - 1);
                    // unscale
                    n *= 10;
                }
                else
                {
                    // high precision
                    n = _ins & ((1 << 9) - 1);
                }
                SETWAIT(n);
                break;
            case 0b0010:
                // Instruction : For
                if (_forstackindex == 0 || FOR_ADDR(_forstackindex - 1) != _addr)
                {
                    // loop initialize
                    _forstackindex++;
                    FOR_I(_forstackindex - 1) = 0;
                    FOR_ADDR(_forstackindex - 1) = _addr;
                    FOR_NEXT(_forstackindex - 1) = _ins & ((1 << 11) - 1);
                    // pre-loaded arguments
                    if (E_SET)
                    {
                        // iterator
                        _ri0 = REG(_e_val) & 0xF;
                        if (_ri0 != 0)
                        {
                            // store iterator
                            FOR_I(_forstackindex - 1) = _ri0 | 0x80000000;
                        }
                        // count
                        _ri1 = (REG(_e_val) >> 4) & 0xF;
                        if (_ri1 != 0)
                        {
                            // write loop count
                            FOR_C(_forstackindex - 1) = REG(_ri1);
                            // Mode 2 : loop count overwritten
                            E(2);
                            // jump to next (for condition checking)
                            JUMP(FOR_NEXT(_forstackindex - 1));
                            break;
                        }
                    }
                    // Mode 0 : init
                    E(0);
                    // jump to next (for further initialization)
                    JUMP(FOR_NEXT(_forstackindex - 1));
                    break;
                }
                break;
            case 0b0011:
                // Instruction : Next
                if (_ins0 & 0b100)
                {
                    // extended
                    _ins2 = EEP_Read_Byte(script_addr++);
                    _ins3 = EEP_Read_Byte(script_addr++);
                }
                if (E_SET)
                {
                    if (_e_val == 1)
                    {
                        // Mode 1 : break
                        _forstackindex--;
                        break;
                    }
                    else if (_e_val == 0)
                    {
                        // Mode 0 : init
                        if (_e_val == 0)
                        {
                            // initialize loop count
                            if ((_ins0 & 0b100) == 0)
                            {
                                // small number
                                FOR_C(_forstackindex - 1) = _ins & ((1 << 10) - 1);
                                if (FOR_C(_forstackindex - 1) == 0)
                                {
                                    // infinite loop
                                    FOR_C(_forstackindex - 1) = 0x80000000;
                                }
                            }
                            else
                            {
                                // large number
                                FOR_C(_forstackindex - 1) = _insEx & ((1L << 26) - 1);
                            }
                        }
                    }
                    else if (_e_val == 2)
                    {
                        // Mode 2 : loop count overwritten
                        // do nothing here
                    }
                }
                else
                {
                    // normal loop step
                    if (FOR_I(_forstackindex - 1) & 0x80000000)
                    {
                        // iterator
                        REG(FOR_I(_forstackindex - 1) & 0xF) += 1;
                    }
                    else
                    {
                        // loop variable
                        FOR_I(_forstackindex - 1) += 1;
                    }
                }
                // check condition
                if (FOR_I(_forstackindex - 1) & 0x80000000)
                    n = REG(FOR_I(_forstackindex - 1) & 0xF);
                else
                    n = FOR_I(_forstackindex - 1);
                if (FOR_C(_forstackindex - 1) != 0x80000000 && n >= FOR_C(_forstackindex - 1))
                {
                    // end for
                    _forstackindex--;
                }
                else
                {
                    // jump back
                    JUMP(FOR_ADDR(_forstackindex - 1));
                }
                break;
            case 0b0100:
                if (_ins0 & 0b100)
                {
                    // comparisons
                    _ri0 = (_ins1 >> 3) & 0b111;
                    _ri1 = _ins1 & 0b111;
                    switch (_ins0 & 0b11)
                    {
                    case 0b00:
                        // Instruction : Equal
                        _v = REG(_ri0) == REG(_ri1);
                        break;
                    case 0b01:
                        // Instruction : NotEqual
                        _v = REG(_ri0) != REG(_ri1);
                        break;
                    case 0b10:
                        // Instruction : LessThan
                        _v = REG(_ri0) < REG(_ri1);
                        break;
                    case 0b11:
                        // Instruction : LessOrEqual
                        _v = REG(_ri0) <= REG(_ri1);
                        break;
                    }
                    _v = (bool)_v;
                    _flag = (bool)_flag;
                    switch (_ins1 >> 6)
                    {
                    case 0b00:
                        // assign
                        _flag = _v;
                        break;
                    case 0b01:
                        // and
                        _flag &= _v;
                        break;
                    case 0b10:
                        // or
                        _flag |= _v;
                        break;
                    case 0b11:
                        // xor
                        _flag ^= _v;
                        break;
                    }
                }
                else
                {
                    switch (_ins1 >> 5)
                    {
                    case 0b000:
                        // Instruction : Break
                        if ((_ins1 & 0b10000) && !_flag)
                            break;
                        _v = _ins1 & 0b1111;
                        _forstackindex -= _v;
                        E(1);
                        JUMP(FOR_NEXT(_forstackindex - 1));
                        break;
                    case 0b001:
                        // Instruction : Continue
                        if ((_ins1 & 0b10000) && !_flag)
                            break;
                        _v = _ins1 & 0b1111;
                        _forstackindex -= _v;
                        JUMP(FOR_NEXT(_forstackindex - 1));
                        break;
                    case 0b111:
                        // Instruction : Return
                        if ((_ins1 & 0b10000) && !_flag)
                            break;
                        if (_callstackindex)
                        {
                            // sub function
                            // pop return address
                            JUMP(CALLSTACK(_callstackindex - 1));
                            _callstackindex--;
                            break;
                        }
                        else
                        {
                            // main function
                            Script_Stop();
                            break;
                        }
                        break;
                    }
                }
                break;
            case 0b0101:
                if ((_ins0 & 0b100) == 0)
                {
                    _ri0 = (_ins >> 7) & 0b111;
                    if (_ri0 == 0)
                    {
                        if ((_ins1 & (1 << 6)) == 0)
                        {
                            // binary operations on instant
                            _ins2 = EEP_Read_Byte(script_addr++);
                            _ins3 = EEP_Read_Byte(script_addr++);
                            _v = (_ins >> 3) & 0b111;
                            _ri0 = _ins & 0b111;
                            reg = _insEx;
                            BinaryOp(_v, _ri0, reg);
                        }
                        else
                        {
                            // preserved
                        }
                    }
                    else
                    {
                        // Instruction : Mov
                        reg = _ins1 & 0b01111111;
                        // fill sign bit
                        reg <<= 9;
                        reg >>= 9;
                        REG(_ri0) = reg;
                    }
                }
                else if ((_ins0 & 0b110) == 0b100)
                {
                    // binary operations on register
                    _v = (_ins >> 6) & 0b111;
                    _ri0 = (_ins >> 3) & 0b111;
                    _ri1 = _ins & 0b111;
                    BinaryOp(_v, _ri0, REG(_ri1));
                }
                else if ((_ins0 & 0b111) == 0b110)
                {
                    // bitwise shift
                    _ri0 = (_ins1 >> 4) & 0b111;
                    _v = _ins1 & 0b1111;
                    if (_ri0 == 0)
                        break;
                    if ((_ins1 & 0b10000000) == 0)
                    {
                        // Instruction : ShL
                        REG(_ri0) <<= _v;
                    }
                    else
                    {
                        // Instruction : ShR
                        REG(_ri0) >>= _v;
                    }
                }
                else
                {
                    // unary operations
                    _ri0 = _ins1 & 0b111;
                    switch ((_ins1 >> 3) & 0b1111)
                    {
                    case 0b0010:
                        // Instruction : Negative
                        if (_ri0 == 0)
                            break;
                        REG(_ri0) = -REG(_ri0);
                        break;
                    case 0b0011:
                        // Instruction : Not
                        if (_ri0 == 0)
                            break;
                        REG(_ri0) = ~REG(_ri0);
                        break;
                    case 0b0100:
                        // Instruction : Push
                        _stackindex++;
                        STACK(_stackindex - 1) = REG(_ri0);
                        break;
                    case 0b0101:
                        // Instruction : Pop
                        if (_ri0 == 0)
                            break;
                        REG(_ri0) = STACK(_stackindex - 1);
                        _stackindex--;
                        break;
                    case 0b0111:
                        // Instruction : StoreOp
                        E(_ri0);
                        break;
                    case 0b1000:
                        // Instruction : Bool
                        if (_ri0 == 0)
                            break;
                        REG(_ri0) = (bool)REG(_ri0);
                        break;
                    case 0b1001:
                        // Instruction : Rand
                        if (_ri0 == 0)
                            break;
                        if (!_seed)
                        {
                            _seed = timer_ms;
                            EEP_Write_Word((uint16_t *)SEED_OFFSET, _seed);
                            srand(_seed);
                        }
                        REG(_ri0) = rand() % REG(_ri0);
                        break;
                    }
                }
                break;
            case 0b0110:
                // branches
                reg = _ins & ((1 << 9) - 1);
                reg = reg << 7 >> 6;
                switch ((_ins0 >> 1) & 0b11)
                {
                case 0b00:
                    // Instruction : Branch
                    JUMPNEAR(reg);
                    break;
                case 0b01:
                    // Instruction : BranchTrue
                    if (_flag)
                        JUMPNEAR(reg);
                    break;
                case 0b10:
                    // Instruction : BranchFalse
                    if (!_flag)
                        JUMPNEAR(reg);
                    break;
                case 0b11:
                    // Instruction : Call
                    _callstackindex++;
                    STACK(_callstackindex - 1) = (int16_t)script_addr;
                    JUMPNEAR(reg);
                    break;
                }
                break;
            }
        }
    }
}

// Perform binary operations by operator code
void BinaryOp(uint8_t op, uint8_t reg, int16_t value)
{
    if (reg == 0)
        return;
    switch (op)
    {
    case 0b000:
        // Mov
        REG(reg) = value;
        break;
    case 0b001:
        // Add
        REG(reg) += value;
        break;
    case 0b010:
        // Mul
        REG(reg) *= value;
        break;
    case 0b011:
        // Div
        REG(reg) /= value;
        break;
    case 0b100:
        // Mod
        REG(reg) %= value;
        break;
    case 0b101:
        // And
        REG(reg) &= value;
        break;
    case 0b110:
        // Or
        REG(reg) |= value;
        break;
    case 0b111:
        // Xor
        REG(reg) ^= value;
        break;
    }
}

// Process data from serial port.
void Serial_Task(int16_t byte)
{
    if (byte < 0)
        return;
    BlinkLED();
    if (flash_index < flash_count)
    {
        // flashing
        SERIAL_BUFFER(flash_index) = byte;
        flash_index++;
        if (flash_index == flash_count)
        {
            // all bytes received
            for (flash_index = 0; flash_index < flash_count; flash_index++, flash_addr++)
                EEP_Write_Byte(flash_addr, SERIAL_BUFFER(flash_index));
            Serial_Send(REPLY_FLASHEND);
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
                    Serial_Send(REPLY_BUSY);
                }
                else
                {
                    //memset(&next_report, 0, sizeof(USB_JoystickReport_Input_t));
                    SetButtons( (SERIAL_BUFFER(0) << 9) | (SERIAL_BUFFER(1) << 2) | (SERIAL_BUFFER(2) >> 5) );
                    SetHATSwitch( (uint8_t)((SERIAL_BUFFER(2) << 3) | (SERIAL_BUFFER(3) >> 4)) );
                    SetLeftStick( (uint8_t)((SERIAL_BUFFER(3) << 4) | (SERIAL_BUFFER(4) >> 3)),
                                    (uint8_t)((SERIAL_BUFFER(4) << 5) | (SERIAL_BUFFER(5) >> 2)));
                    SetRightStick( (uint8_t)((SERIAL_BUFFER(5) << 6) | (SERIAL_BUFFER(6) >> 1)),
                                    (uint8_t)((SERIAL_BUFFER(6) << 7) | (SERIAL_BUFFER(7) & 0x7f)));
                    // set flag
                    _report_echo = ECHO_TIMES;
                    // send ACK
                    Serial_Send(REPLY_ACK);
                }
                serial_command_ready = false;
            }
            else if (serial_command_ready)
            {
                serial_command_ready = false;
                // command
                switch (byte)
                {
                case CMD_DEBUG:;
                    uint32_t n;
                    // instruction count
                    //uint8_t* count = (uint8_t*)(eeprom_read_byte((uint8_t*)0) | (eeprom_read_byte((uint8_t*)1) << 8));
                    //for (uint8_t* i = 0; i < count; i++)
                    //Serial_Send(eeprom_read_byte(i));

                    // current loop variable
                    n = FOR_I(_forstackindex - 1);
                    for (int i = 0; i < 4; i++)
                    {
                        Serial_Send(n);
                        n >>= 8;
                    }

                    // time elapsed
                    n = timer_elapsed;
                    for (int i = 0; i < 4; i++)
                    {
                        Serial_Send(n);
                        n >>= 8;
                    }

                    // PC
                    n = (uint16_t)script_addr;
                    for (int i = 0; i < 2; i++)
                    {
                        Serial_Send(n);
                        n >>= 8;
                    }
                    break;
                case CMD_VERSION:
                    Serial_Send(VERSION);
                    break;
                case CMD_READY:
                    serial_command_ready = true;
                    break;
                case CMD_HELLO:
                    Serial_Send(REPLY_HELLO);
                    break;
                case CMD_FLASH:
                    if (serial_buffer_length != 5)
                    {
                        Serial_Send(REPLY_ERROR);
                        break;
                    }
                    Script_Stop();
                    flash_addr = (uint8_t *)(SERIAL_BUFFER(0) | (SERIAL_BUFFER(1) << 7));
                    flash_count = (SERIAL_BUFFER(2) | (SERIAL_BUFFER(3) << 7));
                    flash_index = 0;
                    Serial_Send(REPLY_FLASHSTART);
                    break;
                case CMD_SCRIPTSTART:
                    Script_Start();
                    Serial_Send(REPLY_SCRIPTACK);
                    break;
                case CMD_SCRIPTSTOP:
                    Script_Stop();
                    Serial_Send(REPLY_SCRIPTACK);
                    break;
                default:
                    // error
                    Serial_Send(REPLY_ERROR);
                    break;
                }
            }
            else
            {
                // error
                Serial_Send(serial_buffer_length);
            }
            // clear buffer
            serial_buffer_length = 0;
        }
        // overflow protection
        if (serial_buffer_length >= SERIAL_BUFFER_SIZE)
            serial_buffer_length = 0;
    }
}