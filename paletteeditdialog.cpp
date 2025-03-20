#include "paletteeditdialog.h"
#include "ui_paletteeditdialog.h"

PaletteEditDialog::PaletteEditDialog(QWidget *parent):
    QDialog(parent),
    ui(new Ui::PaletteEditDialog),
    m_geometry_set(false)
{
    ui->setupUi(this);

    insertColorControls(0);
    insertColorControls(1);
}

PaletteEditDialog::~PaletteEditDialog()
{
    delete ui;
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
PaletteEditDialog::insertColorControls( int a_pos )
{
    QFrame *frame = new QFrame(this);
    QHBoxLayout *layout = new QHBoxLayout();
    layout->addWidget( new QPushButton("1",frame));
    layout->addWidget( new QPushButton("2",frame));
    layout->addWidget( new QPushButton("3",frame));
    frame->setLayout(layout);

    QBoxLayout* box = static_cast<QBoxLayout*>(ui->frameControls->layout());
    box->insertWidget( a_pos, frame );
}
