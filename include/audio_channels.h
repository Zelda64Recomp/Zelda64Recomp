#pragma once

typedef enum AudioChannelsSetting { 
    audioStereo, 
    audioMatrix51, 
    audioRaw51, 
    audioMax 
} AudioChannelsSetting;

inline const char* AudioChannelsSettingName(AudioChannelsSetting setting) {
    switch (setting) {
        case audioStereo:
            return "Stereo";
        case audioMatrix51:
            return "5.1 Matrix";
        case audioRaw51:
            return "5.1 Raw";
        default:
            return "Unknown";
    }
}
