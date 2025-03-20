#include <iostream>

#include <QImage>
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <QFileDialog>
#include <QImageWriter>

#include "mainwindow.h"
#include "ui_mainwindow.h"

using namespace std;

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_calc(true,8),
    m_palette_scale(1),
    m_palette_offset(0),
    m_ignore_pal_sig(false),
    m_ignore_off_sig(false),
    m_calc_history_idx(0)
{
    ui->setupUi(this);
    ui->menubar->hide();
    ui->lineEditResolution->setValidator( new QIntValidator(8, 7680, this) );
    ui->lineEditIterMax->setValidator( new QIntValidator(1, 65536, this) );
    ui->buttonSave->setDisabled(true);
    ui->buttonTop->setDisabled(true);
    ui->buttonNext->setDisabled(true);
    ui->buttonPrev->setDisabled(true);

    setWindowTitle("Mandelbrot App");

    // Setup built-in palettes

    m_ignore_pal_sig = true;

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

    // Setup palette sliders
    m_ignore_off_sig = true;
    ui->sliderPalOffset->setMaximum( ps );
    m_ignore_off_sig = false;

    // Setup MandelbrotViewer
    m_viewer = new MandelbrotViewer( *ui->frameViewer, *this );

    m_calc_params.x1 = -2;
    m_calc_params.y1 = -2;
    m_calc_params.x2 = 2;
    m_calc_params.y2 = 2;

    Calculate();
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
        QImage image = m_viewer->getImage();

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
    ui->buttonCalc->setDisabled(true);

    m_calc_ss = ui->spinBoxSuperSample->value();

    m_calc_params.res = ui->lineEditResolution->text().toUShort() * m_calc_ss;
    m_calc_params.iter_mx = ui->lineEditIterMax->text().toUShort();
    m_calc_params.th_cnt = ui->spinBoxThreadCount->value();

    m_calc_result = m_calc.calculate( m_calc_params );

    drawImage();

    setWindowTitle( QString("Mandelbrot App (%1,%2)->(%3,%4), it: %5, w: %6, h: %7, tc: %8, ss: %9, ms: %10")
       .arg(m_calc_result.x1)
       .arg(m_calc_result.y1)
       .arg(m_calc_result.x2)
       .arg(m_calc_result.y2)
       .arg(m_calc_result.iter_mx)
       .arg(m_calc_result.img_width)
       .arg(m_calc_result.img_height)
       .arg(m_calc_result.th_cnt)
       .arg(m_calc_ss)
       .arg(m_calc_result.time_ms));

    /*
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
*/
    ui->buttonCalc->setDisabled(false);
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
        m_viewer->setImage( image.scaled( m_calc_result.img_width / m_calc_ss, m_calc_result.img_height / m_calc_ss, Qt::KeepAspectRatio, Qt::SmoothTransformation ));
        //ui->labelImage->setPixmap(QPixmap::fromImage(image.scaled( m_calc_result.img_width / m_calc_ss, m_calc_result.img_height / m_calc_ss, Qt::KeepAspectRatio, Qt::SmoothTransformation )));
    }
    else
    {
        m_viewer->setImage( image );
        //ui->labelImage->setPixmap(QPixmap::fromImage(image));
    }
}

void
MainWindow::ZoomTop()
{
    m_calc_params.x1 = -2;
    m_calc_params.y1 = -2;
    m_calc_params.x2 = 2;
    m_calc_params.y2 = 2;

    Calculate();

    ui->buttonTop->setDisabled(true);
    ui->buttonNext->setDisabled(m_calc_history.size() == 0);
    ui->buttonPrev->setDisabled(true);
    m_calc_history_idx = 0;
}


void
MainWindow::prev()
{
    if ( m_calc_history_idx > 0 )
    {
        ui->buttonNext->setDisabled(false);

        if( --m_calc_history_idx == 0 )
        {
            ui->buttonTop->setDisabled(true);
            ui->buttonPrev->setDisabled(true);
            m_calc_params.x1 = -2;
            m_calc_params.y1 = -2;
            m_calc_params.x2 = 2;
            m_calc_params.y2 = 2;
        }
        else
        {
            const CalcPos & pos = m_calc_history[m_calc_history_idx-1];
            m_calc_params.x1 = pos.x1;
            m_calc_params.y1 = pos.y1;
            m_calc_params.x2 = pos.x2;
            m_calc_params.y2 = pos.y2;
        }

        Calculate();
    }
}


void
MainWindow::next()
{
    if ( m_calc_history_idx < m_calc_history.size() )
    {
        m_calc_history_idx++;
        ui->buttonTop->setDisabled( false );
        ui->buttonPrev->setDisabled( false );
        ui->buttonNext->setDisabled( m_calc_history_idx >= m_calc_history.size() );

        const CalcPos & pos = m_calc_history[m_calc_history_idx-1];
        m_calc_params.x1 = pos.x1;
        m_calc_params.y1 = pos.y1;
        m_calc_params.x2 = pos.x2;
        m_calc_params.y2 = pos.y2;

        Calculate();
    }
}


void
MainWindow::zoomIn( const QRectF & rect )
{
    //cout << "rect: " << rect.x() << " " << rect.y() << " " << rect.width() << " " << rect.height() << endl;

    // Calc new set coords based on new image coords (rect)
    double sx = (m_calc_params.x2-m_calc_params.x1)*m_calc_ss/m_calc_result.img_width;
    double sy = (m_calc_params.y2-m_calc_params.y1)*m_calc_ss/m_calc_result.img_height;

    m_calc_params.x1 = m_calc_params.x1 + rect.x()*sx;
    m_calc_params.x2 = m_calc_params.x1 + (rect.width()-1)*sx;
    m_calc_params.y1 = m_calc_params.y1 + ((m_calc_result.img_height/m_calc_ss) - (rect.y() + rect.height() - 1))*sy;
    m_calc_params.y2 = m_calc_params.y1 + (rect.height()-1)*sy;

    //cout << "bounds: " << m_calc_params.x1 << " " << m_calc_params.y1 << " " << m_calc_params.x2 << " " << m_calc_params.y2 << endl;

    Calculate();

    CalcPos pos = {m_calc_params.x1,m_calc_params.y1,m_calc_params.x2,m_calc_params.y2};

    // Zooming in truncates any positions past current history index
    m_calc_history.resize(m_calc_history_idx);
    m_calc_history.push_back( pos );
    m_calc_history_idx++;

    ui->buttonTop->setDisabled(false);
    ui->buttonNext->setDisabled(true);
    ui->buttonPrev->setDisabled(false);
}


void
MainWindow::recenter( const QPointF & a_pos )
{
    double sx = (m_calc_params.x2-m_calc_params.x1)*m_calc_ss/m_calc_result.img_width;
    double sy = (m_calc_params.y2-m_calc_params.y1)*m_calc_ss/m_calc_result.img_height;
    double dx = (a_pos.x() - (m_calc_result.img_width/(2*m_calc_ss)))*sx;
    double dy = -(a_pos.y() - (m_calc_result.img_height/(2*m_calc_ss)))*sy;

    m_calc_params.x1 += dx;
    m_calc_params.x2 += dx;
    m_calc_params.y1 += dy;
    m_calc_params.y2 += dy;

    Calculate();

    CalcPos pos = {m_calc_params.x1,m_calc_params.y1,m_calc_params.x2,m_calc_params.y2};

    // Recentering in truncates any positions past current history index
    m_calc_history.resize(m_calc_history_idx);
    m_calc_history.push_back( pos );
    m_calc_history_idx++;

    ui->buttonTop->setDisabled(false);
    ui->buttonNext->setDisabled(true);
    ui->buttonPrev->setDisabled(false);
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

