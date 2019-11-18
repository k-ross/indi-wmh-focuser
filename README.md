# indi-wmh-focuser
INDI focus driver for Waveshare Motor HAT stepper motor controller board. This allows controlling a stepper motor directly from a Raspberry Pi, with no external controller necessary. Additionally, the HAT can power the Raspberry Pi, so only a single 12V power connection will be needed, instead of a 12V and a 5V connection.

# Building
This assumes you already have INDI installed.

First, let's install some build tools, if you haven't already.
```
sudo apt install build-essential cmake
```

If you built INDI from source, you can skip this step. Otherwise, you must install the INDI development libraries.
```
sudo apt install libindi-dev
```

We need wiringPi, so let's install it.
```
sudo apt install wiringpi
```

Okay, we're ready to get the source and build it.
```
git clone https://github.com/k-ross/indi-wmh-focuser.git
cd indi-wmh-focuser
mkdir build
cd build
cmake ..
make
sudo make install
```
If you're going to run KStars on a computer other than your Raspberry Pi, then you need to install an XML file to the computer with KStars. Open KStars, go to Settings, then Configure KStars, then go down to INDI. There will be an entry titled "INDI Drivers XML Directory". That directory is where you will want to copy the indi_wmh_focuser.xml file. On my Windows machine, that location is "C:/Users/Kevin/AppData/Local/indi"

# Running
Start your INDI server with Waveshare Motor HAT driver:

`indiserver <your normal indiserver arguments> indi_wmh_focuser`

Start KStars with Ekos, edit your profile (or create a new one), and select "Waveshare Motor HAT Focuser" in the Focuser dropdown. If you don't see Waveshare Motor HAT Focuser as an option, then you didn't copy the XML file to the right place, or you need to restart KStars after copying the XML file.

Now you can enjoy autofocus!
