#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <map>
#include <vector>
#include "MandelbrotViewer.h"
#include "mandelbrotcalc.h"
#include "palette.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE


class MainWindow : public QMainWindow, IMandelbrotViewerObserver
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void Exit();
    void Calculate();
    void ZoomTop();
    void prev();
    void next();
    void aspectChange( int index );
    void PaletteChange( const QString &text );
    void PaletteOffsetSliderChanged(int);
    void PaletteScaleSliderChanged(int);
    void SaveImage();

private:
    uchar * renderImage();
    void    drawImage();
    void    zoomIn( const QRectF & rect );
    void    recenter( const QPointF & pos );

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
    MandelbrotCalc              m_calc;
    MandelbrotViewer *          m_viewer;
    Palette                     m_palette;
    MandelbrotCalc::CalcResult  m_calc_result;
    uint8_t                     m_calc_ss;
    std::map<std::string,std::vector<Palette::ColorBand>>  m_palette_map;
    uint16_t                    m_palette_scale;
    uint32_t                    m_palette_offset;
    bool                        m_ignore_pal_sig;
    bool                        m_ignore_off_sig;
    MandelbrotCalc::CalcParams  m_calc_params;
    std::vector<AspectRatio>    m_aspect_ratios;
    std::vector<CalcPos>        m_calc_history;
    uint32_t                    m_calc_history_idx;

    friend class MandelbrotViewer;
};

#endif // MAINWINDOW_H
