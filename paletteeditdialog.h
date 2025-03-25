#ifndef PALETTEEDITDIALOG_H
#define PALETTEEDITDIALOG_H

#include <QDialog>
#include <QFrame>
#include <QCloseEvent>
#include "paletteinfo.h"

class IPaletteEditObserver
{
public:
    virtual void paletteChanged() = 0;
    virtual void paletteNew() = 0;
    virtual void paletteDuplicate( const PaletteInfo & palette_info ) = 0;
    virtual void paletteSave( PaletteInfo & palette_info ) = 0;
    virtual void paletteDelete( const PaletteInfo & palette_info ) = 0;
};

namespace Ui {
class PaletteEditDialog;
}

class PaletteEditDialog : public QDialog
{
    Q_OBJECT

public:
    PaletteEditDialog( QWidget *parent, IPaletteEditObserver & observer );
    ~PaletteEditDialog();

    void
    setPaletteInfo( PaletteInfo & palette_info );

    PaletteInfo &
    getPaletteInfo();

public slots:
    void showWithPos();
    void hideWithPos();
    void moveColorUp();
    void moveColorDown();
    void insertColor();
    void deleteColor();
    void updateRedValue( int value );
    void updateGreenValue( int value );
    void updateBlueValue( int value );
    void colorTextChanged( const QString & text );
    void widthValueChanged( int value );
    void modeIndexChanged( int value );
    void paletteNew();
    void paletteDuplicate();
    void paletteSave();
    void paletteDelete();

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


    void    updateWindowTitle();
    void    paletteChanged();
    void    closeEvent( QCloseEvent *event );
    void    insertColorControls( int index );
    QFrame* getColorControl( int index );
    void    setColor( int index, const PaletteGenerator::ColorBand & band, bool update_palette = true );
    void    setColor( QFrame * frame, const PaletteGenerator::ColorBand & band, bool update_palette = true );
    void    setColor( QFrame * frame, uint32_t color, bool update_palette = true );
    void    setColorSwatch( QFrame * frame, uint32_t color );
    void    swapColors( int index1, int index2 );
    void    setColorSliders( uint32_t color );
    void    setFocus( QFrame * a_frame, bool force_refresh = false );
    void    setFocus( int index, bool force_refresh = false );
    int     getColorFrameIndex( QFrame * frame = nullptr );

    IPaletteEditObserver &  m_observer;
    PaletteInfo             m_pal_info;
    EventHandler            m_event_handler;
    Ui::PaletteEditDialog*  ui;
    QRect                   m_geometry;
    bool                    m_geometry_set;
    QFrame *                m_cur_frame;
    bool                    m_ignore_color_slider_sig;
    bool                    m_ignore_color_change_sig;

    friend class EventHandler;
};

#endif // PALETTEEDITDIALOG_H
