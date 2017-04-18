## AdaLight Driver in C++

This is a port to C++ of the Processing code for AdaLight. The performance is much better on higher resolution displays
such as 4K screens.

> "Adalight" is a do-it-yourself facsimile of the Philips Ambilight concept
> for desktop computers and home theater PCs.  This is the host PC-side code
> written in Processing, intended for use with a USB-connected Arduino
> microcontroller running the accompanying LED streaming code.  Requires one
> or more strands of Digital RGB LED Pixels (Adafruit product ID #322,
> specifically the newer WS2801-based type, strand of 25) and a 5 Volt power
> supply (such as Adafruit #276).  You may need to adapt the code and the
> hardware arrangement for your specific display configuration.
> Screen capture adapted from code by Cedrik Kiefer (processing.org forum)

### How to use this project

Start with the original [AdaLight guide](https://learn.adafruit.com/adalight-diy-ambient-tv-lighting/pieces?view=all)
to make your AdaLight and get the Arduino sketch up and running. This driver program runs on the PC and communicates
with the same serial protocol as the Processing script referenced in the guide, but with much lower overhead.

### Building the CppDriver project

*You can use pre-built binaries instead if you prefer, I put them in [AdaLight-Cpp.zip](../AdaLight-Cpp.zip). To run these binaries on a system that does not have Visual Studio 2015 installed, you will need to install the [Visual C++ 2015 Redistributable Packages (x86 version)](https://www.microsoft.com/en-us/download/details.aspx?id=48145).*

At the point where the guide says to download Processing, instead you will need Visual Studio with the basic C++
features enabled; you don't need the Windows XP support or MFC libraries for this project. If you don't already
have Visual Studio installed, you can get a free Community edition from https://www.visualstudio.com/free-developer-offers/.

### Configuring your displays

When you have AdaLight.exe built, look for [AdaLight.config.json](./AdaLight.config.json) and follow along with
the comment blocks to make it match your own setup configuration. Pay particular attention to the `displays` member, that's
where you'll define the layout of your LEDs around the edge of the monitor. Put your customized configuration file in the
same working directory as AdaLight.exe, then try running AdaLight.exe.

If you get stuck and need some help, go ahead and try the original Processing version. It has a preview window that
lets you visualize which blocks of the screen are getting mapped to the LEDs. The configuration of `displays` and
`leds` in AdaLight.pde maps almost exactly to `displays` in settings.h, but without the extra display index field
for each LED.

### But Why?

The AdaLight.pde script is well optimized, and it has some nice features like the well commented settings block and
a preview window. It's great for getting started, but it falls over if you try to run it for long with a 4K display.
On my machine, the screenshots eat up too much memory and take too long to push to system memory from the GPU for
sampling. The best framerate I could manage with Processing was ~14 FPS at 4K, and it took 10% CPU with 1.5 GB of
RAM to do that.

By porting the core program logic to C++ and using the DirectX 11 `IDXGIOutputDuplication` interface, I was able
to boost the framerate to 29 FPS with 2% - 4% CPU and 49 MB of RAM. I'm also able to listen for some lower level
Windows notifications, so this program works much better if you switch between multiple users, lock your screen,
use remote desktop, or just want to put your computer to sleep. Of course, your mileage may vary, but if you intend
to use AdaLight for more immersive gaming experiences, this makes its resource usage much more acceptable.

Thanks for trying it out, hope you like it!
