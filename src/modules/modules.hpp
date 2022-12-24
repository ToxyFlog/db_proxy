#ifndef _MODULES_H
#define _MODULES_H

#include <string>

#include "module.hpp"

class VolumeModule : public AudioModule {
public:
    using AudioModule::AudioModule;
    std::string update(void) const;
};

class MicrophoneModule : public AudioModule {
public:
    using AudioModule::AudioModule;
    std::string update(void) const;
};

class LanguageModule : public Module {
public:
    using Module::Module;
    std::string update(void) const;
};

class DateModule : public Module {
private:
    const std::string format; 
public:
    DateModule(std::string _name, std::string _color, std::string _format): Module(_name, _color), format(_format) {};
    std::string update(void) const;
};

#endif // _MODULES_H