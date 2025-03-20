#ifndef MANDELBROTVIEWER_H
#define MANDELBROTVIEWER_H

#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsPixmapItem>

class IMandelbrotViewerObserver
{
public:
    virtual void zoomIn( const QRectF & rect ) = 0;
    virtual void recenter( const QPointF & pos ) = 0;
};

class MandelbrotViewer : public QGraphicsView
{
public:
    MandelbrotViewer( QFrame& a_parent, IMandelbrotViewerObserver & a_observer );
    ~MandelbrotViewer();

    QImage      getImage();
    void        setImage( const QImage & image );

private:
    void mousePressEvent( QMouseEvent *event );
    void mouseMoveEvent( QMouseEvent *event );
    void mouseReleaseEvent( QMouseEvent *event );
    bool inBounds( const QPointF &a_point );
    bool selecetRectIntersect( const QPointF &a_p1, const QPointF &a_p2 );

    QFrame &                    m_parent;
    IMandelbrotViewerObserver & m_observer;
    QGraphicsRectItem *         m_view_rect;
    QGraphicsPixmapItem *       m_view_pixmap;
    bool                        m_dragging;
    uint32_t                    m_buttons;
    QPointF                     m_sel_origin;
    QRectF                      m_sel_rect;
    int                         m_width;
    int                         m_height;
};

#endif // MANDELBROTVIEWER_H
