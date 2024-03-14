# Basilisk
Basilisk is a plugin to help orchestrate simultaneous motion capture for Live Link and Rokoko, and help automate the importing process.

## Setup
This is NOT a plug-and-play plugin, and some manual setup is required. Knowledge of C++, blueprints, python and conda environments helps a lot. We can't really offer support on this plugin, since we are heads-down focused on our own project.

### Metahuman Identity Setup
1) You will need to set up your own metahuman identity. You should put this in `Basilisk Content/Capture/Actors`. See https://www.youtube.com/watch?v=qPhn28Jk3Mo for a setup tutorial.
2) Afterwards, edit `Basilisk Content/Capture/MH_Performance_Importer` and set the metahuman identity to your new identity.

### Conda Setup
The python scripts in this library are expected to run under the PantomimeVoice conda environment. There is a yml file to set this up, so just follow standard Conda tutorials.

### Importer Setup
1) Right Click `Basilisk Content/BP_BasiliskUtilityWidget` and run the editor utility widget. Press the yellow icons to set the folders for:
    a) Your face mocap import path (this will be created once you start importing iPhone data from your phone)
    b) Your body mocap path (ie Rokoko export folder)
    c) Your conda root path. Generally this is `C:/Users/zacha/anaconda3/envs`

### Phone Shortcuts Setup
Refer to the video for shortcut setup https://youtu.be/rPDjWxF4Qyg

### Phone Voice Commands Setup
Refer to the video for voice command setup https://youtu.be/rPDjWxF4Qyg

### Live Link / iPhone Importing Setup
Refer to the video for OSC server setup https://youtu.be/rPDjWxF4Qyg
Set up `Basilisk Content/Capture/Devices/CS_iPhoneCapture0` to point to your iPhone IP. I suggest making a bunch of these to cover the entire local IP range that your phone may be.
