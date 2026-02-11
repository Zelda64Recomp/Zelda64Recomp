#pragma once

typedef enum AudioChannelsSetting { 
    audioStereo, 
    audioMatrix51, 
    audioMax 
} AudioChannelsSetting;

inline const char* AudioChannelsSettingName(AudioChannelsSetting setting) {
    switch (setting) {
        case audioStereo:
            return "Stereo";
        case audioMatrix51:
            return "5.1 Matrix";
        default:
            return "Unknown";
    }
}
