#ifndef PALETTE_H
#define PALETTE_H

#include<cstdint>
#include<vector>

class Palette
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

    Palette();

    void
    setColors( const std::vector<ColorBand> & a_bands );

    uint32_t
    getPaletteSize()
    {
        return m_palette_size*m_scale;
    }

    const std::vector<uint32_t> &
    render( uint8_t scale = 1 );

private:
    void generatePalette();

    std::vector<ColorBand>  m_bands;
    std::vector<uint32_t>   m_palette;
    uint32_t                m_palette_size;
    uint8_t                 m_scale;
};

#endif // PALETTE_H
