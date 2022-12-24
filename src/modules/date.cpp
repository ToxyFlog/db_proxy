#include <string>
#include <locale>
#include <iostream>

#include "modules.hpp"

static const std::size_t date_size = 25;

std::string DateModule::update(void) const {
    char date[date_size];
    const std::time_t time = std::time(nullptr);

    strftime(date, date_size, format.c_str(), std::localtime(&time));

    return date;
};