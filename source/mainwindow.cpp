#include <iostream>

#include <QImage>
#include <QPixmap>
#include <QGraphicsPixmapItem>
#include <QFileDialog>
#include <QImageWriter>
#include <QMessageBox>
#include <QInputDialog>
#include <QStringList>
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>

#include "mainwindow.h"
#include "ui_mainwindow.h"

using namespace std;

/**
 * @brief MainWindow constructor
 * @param parent - Parent widget (null in this case)
 */
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
    m_ignore_scale_sig(false),
    m_ignore_off_sig(false),
    m_calc_history_idx(0),
    m_app_name( QString("MandelbrotApp ") + APP_VERSION )
{
    setWindowTitle( m_app_name );
    ui->setupUi(this);

    // Setup MandelbrotViewer
    m_viewer = new MandelbrotViewer( *ui->frameViewer, *this );

    // Default calc params (captures first circle of Mandelbrot set)
    m_calc_params.x1 = -2;
    m_calc_params.y1 = -2;
    m_calc_params.x2 = 2;
    m_calc_params.y2 = 2;

    // Adjust various UI components
    ui->menubar->hide();
    ui->lineEditResolution->setValidator( new QIntValidator(8, 7680, this) );
    ui->lineEditIterMax->setValidator( new QIntValidator(1, 65536, this) );
    ui->buttonImageSave->setDisabled(true);
    ui->buttonViewTop->setDisabled(true);
    ui->buttonViewNext->setDisabled(true);
    ui->buttonViewPrev->setDisabled(true);
    ui->buttonZoomOut->setDisabled(true);

    // Set max threads to supported H/W concurrency
    unsigned int max_th = std::thread::hardware_concurrency();
    ui->spinBoxThreadCount->setMaximum( max_th );
    ui->spinBoxThreadCount->setValue( max_th * .75 ); // Don't use all by default

    // Because Qt doesn't allow widgets to be sized with font points, we must
    // adjust fixed size line edits manually. QLineEdit has hidden padding,
    // thus string to calculate width had to be determined experimentally
    // (i.e. Qt 'desktop' does not accommodate proper UI scaling based on display DPI)
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

    // Load user-defined palettes from settings
    settingsPaletteLoadAll();

    // Set initial palette
    PaletteInfo & pal_info = m_palette_map["Default"];
    m_palette_gen.setPaletteColorBands( pal_info.color_bands, pal_info.repeat );
    m_palette_edit_dlg.setPaletteInfo( pal_info );

    // Setup palette sliders
    m_ignore_off_sig = true;
    ui->sliderPalOffset->setMaximum( m_palette_gen.size() );
    m_ignore_off_sig = false;

    // Generate initial image
    calculate();
}

/**
 * @brief MainWindow destructor
 */
MainWindow::~MainWindow()
{
    delete ui;
}

//==================== Public slots ====================

/**
 * @brief Slot to receive aspect ratio combo box selection signal
 * @param a_index - Index to the selected aspect ratio item
 */
void
MainWindow::aspectChange( int a_index )
{
    if ( m_ignore_aspect_sig )
        return;

    const AspectRatio & ar = m_aspect_ratios[a_index];
    m_viewer->setAspectRatio( ar.major, ar.minor );
}

/**
 * @brief Slot to receive image calculate / stop UI signals
 *
 * This slot is also use internally to recalculate the image when needed.
 */
void
MainWindow::calculate()
{
    ui->buttonCalc->setDisabled(true);
    //runCalculate();

    // Start calculation in a different processing thread to free UI thread
    //QApplication::processEvents(); // Force UI to update
    //QTimer::singleShot(0, this, &MainWindow::runCalculate );

    m_calc_ss = ui->spinBoxSuperSample->value();
    m_calc_params.res = ui->lineEditResolution->text().toUShort() * m_calc_ss;
    m_calc_params.iter_mx = ui->lineEditIterMax->text().toUShort();
    m_calc_params.th_cnt = ui->spinBoxThreadCount->value();

    // Returns immediately, observer notified on progress/completion
    m_calc.calculate( *this, m_calc_params );
}

/**
 * @brief MainWindow close event handler (slot)
 * @param a_event - Close event
 */
void
MainWindow::closeEvent( QCloseEvent *a_event )
{
    // Check if there any unsaved palettes
    bool unsaved = false;
    for ( PaletteMap_t::const_iterator p = m_palette_map.begin(); p != m_palette_map.end(); p++ )
    {
        if ( !p->second.built_in && p->second.changed )
        {
            unsaved = true;
            break;
        }
    }

    // If there are unsaved palettes, get close confirmation from user
    if ( unsaved )
    {
        QMessageBox mb( QMessageBox::Warning, "Mandelbrot App", "There are unsaved palettes - exit anyway?", QMessageBox::Ok | QMessageBox::Cancel, this );
        if ( mb.exec() == QMessageBox::Cancel )
        {
            // Cancel close
            a_event->ignore();
            return;
        }
    }
}

/**
 * @brief Slot to receive paletteEdit UI signals
 *
 * Toggles the palette edit modeless dialog.
 */
void
MainWindow::paletteEdit()
{
    if ( m_palette_edit_dlg.isHidden() )
    {
        m_palette_edit_dlg.showWithPos();

        if ( m_palette_dlg_edit_init )
        {
            // Adjust initial position to right side of main window
            m_palette_dlg_edit_init = false;
            QRect rect1 = m_palette_edit_dlg.geometry();
            QRect rect2 = this->geometry();
            rect1.moveRight( rect2.x() + rect2.width() );
            m_palette_edit_dlg.setGeometry( rect1 );
        }
    }
}

/**
 * @brief Slot to receive palette offset slider value signals
 * @param a_offset - New palette offset value
 */
void
MainWindow::paletteOffsetSliderChanged( int a_offset )
{
    if ( m_ignore_off_sig )
    {
        return;
    }

    m_palette_offset = a_offset;

    if ( m_calc_result.img_data )
    {
        // Redraw image (regenerates palette as needed)
        imageDraw();
    }
}

/**
 * @brief Slot to receive palette scale slider value change signals
 * @param a_scale - New palette scale value
 */
void
MainWindow::paletteScaleSliderChanged( int a_scale )
{
    if ( m_ignore_scale_sig )
        return;

    // Adjust offset slider based on new scale
    adjustScaleSliderChanged( a_scale );

    if ( m_calc_result.img_data )
    {
        // Redraw image (regenerates palette as needed)
        imageDraw();
    }
}

/**
 * @brief Slot to receive paletteSelect UI signals
 * @param a_text - New palette name
 */
void
MainWindow::paletteSelect( const QString &a_text )
{
    if ( m_ignore_pal_sig )
        return;

    // Setup new palette data and adjust other UI inputs
    adjustPalette( a_text );

    if ( m_calc_result.img_data )
    {
        // Redraw image (regenerates palette as needed)
        imageDraw();
    }
}

/**
 * @brief Slot to receive imageSave UI signals
 *
 * Shows a save file dialog and writes image and metadata files.
 */
void
MainWindow::imageSave()
{
    QString path = m_cur_dir.length() ? m_cur_dir : "image.png";
    QString fname = QFileDialog::getSaveFileName( this, "Save Image", path, "Images (*.png *.jpg)");

    if ( fname.length() )
    {
        QFileInfo fi( fname );
        m_cur_dir = fi.absolutePath();

        // TODO: QImageWriter does not support EXIF metadata, must eventually use an external library

        // Save image file
        QImageWriter writer(fname);
        QImage image = m_viewer->getImage();
        writer.write( image );

        // Save JSON metadata file
        // Metadata filename is image filename with ".json" extention
        fname.resize( fname.length() - 3 );
        fname += "json";

        // NOTE: json doubles have lower precision than standard 64-bit doubles
        // For this reason, they must be written and read as strings

        QString json = QString("{\n  \"x1\":\"%1\",\n  \"y1\":\"%2\",\n  \"x2\":\"%3\",\n  \"y2\":\"%4\",\n  \"iter_mx\":%5,\n  \"img_width\":%6,\n  \"img_height\":%7,\n  \"th_cnt\":%8,\n  \"ss\":%9,\n  \"time_ms\":%10,\n")
            .arg(m_calc_result.x1,0,'g',17)
            .arg(m_calc_result.y1,0,'g',17)
            .arg(m_calc_result.x2,0,'g',17)
            .arg(m_calc_result.y2,0,'g',17)
            .arg(m_calc_result.iter_mx)
            .arg(m_calc_result.img_width/m_calc_ss)
            .arg(m_calc_result.img_height/m_calc_ss)
            .arg(m_calc_result.th_cnt)
            .arg(m_calc_ss)
            .arg(m_calc_result.time_ms);

        const PaletteInfo & pal_info = m_palette_edit_dlg.getPaletteInfo();
        json += QString("  \"palette\":{\n    \"name\":\"%1\",\n    \"scale\":%2,\n    \"offset\":%3,\n    \"repeat\":%4,\n    \"colors\":[")
                    .arg(QString::fromStdString(pal_info.name))
                    .arg(m_palette_scale)
                    .arg(m_palette_offset)
                    .arg(pal_info.repeat?"true":"false");

        for ( PaletteGenerator::ColorBands::const_iterator cb = pal_info.color_bands.begin(); cb != pal_info.color_bands.end(); cb++ )
        {
            json += QString("\n      {\"color\":%1, \"width\":%2, \"mode\":%3}")
                        .arg(cb->color) // TODO: integer representation is platform dependent
                        .arg(cb->width)
                        .arg((int)cb->mode);

            if ( cb != pal_info.color_bands.end() - 1 )
            {
                json += ",";
            }
        }

        json += "\n    ]\n  }\n}";

        QFile mdfile(fname);
        if ( mdfile.open( QIODevice::WriteOnly ))
        {
            QTextStream stream( &mdfile );
            stream << json;
        }
    }
}

/**
 * @brief Slot to receive imageLoad UI signals
 *
 * Shows an open file dialog, reads metadata file, and regenerates image.
 */
void
MainWindow::imageLoad()
{
    QString path = m_cur_dir.length() ? m_cur_dir : "image.png";
    QString fname = QFileDialog::getOpenFileName( this, "Open Image", path, "Images (*.png *.jpg)" );

    if ( fname.length() )
    {
        QFileInfo fi( fname );
        m_cur_dir = fi.absoluteFilePath();

        fname.resize( fname.length() - 3 );
        fname += "json";

        QFile mdfile(fname);
        if ( mdfile.open( QIODevice::ReadOnly ))
        {
            // Read metadata file contents
            QTextStream stream( &mdfile );
            QString json = stream.readAll();
            mdfile.close();

            // Parse metadata JSON content
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson( json.toUtf8(), &err );
            if ( err.error == QJsonParseError::NoError )
            {
                try
                {
                    if ( !doc.isObject() )
                        throw -1;

                    QJsonObject obj = doc.object();

                    m_calc_params.x1 = jsonReadDouble( obj, "x1" );
                    m_calc_params.y1 = jsonReadDouble( obj, "y1" );
                    m_calc_params.x2 = jsonReadDouble( obj, "x2" );
                    m_calc_params.y2 = jsonReadDouble( obj, "y2" );
                    m_calc_params.iter_mx = jsonReadInt( obj, "iter_mx" );
                    m_calc_result.th_cnt = jsonReadInt( obj, "th_cnt" );
                    m_calc_ss = jsonReadInt( obj, "ss" );
                    int w = jsonReadInt( obj, "img_width" );
                    int h = jsonReadInt( obj, "img_height" );

                    QJsonValue val = obj["palette"];
                    if ( !val.isObject() )
                        throw -1;

                    obj = val.toObject();
                    QString pal_name = jsonReadString( obj, "name" );
                    int palette_offset = jsonReadInt( obj, "offset" );
                    m_palette_scale = jsonReadInt( obj, "scale" );

                    PaletteInfo pal_info;
                    pal_info.name = pal_name.toStdString();
                    pal_info.repeat = jsonReadBool( obj, "repeat" );
                    pal_info.built_in = false;
                    pal_info.changed = false;

                    val = obj["colors"];
                    if ( !val.isArray() )
                        throw -1;

                    PaletteGenerator::ColorBand cb;
                    QJsonArray colors = val.toArray();
                    for ( QJsonArray::ConstIterator c = colors.begin(); c != colors.end(); c++ )
                    {
                        if ( !c->isObject() )
                            throw -1;

                        obj = c->toObject();
                        cb.color = jsonReadInt(obj,"color");
                        cb.width = jsonReadInt(obj,"width");
                        cb.mode = (PaletteGenerator::ColorMode)jsonReadInt(obj,"mode");
                        pal_info.color_bands.push_back(cb);
                    }

                    // TODO: All values should be checked for proper range in case file was manually edited

                    // Determine if loaded palette needs to be added to palette selection
                    PaletteMap_t::const_iterator pi = m_palette_map.find( pal_info.name );
                    if ( pi == m_palette_map.end() )
                    {
                        // Palette doesn't exist, add as-is
                        m_palette_map[pal_info.name] = pal_info;
                        m_ignore_pal_sig = true;
                        ui->comboBoxPalette->addItem( pal_name );
                        m_ignore_pal_sig = false;
                    }
                    else
                    {
                        // Palette already exists, check if the loaded version is different
                        bool same = false;
                        if ( pi->second.repeat == pal_info.repeat && pi->second.color_bands.size() == pal_info.color_bands.size() )
                        {
                            same = true;
                            PaletteGenerator::ColorBands::const_iterator cb1 = pi->second.color_bands.begin(), cb2 = pal_info.color_bands.begin();
                            for ( ; cb2 != pal_info.color_bands.end(); cb1++, cb2++ )
                            {
                                if ( cb1->color != cb2->color || cb1->width != cb2->width || cb1->mode != cb2->mode )
                                {
                                    same = false;
                                    break;
                                }
                            }
                        }

                        if ( !same )
                        {
                            // Loaded palette has same name, but different settings
                            string base_name = pal_info.name + "_", new_name;

                            for ( int i = 0; i < 99; i++ )
                            {
                                new_name = base_name + to_string(i);
                                if ( m_palette_map.find( new_name ) == m_palette_map.end() )
                                {
                                    pal_info.name = new_name;
                                    pal_name = QString::fromStdString( new_name );
                                    m_palette_map[pal_info.name] = pal_info;
                                    m_ignore_pal_sig = true;
                                    ui->comboBoxPalette->addItem( pal_name );
                                    m_ignore_pal_sig = false;
                                    break;
                                }
                            }
                            // If no free name found, will just use existing palette
                        }
                    }

                    // Setup palette and UI

                    adjustPalette( pal_name );
                    adjustScaleSliderChanged( m_palette_scale ); // Resets m_palette_offset to 0

                    m_ignore_pal_sig = true;
                    ui->comboBoxPalette->setCurrentText( pal_name );
                    m_ignore_pal_sig = false;

                    m_ignore_scale_sig = true;
                    ui->sliderPalScale->setValue( m_palette_scale );
                    m_ignore_scale_sig = false;

                    m_ignore_off_sig = true;
                    ui->sliderPalOffset->setValue( palette_offset );
                    m_palette_offset = palette_offset;
                    m_ignore_off_sig = false;

                    ui->lineEditResolution->setText( w > h ? QString::number( w ) : QString::number( h ));
                    ui->lineEditIterMax->setText( QString::number( m_calc_params.iter_mx ));
                    ui->spinBoxSuperSample->setValue( m_calc_ss );

                    // Ensure home button is enabled
                    ui->buttonViewTop->setDisabled( false );

                    // Recalc image
                    calculate();

                    CalcPos pos = {m_calc_params.x1,m_calc_params.y1,m_calc_params.x2,m_calc_params.y2};

                    // Loading image clears position history
                    m_calc_history.resize(0);
                    m_calc_history.push_back( pos );
                    m_calc_history_idx = 1;

                    ui->buttonViewTop->setDisabled(false);
                    ui->buttonViewNext->setDisabled(true);
                    ui->buttonViewPrev->setDisabled(false);
                }
                catch ( int )
                {
                    QMessageBox mb( QMessageBox::Warning, "Mandelbrot App Error", "Image metadata file contains missing or unexpected data.", QMessageBox::Ok, this );
                    mb.exec();
                }
            }
            else
            {
                cout << err.errorString().toStdString() << endl;
                QMessageBox mb( QMessageBox::Warning, "Mandelbrot App Error", "Image metadata file contains invalid JSON formatting.", QMessageBox::Ok, this );
                mb.exec();
            }
        }
        else
        {
            QMessageBox mb( QMessageBox::Warning, "Mandelbrot App Error", "Could not open image metadata file.", QMessageBox::Ok, this );
            mb.exec();
        }
    }
}

/**
 * @brief Slot to receive viewNext UI signals
 *
 * If available, moves to next entry in view history and regenerates image.
 */
void
MainWindow::viewNext()
{
    if ( m_calc_history_idx < m_calc_history.size() )
    {
        m_calc_history_idx++;
        ui->buttonViewTop->setDisabled( false );
        ui->buttonViewPrev->setDisabled( false );
        ui->buttonViewNext->setDisabled( m_calc_history_idx >= m_calc_history.size() );
        ui->buttonZoomOut->setDisabled( false );

        const CalcPos & pos = m_calc_history[m_calc_history_idx-1];
        m_calc_params.x1 = pos.x1;
        m_calc_params.y1 = pos.y1;
        m_calc_params.x2 = pos.x2;
        m_calc_params.y2 = pos.y2;

        calculate();
    }
}

/**
 * @brief Slot to receive viewPrev UI signals
 *
 * If available, moves to previous entry in view history and regenerates image.
 */
void
MainWindow::viewPrev()
{
    if ( m_calc_history_idx > 0 )
    {
        ui->buttonViewNext->setDisabled(false);

        if( --m_calc_history_idx == 0 )
        {
            ui->buttonViewTop->setDisabled(true);
            ui->buttonViewPrev->setDisabled(true);
            ui->buttonZoomOut->setDisabled(true);

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

        calculate();
    }
}

/**
 * @brief Slot to receive viewTop UI signals
 *
 * Moves to top entry in view history and regenerates image.
 */
void
MainWindow::viewTop()
{
    m_calc_params.x1 = -2;
    m_calc_params.y1 = -2;
    m_calc_params.x2 = 2;
    m_calc_params.y2 = 2;

    calculate();

    ui->buttonViewTop->setDisabled(true);
    ui->buttonViewNext->setDisabled(m_calc_history.size() == 0);
    ui->buttonViewPrev->setDisabled(true);
    ui->buttonZoomOut->setDisabled(true);
    m_calc_history_idx = 0;
}

/**
 * @brief Slot to receive zoomIn UI signals
 *
 * Zooms into the current image by a factor of 2. Zooming in truncates the
 * current position history.
 */
void
MainWindow::zoomIn()
{
    double dx = (m_calc_params.x2 - m_calc_params.x1)/4;
    double dy = (m_calc_params.y2 - m_calc_params.y1)/4;

    m_calc_params.x1 += dx;
    m_calc_params.y1 += dy;
    m_calc_params.x2 -= dx;
    m_calc_params.y2 -= dy;

    calculate();

    CalcPos pos = {m_calc_params.x1,m_calc_params.y1,m_calc_params.x2,m_calc_params.y2};

    // Zooming in truncates any positions past current history index
    m_calc_history.resize(m_calc_history_idx);
    m_calc_history.push_back( pos );
    m_calc_history_idx++;

    ui->buttonViewTop->setDisabled(false);
    ui->buttonViewNext->setDisabled(true);
    ui->buttonViewPrev->setDisabled(false);
    ui->buttonZoomOut->setDisabled(false);
}

/**
 * @brief Slot to receive zoomIn UI signals
 *
 * Zooms out of current image by a factor of 2. Zooming out, like zooming in,
 * truncates the current position history. Zoom out past top position is
 * disallowed.
 */
void
MainWindow::zoomOut()
{
    double dx = (m_calc_params.x2 - m_calc_params.x1)/2;
    double dy = (m_calc_params.y2 - m_calc_params.y1)/2;

    if ( m_calc_params.x1 - dx <= -2 || m_calc_params.y1 - dy <= -2 || m_calc_params.x2 + dx >= 2 || m_calc_params.y2 + dy >= 2 )
    {
        m_calc_history.resize(0);
        m_calc_history_idx = 0;

        viewTop();

        return;
    }

    m_calc_params.x1 -= dx;
    m_calc_params.y1 -= dy;
    m_calc_params.x2 += dx;
    m_calc_params.y2 += dy;

    calculate();

    CalcPos pos = {m_calc_params.x1,m_calc_params.y1,m_calc_params.x2,m_calc_params.y2};

    // Zooming out backs-up history and truncates any positions past current history index
    m_calc_history.resize(m_calc_history_idx);
    m_calc_history.back() = pos;

    ui->buttonViewNext->setDisabled(true);
}

/**
 * @brief Regenerates palette on palette selection change
 * @param a_text - Newly selected palette name
 *
 * Also updates palette edit dialog with new palette colors.
 */
void
MainWindow::adjustPalette( const QString &a_text )
{
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

}

/**
 * @brief Regenerates palette on palette scale slider change
 * @param a_scale - New palette scale value
 *
 * Also updates palette edit dialog with new palette colors, and
 * updates offset slider (max/value).
 */
void
MainWindow::adjustScaleSliderChanged( int a_scale )
{
    m_palette_scale = a_scale;

    // Render palette to determine new size
    uint32_t ps1 = m_palette_gen.size();
    m_palette_gen.renderPalette( m_palette_scale );
    uint32_t ps2 = m_palette_gen.size();

    m_ignore_off_sig = true;

    // Adjust offset slider to width of palette
    ui->sliderPalOffset->setMaximum( ps2 );

    // Adjust offset based on new scale to stay in same relative position
    m_palette_offset = m_palette_offset*ps2/ps1;
    ui->sliderPalOffset->setValue(m_palette_offset);

    m_ignore_off_sig = false;
}

/**
 * @brief Rerenders and draws image
 */
void
MainWindow::imageDraw()
{
    uchar *imbuffer = imageRender();

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

/**
 * @brief Renders calculation results to an image buffer
 * @return New image buffer
 *
 * The current palette, scale, and offset are used to render the image. If
 * super sampling is used, the image width and height are multiplies by
 * the super sampling factor.
 */
uchar *
MainWindow::imageRender()
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

/**
 * @brief Displays an input dialog to get new palette name
 * @param a_title - Sub-title of dialog window
 * @return New palette name (QString)
 *
 * Various format checks are performed on name returned from the dialog box
 * before returning it. If checks fail, the user is returned to the input
 * dialog to make corrections, or cancel the process.
 */
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
        // TODO: Should convert to a regular expression match
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

/**
 * @brief Runs the Mandelbrot set calculation.
 *
 * This method is run separately from the calculate slot to prevent the
 * UI from hanging for the duration of the calculation.
 */
/*void
MainWindow::runCalculate()
{
    m_calc_ss = ui->spinBoxSuperSample->value();

    m_calc_params.res = ui->lineEditResolution->text().toUShort() * m_calc_ss;
    m_calc_params.iter_mx = ui->lineEditIterMax->text().toUShort();
    m_calc_params.th_cnt = ui->spinBoxThreadCount->value();

    //m_calc_result = m_calc.calculate( m_calc_params );
    m_calc.calculate( *this, m_calc_params );
}*/

// MandelbrotCalc::IObserver methods
void
MainWindow::calcProgress( int a_progress )
{
    cout << " " << a_progress << endl;
}

void
MainWindow::calcCompleted( MandelbrotCalc::CalcResult a_result )
{
    cout << "calc done" << endl;

    m_calc_result = a_result;

    imageDraw();

    // Update window title with important calc results
    setWindowTitle( QString("%1  (%2,%3)->(%4,%5)  %6w x %7h  msec: %8")
                       .arg(m_app_name)
                       .arg(m_calc_result.x1)
                       .arg(m_calc_result.y1)
                       .arg(m_calc_result.x2)
                       .arg(m_calc_result.y2)
                       .arg(m_calc_result.img_width)
                       .arg(m_calc_result.img_height)
                       .arg(m_calc_result.time_ms)
                   );

    ui->buttonCalc->setDisabled(false);
    ui->buttonImageSave->setDisabled(false);
}

void
MainWindow::calcCancelled()
{
    cout << "calc cancelled" << endl;
}


/**
 * @brief Deletes specified palette from app settings
 * @param a_palette_name - Name of palette to delete
 */
void
MainWindow::settingsPaletteDelete( const string & a_palette_name )
{
    m_settings.beginGroup( "palettes" );
    m_settings.remove( a_palette_name );
    m_settings.endGroup();
}

/**
 * @brief Loads all palettes from app settings
 */
void
MainWindow::settingsPaletteLoadAll()
{
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
        m_settings.beginGroup(*k);

        pi.name = k->toStdString();
        pi.repeat = m_settings.value( "repeat" ).toBool();
        pi.color_bands.resize(0);

        size = m_settings.beginReadArray( "colors" );
        for ( i = 0; i < size; ++i )
        {
            m_settings.setArrayIndex( i );

            cb.color = m_settings.value( "color" ).toUInt();
            cb.width = m_settings.value( "width" ).toUInt();
            cb.mode = (PaletteGenerator::ColorMode)m_settings.value( "mode" ).toUInt();

            pi.color_bands.push_back( cb );
        }

        m_settings.endArray();

        m_palette_map[pi.name] = pi;
        ui->comboBoxPalette->addItem( QString::fromStdString( pi.name ));

        m_settings.endGroup();
    }

    m_settings.endGroup();
    m_ignore_pal_sig = false;
}

/**
 * @brief Saves specified palette from app settings
 * @param a_pal_info - Name of palette to
 */
void
MainWindow::settingsPaletteSave( PaletteInfo & a_pal_info )
{
    m_settings.beginGroup("palettes");

    m_settings.beginGroup(a_pal_info.name);
    m_settings.setValue( "repeat", a_pal_info.repeat );
    m_settings.beginWriteArray( "colors" );

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
    m_settings.endGroup();

    a_pal_info.changed = false;
}


//==================== IMandelbrotViewerObserver methods ====================


/**
 * @brief Callback from MandelbrotViewer requesting image re-centering
 * @param a_pos - New center position of image (pixels)
 */
void
MainWindow::imageRecenter( const QPointF & a_pos )
{
    double sx = (m_calc_params.x2-m_calc_params.x1)*m_calc_ss/m_calc_result.img_width;
    double sy = (m_calc_params.y2-m_calc_params.y1)*m_calc_ss/m_calc_result.img_height;
    double dx = (a_pos.x() - (m_calc_result.img_width/(2*m_calc_ss)))*sx;
    double dy = -(a_pos.y() - (m_calc_result.img_height/(2*m_calc_ss)))*sy;

    m_calc_params.x1 += dx;
    m_calc_params.x2 += dx;
    m_calc_params.y1 += dy;
    m_calc_params.y2 += dy;

    calculate();

    CalcPos pos = {m_calc_params.x1,m_calc_params.y1,m_calc_params.x2,m_calc_params.y2};

    // Re-centering truncates any positions past current history index
    m_calc_history.resize(m_calc_history_idx);
    m_calc_history.push_back( pos );
    m_calc_history_idx++;

    ui->buttonViewTop->setDisabled(false);
    ui->buttonViewNext->setDisabled(true);
    ui->buttonViewPrev->setDisabled(false);
}

/**
 * @brief Callback from MandelbrotViewer requesting image zoom-in
 * @param a_rect - New bounding rectangle of image (pixels)
 */
void
MainWindow::imageZoomIn( const QRectF & a_rect )
{
    // Calc new set coords based on new image coords (rect)
    double sx = (m_calc_params.x2-m_calc_params.x1)*m_calc_ss/m_calc_result.img_width;
    double sy = (m_calc_params.y2-m_calc_params.y1)*m_calc_ss/m_calc_result.img_height;

    m_calc_params.x1 = m_calc_params.x1 + a_rect.x()*sx;
    m_calc_params.x2 = m_calc_params.x1 + (a_rect.width()-1)*sx;
    m_calc_params.y1 = m_calc_params.y1 + ((m_calc_result.img_height/m_calc_ss) - (a_rect.y() + a_rect.height() - 1))*sy;
    m_calc_params.y2 = m_calc_params.y1 + (a_rect.height()-1)*sy;

    calculate();

    CalcPos pos = {m_calc_params.x1,m_calc_params.y1,m_calc_params.x2,m_calc_params.y2};

    // Zooming in truncates any positions past current history index
    m_calc_history.resize(m_calc_history_idx);
    m_calc_history.push_back( pos );
    m_calc_history_idx++;

    ui->buttonViewTop->setDisabled(false);
    ui->buttonViewNext->setDisabled(true);
    ui->buttonViewPrev->setDisabled(false);
    ui->buttonZoomOut->setDisabled(false);
}


//==================== IPaletteEditObserver methods ====================


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

    imageDraw();
}

/**
 * @brief Callback from palette edit dialog to notify main window that palette should be deleted
 * @param a_pal_info - Palette to delete
 *
 * User is prompted to confirm deletion, then palette is removed from app
 * settings. The default palette is then used to rerender the current image.
 */
void
MainWindow::paletteDelete( const PaletteInfo & a_pal_info )
{
    // Can't delete built-in palettes
    if ( a_pal_info.built_in )
    {
        QMessageBox mb( QMessageBox::Warning, "Mandelbrot App Error", "Cannot delete built-in palette.", QMessageBox::Ok, this );
        mb.exec();
    }
    else
    {
        QMessageBox mb( QMessageBox::Question, "Mandelbrot App", QString("Delete palette '%1'?").arg(QString::fromStdString(a_pal_info.name)), QMessageBox::Ok | QMessageBox::Cancel, this );
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

            imageDraw();
        }
    }
}

/**
 * @brief Callback from palette edit dialog to notify main window that palette should be duplicated
 * @param a_pal_info - Palette to duplicate
 *
 * User is prompted for a new (unique) palette name, then the palette is
 * copied and used to rerender the current image. Note that the duplicate
 * palette is not saved at this point.
 */
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

/**
 * @brief Callback from palette edit dialog to notify main window that a new palette should be created
 *
 * User is prompted for a new (unique) palette name, then a new palette is
 * created and used to rerender the current image. Note that the new palette is
 * not saved at this point.
 */
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
                    {0xFFFFFFFF,10,PaletteGenerator::CM_LINEAR},
                    {0xFF000000,10,PaletteGenerator::CM_LINEAR}
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

        imageDraw();
    }
}

/**
 * @brief Callback from palette edit dialog to notify main window that palette should be saved
 * @param a_pal_info - Palette to save
 *
 * Custom palettes are saved to app settings.
 */
bool
MainWindow::paletteSave( PaletteInfo & a_pal_info )
{
    // Cannot save built-in palettes
    if ( a_pal_info.built_in )
    {
        QMessageBox mb( QMessageBox::Warning, "Mandelbrot App Error", "Cannot save built-in palette.", QMessageBox::Ok, this );
        mb.exec();
        return false;
    }

    settingsPaletteSave( a_pal_info );
    m_palette_map[a_pal_info.name].changed = false;

    return true;
}


//==================== JSON helper methods ====================


/**
 * @brief Parse a bool value from a QJsonObject key-value pair
 * @param a_obj - Object to read from
 * @param a_key - Key to read
 * @return Bool result. Throws exception on error.
 */
bool
MainWindow::jsonReadBool( const QJsonObject & a_obj, const QString & a_key )
{
    const QJsonValue val = a_obj[a_key];
    if ( !val.isBool() )
        throw -1;

    return val.toBool();
}

/**
 * @brief Parse a double value from a QJsonObject key-value pair
 * @param a_obj - Object to read from
 * @param a_key - Key to read
 * @return Double result. Throws exception on error.
 *
 * Due to JSON not supporting 64-bit doubles, double-formatted strings are
 * used instead.
 */
double
MainWindow::jsonReadDouble( const QJsonObject & a_obj, const QString & a_key )
{
    // NOTE: json doubles have lower precision than standard 64-bit doubles
    // For this reason, the must be written and read as strings, and parsed
    const QJsonValue val = a_obj[a_key];
    if ( !val.isString() )
        throw -1;

    bool ok;
    double res = val.toString().toDouble( &ok );
    if ( !ok )
        throw -1;

    return res;
}

/**
 * @brief Parse an integer value from a QJsonObject key-value pair
 * @param a_obj - Object to read from
 * @param a_key - Key to read
 * @return Integer result. Throws exception on error.
 *
 * Note: JSON does not support integer types, thus doubles are used.
 */
int
MainWindow::jsonReadInt( const QJsonObject & a_obj, const QString & a_key )
{
    const QJsonValue val = a_obj[a_key];
    if ( !val.isDouble() )
        throw -1;

    return val.toInteger();
}

/**
 * @brief Parse a string value from a QJsonObject key-value pair
 * @param a_obj - Object to read from
 * @param a_key - Key to read
 * @return QString result. Throws exception on error.
 */
QString
MainWindow::jsonReadString( const QJsonObject & a_obj, const QString & a_key )
{
    const QJsonValue val = a_obj[a_key];
    if ( !val.isString() )
        throw -1;

    return val.toString();
}

