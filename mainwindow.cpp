#include <iostream>

#include <QImage>
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <QFileDialog>
#include <QImageWriter>
#include <QMessageBox>
#include <QInputDialog>
#include <QStringList>

#include "mainwindow.h"
#include "ui_mainwindow.h"


using namespace std;

MainWindow::MainWindow(QWidget *parent):
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_settings(QSettings::UserScope),
    m_calc(true,8),
    m_palette_edit_dlg(this,*this),
    m_palette_dlg_edit_init(true),
    m_palette_scale(1),
    m_palette_offset(0),
    m_ignore_pal_sig(false),
    m_ignore_off_sig(false),
    m_calc_history_idx(0)
{
    setWindowTitle("Mandelbrot App");
    ui->setupUi(this);

    // Setup MandelbrotViewer
    m_viewer = new MandelbrotViewer( *ui->frameViewer, *this );

    m_calc_params.x1 = -2;
    m_calc_params.y1 = -2;
    m_calc_params.x2 = 2;
    m_calc_params.y2 = 2;

    // Adjust various UI components
    ui->menubar->hide();
    ui->lineEditResolution->setValidator( new QIntValidator(8, 7680, this) );
    ui->lineEditIterMax->setValidator( new QIntValidator(1, 65536, this) );
    ui->buttonLoad->setDisabled(true); // TODO Reenable when feature implemented
    ui->buttonSave->setDisabled(true);
    ui->buttonTop->setDisabled(true);
    ui->buttonNext->setDisabled(true);
    ui->buttonPrev->setDisabled(true);

    // Because Qt doesn't allow widgets to be sized with points, must adjust fixed size line edits manually
    // QLineEdit has hidden padding, thus string to set width had to be determined experimentally
    // (i.e. Qt does not accomodate proper scaling on different display DPI)
    QFontMetrics fm = ui->lineEditIterMax->fontMetrics();
    ui->lineEditIterMax->setMinimumWidth( 10 );
    ui->lineEditIterMax->setMaximumWidth( fm.horizontalAdvance("00000000") );
    ui->lineEditResolution->setMinimumWidth( 10 );
    ui->lineEditResolution->setMaximumWidth( fm.horizontalAdvance("0000000") );

    // Setup aspect ratios
    m_aspect_ratios = {
        {"Any",0,0},
        {"32:9",32,9},
        {"21:9",21,9},
        {"16:10",16,10},
        {"16:9",16,9},
        {"5:4",5,4},
        {"5:3",5,3},
        {"4:3",4,3},
        {"3:2",3,2},
        {"1:1",1,1}
    };

    m_ignore_aspect_sig = true;
    for ( vector<AspectRatio>::const_iterator a = m_aspect_ratios.begin(); a != m_aspect_ratios.end(); a++ )
    {
        ui->comboBoxAspect->addItem(a->name);
    }

    ui->comboBoxAspect->setCurrentIndex( 0 );
    m_viewer->setAspectRatio( 0, 0 );
    m_ignore_aspect_sig = false;

    // Setup built-in palettes

    m_ignore_pal_sig = true;

    m_palette_map["Default"] =
    {
        "Default",
        {
            {0xFF0000FF,10,PaletteGenerator::CM_LINEAR},
            {0xFFFF00FF,10,PaletteGenerator::CM_LINEAR},
            {0xFFFF0000,10,PaletteGenerator::CM_LINEAR},
            {0xFFFFFF00,10,PaletteGenerator::CM_LINEAR},
            {0xFF00FF00,10,PaletteGenerator::CM_LINEAR},
            {0xFF00FFFF,10,PaletteGenerator::CM_LINEAR}
        },
        false,
        true,
        false
    };

    ui->comboBoxPalette->addItem("Default");

    m_palette_map["Mono"] =
    {
        "Mono",
        {
            {0xFF000000,1,PaletteGenerator::CM_FLAT},
            {0xFFFFFFFF,1,PaletteGenerator::CM_FLAT}
        },
        true,
        true,
        false
    };

    ui->comboBoxPalette->addItem("Mono");

    m_palette_map["Fire"] =
    {
        "Fire",
        {
            {0xFFFF0000,10,PaletteGenerator::CM_LINEAR},
            {0xFFFFFF00,10,PaletteGenerator::CM_LINEAR}
        },
        true,
        true,
        false
    };

    ui->comboBoxPalette->addItem("Fire");

    ui->comboBoxPalette->setCurrentIndex(0);
    m_ignore_pal_sig = false;

    // Load custom palettes from settings
    settingsPaletteLoadAll();

    // Set initial palette
    PaletteInfo & pal_info = m_palette_map["Default"];
    m_palette_gen.setPaletteColorBands( pal_info.color_bands, pal_info.repeat );
    m_palette_edit_dlg.setPaletteInfo( pal_info );

    // Setup palette sliders
    m_ignore_off_sig = true;
    ui->sliderPalOffset->setMaximum( m_palette_gen.size() );
    m_ignore_off_sig = false;

    Calculate();
}

MainWindow::~MainWindow()
{
    delete ui;
    m_calc.stop();
}

// Public slots
void
MainWindow::closeEvent( QCloseEvent *event )
{
    // Check if any unsaved palettes
    bool unsaved = false;
    for ( PaletteMap_t::const_iterator p = m_palette_map.begin(); p != m_palette_map.end(); p++ )
    {
        if ( !p->second.built_in && p->second.changed )
        {
            unsaved = true;
            break;
        }
    }

    if ( unsaved )
    {
        QMessageBox mb( QMessageBox::Warning, "Mandlebrot App", "There are unsaved palettes - exit anyway?", QMessageBox::Ok | QMessageBox::Cancel, this );
        if ( mb.exec() == QMessageBox::Cancel )
        {
            event->ignore();
            return;
        }
    }
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
MainWindow::aspectChange( int a_index )
{
    if ( m_ignore_aspect_sig )
        return;

    m_viewer->setAspectRatio( m_aspect_ratios[a_index].major, m_aspect_ratios[a_index].minor );
}


void
MainWindow::paletteSelect( const QString &a_text )
{
    if ( m_ignore_pal_sig )
        return;

    PaletteInfo & pal_info = m_palette_map[a_text.toStdString()];
    m_palette_gen.setPaletteColorBands( pal_info.color_bands, pal_info.repeat );
    m_palette_edit_dlg.setPaletteInfo( pal_info );

    uint32_t ps = m_palette_gen.size();

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
MainWindow::paletteEdit()
{
    if ( m_palette_edit_dlg.isHidden() )
    {
        m_palette_edit_dlg.showWithPos();

        if ( m_palette_dlg_edit_init )
        {
            m_palette_dlg_edit_init = false;
            QRect rect1 = m_palette_edit_dlg.geometry();
            QRect rect2 = this->geometry();
            rect1.moveRight( rect2.x() + rect2.width() );
            m_palette_edit_dlg.setGeometry( rect1 );
        }
    }
}


void
MainWindow::PaletteOffsetSliderChanged( int a_offset )
{
    if ( m_ignore_off_sig )
    {
        return;
    }

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
    uint32_t ps1 = m_palette_gen.size();
    m_palette_gen.renderPalette( m_palette_scale );
    uint32_t ps2 = m_palette_gen.size();

    cout << "scale old sz " << ps1 << " new " << ps2 << endl;

    m_ignore_off_sig = true;

    // Adjust offset slider to width of palette
    ui->sliderPalOffset->setMaximum( ps2 );

    // Adjust offset based on new scale
    m_palette_offset = m_palette_offset*ps2/ps1;
    //m_palette_offset = round(m_palette_offset*ps2/ps1);

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
    }
    else
    {
        m_viewer->setImage( image );
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

/**
 * @brief Callback from palette edit dialog to notify main window that palette has been altered
 *
 * This method is called by the PaletteEditDialog whenever the user changes the color content
 * of the currently selected palette. The main window will redraw the current image, and, if
 * the palette is not built-in, it will update the stored palette map and marked it as changed.
 */
void
MainWindow::paletteChanged()
{
    PaletteInfo & pal = m_palette_edit_dlg.getPaletteInfo();
    m_palette_repeat = pal.repeat;
    m_palette_gen.setPaletteColorBands( pal.color_bands, pal.repeat );

    if ( !pal.built_in )
    {
        pal.changed = true;
        m_palette_map[pal.name] = pal;
    }

    drawImage();
}

void
MainWindow::paletteNew()
{
    // Get palette name
    QString input = inputPaletteName( "New Palette" );

    if ( input.length() )
    {
        string name = input.toStdString();

        m_palette_map[name] =
            {
                name,
                {
                    {0xFFFFFFFF,5,PaletteGenerator::CM_LINEAR},
                    {0xFF000000,5,PaletteGenerator::CM_LINEAR}
                },
                true,
                false,
                true
            };

        m_ignore_pal_sig = true;
        ui->comboBoxPalette->addItem( input );
        ui->comboBoxPalette->setCurrentIndex(ui->comboBoxPalette->count()-1);
        m_ignore_pal_sig = false;

        m_palette_gen.setPaletteColorBands( m_palette_map[name].color_bands, true );
        m_palette_edit_dlg.setPaletteInfo( m_palette_map[name] );

        drawImage();
    }
}


void
MainWindow::paletteDuplicate( const PaletteInfo & a_pal_info )
{
    // Get palette name
    QString input = inputPaletteName( "Duplicate Palette" );

    if ( input.length() )
    {
        string name = input.toStdString();

        PaletteInfo info = {
            name,
            a_pal_info.color_bands,
            a_pal_info.repeat,
            false,
            true
        };

        m_palette_map[name] = info;

        m_ignore_pal_sig = true;
        ui->comboBoxPalette->addItem( input );
        ui->comboBoxPalette->setCurrentIndex(ui->comboBoxPalette->count()-1);
        m_ignore_pal_sig = false;

        m_palette_gen.setPaletteColorBands( info.color_bands, a_pal_info.repeat );
        m_palette_edit_dlg.setPaletteInfo( info );
    }
}

QString
MainWindow::inputPaletteName( const QString & a_title )
{
    bool ok;
    QString input = "name";

    while( 1 )
    {
        input = QInputDialog::getText( this, QString("Mandelbrot App - %1").arg(a_title),
                        "Name (max 20 char, alphanumeric only):", QLineEdit::Normal, input, &ok );
        if ( !ok )
            break;

        // Enforce max length
        if ( input.length() > 20 )
        {
            QMessageBox mb( QMessageBox::Warning, "Mandlebrot App Error", "Palette name too long.", QMessageBox::Ok, this );
            mb.exec();
            continue;
        }

        // Enforce character set
        // TODO convert to regular expression
        for ( QString::ConstIterator c = input.cbegin(); c != input.cend(); c++ )
        {
            if ( !isalnum( c->toLatin1() ))
            {
                QMessageBox mb( QMessageBox::Warning, "Mandlebrot App Error", "Palette name contains invalid characters.", QMessageBox::Ok, this );
                mb.exec();
                ok = false;
                break;
            }
        }

        if ( !ok )
        {
            continue;
        }

        // Make sure name doesn't already exist
        if ( m_palette_map.find( input.toStdString() ) != m_palette_map.end() )
        {
            QMessageBox mb( QMessageBox::Warning, "Mandlebrot App Error", "Palette name already exists.", QMessageBox::Ok, this );
            mb.exec();
            continue;
        }

        return input;
    }

    return "";
}


void
MainWindow::paletteSave( PaletteInfo & a_pal_info )
{
    cout << "pal save" << endl;

    // Saving built-in palette triggers Save As processing
    if ( a_pal_info.built_in )
    {
        QMessageBox mb( QMessageBox::Warning, "Mandlebrot App Error", "Cannot save built-in palette.", QMessageBox::Ok, this );
        mb.exec();
    }

    settingsPaletteSave( a_pal_info );
    m_palette_map[a_pal_info.name].changed = false;
}

void
MainWindow::paletteDelete( const PaletteInfo & a_pal_info )
{
    // Can't delete built-in palettes
    if ( a_pal_info.built_in )
    {
        QMessageBox mb( QMessageBox::Warning, "Mandlebrot App Error", "Cannot delete built-in palette.", QMessageBox::Ok, this );
        mb.exec();
    }
    else
    {
        QMessageBox mb( QMessageBox::Question, "Mandlebrot App", QString("Delete palette '%1'?").arg(QString::fromStdString(a_pal_info.name)), QMessageBox::Ok | QMessageBox::Cancel, this );
        int ret = mb.exec();
        if ( ret == QMessageBox::Ok )
        {
            m_ignore_pal_sig = true;
            int index = ui->comboBoxPalette->findText(QString::fromStdString(a_pal_info.name));
            ui->comboBoxPalette->removeItem( index );
            ui->comboBoxPalette->setCurrentIndex(0);
            m_ignore_pal_sig = false;

            m_palette_map.erase( a_pal_info.name );
            settingsPaletteDelete( a_pal_info.name );

            PaletteInfo & pal_info = m_palette_map["Default"];

            m_palette_gen.setPaletteColorBands( pal_info.color_bands, pal_info.repeat );
            m_palette_edit_dlg.setPaletteInfo( pal_info );

            drawImage();
        }
    }
}

// Load all palettes from settings
void
MainWindow::settingsPaletteLoadAll()
{
    cout << "pal load all" << endl;

    m_ignore_pal_sig = true;

    PaletteInfo pi;
    PaletteGenerator::ColorBand cb;
    int i, size;

    pi.built_in = false;
    pi.changed = false;

    m_settings.beginGroup("palettes");

    QStringList keys = m_settings.childGroups();

    for ( QStringList::const_iterator k = keys.cbegin(); k != keys.cend(); k++ )
    {
        pi.name = k->toStdString();
        pi.color_bands.resize(0);

        cout << "pal load " << pi.name << endl;

        size = m_settings.beginReadArray( *k );
        for ( i = 0; i < size; ++i )
        {
            m_settings.setArrayIndex( i );

            cb.color = m_settings.value( "color" ).toUInt();
            cb.width = m_settings.value( "width" ).toUInt();
            cb.mode = (PaletteGenerator::ColorMode)m_settings.value( "mode" ).toUInt();

            pi.color_bands.push_back( cb );
        }

        m_palette_map[pi.name] = pi;
        ui->comboBoxPalette->addItem( QString::fromStdString( pi.name ));

        m_settings.endArray();
    }

    m_settings.endGroup();
    m_ignore_pal_sig = false;
}

void
MainWindow::settingsPaletteSave( PaletteInfo & a_pal_info )
{
    cout << "set pal save " << a_pal_info.name << endl;

    m_settings.beginGroup("palettes");
    m_settings.beginWriteArray( a_pal_info.name );

    int idx = 0;
    for ( PaletteGenerator::ColorBands::const_iterator cb = a_pal_info.color_bands.begin(); cb != a_pal_info.color_bands.end(); cb++, idx++ )
    {
        m_settings.setArrayIndex( idx );
        m_settings.setValue( "color", cb->color );
        m_settings.setValue( "width", cb->width );
        m_settings.setValue( "mode", (int)cb->mode );
    }

    m_settings.endArray();
    m_settings.endGroup();

    a_pal_info.changed = false;
}

void
MainWindow::settingsPaletteDelete( const string & a_palette_name )
{
    m_settings.beginGroup( "palettes" );
    m_settings.remove( a_palette_name );
    m_settings.endGroup();
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
    const uint16_t * itbuf = m_calc_result.img_data;
    uint32_t *imbuf;
    const std::vector<uint32_t> & palette = m_palette_gen.renderPalette( m_palette_scale );
    bool repeats = m_palette_gen.repeats();
    size_t pal_size = palette.size();
    uint32_t pal_lim = m_palette_offset + palette.size();
    uint32_t col_first = palette[0];
    uint32_t col_last = palette[pal_size-1];

    cout << "pal_lim " << pal_lim << endl;
    cout << "col_last " << hex << col_last << endl;

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
                if ( repeats )
                {
                    *imbuf++ = palette[(*itbuf + m_palette_offset) % pal_size];
                }
                else
                {
                    if ( *itbuf < m_palette_offset )
                    {
                        *imbuf++ = col_first;
                    }
                    else if ( *itbuf < pal_lim )
                    {
                        *imbuf++ = palette[*itbuf - m_palette_offset];
                    }
                    else
                    {
                        *imbuf++ = col_last;
                    }
                }
            }
        }
    }

    return imbuffer;
}

