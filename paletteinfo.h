#ifndef PALETTEINFO_H
#define PALETTEINFO_H

#include <string>
#include "palettegenerator.h"

/**
 * @brief The PaletteInfo class contains palette parameters
 */
struct PaletteInfo
{
    std::string                     name;           // Palette name
    PaletteGenerator::ColorBands    color_bands;    // Palette color bands
    bool                            repeat;         // Palette repeat flag
    bool                            built_in;       // Palette built-in (system) flag
    bool                            changed;        // Palette changed flag (unsaved)
};

#endif // PALETTEINFO_H
