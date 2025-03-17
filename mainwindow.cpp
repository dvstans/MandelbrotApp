#include <iostream>

#include <QImage>
#include <QPixmap>

#include "mainwindow.h"
#include "ui_mainwindow.h"

using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_calc(true,8)
    , m_palette_scale(1)
    , m_palette_offset(0)
    , m_ignore_pal_sig(false)
    , m_ignore_off_sig(false)
{
    //setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    //setWindowFlags(Qt::Window);
    ui->setupUi(this);
    ui->menubar->hide();
    ui->lineEditResolution->setValidator( new QIntValidator(8, 7680, this) );
    ui->lineEditIterMax->setValidator( new QIntValidator(1, 65536, this) );

    m_ignore_pal_sig = true;

    // Add built-in palettes
    m_palette_map["Default"] = {
        {0xFFFF0000,10,Palette::CM_LINEAR},
        {0xFFFFFF00,10,Palette::CM_LINEAR},
        {0xFF00FF00,10,Palette::CM_LINEAR},
        {0xFF00FFFF,10,Palette::CM_LINEAR},
        {0xFF0000FF,10,Palette::CM_LINEAR},
        {0xFFFF00FF,10,Palette::CM_LINEAR}
    };

    ui->comboBoxPalette->addItem("Default");

    m_palette_map["Stripes"] = {
        {0xFF000000,1,Palette::CM_FLAT},
        {0xFFFFFFFF,1,Palette::CM_FLAT}
    };

    ui->comboBoxPalette->addItem("Stripes");

    ui->comboBoxPalette->setCurrentIndex(0);
    m_ignore_pal_sig = false;

    m_palette.setColors( m_palette_map["Default"] );

    uint32_t ps = m_palette.getPaletteSize();

    m_ignore_off_sig = true;
    ui->sliderPalOffset->setMaximum( ps );
    m_ignore_off_sig = false;


}

MainWindow::~MainWindow()
{
    delete ui;
}

// Public slots
void
MainWindow::Exit()
{
    m_calc.stop();
    close();
}

void
MainWindow::PaletteChange( const QString &a_text )
{
    if ( m_ignore_pal_sig )
        return;

    m_palette.setColors( m_palette_map[a_text.toStdString()] );

    uint32_t ps = m_palette.getPaletteSize();

    m_ignore_off_sig = true;

    m_palette_offset = 0;

    // Adjust offset slider to width of palette
    ui->sliderPalOffset->setMaximum( ps );

    // Update slider value
    ui->sliderPalOffset->setValue(m_palette_offset);

    m_ignore_off_sig = false;

    if ( m_calc_result.img_data )
    {
        drawImage();
    }
}

void
MainWindow::PaletteOffsetSliderChanged( int a_offset )
{
    if ( m_ignore_off_sig )
    {
        return;
    }

    //cout << "off " << a_offset << endl;

    m_palette_offset = a_offset;

    if ( m_calc_result.img_data )
    {
        drawImage();
    }
}

void
MainWindow::PaletteScaleSliderChanged( int a_scale )
{
    //cout << "scl " << a_scale << endl;

    m_palette_scale = a_scale;

    // Render palette to determine new size
    uint32_t ps1 = m_palette.getPaletteSize();
    m_palette.render( m_palette_scale );
    uint32_t ps2 = m_palette.getPaletteSize();

    m_ignore_off_sig = true;

    // Adjust offset slider to width of palette
    ui->sliderPalOffset->setMaximum( ps2 );

    // Adjust offset based on new scale
    m_palette_offset = round(m_palette_offset*ps2/ps1);

    // Update slider value
    ui->sliderPalOffset->setValue(m_palette_offset);

    m_ignore_off_sig = false;

    if ( m_calc_result.img_data )
    {
        drawImage();
    }
}

void
MainWindow::Calculate()
{
    MandelbrotCalc::CalcParams params;

    params.res = ui->lineEditResolution->text().toUShort();
    params.iter_mx = ui->lineEditIterMax->text().toUShort();
    params.th_cnt = ui->spinBoxThreadCount->value();
    params.x1 = -2;
    params.y1 = -2;
    params.x2 = 2;
    params.y2 = 2;

    m_calc_result = m_calc.calculate( params );

    drawImage();

    ui->labelImageInfo->setText(
        QString("(%1,%2)->(%3,%4), iter: %5, w: %6, h: %7, tc: %8, msec: %9")
            .arg(m_calc_result.x1)
            .arg(m_calc_result.y1)
            .arg(m_calc_result.x2)
            .arg(m_calc_result.y2)
            .arg(m_calc_result.iter_mx)
            .arg(m_calc_result.img_width)
            .arg(m_calc_result.img_height)
            .arg(m_calc_result.th_cnt)
            .arg(m_calc_result.time_ms)
    );
}

void
MainWindow::drawImage()
{
    uchar *imbuffer = renderImage( m_calc_result );

    QImage image(imbuffer, m_calc_result.img_width, m_calc_result.img_height, QImage::Format_ARGB32, [](void* a_data){
            delete[] (uchar*)a_data;
        }, imbuffer );

    ui->labelImage->setPixmap(QPixmap::fromImage(image));
}


uchar *
MainWindow::renderImage( MandelbrotCalc::CalcResult & a_result )
{
    int stride = a_result.img_width*4; // + pad;
    uchar *imbuffer = new uchar[stride*a_result.img_height];
    const uint16_t * res = a_result.img_data;
    int x;
    uint32_t *ibuf;

    const std::vector<uint32_t> & palette = m_palette.render( m_palette_scale );

    // Must reverse y-axis due to difference in mathematical and graphical origin
    for ( int y = a_result.img_height - 1; y > -1; y-- ){
        ibuf = (uint32_t *)(imbuffer + y*stride);

        for ( x = 0; x < a_result.img_width; x++, res++ ){
            if ( *res == 0 )
            {
                *ibuf++ = 0xFF000000;
            }
            else
            {
                *ibuf++ = palette[(*res + m_palette_offset)% palette.size()];
            }
        }
    }

    return imbuffer;
}

