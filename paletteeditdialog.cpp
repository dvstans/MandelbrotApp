#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QStyleFactory>

#include "paletteeditdialog.h"
#include "ui_paletteeditdialog.h"

using namespace std;

PaletteEditDialog::PaletteEditDialog( QWidget *a_parent, IPaletteEditObserver & a_observer ):
    QDialog(a_parent),
    m_observer( a_observer ),
    m_event_handler(*this),
    ui(new Ui::PaletteEditDialog),
    m_geometry_set(false),
    m_cur_frame(0),
    m_ignore_color_slider_sig(false),
    m_ignore_color_change_sig(false)
{
    ui->setupUi(this);
}


PaletteEditDialog::~PaletteEditDialog()
{
    delete ui;
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

PaletteInfo &
PaletteEditDialog::getPaletteInfo()
{
    return m_pal_info;
}


void
PaletteEditDialog::showWithPos()
{
    QDialog::show();

    if ( m_geometry_set )
    {
        setGeometry(m_geometry);
    }
}

void
PaletteEditDialog::hideWithPos()
{
    m_geometry = geometry();
    m_geometry_set = true;
    QDialog::hide();
}

void PaletteEditDialog::moveColorUp()
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

void PaletteEditDialog::moveColorDown()
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


void PaletteEditDialog::insertColor()
{
    int index = getColorFrameIndex();
    insertColorControls( index + 1 );

    PaletteGenerator::ColorBand band {
        0,
        5,
        PaletteGenerator::CM_LINEAR
    };

    m_pal_info.color_bands.insert( m_pal_info.color_bands.begin() + index + 1, band );
    setColor( index + 1, band, false );
    setFocus( index + 1 );
    setColorSliders( band.color );
    paletteChanged();
}


void PaletteEditDialog::deleteColor()
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


void
PaletteEditDialog::updateRedValue( int a_value )
{
    if ( m_cur_frame && !m_ignore_color_slider_sig  )
    {
        uint32_t color = (a_value << 16) | (ui->sliderGreen->value() << 8) | ui->sliderBlue->value();
        setColor( m_cur_frame, color );
        paletteChanged();
    }
}


void
PaletteEditDialog::updateGreenValue( int a_value )
{
    if ( m_cur_frame && !m_ignore_color_slider_sig  )
    {
        uint32_t color = (ui->sliderRed->value() << 16) | (a_value << 8) | ui->sliderBlue->value();
        setColor( m_cur_frame, color );
        paletteChanged();
    }
}


void
PaletteEditDialog::updateBlueValue( int a_value )
{
    if ( m_cur_frame && !m_ignore_color_slider_sig )
    {
        uint32_t color = (ui->sliderRed->value() << 16) | (ui->sliderGreen->value() << 8) | a_value;
        setColor( m_cur_frame, color );
        paletteChanged();
    }
}


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

void
PaletteEditDialog::repeatStateChanged( int a_state )
{
    if ( m_cur_frame && !m_ignore_color_change_sig )
    {
        m_pal_info.repeat = ( a_state == Qt::Checked ? true : false );
        paletteChanged();
    }
}

void
PaletteEditDialog::paletteNew()
{
    m_observer.paletteNew();
}

void
PaletteEditDialog::paletteDuplicate()
{
    m_observer.paletteDuplicate( m_pal_info );
}

void
PaletteEditDialog::paletteSave()
{
    m_observer.paletteSave( m_pal_info );

    // Update UI based on new status
    updateWindowTitle();
    ui->buttonSavePal->setDisabled( true );
}

void
PaletteEditDialog::paletteDelete()
{
    m_observer.paletteDelete( m_pal_info );
}

void
PaletteEditDialog::closeEvent( QCloseEvent *a_event )
{
    (void)a_event;
    hideWithPos();
}

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

void
PaletteEditDialog::setColor( int a_index, const PaletteGenerator::ColorBand & a_color, bool a_update_palette )
{
    QFrame *frame = static_cast<QFrame*>(ui->frameControls->layout()->itemAt( a_index )->widget());
    setColor( frame, a_color, a_update_palette );
}

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

void
PaletteEditDialog::setColorSwatch( QFrame *a_frame, uint32_t a_color )
{
    QLabel *label = static_cast<QLabel*>(a_frame->layout()->itemAt(0)->widget()); // Color swatch
    QString color_hex = QString("%1").arg( a_color & 0xFFFFFF,6,16,QChar('0'));
    label->setStyleSheet(QString("QLabel {background: #%1}").arg(color_hex));
}

void
PaletteEditDialog::swapColors( int a_index1, int a_index2 )
{
    QVBoxLayout * layout = static_cast<QVBoxLayout*>(ui->frameControls->layout());
    QLayoutItem * item = layout->itemAt(a_index1);

    layout->removeItem( item );
    layout->insertItem( a_index2, item );

    iter_swap(m_pal_info.color_bands.begin() + a_index1, m_pal_info.color_bands.begin() + a_index2 );
}


void
PaletteEditDialog::setColorSliders( uint32_t a_color )
{
    m_ignore_color_slider_sig = true;
    ui->sliderRed->setValue(( a_color >> 16 ) & 0xFF );
    ui->sliderGreen->setValue(( a_color >> 8 ) & 0xFF );
    ui->sliderBlue->setValue( a_color & 0xFF );
    m_ignore_color_slider_sig = false;
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
 * @brief Gets the index of the color control frame relative to the containing frame/layout
 * @param a_frame - Control frame pointer to get the index of
 * @return Frame index or -1 if frame not found
 */
int
PaletteEditDialog::getColorFrameIndex( QFrame * a_frame )
{
    return ui->frameControls->layout()->indexOf( a_frame?a_frame:m_cur_frame );
}


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
