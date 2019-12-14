This is the Pokemon Sword and Shield Auto Egg Hatcher!

I saw someone post a video of something that did this for you, so I figured it would be easy enough to do it. 

Found this:
https://github.com/progmem/Switch-Fightstick

Which lead to this:
https://github.com/shinyquagsire23/Switch-Fightstick

Which lead to this:
https://github.com/bertrandom/snowball-thrower

Used the last one extensively, modified the movement set to work for hatching eggs.

How to get it to work:
What you need: Arduino Uno R3, Arduino USB (Looks like a printer cable), and a Nintendo Switch

You will need to download Flip to push the payload to the Arduino

https://www.microchip.com/DevelopmentTools/ProductDetails/PartNO/FLIP

Download that, plug in your Arduino and get it into DFU mode (you connect the Ground Pin to the RESET2 pin)
See here for help: https://www.circuito.io/blog/arduino-uno-pinout/

Once those pins are connected, windows should recognize that is gone, disconnect the pins

Check to see if Device Manager recognizes the device, should be under ports. 

If it says Arduino still, go back and make sure you get it into DFU mode

If it says something like Unrecognized Device, right click it, click update driver.

Click Browse my Computer, Click let me pick from my computer, Click Have disk

Browse to the program folder that has Flip in it. Inside the USB folder, select atmel_usb_dfu.inf

Once you do that, it should be recognized and you are good to go!

Open flip, click select target device, pick ATmega16u2

Click Open Communication Medium, click open

Click load Hex File, select Joystick.hex and press run

In the game, fly to the daycare in the wild area. 

Make sure your party is full, and the town map is in the bottom right corner of your menu

After that you should be able to plug it into the switch and watch it go!

Good Luck! If you have instructions on other platforms please let me know and I will update this!

Check Releases for the Joystick.hex

A note: The above is timed with oval charm, flame body, and hatching magikarp. I will make different versions later to save everyone else the trouble of dealing with it, but for now, mileage may vary

If you want to compile this yourself, download this:
https://www.microchip.com/mplab/avr-support/avr-and-arm-toolchains-c-compilers

Add it to your path, direct yourself to the project folder and run make
