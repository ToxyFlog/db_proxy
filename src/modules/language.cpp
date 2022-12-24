#include <string>
#include <unordered_map>
#include <X11/XKBlib.h>

#include "modules.hpp"

std::string LanguageModule::update(void) const {
    XkbStateRec xkbState;
    XkbGetState(XOpenDisplay(nullptr), XkbUseCoreKbd, &xkbState);
    
    return group_to_name.at(xkbState.group);
};