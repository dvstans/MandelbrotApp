#include <iostream>

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
//#include <QPalette>
#include <QStyleFactory>

#include "paletteeditdialog.h"
#include "ui_paletteeditdialog.h"

using namespace std;

PaletteEditDialog::PaletteEditDialog(QWidget *parent):
    QDialog(parent),
    m_event_handler(*this),
    ui(new Ui::PaletteEditDialog),
    m_geometry_set(false),
    m_cur_frame(0)
{
    ui->setupUi(this);
}


PaletteEditDialog::~PaletteEditDialog()
{
    delete ui;
}


void
PaletteEditDialog::setPalette( Palette::Colors & a_colors )
{
    QLayout *layout = ui->frameControls->layout();
    size_t i;
    size_t color_count = layout->count() - 1; // -1 for vertical spacer at bottom

    m_colors = a_colors;

    //cout << "cur col count " << color_count << " new: " << a_colors.size() << endl;

    if ( a_colors.size() > color_count )
    {
        for ( i = color_count; i < a_colors.size(); i++ )
        {
            insertColorControls( i );
        }
    }
    else if ( color_count > a_colors.size() )
    {
        QLayoutItem *color_layout;
        QWidget * frame;

        for ( i = a_colors.size(); i < color_count; i++ )
        {
            color_layout = layout->itemAt(a_colors.size());
            frame = color_layout->widget();
            layout->removeItem( color_layout );
            delete frame;
        }
    }

    Palette::Colors::iterator c = a_colors.begin();
    for ( i = 0; i < a_colors.size(); i++, c++ )
    {
        setColor( i, *c );
    }

    //cout << "cc " << m_colors.size() << endl;

    setFocus( 0 );
}


void
PaletteEditDialog::showWithPos()
{
    QDialog::show();
    if ( m_geometry_set )
        setGeometry(m_geometry);
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
    if ( m_colors.size() > 1 )
    {
        int index = getFocusIndex();
        if ( index > 0 )
        {
            swapColors( index, index - 1 );
        }
    }
}

void PaletteEditDialog::moveColorDown()
{
    if ( m_colors.size() > 1 )
    {
        int index = getFocusIndex();
        if ( index < (int)m_colors.size() - 1 )
        {
            swapColors( index, index + 1 );
        }
    }
}


void PaletteEditDialog::insertColor()
{
    int index = getFocusIndex();
    insertColorControls( index + 1 );

    Palette::ColorBand band {
        0,
        10,
        Palette::CM_LINEAR
    };

    m_colors.insert( m_colors.begin() + index + 1, band );
    setColor( index + 1, band );

    setFocus( index + 1 );
}


void PaletteEditDialog::deleteColor()
{
    if ( m_colors.size() > 1 )
    {
        int index = getFocusIndex();

        QLayout *layout = ui->frameControls->layout();
        QLayoutItem *color_layout = layout->itemAt(index);
        QWidget * frame = color_layout->widget();

        layout->removeItem( color_layout );
        delete frame;

        m_colors.erase( m_colors.begin() + index );
        setFocus( index < (int)m_colors.size()?index:index-1 );
    }
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
    // Create frame to hold all color controls for given palette index
    QFrame *frame = new QFrame(this);
    frame->setFrameStyle(QFrame::Plain);
    frame->setStyleSheet("QFrame { border: 1px solid transparent}");
    frame->installEventFilter( &m_event_handler );

    QHBoxLayout *layout = new QHBoxLayout();

    QLabel *labelColor = new QLabel("      ",frame);
    labelColor->setStyleSheet("QLabel {background: black}");
    layout->addWidget( labelColor ); // Color swatch

    QLineEdit * edit = new QLineEdit(frame);
    edit->setText("000000");
    layout->addWidget( edit ); // Color value in hex
    edit->installEventFilter( &m_event_handler );

    edit = new QLineEdit(frame);
    edit->setText(QString("%1").arg(10));
    layout->addWidget( edit ); // Width input
    edit->installEventFilter( &m_event_handler );

    // TODO style is not being applied to combobox
    //combo->setStyle(QStyleFactory::create("Fusion"));
    QComboBox *combo = new QComboBox(frame);
    combo->addItem( "Flat" );
    combo->addItem( "Linear" );
    combo->setCurrentIndex(1);
    layout->addWidget( combo ); // Mode selection
    combo->installEventFilter( &m_event_handler );

    frame->setLayout(layout);

    QBoxLayout* box = static_cast<QBoxLayout*>(ui->frameControls->layout());
    box->insertWidget( a_index, frame );

}


void
PaletteEditDialog::setColor( int a_index, const Palette::ColorBand & a_color )
{
    QFrame *frame = static_cast<QFrame*>(ui->frameControls->layout()->itemAt( a_index )->widget());

    QLabel *label = static_cast<QLabel*>(frame->layout()->itemAt(0)->widget()); // Color swatch
    QString color_hex = QString("%1").arg(a_color.color & 0xFFFFFF,6,16,QChar('0'));
    label->setStyleSheet(QString("QLabel {background: #%1}").arg(color_hex));

    QLineEdit *edit = static_cast<QLineEdit*>(frame->layout()->itemAt(1)->widget()); // Color input
    edit->setText( color_hex );

    edit = static_cast<QLineEdit*>(frame->layout()->itemAt(2)->widget()); // Width input
    edit->setText( QString("%1").arg( a_color.width ));

    QComboBox *combo = static_cast<QComboBox*>(frame->layout()->itemAt(3)->widget()); // Mode combobox
    combo->setCurrentIndex((int)a_color.mode);
}


void
PaletteEditDialog::swapColors( int a_index1, int a_index2 )
{
    QVBoxLayout * layout = static_cast<QVBoxLayout*>(ui->frameControls->layout());
    QLayoutItem * item = layout->itemAt(a_index1);

    layout->removeItem( item );
    layout->insertItem( a_index2, item );

    iter_swap(m_colors.begin() + a_index1, m_colors.begin() + a_index2 );
}


void
PaletteEditDialog::setFocus( QFrame * a_frame )
{
    if ( a_frame == m_cur_frame )
        return;

    if ( m_cur_frame  )
    {
        m_cur_frame->setStyleSheet("QFrame {border: 1px solid transparent}");
    }

    m_cur_frame = a_frame;
    m_cur_frame->setStyleSheet("QFrame {border: 1px solid #b0b000}");
}

void
PaletteEditDialog::setFocus( int a_index )
{
    setFocus(static_cast<QFrame*>(ui->frameControls->layout()->itemAt(a_index)->widget()));
}

int
PaletteEditDialog::getFocusIndex()
{
    return ui->frameControls->layout()->indexOf( m_cur_frame );
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
