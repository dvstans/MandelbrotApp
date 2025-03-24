#ifndef PALETTE_H
#define PALETTE_H

#include<cstdint>
#include<vector>

class PaletteGenerator
{
public:
    enum ColorMode : uint8_t
    {
        CM_FLAT = 0,
        CM_LINEAR
    };

    struct ColorBand
    {
        uint32_t    color;
        uint16_t    width;
        ColorMode   mode;
    };

    typedef std::vector<ColorBand>  ColorBands;
    typedef std::vector<uint32_t>   Palette;

    PaletteGenerator();

    void
    setPaletteColorBands( const ColorBands & a_bands );

    const Palette &
    renderPalette( uint8_t scale = 1 );

    size_t
    getPaletteSize()
    {
        return m_palette_size*m_scale;
    }


private:
    void        generatePalette();

    ColorBands  m_bands;
    Palette     m_palette;
    uint32_t    m_palette_size;
    uint8_t     m_scale;
};

#endif // PALETTE_H
