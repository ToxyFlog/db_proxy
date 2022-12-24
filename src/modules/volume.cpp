#include <string>
#include <math.h>
#include <alsa/asoundlib.h>

#include "modules.hpp"

static const std::string SILENT_SPEAKER_ICON = "奄";
static const std::string LOW_SPEAKER_ICON    = "奔";
static const std::string HIGH_SPEAKER_ICON   = "墳";

std::string VolumeModule::update(void) const {
    long min, max, value;

    snd_mixer_selem_get_playback_volume_range(element, &min, &max);
    snd_mixer_selem_get_playback_volume(element, SND_MIXER_SCHN_FRONT_LEFT, &value);

    int volume_percent = ceilf((value - min) / (double) (max - min) * 100);
    std::string icon;

    if (volume_percent == 0) icon = SILENT_SPEAKER_ICON;
    else if (volume_percent <= 30) icon = LOW_SPEAKER_ICON;
    else icon = HIGH_SPEAKER_ICON;

    return std::to_string(volume_percent) + "% " + icon;
};