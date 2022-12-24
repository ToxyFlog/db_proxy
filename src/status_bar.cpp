#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/X.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include "status_bar.hpp"
#include "modules/modules.hpp"

StatusBar::StatusBar(std::string  _delimiter, std::string _background, std::vector<Module*> _modules): delimiter(_delimiter), background(_background), modules(_modules), values(_modules.size(), "") {}; 

void StatusBar::update_all(void) {
    for (short i = 0;i < modules.size();i++) {
        values[i] = modules[i]->update();
    }

    display();
};

void StatusBar::update(std::string &module) {
    short i;
    for (i = 0;i < modules.size() && modules[i]->name != module;i++);
            
    values[i] = modules[i]->update();
    
    display();
};

void StatusBar::display(void) const {
    std::string status_bar = "^b" + background + "^ ";

    bool is_first = true;
    for (short i = 0;i < values.size();i++) {
        if (!is_first) status_bar += delimiter;

        status_bar += "^c" + modules.at(i)->color + "^" + values.at(i);
        
        is_first = false;
    }

    XTextProperty *text = new XTextProperty();
    text->value = (unsigned char *) status_bar.c_str();
    text->encoding = (Atom) "STRING";
    text->format = 8;
    text->nitems = 1;

    Display *d = XOpenDisplay(0);
    int s = DefaultScreen(d);
    XSetWMName(d, RootWindow(d, s), text);



    char *args[] = {"/usr/bin/xsetroot", "-name", (char *) status_bar.c_str(), NULL};
    execvp(args[0], args);
};