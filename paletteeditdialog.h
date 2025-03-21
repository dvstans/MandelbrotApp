#ifndef PALETTEEDITDIALOG_H
#define PALETTEEDITDIALOG_H

#include <QDialog>
#include <QFrame>
#include <QCloseEvent>
#include "palette.h"

namespace Ui {
class PaletteEditDialog;
}

class PaletteEditDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PaletteEditDialog(QWidget *parent = nullptr);
    ~PaletteEditDialog();

    void setPalette( Palette::Colors & colors );

public slots:
    void showWithPos();
    void hideWithPos();

private:
    class EventHandler : public QObject
    {
    public:
        EventHandler( PaletteEditDialog & a_dlg ) :
            m_palette_edit_dlg( a_dlg )
        {}

    protected:
        PaletteEditDialog & m_palette_edit_dlg;

        bool eventFilter( QObject *obj, QEvent *event ) override;
    };

    void    closeEvent( QCloseEvent *event );
    void    insertColorControls( int index );
    void    setColorControl( int index, uint32_t color, uint16_t width, Palette::ColorMode mode );
    void    setFocus( int index );
    void    setFocus( QFrame * a_frame );

    EventHandler            m_event_handler;
    Ui::PaletteEditDialog*  ui;
    QRect                   m_geometry;
    bool                    m_geometry_set;
    QFrame *                m_cur_frame;
    int                     m_cur_index;

    friend class EventHandler;
};

#endif // PALETTEEDITDIALOG_H
