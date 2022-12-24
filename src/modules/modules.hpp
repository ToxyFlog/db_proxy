#ifndef _MODULES_H
#define _MODULES_H

#include <string>
#include <unordered_map>

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
private:
    std::unordered_map<unsigned char, std::string> group_to_name;
public:
    LanguageModule(std::string _name, std::string _color, std::unordered_map<unsigned char, std::string> _group_to_name): Module(_name, _color), group_to_name(_group_to_name){};
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