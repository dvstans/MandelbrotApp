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
#include "calcstatusdialog.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

/**
 * @brief The MainWindow class
 *
 * Standard Qt main window class with public slots for handling UI signals.
 * A contained MandelbrotCalc instance is used for computing the mandelbrot set
 * depth buffer (not images). A MandelbrotViewer instance is used to display
 * rendered images and handle mouse and keyboard events within the view. The
 * MainWindow inherits from the IMandelbrotViewerObserver interface to receive
 * callbacks from the viewer instance.
 *
 * A PaletteEditDialog instance is also contained within the MainWindow -
 * which inherits from the IPaletteEditObserver interface to receive callbacks
 * from the palette edit dialog.
 */
class MainWindow : public QMainWindow, IMandelbrotViewerObserver, IPaletteEditObserver, MandelbrotCalc::IObserver
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void calcCompleted();

public slots:
    void aspectChange( int index );
    void calculate();
    void closeEvent( QCloseEvent *event );
    void paletteEdit();
    void paletteOffsetSliderChanged(int);
    void paletteScaleSliderChanged(int);
    void paletteSelect( const QString &text );
    void imageSave();
    void imageLoad();
    void viewNext();
    void viewPrev();
    void viewTop();
    void zoomIn();
    void zoomOut();

private:
    typedef std::map<std::string,PaletteInfo> PaletteMap_t;

    void    adjustPalette( const QString &a_text );
    void    adjustScaleSliderChanged( int a_scale );
    void    imageDraw();
    uchar * imageRender();
    QString inputPaletteName( const QString & a_title );
    void    runCalculate();
    void    settingsPaletteDelete( const std::string & palette_name );
    void    settingsPaletteLoadAll();
    void    settingsPaletteSave( PaletteInfo & palette_info );

    // IMandelbrotViewerObserver methods
    void    imageRecenter( const QPointF & pos );
    void    imageZoomIn( const QRectF & rect );

    // IPaletteEditObserver methods
    void    paletteChanged();
    void    paletteDelete( const PaletteInfo & palette_info );
    void    paletteDuplicate( const PaletteInfo & palette_info );
    void    paletteNew();
    bool    paletteSave( PaletteInfo & palette_info );

    // MandelbrotCalc::IObserver methods
    void    cbCalcProgress( int a_progress );
    void    cbCalcCompleted( MandelbrotCalc::Result a_result );
    void    cbCalcCancelled();

    // JSON helper methods
    bool    jsonReadBool( const QJsonObject & obj, const QString & key );
    double  jsonReadDouble( const QJsonObject & obj, const QString & key );
    int     jsonReadInt( const QJsonObject & obj, const QString & key );
    QString jsonReadString( const QJsonObject & obj, const QString & key );

    /**
     * @brief The AspectRatio class defines zoom-window name and major/minor axis proportions
     *
     * A selection of standard image aspect ratios are provided on the main
     * window. The MandelbrotViewer uses the selected ratio to restrict the
     * aspect ratio of the mouse zoom window.
     */
    struct AspectRatio
    {
        QString         name;   // Aspect ration name (i.e. "16:9")
        uint8_t         major;  // Major axis (i.e. 16)
        uint8_t         minor;  // Minor axis (i.e. 9)
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
    bool                        m_palette_repeat;
    MandelbrotCalc::Params      m_calc_params;
    MandelbrotCalc::Result      m_calc_result;
    uint8_t                     m_calc_ss;
    PaletteMap_t                m_palette_map;
    uint16_t                    m_palette_scale;
    uint32_t                    m_palette_offset;
    bool                        m_ignore_pal_sig;
    bool                        m_ignore_scale_sig;
    bool                        m_ignore_off_sig;
    bool                        m_ignore_aspect_sig;
    std::vector<AspectRatio>    m_aspect_ratios;
    std::vector<CalcPos>        m_calc_history;
    uint32_t                    m_calc_history_idx;
    QString                     m_cur_dir;
    QString                     m_app_name;
    CalcStatusDialog            m_status_dlg;

    friend class MandelbrotViewer;
};

#endif // MAINWINDOW_H
