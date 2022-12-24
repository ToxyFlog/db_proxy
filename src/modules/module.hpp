#ifndef _MODULE_H
#define _MODULE_H

#include <string>
#include <alsa/asoundlib.h>

class Module {
public:
    const std::string name;
    const std::string color;

    Module() = default;
    Module(std::string _name, std::string _color): name(_name), color(_color) {};

    virtual std::string update(void) const { throw "Module::update method should be implemented!"; };
};

class AudioModule : public Module {
private:
    snd_mixer_t *handle;
    const char *device_name;
protected:
    snd_mixer_elem_t* element;
public:
    AudioModule(std::string _name, std::string _color, char *_device_name);
    ~AudioModule();

    using Module::update;
};

#endif // _MODULE_H