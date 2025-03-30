#ifndef MANDELBROTVIEWER_H
#define MANDELBROTVIEWER_H

#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsPixmapItem>

/**
 * @brief The IMandelbrotViewerObserver class
 *
 * This interface is used by clients to receive callbacks from an owned
 * MandelbrotViewer instance.
 */
class IMandelbrotViewerObserver
{
public:
    virtual void imageZoomIn( const QRectF & rect ) = 0;
    virtual void imageRecenter( const QPointF & pos ) = 0;
};

/**
 * @brief The MandelbrotViewer class
 *
 * This class is inherited from a QGraphicsView in order to display a rendered
 * Mandelbrot image and capture mouse and keyboard events. Mouse dragging is
 * used to pan, recenter, and zoom-in on the displayed image.
 */
class MandelbrotViewer : public QGraphicsView
{
public:
    MandelbrotViewer( QFrame& parent, IMandelbrotViewerObserver & observer );
    ~MandelbrotViewer();

    QImage      getImage();
    void        setImage( const QImage & image );
    void        setAspectRatio( uint8_t major, uint8_t minor );

private:
    void mousePressEvent( QMouseEvent *event );
    void mouseMoveEvent( QMouseEvent *event );
    void mouseReleaseEvent( QMouseEvent *event );
    void keyReleaseEvent( QKeyEvent *event );
    bool inBounds( const QPointF &point );
    bool selectRectIntersect( const QPointF &origin, const QPointF &cursor );

    QFrame &                    m_parent;               // Parent QFrame of the viewer
    IMandelbrotViewerObserver & m_observer;             // The viewer observer (for callbacks)
    QGraphicsRectItem *         m_view_rect;            // A rect used to display zoom window while dragging
    QGraphicsPixmapItem *       m_view_pixmap;          // The image to display
    bool                        m_zooming;              // Flag indicating zoom window dragging in progress
    bool                        m_panning;              // Flag indicating panning in progress
    uint32_t                    m_buttons;              // Contains buttons used at start of mouse drag
    QPointF                     m_origin;               // Origin of mouse event
    QRectF                      m_sel_rect;             // Selection rectangle
    int                         m_width;                // Image width
    int                         m_height;               // Image height
    bool                        m_use_aspect_ratio;     // Flag indicating restricted aspect ratio shuld be used
    double                      m_aspect_ratio;         // Aspect ratio for zoom window
    int                         m_hscroll_val;          // Horizontal scroll bar position need when panning
    int                         m_vscroll_val;          // Vertical scroll bar position need when panning
};

#endif // MANDELBROTVIEWER_H
