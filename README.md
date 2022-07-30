# EasyMCU

为EasyCon而开发的通过上位机控制Arduino UNO R3、Leonardo、Teensy2、TEENSY2++，进而操作Nintendo Switch的固件
上位机项目连接：

> https://github.com/EasyConNS/EasyCon
>



上位机通过USB-TTL芯片发送串口数据，由单片机接收处理后向NS发送按键操作 
亦可在EEPROM中写入脚本字节码后脱机运行脚本，且使用8位计时器实现更精确的时间控制。具体指令集见上位机项目说明文档。  



## 适配MCU

主要适配atmega16u2、atmega32u4、at90usb1286等8位AVR单片机

使用了EasyConAPI，整体工程比较简单易懂



## 环境

编译环境需要以下文件

- WinAVR-20100110
- msys-1.0.dll替换到WinAVR-20100110\utils\bin

> https://sourceforge.net/projects/winavr/files/
>
> http://www.lab-z.com/wp-content/uploads/2018/10/msys-1.0-vista64.zip



## 编译

进入目录，修改makefile，选择实际编译的板子类型，然后`make`即可

```makefile
#REAL_BOARD   := Leonardo
#BOARD        := LEONARDO

#REAL_BOARD   := Beetle
#BOARD        := LEONARDO

#REAL_BOARD   := UNO
#BOARD        := UNO

REAL_BOARD   := Teensy2
BOARD        := TEENSY2

#REAL_BOARD   := TEENSY2pp
#BOARD        := TEENSY2

#ifeq($(CC),gcc)
#ifdef foo
 #frobozz=yes
#else
 #libs=$(normal_libs)
#endif

all:
include makefile.core.mk

```

添加文件请修改`makefile.core.mk`



## Refer

> http://elmagnifico.tech/2019/12/15/NintendoSwitch-Auto-Joystick/
>
> https://github.com/MHeironimus/ArduinoJoystickLibrary
>
> https://github.com/xiaoliang314/lw_coroutine
>
> https://github.com/xiaoliang314/libatask
>
