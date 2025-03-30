#include <iostream>
#include <cmath>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QStyleFactory>

#include "paletteeditdialog.h"
#include "ui_paletteeditdialog.h"

using namespace std;

#define HSV_SLIDER_VAL_MAX 400
#define HSV_SLIDER_SAT_MAX 400

/**
 * @brief PaletteEditDialog constructor
 * @param a_parent - Parent widget
 * @param a_observer - Palette edit observer
 */
PaletteEditDialog::PaletteEditDialog( QWidget *a_parent, IPaletteEditObserver & a_observer ):
    QDialog(a_parent),
    m_observer( a_observer ),
    m_event_handler(*this),
    ui(new Ui::PaletteEditDialog),
    m_geometry_set(false),
    m_cur_frame(0),
    m_ignore_color_slider_sig(false),
    m_ignore_color_change_sig(false),
    m_use_hsv(false)
{
    ui->setupUi(this);
}

/**
 * @brief PaletteEditDialog destructor
 */
PaletteEditDialog::~PaletteEditDialog()
{
    delete ui;
}

/**
 * @brief Get current palette info
 * @return Ref to current PaletteInfo struct
 *
 * The internal PaletteInfo is kept synchronized with dialog controls.
 */
PaletteInfo &
PaletteEditDialog::getPaletteInfo()
{
    return m_pal_info;
}

/**
 * @brief Set (replace) current palette in edit dialog
 * @param a_palette_info - Palette information structure
 */
void
PaletteEditDialog::setPaletteInfo( PaletteInfo & a_palette_info )
{
    m_pal_info = a_palette_info;
    updateWindowTitle();

    QLayout *layout = ui->frameControls->layout();
    size_t i;
    size_t color_count = layout->count() - 1; // -1 for vertical spacer at bottom

    if ( a_palette_info.color_bands.size() > color_count )
    {
        for ( i = color_count; i < a_palette_info.color_bands.size(); i++ )
        {
            insertColorControls( i );
        }
    }
    else if ( color_count > a_palette_info.color_bands.size() )
    {
        QLayoutItem *color_layout;
        QWidget * frame;

        for ( i = a_palette_info.color_bands.size(); i < color_count; i++ )
        {
            color_layout = layout->itemAt(a_palette_info.color_bands.size());
            frame = color_layout->widget();
            layout->removeItem( color_layout );
            delete frame;
        }
    }

    // Ignore callbacks on init
    m_ignore_color_change_sig = true;
    PaletteGenerator::ColorBands::iterator c = a_palette_info.color_bands.begin();
    for ( i = 0; i < a_palette_info.color_bands.size(); i++, c++ )
    {
        setColor( i, *c, false );
    }

    ui->checkBoxRepeat->setCheckState( a_palette_info.repeat ? Qt::Checked : Qt::Unchecked );

    m_ignore_color_change_sig = false;

    setFocus( 0, true );


    // Enable/disable specific UI controls
    if ( m_pal_info.built_in )
    {
        ui->buttonSavePal->setDisabled( true );
        ui->buttonDeletePal->setDisabled( true);
    }
    else
    {
        if ( m_pal_info.changed )
        {
            ui->buttonSavePal->setDisabled( false );
        }
        else
        {
            ui->buttonSavePal->setDisabled( true );
        }
        ui->buttonDeletePal->setDisabled( false );
    }
}

//=================== Public Slot Methods ===================

/**
 * @brief Slot to receive color delete signals from UI
 *
 * Removes currently focused color, updates palette, and notifies observer.
 */
void PaletteEditDialog::colorDelete()
{
    if ( m_pal_info.color_bands.size() > 1 )
    {
        int index = getColorFrameIndex();

        QLayout *layout = ui->frameControls->layout();
        QLayoutItem *color_layout = layout->itemAt(index);
        QWidget * frame = color_layout->widget();

        layout->removeItem( color_layout );
        delete frame;

        m_pal_info.color_bands.erase( m_pal_info.color_bands.begin() + index );
        setFocus( index < (int)m_pal_info.color_bands.size() ? index : index - 1 );
        paletteChanged();
    }
}

/**
 * @brief Slot to receive color insert signals from UI
 *
 * Inserts a new color below currently focused color, updates palette, and notifies observer.
 */
void PaletteEditDialog::colorInsert()
{
    int index = getColorFrameIndex();
    insertColorControls( index + 1 );

    PaletteGenerator::ColorBand band {
        0,
        10,
        PaletteGenerator::CM_LINEAR
    };

    m_pal_info.color_bands.insert( m_pal_info.color_bands.begin() + index + 1, band );
    setColor( index + 1, band, false );
    setFocus( index + 1 );
    setColorSliders( band.color );
    paletteChanged();
}

/**
 * @brief Slot to receive color move down signals from UI
 *
 * Moves currently focused color up in palette and notifies observer.
 */
void PaletteEditDialog::colorMoveDown()
{
    if ( m_pal_info.color_bands.size() > 1 )
    {
        int index = getColorFrameIndex();
        if ( index < (int)m_pal_info.color_bands.size() - 1 )
        {
            swapColors( index, index + 1 );
            paletteChanged();
        }
    }
}

/**
 * @brief Slot to receive color move up signals from UI
 *
 * Moves currently focused color down in palette and notifies observer.
 */
void PaletteEditDialog::colorMoveUp()
{
    if ( m_pal_info.color_bands.size() > 1 )
    {
        int index = getColorFrameIndex();
        if ( index > 0 )
        {
            swapColors( index, index - 1 );
            paletteChanged();
        }
    }
}

/**
 * @brief Slot to receive color text changed signals from UI (text entry)
 * @param a_text - New color text
 *
 * Converts received text into a 32-bit color value then updates palette
 * and notifies observer.
 */
void
PaletteEditDialog::colorTextChanged( const QString & a_text )
{
    // On color text change update color swatch, color sliders, and palette color then trigger image refresh
    if ( m_cur_frame && !m_ignore_color_change_sig )
    {
        bool ok;
        uint32_t color = a_text.toUInt( &ok, 16 );

        // Ignore text that does not convert correctly
        if ( ok )
        {
            setColorSwatch( m_cur_frame, color );
            setColorSliders( color );
            m_pal_info.color_bands[getColorFrameIndex()].color = 0xFF000000 | color;
            paletteChanged();
        }
    }
}

/**
 * @brief Hides palette edit dialog and save geometry
 */
void
PaletteEditDialog::hideWithPos()
{
    m_geometry = geometry();
    m_geometry_set = true;
    QDialog::hide();
}

/**
 * @brief Slot to receive hsv state changed signals from UI
 * @param a_state - New checked state
 *
 * Changes color slider labels and controls for either RGB or HSV mode.
 */
void
PaletteEditDialog::hsvStateChanged( int a_state )
{
    bool new_state = ( a_state == Qt::Checked ? true : false );
    if ( new_state != m_use_hsv )
    {
        m_ignore_color_slider_sig = true;
        m_use_hsv = new_state;

        if ( m_use_hsv )
        {
            ui->labelRed->setText("H");
            ui->labelGreen->setText("S");
            ui->labelBlue->setText("V");
            ui->sliderRed->setMaximum( 360 );
            // Note: larger slider max is used to ensure color changes are smooth
            ui->sliderGreen->setMaximum( HSV_SLIDER_SAT_MAX );
            ui->sliderBlue->setMaximum( HSV_SLIDER_VAL_MAX );
        }
        else
        {
            ui->labelRed->setText("R");
            ui->labelGreen->setText("G");
            ui->labelBlue->setText("B");
            ui->sliderRed->setMaximum( 255 );
            ui->sliderGreen->setMaximum( 255 );
            ui->sliderBlue->setMaximum( 255 );
        }

        m_ignore_color_slider_sig = false;

        setColorSliders( m_pal_info.color_bands[getColorFrameIndex()].color );
    }
}

/**
 * @brief Slot to receive color mode state change signals from UI
 * @param a_value - New color mode
 *
 * Updates the currently focused color and notifies observer.
 */
void
PaletteEditDialog::modeIndexChanged( int a_value )
{
    // On mode index change update palette mode then trigger image refresh

    if ( m_cur_frame && !m_ignore_color_change_sig )
    {
        m_pal_info.color_bands[getColorFrameIndex()].mode = (PaletteGenerator::ColorMode)a_value;
        paletteChanged();
    }
}

/**
 * @brief Slot to receive palette delete signals from UI
 *
 * Passes notification through to observer.
 */
void
PaletteEditDialog::paletteDelete()
{
    m_observer.paletteDelete( m_pal_info );
}

/**
 * @brief Slot to receive palette duplicate signals from UI
 *
 * Passes notification through to observer.
 */
void
PaletteEditDialog::paletteDuplicate()
{
    m_observer.paletteDuplicate( m_pal_info );
}

/**
 * @brief Slot to receive palette new signals from UI
 *
 * Passes notification through to observer.
 */
void
PaletteEditDialog::paletteNew()
{
    m_observer.paletteNew();
}

/**
 * @brief Slot to receive palette save signals from UI
 *
 * Passes notification through to observer, then updates UI if save completed.
 */
void
PaletteEditDialog::paletteSave()
{
    if ( m_observer.paletteSave( m_pal_info ))
    {
        // If save completed, update UI based on new status
        updateWindowTitle();
        ui->buttonSavePal->setDisabled( true );
    }
}

/**
 * @brief Slot to receive repeat state change signals from UI
 * @param a_state - New repeat state
 *
 * Updates the palette state and notifies observer.
 */
void
PaletteEditDialog::repeatStateChanged( int a_state )
{
    if ( m_cur_frame && !m_ignore_color_change_sig )
    {
        m_pal_info.repeat = ( a_state == Qt::Checked ? true : false );
        paletteChanged();
    }
}

/**
 * @brief Shows palette edit dialog and restores geometry
 */
void
PaletteEditDialog::showWithPos()
{
    QDialog::show();

    if ( m_geometry_set )
    {
        setGeometry(m_geometry);
    }
}

/**
 * @brief Slot to receive red color value change signals from UI
 * @param a_value - New red value
 *
 * Depending on mode, the value will either be the red value (0 - 255) for RGB,
 * or the hue value (0-360) for HSV. For either case, the new color is
 * calculated, the palette updated, and the observer notified.
 */
void
PaletteEditDialog::updateRedValue( int a_value )
{
    if ( m_cur_frame && !m_ignore_color_slider_sig  )
    {
        uint32_t color;

        if ( m_use_hsv )
        {
            color = HSVToRGB( a_value, ui->sliderGreen->value(), ui->sliderBlue->value());
        }
        else
        {
            color = (a_value << 16) | (ui->sliderGreen->value() << 8) | ui->sliderBlue->value();
        }

        setColor( m_cur_frame, color );
        paletteChanged();
    }
}

/**
 * @brief Slot to receive green color value change signals from UI
 * @param a_value - New green value
 *
 * Depending on mode, the value will either be the green value (0 - 255) for RGB,
 * or the saturation value (0-360) for HSV. For either case, the new color is
 * calculated, the palette updated, and the observer notified.
 */
void
PaletteEditDialog::updateGreenValue( int a_value )
{
    if ( m_cur_frame && !m_ignore_color_slider_sig  )
    {
        uint32_t color;

        if ( m_use_hsv )
        {
            color = HSVToRGB( ui->sliderRed->value(), a_value, ui->sliderBlue->value());
        }
        else
        {
            color = (ui->sliderRed->value() << 16) | (a_value << 8) | ui->sliderBlue->value();
        }

        setColor( m_cur_frame, color );
        paletteChanged();
    }
}

/**
 * @brief Slot to receive blue color value change signals from UI
 * @param a_value - New blue value
 *
 * Depending on mode, the value will either be the blue value (0 - 255) for RGB,
 * or the "value" value (0-360) for HSV. For either case, the new color is
 * calculated, the palette updated, and the observer notified.
 */
void
PaletteEditDialog::updateBlueValue( int a_value )
{
    cout << "updateBlueValue" << endl;

    if ( m_cur_frame && !m_ignore_color_slider_sig )
    {
        uint32_t color;

        if ( m_use_hsv )
        {
            cout << "hsv" << endl;
            color = HSVToRGB( ui->sliderRed->value(), ui->sliderGreen->value(), a_value);
        }
        else
        {
            cout << "rgb" << endl;
            color = (ui->sliderRed->value() << 16) | (ui->sliderGreen->value() << 8) | a_value;
        }

        cout << "color " << hex << color << endl;

        setColor( m_cur_frame, color );
        paletteChanged();
    }
}

/**
 * @brief Slot to receive color band width change signals from UI
 * @param a_value - New color band width
 *
 * The palette will be updated and observer notified.
 */
void
PaletteEditDialog::widthValueChanged( int a_value )
{
    // On width text change update palette width then trigger image refresh

    if ( m_cur_frame && !m_ignore_color_change_sig )
    {
        m_pal_info.color_bands[getColorFrameIndex()].width = a_value;
        paletteChanged();
    }
}


//=================== Private Methods ===================


/**
 * @brief Handler method for the dialog close event
 * @param a_event - Close event
 *
 * The dialog will be closed using the hideWithPos() method to save geometry.
 */
void
PaletteEditDialog::closeEvent( QCloseEvent *a_event )
{
    (void)a_event;
    hideWithPos();
}

/**
 * @brief Gets the index of the color control frame relative to the containing frame/layout
 * @param a_frame - Control frame pointer to get the index of
 * @return Frame index or -1 if frame not found
 */
int
PaletteEditDialog::getColorFrameIndex( QFrame * a_frame )
{
    return ui->frameControls->layout()->indexOf( a_frame?a_frame:m_cur_frame );
}

/**
 * @brief Converts HSV color values to an RGB color.
 * @param h - Hue value
 * @param s - Saturation value
 * @param v - "Value" value (intensity)
 * @return An RGB color as an unsigned 32 bit integer (with alpha channel set to 255)
 */
uint32_t
PaletteEditDialog::HSVToRGB( uint16_t h, uint16_t s, uint16_t v)
{
    //uint8_t r, g, b;
    double  c = v * s / ((double)HSV_SLIDER_VAL_MAX * HSV_SLIDER_SAT_MAX ),
        x = c * (1 - fabs(fmod(h/60.0, 2.0) - 1)),
        m = ((double)v/HSV_SLIDER_VAL_MAX) - c,
        r, g, b;

    if ( h <= 60 )
    {
        r = c;  g = x;  b = 0;
    }
    else if ( h <= 120 )
    {
        r = x;  g = c;  b = 0;
    }
    else if ( h <= 180 )
    {
        r = 0;  g = c;  b = x;
    }
    else if ( h <= 240 )
    {
        r = 0;  g = x;  b = c;
    }
    else if ( h <= 300 )
    {
        r = x;  g = 0;  b = c;
    }
    else
    {
        r = c;  g = 0;  b = x;
    }

    uint8_t r2 = (r + m)*255,
        g2 = (g + m)*255,
        b2 = (b + m)*255;

    return 0xFF000000 | ( r2 << 16 ) | ( g2 << 8 ) | b2;
}

/**
 * @brief Inserts a new color control panel at the specified index
 * @param a_index - The insertion index within the color control frame
 *
 * Inserts a new set of color control (with black color) at the specified
 * index. The palette is updated and the observer notified.
 */
void
PaletteEditDialog::insertColorControls( int a_index )
{
    m_ignore_color_change_sig = true;

    // Create frame to hold all color controls for given palette index
    QFrame *frame = new QFrame(this);
    frame->setFrameStyle(QFrame::Plain);
    frame->setStyleSheet("QFrame { border: 1px solid transparent}");
    frame->installEventFilter( &m_event_handler );

    QHBoxLayout *layout = new QHBoxLayout();

    QLabel *labelColor = new QLabel("      ",frame);
    labelColor->setStyleSheet("QLabel {background: black}");
    layout->addWidget( labelColor ); // Color swatch

    // Color value in hex
    QLineEdit * edit = new QLineEdit(frame);
    edit->setText("000000");
    edit->setInputMask("HHHHHH");
    edit->setMaxLength(6);
    layout->addWidget( edit );
    edit->installEventFilter( &m_event_handler );
    connect( edit, SIGNAL(textEdited(QString)), this, SLOT(colorTextChanged(QString)));

     // Width input
    QSpinBox *spin = new QSpinBox(frame);
    spin->setMinimum(1);
    spin->setMaximum(100);
    spin->setValue(5);
    layout->addWidget( spin );
    spin->installEventFilter( &m_event_handler );
    connect( spin, SIGNAL(valueChanged(int)), this, SLOT(widthValueChanged(int)));

    // Mode selection
    QComboBox *combo = new QComboBox(frame);
    combo->addItem( "Flat" );
    combo->addItem( "Linear" );
    combo->setCurrentIndex(1);
    layout->addWidget( combo );
    combo->installEventFilter( &m_event_handler );
    connect( combo, SIGNAL( currentIndexChanged(int)), this, SLOT( modeIndexChanged(int)));

    frame->setLayout(layout);

    QBoxLayout* box = static_cast<QBoxLayout*>(ui->frameControls->layout());
    box->insertWidget( a_index, frame );

    m_ignore_color_change_sig = false;
}

/**
 * @brief Handles palette change processing.
 *
 * This method is called whenever any UI inputs cause the palette state to change.
 * The observer is notified, and if the changed (saved) state is altered, the dialog
 * title and save button enabler is updated.
 */
void
PaletteEditDialog::paletteChanged()
{
    bool prev = m_pal_info.changed;

    m_observer.paletteChanged();

    if ( prev != m_pal_info.changed )
    {
        updateWindowTitle();
        ui->buttonSavePal->setDisabled( !m_pal_info.changed );
    }
}

/**
 * @brief Converts RGB color values to an HSV color.
 * @param r - Red value
 * @param g - Green value
 * @param b - Blue value
 * @return An HSV color struct
 */
PaletteEditDialog::HSV_t
PaletteEditDialog::RGBToHSV( uint16_t r, uint16_t g, uint16_t b )
{
    double  cmax = max({r,g,b}),
        cmin = min({r,g,b}),
        delta = cmax - cmin,
        h1;

    HSV_t hsv;

    if ( cmax == cmin ) // Ok b/c doubles initialized with integers
    {
        hsv.h = 0;
    }
    else
    {
        if ( r == cmax )
        {
            h1 = (double)( g - b )/delta;
            if ( h1 < 0 )
                h1 += 6;
        }
        else if ( g == cmax )
        {
            h1 = (double)( b - r )/delta + 2;
        }
        else
        {
            h1 = (double)( r - g )/delta + 4;
        }

        hsv.h = 60.0 * h1;
    }

    hsv.s = cmax > 0 ? delta/cmax : 0;
    hsv.v = cmax/255.0;

    return hsv;
}

/**
 * @brief Sets color band info for palette at given index
 * @param a_index - Index of color within palette
 * @param a_color - New color band information
 * @param a_update_palette - Flag indicating if dialog palette should be updated
 */
void
PaletteEditDialog::setColor( int a_index, const PaletteGenerator::ColorBand & a_color, bool a_update_palette )
{
    QFrame *frame = static_cast<QFrame*>(ui->frameControls->layout()->itemAt( a_index )->widget());
    setColor( frame, a_color, a_update_palette );
}

/**
 * @brief Sets color band info for specified color control frame
 * @param a_frame - Color control frame to update
 * @param a_color - New color band information
 * @param a_update_palette - Flag indicating if dialog palette should be updated
 */
void
PaletteEditDialog::setColor( QFrame *a_frame, const PaletteGenerator::ColorBand & a_color, bool a_update_palette )
{

    QLabel *label = static_cast<QLabel*>(a_frame->layout()->itemAt(0)->widget()); // Color swatch
    QString color_hex = QString("%1").arg(a_color.color & 0xFFFFFF,6,16,QChar('0'));
    label->setStyleSheet(QString("QLabel {background: #%1}").arg(color_hex));

    QLineEdit *edit = static_cast<QLineEdit*>(a_frame->layout()->itemAt(1)->widget()); // Color input
    edit->setText( color_hex );

    QSpinBox *spin = static_cast<QSpinBox*>(a_frame->layout()->itemAt(2)->widget()); // Width input
    spin->setValue( a_color.width );

    QComboBox *combo = static_cast<QComboBox*>(a_frame->layout()->itemAt(3)->widget()); // Mode combobox
    combo->setCurrentIndex((int)a_color.mode);

    if ( a_update_palette )
    {
        m_pal_info.color_bands[getColorFrameIndex(a_frame)] = a_color;
    }
}

/**
 * @brief Sets color for specified color control frame
 * @param a_frame - Color control frame to update
 * @param a_color - New color
 * @param a_update_palette - Flag indicating if dialog palette should be updated
 */
void
PaletteEditDialog::setColor( QFrame *a_frame, uint32_t a_color, bool a_update_palette )
{
    QLabel *label = static_cast<QLabel*>(a_frame->layout()->itemAt(0)->widget()); // Color swatch
    QString color_hex = QString("%1").arg( a_color & 0xFFFFFF,6,16,QChar('0'));
    label->setStyleSheet(QString("QLabel {background: #%1}").arg(color_hex));

    QLineEdit *edit = static_cast<QLineEdit*>(a_frame->layout()->itemAt(1)->widget()); // Color input
    edit->setText( color_hex );

    if ( a_update_palette )
    {
        m_pal_info.color_bands[getColorFrameIndex(a_frame)].color = 0xFF000000 | a_color;
    }
}

/**
 * @brief Updates color sliders to match specified color
 * @param a_color - New color
 *
 * This method updates the color sliders based on the current color mode (RGB or HSV).
 */
void
PaletteEditDialog::setColorSliders( uint32_t a_color )
{
    m_ignore_color_slider_sig = true;

    if ( m_use_hsv )
    {
        int r = ( a_color >> 16 ) & 0xFF,
            g = ( a_color >> 8 ) & 0xFF,
            b = a_color & 0xFF;

        HSV_t hsv = RGBToHSV( r, g, b );

        ui->sliderRed->setValue( hsv.h );
        ui->sliderGreen->setValue( hsv.s * HSV_SLIDER_SAT_MAX );
        ui->sliderBlue->setValue( hsv.v * HSV_SLIDER_VAL_MAX);
    }
    else
    {
        ui->sliderRed->setValue(( a_color >> 16 ) & 0xFF );
        ui->sliderGreen->setValue(( a_color >> 8 ) & 0xFF );
        ui->sliderBlue->setValue( a_color & 0xFF );
    }

    m_ignore_color_slider_sig = false;
}

/**
 * @brief Sets the color for the color swatch in the specified color controls
 * @param a_frame - The color control frame containing the swatch to be updated
 * @param a_color - The new color
 */
void
PaletteEditDialog::setColorSwatch( QFrame *a_frame, uint32_t a_color )
{
    QLabel *label = static_cast<QLabel*>(a_frame->layout()->itemAt(0)->widget()); // Color swatch
    QString color_hex = QString("%1").arg( a_color & 0xFFFFFF,6,16,QChar('0'));
    label->setStyleSheet(QString("QLabel {background: #%1}").arg(color_hex));
}

/**
 * @brief Updates UI when focused control frame is changed
 * @param a_frame - The frame that is receiving focus
 * @param a_force_refresh - Force refresh even if current focus hasn't changed
 */
void
PaletteEditDialog::setFocus( QFrame * a_frame, bool a_force_refresh )
{
    if ( !a_force_refresh && a_frame == m_cur_frame )
        return;

    if ( m_cur_frame )
    {
        m_cur_frame->setStyleSheet("QFrame {border: 1px solid transparent}");
    }

    m_cur_frame = a_frame;
    m_cur_frame->setStyleSheet("QFrame {border: 1px solid #b0b000}");

    setColorSliders( m_pal_info.color_bands[getColorFrameIndex()].color );
}

/**
 * @brief Updates UI when focused control frame is changed
 * @param a_index - The frame index that is receiving focus
 * @param a_force_refresh - Force refresh even if current focus hasn't changed
 */
void
PaletteEditDialog::setFocus( int a_index, bool a_force_refresh )
{
    setFocus( static_cast<QFrame*>(ui->frameControls->layout()->itemAt(a_index)->widget()), a_force_refresh );
}

/**
 * @brief Swaps the color controls at the specified indices
 * @param a_index1 - First color control index
 * @param a_index2 - Second color control index
 */
void
PaletteEditDialog::swapColors( int a_index1, int a_index2 )
{
    QVBoxLayout * layout = static_cast<QVBoxLayout*>(ui->frameControls->layout());
    QLayoutItem * item = layout->itemAt(a_index1);

    layout->removeItem( item );
    layout->insertItem( a_index2, item );

    iter_swap(m_pal_info.color_bands.begin() + a_index1, m_pal_info.color_bands.begin() + a_index2 );
}

/**
 * @brief Updates the dialog title based on current palette and state
 */
void
PaletteEditDialog::updateWindowTitle()
{
    QString title = QString("Palette Edit - %1").arg( QString::fromStdString( m_pal_info.name ));
    if ( m_pal_info.built_in )
    {
        title += " (read only)";
    }
    else if ( m_pal_info.changed )
    {
        title += "*";
    }

    setWindowTitle( title );
}


//=================== EventHandler Methods ===================

/**
 * @brief The handler for events of interest within color controls
 * @param a_object - The object associated with the event
 * @param a_event - The event
 * @return Always false to allow further processing
 *
 * This event handler intercepts specific events that occur on color controls
 * in order to shift focus to the associated color control frame. This is needed
 * to ensure control changes are reflected to the correct palette entry.
 */
bool
PaletteEditDialog::EventHandler::eventFilter( QObject * a_object, QEvent *a_event )
{
    if ( a_event->type() == QEvent::MouseButtonPress || a_event->type() == QEvent::FocusIn )
    {
        QFrame * frame = dynamic_cast<QFrame*>(a_object);
        if ( !frame && a_object )
        {
            frame = static_cast<QFrame*>( a_object->parent() );
        }

        if ( frame )
        {
            m_palette_edit_dlg.setFocus( frame );
        }
    }

    return false;
}
