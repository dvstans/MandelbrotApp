#ifndef PALETTEGENERATOR_H
#define PALETTEGENERATOR_H

#include<cstdint>
#include<vector>

/**
 * @brief The PaletteGenerator class renders a palette from palette parameters.
 *
 * This class generates a palette of colors that can be used to render images
 * by using the parameters stored in a vector of ColorBand information (i.e.
 * ColorBands). A given ColorBand struct in a palette specified the RGB color
 * of the band, the width of the band, and the color mode to use when rendering.
 * Additionally, palette scale (size multiplier) and repeat (loop) mode may
 * be specified later.
 *
 * Each color band include a color mode that determines how the color is rendered.
 * "Flat" mode simply uses the same color across the full width of the color band;
 * whereas "linear" mode blends the color band with the next color in the palette.
 * Non-repeating palettes do not blend the final color in the palette.
 */
class PaletteGenerator
{
public:
    /**
     * @brief The ColorMode enum specifies how a color is blended in the palette.
     */
    enum ColorMode : uint8_t
    {
        CM_FLAT = 0,    // Flat mode performs no blending
        CM_LINEAR       // Linear mode blends color with next entry in palette
    };

    /**
     * @brief The ColorBand struct contains all parameters for a given color band.
     */
    struct ColorBand
    {
        uint32_t    color;  // RGB color of band
        uint16_t    width;  // Width of band
        ColorMode   mode;   // Blending mode of band
    };

    typedef std::vector<ColorBand>  ColorBands;
    typedef std::vector<uint32_t>   Palette;

    PaletteGenerator();

    void
    setPaletteColorBands( const ColorBands & a_bands, bool a_repeat = true );

    const Palette &
    renderPalette( uint8_t scale = 1 );

    /**
     * @brief Returns the size of the rendered palette
     * @return Rendered palette size
     */
    size_t
    size()
    {
        return m_palette_size*m_scale;
    }

    /**
     * @brief Returns the repeat mode of the palette
     * @return True if palette repeats; false otherwise
     */
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

#endif // PALETTEGENERATOR_H
