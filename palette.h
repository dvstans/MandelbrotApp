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
    setPaletteColorBands( const ColorBands & a_bands, bool a_repeat = true );

    const Palette &
    renderPalette( uint8_t scale = 1 );

    size_t
    size()
    {
        return m_palette_size*m_scale;
    }

    bool
    repeats()
    {
        return m_repeat;
    }

private:
    void        generatePalette();

    ColorBands  m_bands;
    Palette     m_palette;
    uint32_t    m_palette_size;
    uint8_t     m_scale;
    bool        m_repeat;
};

#endif // PALETTE_H
