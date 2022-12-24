#ifndef _STATUS_BAR_H
#define _STATUS_BAR_H

#include <string>
#include <unordered_map>

#include "modules/modules.hpp"

class StatusBar {
private:
    const std::string delimiter;
    const std::string background;
    std::vector<Module*> modules;
    std::vector<std::string> values;
public:
    StatusBar(std::string _delimiter, std::string _background, std::vector<Module*> _modules);

    void update(std::string &module);
    void update_all(void);
    void display(void) const;
};

#endif // _STATUS_BAR_H