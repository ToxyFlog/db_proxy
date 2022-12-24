#include <string>

#include "module.hpp"

static const char *card = "default";

AudioModule::AudioModule(std::string _name, std::string _color, char *_device_name): Module(_name, _color), device_name(_device_name) {
    snd_mixer_open(&handle, 0);
    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_t *sid;
    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, device_name);

    element = snd_mixer_find_selem(handle, sid);
}

AudioModule::~AudioModule() {
    snd_mixer_close(handle);
}