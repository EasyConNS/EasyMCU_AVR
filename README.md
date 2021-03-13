# SerialCon
为PokemonTycoon而开发的通过上位机控制Arduino UNO R3，进而操作Nintendo Switch的固件。  
上位机项目连接：https://github.com/ca1e/EasyCon2
思路是上位机通过USB-TTL芯片发送串口数据，由单片机接收处理后向NS发送按键操作。  
最初只是要实现上位机控制，后来随着EasyCon的加入，已经可以在EEPROM中写入脚本字节码后脱机运行脚本，且使用8位计时器实现更精确的时间控制。具体指令集见上位机项目说明文档。  
TTL的接线方式：TX接TX(1)，RX接RX(0)，GND接GND

### 通信协议
基本原则是以包为单位，每个包是一串字节，其中前面的所有字节最高位为0，最后一个字节（控制字节）最高位为1。单片机在收到控制字节后根据状态执行相应动作。
##### 按键操作
在除了Flash以外的任何状态下，向串口发送以下8字节包会被视作按键操作。  
首先，Report的定义如下
```C
// C Code
typedef struct {
    uint16_t Button; // 各种按键，具体定义看Joystick.h（下同）
    uint8_t  HAT;    // 十字键
    uint8_t  LX;     // 左摇杆X
    uint8_t  LY;     // 左摇杆Y
    uint8_t  RX;     // 右摇杆X
    uint8_t  RY;     // 右摇杆Y
} Report;
```
Report长度是7个字节，将这7个字节共56位拆成8个字节，每个字节7位。最高位前7个字节为0，最后一个字节为1，即可得到要发送的包。  
*注意，因为历史遗留问题，这里使用的是大字节序*
```C#
// C# Code
// Protocal packet structure:
// bit 7 (highest):    0 = data byte, 1 = end flag
// bit 6~0:            data (Big-Endian)

// serialize data
List<byte> serialized = new List<byte>();
serialized.AddRange(BitConverter.GetBytes(Button).Reverse());
serialized.Add(HAT);
serialized.Add(LX);
serialized.Add(LY);
serialized.Add(RX);
serialized.Add(RY);
if (raw)
    return serialized.ToArray();

// generate packet
List<byte> packet = new List<byte>();
long n = 0;
int bits = 0;
foreach (var b in serialized)
{
    n = (n << 8) | b;
    bits += 8;
    while (bits >= 7)
    {
        bits -= 7;
        packet.Add((byte)(n >> bits));
        n &= (1 << bits) - 1;
    }
}
packet[packet.Count - 1] |= 0x80;
return packet.ToArray();
```
将这个包发给单片机就会被识别为按键操作。成功则返回REPLY_ACK（0xFF），如果单片机正在执行脚本而拒绝外界操作，则返回REPLY_BUSY（0xFE）。  
任何字节丢失会导致操作无效，第8个字节丢失会导致下一个操作也无效。一次成功的操作即可完全恢复按键状态。

##### Ready命令
在除了Flash以外的任何状态下，向串口发送CMD_READY（0xA5），且包长度为1（只有控制字节），即可进入Ready状态。无论成功与否，单片机不会发送任何返回字节。  
Ready状态是执行其它命令的前提条件。接收到下一个包后**无论命令是否成功，都会结束Ready状态**，除非该包依然是Ready命令。  
包长度为1的限制是为了与按键操作区分，实际编程时可以发送两个或更多CMD_READY来确保单片机进入Ready状态。

##### Hello命令
在Ready状态下发送CMD_HELLO（0x81），成功则返回REPLY_HELLO（0x80），用于测试单片机是否连通。

##### Flash命令
用于远程向EEPROM烧录数据。需要Ready状态。包的长度为5，前两个字节为烧录的起始位置，后两个字节为烧录的字节数，最后发送CMD_FLASH（0x82）即可开始烧录。成功则返回REPLY_FLASHSTART（0x81）并进入Flash状态开始接受字节，参数错误则返回REPLY_ERROR（0x00）。
```C#
// C# Code
struct PacketFlash
{
    UInt16 Address;        // 起始地址
    UInt16 NumberOfBytes;  // 烧录字节数
    UInt8 ControlByte;     // CMD_FLASH
}
```
Flash状态下任何字节都被视为烧录数据，直到达到指定的字节数。烧录完毕后会返回REPLY_FLASHEND（0x82）并结束Flash状态。

##### ScriptStart命令
在Ready状态下发送CMD_SCRIPTSTART（0x83），命令单片机开始执行脚本，成功会返回REPLY_SCRIPTACK（0x83）。

##### ScriptStop命令
在Ready状态下发送CMD_SCRIPTSTOP（0x84），命令单片机停止执行脚本，成功会返回REPLY_SCRIPTACK（0x83）。

##### Version命令
在Ready状态下发送CMD_VERSION（0x85），单片机会发回固件版本号（一个字节），用于查询固件版本。

##### 未知命令
在Ready状态下接收到不明控制字节，单片机会返回REPLY_ERROR（0x00）并结束Ready状态。
