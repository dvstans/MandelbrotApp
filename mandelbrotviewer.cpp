#include <QMouseEvent>
#include <QVBoxLayout>
#include "mandelbrotviewer.h"

using namespace std;

MandelbrotViewer::MandelbrotViewer( QFrame& a_parent, IMandelbrotViewerObserver & a_observer ):
    m_parent(a_parent),
    m_observer(a_observer),
    m_dragging(false),
    m_width(0),
    m_height(0),
    m_use_aspect_ratio(false)
{
    // Remove frame and margins
    setFrameShape( QFrame::NoFrame );
    setContentsMargins(0,0,0,0);
    setBackgroundBrush(QBrush(Qt::black));

    QGraphicsScene *scene = new QGraphicsScene();
    QRectF rect(0,0,10,10);
    m_view_rect = scene->addRect( rect, QPen(Qt::yellow,1), QBrush(Qt::NoBrush));

    QPixmap pixmap(500,500);
    pixmap.fill( Qt::red );
    m_view_pixmap = scene->addPixmap(pixmap);
    setScene(scene);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->setContentsMargins( 0, 0, 0, 0 );
    layout->addWidget( this );

    m_parent.setLayout( layout );

    m_view_rect->setZValue( 1 );
    m_view_pixmap->setZValue( 0 );
    m_view_rect->hide();
}

MandelbrotViewer::~MandelbrotViewer()
{
    // TODO - What needs to be cleanup from scene?
}

QImage
MandelbrotViewer::getImage()
{
    return m_view_pixmap->pixmap().toImage();
}

void
MandelbrotViewer::setImage(  const QImage & image )
{
    m_view_pixmap->setPixmap( QPixmap::fromImage( image ));

    m_width = image.width();
    m_height = image.height();

    // Adjust scene size to fix scrollbar extent if image gets smaller
    scene()->setSceneRect(scene()->itemsBoundingRect());
}


void
MandelbrotViewer::setAspectRatio( uint8_t a_major, uint8_t a_minor )
{
    if ( a_major && a_minor )
    {
        m_use_aspect_ratio = true;
        m_aspect_ratio = ((double)a_minor)/a_major;
    }
    else
    {
        m_use_aspect_ratio = false;
    }
}


void
MandelbrotViewer::mousePressEvent( QMouseEvent *a_event )
{
    m_buttons = a_event->buttons();
    QPointF pos = mapToScene(a_event->pos());

    // Start zoom window on left mouse button down event
    if ( m_buttons == Qt::LeftButton && a_event->modifiers() == Qt::NoModifier )
    {
        m_dragging = true;
        m_sel_origin = pos;
        m_sel_rect.setWidth(0);
        m_sel_rect.setHeight(0);

        if ( inBounds( pos ))
        {
            m_sel_rect.setRect( pos.x(), pos.y(), 0, 0 );
            m_view_rect->setRect( m_sel_rect );
            m_view_rect->show();
        }
    }
    else if ( m_buttons == ( Qt::RightButton | Qt::LeftButton ) && a_event->modifiers() == Qt::NoModifier && m_dragging )
    {
        // Right click while dragging cancels zoom
        m_dragging = false;
        m_view_rect->hide();
    }

    QGraphicsView::mousePressEvent( a_event );
}

void
MandelbrotViewer::mouseMoveEvent( QMouseEvent *a_event )
{
    if ( m_dragging )
    {
        QPointF pos = mapToScene(a_event->pos());

        if ( selecetRectIntersect( m_sel_origin, pos ))
        {
            m_view_rect->setRect( m_sel_rect );
            m_view_rect->show();
        }
    }

    QGraphicsView::mousePressEvent( a_event );
}


void
MandelbrotViewer::mouseReleaseEvent( QMouseEvent *a_event )
{
    if ( m_dragging )
    {
        m_dragging = false;
        m_view_rect->hide();

        if ( m_sel_rect.width() && m_sel_rect.height() )
        {
            m_observer.zoomIn( m_sel_rect );
        }
    }
    else
    {
        if ( m_buttons == Qt::LeftButton && a_event->modifiers() == Qt::ControlModifier )
        {
            m_observer.recenter( mapToScene( a_event->pos() ));
        }
    }

    QGraphicsView::mousePressEvent( a_event );
}


bool
MandelbrotViewer::inBounds( const QPointF &a_point )
{
    if ( a_point.x() < 0 || a_point.x() >= m_width || a_point.y() < 0 || a_point.y() >= m_height )
    {
        return false;
    }

    return true;
}

bool
MandelbrotViewer::selecetRectIntersect( const QPointF &a_origin, const QPointF &a_cursor )
{
    if (( a_origin.x() < 0 && a_cursor.x() < 0 ) || ( a_origin.x() >= m_width && a_cursor.x() >= m_width ))
    {
        // No intersection
        m_sel_rect.setWidth(0);
        m_sel_rect.setHeight(0);
        return false;
    }

    if (( a_origin.y() < 0 && a_cursor.y() < 0 ) || ( a_origin.y() >= m_height && a_cursor.y() >= m_height ))
    {
        // No intersection
        m_sel_rect.setWidth(0);
        m_sel_rect.setHeight(0);
        return false;
    }

    // There is an intersection, compute rect with origin in top-left (i.e. positive width and height)
    int x1, x2, y1, y2;


    if ( a_origin.x() <= a_cursor.x() )
    {
        x1 = a_origin.x() < 0 ? 0: a_origin.x();
        x2 = a_cursor.x() >= m_width ? m_width-1: a_cursor.x();
    }
    else
    {
        x1 = a_cursor.x() < 0 ? 0: a_cursor.x();
        x2 = a_origin.x() >= m_width ? m_width-1: a_origin.x();
    }

    if ( a_origin.y() <= a_cursor.y() )
    {
        y1 = a_origin.y() < 0 ? 0: a_origin.y();
        y2 = a_cursor.y() >= m_height ? m_height-1: a_cursor.y();
    }
    else
    {
        y1 = a_cursor.y() < 0 ? 0: a_cursor.y();
        y2 = a_origin.y() >= m_height ? m_height-1: a_origin.y();
    }

    if ( m_use_aspect_ratio )
    {
        int w = x2 - x1 + 1;
        int h = y2 - y1 + 1;

        if ( w > h )
        {
            int h2 = round(w*m_aspect_ratio);
            a_origin.y() <= a_cursor.y() ? y2 = y1 + h2 : y1 = y2 - h2;
        }
        else
        {
            int w2 = round(h*m_aspect_ratio);
            a_origin.x() <= a_cursor.x() ? x2 = x1 + w2 : x1 = x2 - w2;
        }
    }

    m_sel_rect.setRect( x1, y1, (x2-x1+1),(y2-y1+1));

    return true;
}

