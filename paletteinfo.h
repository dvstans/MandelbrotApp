#ifndef PALETTEINFO_H
#define PALETTEINFO_H

#include <string>
#include "palette.h"

struct PaletteInfo
{
    std::string                     name;
    PaletteGenerator::ColorBands    color_bands;
    bool                            repeat;
    bool                            built_in;
    bool                            changed;
};

#endif // PALETTEINFO_H
