#include <iostream>

#include <QImage>
#include <QPixmap>
#include <QFileDialog>
#include <QImageWriter>

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
    ui->buttonSave->setDisabled(true);

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
MainWindow::SaveImage()
{
    QString fname = QFileDialog::getSaveFileName( this, "Save Image", "image.png", "Images (*.png)");

    if ( fname.length() )
    {
        // Save file
        QImageWriter writer(fname);
        QImage image = ui->labelImage->pixmap().toImage();

        // TODO: QImageWriter does not support EXIF metadata, must use an external library
        /*
        QString comments = QString("(%1,%2)->(%3,%4), iter: %5, w: %6, h: %7, tc: %8, ss: %9, msec: %10")
            .arg(m_calc_result.x1)
            .arg(m_calc_result.y1)
            .arg(m_calc_result.x2)
            .arg(m_calc_result.y2)
            .arg(m_calc_result.iter_mx)
            .arg(m_calc_result.img_width)
            .arg(m_calc_result.img_height)
            .arg(m_calc_result.th_cnt)
            .arg(m_calc_ss)
            .arg(m_calc_result.time_ms);

        writer.setFormat("PNG");
        writer.setText("Title", "Mandelbrot Image");
        writer.setText("Comments", comments );
        */

        writer.write( image );
    }
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

    m_calc_ss = ui->spinBoxSuperSample->value();

    params.res = ui->lineEditResolution->text().toUShort() * m_calc_ss;
    params.iter_mx = ui->lineEditIterMax->text().toUShort();
    params.th_cnt = ui->spinBoxThreadCount->value();
    params.x1 = -0.5;
    params.y1 = .5;
    params.x2 = .5;
    params.y2 = 1.5;

    m_calc_result = m_calc.calculate( params );

    drawImage();

    ui->labelImageInfo->setText(
        QString("(%1,%2)->(%3,%4), iter: %5, w: %6, h: %7, tc: %8, ss: %9, msec: %10")
            .arg(m_calc_result.x1)
            .arg(m_calc_result.y1)
            .arg(m_calc_result.x2)
            .arg(m_calc_result.y2)
            .arg(m_calc_result.iter_mx)
            .arg(m_calc_result.img_width)
            .arg(m_calc_result.img_height)
            .arg(m_calc_result.th_cnt)
            .arg(m_calc_ss)
            .arg(m_calc_result.time_ms)
    );

    ui->buttonSave->setDisabled(false);
}

void
MainWindow::drawImage()
{
    uchar *imbuffer = renderImage();

    QImage image(imbuffer, m_calc_result.img_width, m_calc_result.img_height, QImage::Format_ARGB32, [](void* a_data){
            delete[] (uchar*)a_data;
        }, imbuffer );

    if ( m_calc_ss > 1 )
    {
        ui->labelImage->setPixmap(QPixmap::fromImage(image.scaled( m_calc_result.img_width / m_calc_ss, m_calc_result.img_height / m_calc_ss, Qt::KeepAspectRatio, Qt::SmoothTransformation )));
    }
    else
    {
        ui->labelImage->setPixmap(QPixmap::fromImage(image));
    }
}


uchar *
MainWindow::renderImage()
{
    int imstride = m_calc_result.img_width*4;
    uchar *imbuffer = new uchar[imstride*m_calc_result.img_height];
    int x;
    const uint16_t * itbuf;
    uint32_t *imbuf;

    const std::vector<uint32_t> & palette = m_palette.render( m_palette_scale );

    itbuf = m_calc_result.img_data;

    // Must reverse y-axis due to difference in mathematical and graphical origin
    for ( int y = m_calc_result.img_height - 1; y > -1; y-- )
    {
        imbuf = (uint32_t *)(imbuffer + y*imstride);

        for ( x = 0; x < m_calc_result.img_width; x++, itbuf++ )
        {
            if ( *itbuf == 0 )
            {
                *imbuf++ = 0xFF000000;
            }
            else
            {
                *imbuf++ = palette[(*itbuf + m_palette_offset)% palette.size()];
            }
        }
    }

    return imbuffer;
}

