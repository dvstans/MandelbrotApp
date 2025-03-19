#include<iostream>
#include <QMouseEvent>
#include <QVBoxLayout>
#include "mandelbrotviewer.h"

using namespace std;

MandelbrotViewer::MandelbrotViewer( QFrame& a_parent, IMandelbrotViewerObserver & a_observer ):
    m_parent(a_parent),
    m_observer(a_observer),
    m_dragging(false),
    m_width(0),
    m_height(0)
{
    QGraphicsScene *scene = new QGraphicsScene();
    QRectF rect(0,0,10,10);
    m_view_rect = scene->addRect( rect, QPen(Qt::yellow,1), QBrush(Qt::NoBrush));

    QPixmap pixmap(500,500);
    pixmap.fill( Qt::red );
    m_view_pixmap = scene->addPixmap(pixmap);
    setScene(scene);

    QVBoxLayout *layout = new QVBoxLayout();
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
}

void
MandelbrotViewer::mousePressEvent( QMouseEvent *a_event )
{
    QPointF pos = mapToScene(a_event->pos());

    //cout << a_event->pos().x() << "," << a_event->pos().y() << " -> " << pos.x() << "," << pos.y() << endl;

    // Start zoom window on left mouse button down event
    if ( a_event->isBeginEvent() && a_event->buttons() == Qt::LeftButton )
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

    QGraphicsView::mousePressEvent( a_event );
}

void
MandelbrotViewer::mouseMoveEvent( QMouseEvent *a_event )
{
    if ( m_dragging )
    {
        QPointF pos = mapToScene(a_event->pos());

        //cout << a_event->pos().x() << "," << a_event->pos().y() << " -> " << pos.x() << "," << pos.y() << endl;

        if ( selecetRectIntersect( m_sel_origin, pos ))
        {
            //cout << "ib" << endl;
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
        //cout << "drag 0" << endl;
        m_dragging = false;
        m_view_rect->hide();

        if ( m_sel_rect.width() && m_sel_rect.height() )
        {
            m_observer.zoomIn( m_sel_rect );
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
MandelbrotViewer::selecetRectIntersect( const QPointF &a_p1, const QPointF &a_p2 )
{
    if (( a_p1.x() < 0 && a_p2.x() < 0 ) || ( a_p1.x() >= m_width && a_p2.x() >= m_width ))
    {
        // No intersection
        m_sel_rect.setWidth(0);
        m_sel_rect.setHeight(0);
        return false;
    }

    if (( a_p1.y() < 0 && a_p2.y() < 0 ) || ( a_p1.y() >= m_height && a_p2.y() >= m_height ))
    {
        // No intersection
        m_sel_rect.setWidth(0);
        m_sel_rect.setHeight(0);
        return false;
    }

    // There is an intersection, compute rect with origin in top-left (i.e. positive width and height)
    int x1, x2, y1, y2;

    if ( a_p1.x() <= a_p2.x() )
    {
        x1 = a_p1.x() < 0 ? 0: a_p1.x();
        x2 = a_p2.x() >= m_width ? m_width-1: a_p2.x();
    }
    else
    {
        x1 = a_p2.x() < 0 ? 0: a_p2.x();
        x2 = a_p1.x() >= m_width ? m_width-1: a_p1.x();
    }

    if ( a_p1.y() <= a_p2.y() )
    {
        y1 = a_p1.y() < 0 ? 0: a_p1.y();
        y2 = a_p2.y() >= m_height ? m_height-1: a_p2.y();
    }
    else
    {
        y1 = a_p2.y() < 0 ? 0: a_p2.y();
        y2 = a_p1.y() >= m_height ? m_height-1: a_p1.y();
    }

    m_sel_rect.setRect( x1, y1, (x2-x1+1),(y2-y1+1));

    return true;
}

