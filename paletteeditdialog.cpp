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
    m_cur_frame(0),
    m_cur_index(-1)
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
    QLayoutItem *color_layout;
    size_t i;
    size_t color_count = layout->count() - 1; // -1 for vertical spacer at bottom
    QWidget * frame;

    cout << "cur col count " << color_count << " new: " << a_colors.size() << endl;

    if ( a_colors.size() > color_count )
    {
        for ( i = color_count; i < a_colors.size(); i++ )
        {
            insertColorControls(i);
        }
    }
    else if ( color_count > a_colors.size() )
    {
        for ( i = a_colors.size(); i < color_count; i++ )
        {
            color_layout = layout->itemAt(a_colors.size());
            frame = color_layout->widget();

            cout << i << " " << color_layout << endl;

            layout->removeItem( color_layout );
            delete frame;
        }
    }

    Palette::Colors::iterator c = a_colors.begin();
    for ( i = 0; i < a_colors.size(); i++, c++ )
    {
        setColorControl(i,c->color,c->width,c->mode);
    }

    if ( m_cur_index >= (int)a_colors.size() )
    {
        m_cur_frame = 0;
        m_cur_index = -1;
    }

    setFocus(0);
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

    cout << "new frame: " << frame << endl;

    QHBoxLayout *layout = new QHBoxLayout();

    QLabel *labelColor = new QLabel("      ",frame);
    labelColor->setStyleSheet("QLabel {background: white}");
    layout->addWidget( labelColor ); // Color swatch
    layout->addWidget( new QLineEdit(frame)); // Color value in hex
    layout->addWidget( new QLineEdit(frame)); // Width input

    // TODO style is not being applied to combobox
    QComboBox *combo = new QComboBox(frame);
    //combo->setStyle(QStyleFactory::create("Fusion"));
    combo->addItem( "Flat" );
    combo->addItem( "Linear" );
    layout->addWidget( combo ); // Mode selection

    frame->setLayout(layout);

    QBoxLayout* box = static_cast<QBoxLayout*>(ui->frameControls->layout());
    box->insertWidget( a_index, frame );
}


void
PaletteEditDialog::setColorControl( int a_index, uint32_t a_color, uint16_t a_width, Palette::ColorMode a_mode )
{
    QFrame *frame = static_cast<QFrame*>(ui->frameControls->layout()->itemAt( a_index )->widget());
    QLabel *label = static_cast<QLabel*>(frame->layout()->itemAt(0)->widget()); // Color swatch
    QString color_hex = QString("%1").arg(a_color & 0xFFFFFF,6,16,QChar('0'));
    label->setStyleSheet(QString("QLabel {background: #%1}").arg(color_hex));
    QLineEdit *edit = static_cast<QLineEdit*>(frame->layout()->itemAt(1)->widget()); // Color input
    edit->setText( color_hex );
    edit = static_cast<QLineEdit*>(frame->layout()->itemAt(2)->widget()); // Width input
    edit->setText( QString("%1").arg( a_width ));
    QComboBox *combo = static_cast<QComboBox*>(frame->layout()->itemAt(3)->widget()); // Mode combobox
    combo->setCurrentIndex((int)a_mode);
}

void
PaletteEditDialog::setFocus( int a_index )
{
    QFrame * frame = static_cast<QFrame*>(ui->frameControls->layout()->itemAt( a_index )->widget());

    if ( frame == m_cur_frame )
        return;

    if ( m_cur_frame )
    {
        //frame = static_cast<QFrame*>(ui->frameControls->layout()->itemAt( m_cur_focus )->widget());
        m_cur_frame->setStyleSheet("QFrame {border: 1px solid transparent}");
    }

    m_cur_frame = static_cast<QFrame*>(ui->frameControls->layout()->itemAt( a_index )->widget());
    m_cur_frame->setStyleSheet("QFrame {border: 1px solid #b0b000}");
    m_cur_index = a_index;
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
    //m_cur_index = a_index; TODO
}

bool
PaletteEditDialog::EventHandler::eventFilter( QObject * a_object, QEvent *a_event )
{
    if ( a_event->type() == QEvent::MouseButtonPress ) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(a_event);

        cout << "mouse " << a_object << endl;

        QFrame * frame = static_cast<QFrame*>(a_object);
        if ( frame )
            m_palette_edit_dlg.setFocus( frame );

        /*if (keyEvent->key() == Qt::Key_Tab) {
            // Special tab handling
            return true;
        } else
            return false;
        */
    }

    return false;
}
