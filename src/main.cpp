#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>

#include "status_bar.hpp"
#include "modules/modules.hpp"

int main(void) {
    VolumeModule     volume     ("volume",     "#ECB496", "Master");
    MicrophoneModule microphone ("microphone", "#EAAC8B", "Capture");
    LanguageModule   language   ("language",   "#E88C7D", { {0, "EN"}, {1, "RU"} });
    DateModule       date       ("date",       "#E56B6F", "%a, %d %b %I:%M %p");

    StatusBar status_bar("  ", "#2E3440", {&volume, &microphone, &language, &date});

    status_bar.update_all();

    // TODO: main loop => wait for 60s then update_all OR wait for IPC then update(module)

    return 0;
};