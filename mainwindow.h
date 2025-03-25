#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QString>
#include <map>
#include <vector>
#include "MandelbrotViewer.h"
#include "mandelbrotcalc.h"
#include "paletteinfo.h"
#include "paletteeditdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE


class MainWindow : public QMainWindow, IMandelbrotViewerObserver, IPaletteEditObserver
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void closeEvent( QCloseEvent *event );
    void Calculate();
    void ZoomTop();
    void prev();
    void next();
    void aspectChange( int index );
    void paletteSelect( const QString &text );
    void paletteEdit();
    void PaletteOffsetSliderChanged(int);
    void PaletteScaleSliderChanged(int);
    void SaveImage();

private:
    typedef std::map<std::string,PaletteInfo> PaletteMap_t;

    uchar * renderImage();
    void    drawImage();
    void    zoomIn( const QRectF & rect );
    void    recenter( const QPointF & pos );
    void    paletteChanged();   // IPaletteEditObserver
    void    paletteNew();
    void    paletteDuplicate( const PaletteInfo & palette_info );
    void    paletteSave( PaletteInfo & palette_info );      // IPaletteEditObserver
    void    paletteDelete( const PaletteInfo & palette_info );    // IPaletteEditObserver
    void    settingsPaletteLoadAll();
    void    settingsPaletteSave( PaletteInfo & palette_info );
    void    settingsPaletteDelete( const std::string & palette_name );
    QString inputPaletteName( const QString & a_title );

    struct AspectRatio
    {
        QString         name;
        uint8_t         major;
        uint8_t         minor;
    };

    struct CalcPos
    {
        double      x1;
        double      y1;
        double      x2;
        double      y2;
    };

    Ui::MainWindow *            ui;
    QSettings                   m_settings;
    MandelbrotCalc              m_calc;
    MandelbrotViewer *          m_viewer;
    PaletteEditDialog           m_palette_edit_dlg;
    bool                        m_palette_dlg_edit_init;
    PaletteGenerator            m_palette_gen;
    MandelbrotCalc::CalcResult  m_calc_result;
    uint8_t                     m_calc_ss;
    PaletteMap_t                m_palette_map;
    //std::string                 m_cur_palette_name;
    uint16_t                    m_palette_scale;
    uint32_t                    m_palette_offset;
    bool                        m_ignore_pal_sig;
    bool                        m_ignore_off_sig;
    bool                        m_ignore_aspect_sig;
    MandelbrotCalc::CalcParams  m_calc_params;
    std::vector<AspectRatio>    m_aspect_ratios;
    std::vector<CalcPos>        m_calc_history;
    uint32_t                    m_calc_history_idx;

    friend class MandelbrotViewer;
};

#endif // MAINWINDOW_H
