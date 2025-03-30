#ifndef PALETTEEDITDIALOG_H
#define PALETTEEDITDIALOG_H

#include <QDialog>
#include <QFrame>
#include <QCloseEvent>
#include "paletteinfo.h"

/**
 * @brief The IPaletteEditObserver interface defines required callbacks for observers
 */
class IPaletteEditObserver
{
public:
    virtual void paletteChanged() = 0;
    virtual void paletteNew() = 0;
    virtual void paletteDuplicate( const PaletteInfo & palette_info ) = 0;
    virtual bool paletteSave( PaletteInfo & palette_info ) = 0;
    virtual void paletteDelete( const PaletteInfo & palette_info ) = 0;
};

namespace Ui {
class PaletteEditDialog;
}

/**
 * @brief The PaletteEditDialog class displays a modeless dialog to allow palette editing
 *
 * The PaletteEditDialog implements an editor for color palettes and utilizes
 * the IPaletteEditObserver interface to notify the observer when the palette
 * is changed. This permits the associated image to be updated in real time
 * with palette edits.
 */
class PaletteEditDialog : public QDialog
{
    Q_OBJECT

public:
    PaletteEditDialog( QWidget *parent, IPaletteEditObserver & observer );
    ~PaletteEditDialog();

    PaletteInfo &   getPaletteInfo();
    void            setPaletteInfo( PaletteInfo & palette_info );


public slots:
    void colorDelete();
    void colorInsert();
    void colorMoveUp();
    void colorMoveDown();
    void colorTextChanged( const QString & text );
    void hideWithPos();
    void hsvStateChanged( int state );
    void modeIndexChanged( int value );
    void paletteDelete();
    void paletteDuplicate();
    void paletteNew();
    void paletteSave();
    void repeatStateChanged( int state );
    void showWithPos();
    void updateRedValue( int value );
    void updateGreenValue( int value );
    void updateBlueValue( int value );
    void widthValueChanged( int value );

private:
    /**
     * @brief The HSV_t class wraps a color defined by Hue, Saturation, and Value
     */
    struct HSV_t
    {
        double h; // 0 - 360
        double s; // 0 -> 1
        double v; // 0 -> 1
    };

    /**
     * @brief The EventHandler class is used to capture various UI events
     */
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


    void        closeEvent( QCloseEvent *event );
    int         getColorFrameIndex( QFrame * frame = nullptr );
    uint32_t    HSVToRGB( uint16_t h, uint16_t s, uint16_t v );
    void        insertColorControls( int index );
    void        paletteChanged();
    HSV_t       RGBToHSV( uint16_t r, uint16_t g, uint16_t b );
    void        setColor( int index, const PaletteGenerator::ColorBand & band, bool update_palette = true );
    void        setColor( QFrame * frame, const PaletteGenerator::ColorBand & band, bool update_palette = true );
    void        setColor( QFrame * frame, uint32_t color, bool update_palette = true );
    void        setColorSliders( uint32_t color );
    void        setColorSwatch( QFrame * frame, uint32_t color );
    void        setFocus( QFrame * a_frame, bool force_refresh = false );
    void        setFocus( int index, bool force_refresh = false );
    void        swapColors( int index1, int index2 );
    void        updateWindowTitle();

    IPaletteEditObserver &  m_observer;                 // The palette edit dialog observer instance
    PaletteInfo             m_pal_info;                 // Current color palette information
    EventHandler            m_event_handler;            // The internal event handler
    Ui::PaletteEditDialog*  ui;                         // Qt UI pointer
    QRect                   m_geometry;                 // Geometry of the dialog (position, size)
    bool                    m_geometry_set;             // Flag indicating if geometry has been set
    QFrame *                m_cur_frame;                // The current color frame
    bool                    m_ignore_color_slider_sig;  // Flag to cause slider signals to be ignored
    bool                    m_ignore_color_change_sig;  // Flag to cause color change signals to be ignored
    bool                    m_use_hsv;                  // Flag indicating if HSV controls should be used instead of RGB

    friend class EventHandler;
};

#endif // PALETTEEDITDIALOG_H
