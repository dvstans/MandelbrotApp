#include<cmath>
#include "palette.h"

using namespace std;

PaletteGenerator::PaletteGenerator():
    m_palette_size(0),
    m_scale(1)
{}

void
PaletteGenerator::setPaletteColorBands( const ColorBands & a_bands, bool a_repeat )
{
    m_bands = a_bands;
    m_palette.resize(0);
    m_palette_size = 0;
    m_repeat = a_repeat;

    for ( ColorBands::const_iterator cb = m_bands.begin(); cb != m_bands.end(); cb++ )
    {
        if ( !m_repeat && cb == m_bands.end() - 1)
        {
            m_palette_size++;
            continue;
        }
        else
        {
            m_palette_size += cb->width;
        }
    }

    generatePalette();
}

const PaletteGenerator::Palette &
PaletteGenerator::renderPalette( uint8_t a_scale )
{
    if ( m_palette.size() == 0 || a_scale != m_scale )
    {
        m_scale = a_scale;

        generatePalette();
    }

    return m_palette;
}

void
PaletteGenerator::generatePalette()
{
    m_palette.resize(m_palette_size*m_scale - (m_repeat?0:m_scale-1));

    ColorBands::const_iterator cb2;
    uint32_t * color = m_palette.data();
    uint32_t i, w;
    uint8_t r, g, b;
    uint8_t r1, g1, b1;
    uint8_t r2, g2, b2;
    double  dr,dg,db;

    for ( ColorBands::const_iterator cb = m_bands.begin(); cb != m_bands.end(); cb++ )
    {
        if ( !m_repeat && cb == m_bands.end() - 1)
        {
            *color++ = cb->color;
            continue;
        }

        w = cb->width * m_scale;

        if ( cb->mode == CM_LINEAR )
        {
            r1 = (cb->color >> 16) & 0xFF;
            g1 = (cb->color >> 8) & 0xFF;
            b1 = cb->color & 0xFF;

            if ( cb < m_bands.end() - 1 )
            {
                cb2 = cb + 1;
            }
            else
            {
                cb2 = m_bands.begin();
            }

            r2 = (cb2->color >> 16) & 0xFF;
            g2 = (cb2->color >> 8) & 0xFF;
            b2 = cb2->color & 0xFF;

            dr = (double)(r2 - r1)/w;
            dg = (double)(g2 - g1)/w;
            db = (double)(b2 - b1)/w;

            for ( i = 0; i < w; i++ )
            {
                r = (uint8_t)round(r1 + i*dr);
                g = (uint8_t)round(g1 + i*dg);
                b = (uint8_t)round(b1 + i*db);
                *color++ = 0xFF000000 | r << 16 | g << 8 | b;
            }
        }
        else // CM_FLAT
        {
            for ( i = 0; i < w; i++ )
            {
                *color++ = cb->color;
            }
        }
    }
}
