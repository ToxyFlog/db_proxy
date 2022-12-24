#include <string>
#include <alsa/asoundlib.h>

#include "modules.hpp"

static const std::string MUTED_MICROPHONE_ICON = "";
static const std::string MICROPHONE_ICON       = "";

std::string MicrophoneModule::update(void) const {
    int val;
    snd_mixer_selem_get_capture_switch(element, SND_MIXER_SCHN_FRONT_LEFT, &val);

    return val ? MICROPHONE_ICON : MUTED_MICROPHONE_ICON;
};