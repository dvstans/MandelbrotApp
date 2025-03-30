#include <QScrollbar>
#include <QMouseEvent>
#include <QVBoxLayout>
#include "mandelbrotviewer.h"

using namespace std;

/**
 * @brief MandelbrotViewer constructor
 * @param a_parent - Parent QFrame
 * @param a_observer - Instance to receive callbacks
 */
MandelbrotViewer::MandelbrotViewer( QFrame& a_parent, IMandelbrotViewerObserver & a_observer ):
    m_parent(a_parent),
    m_observer(a_observer),
    m_zooming(false),
    m_panning(false),
    m_width(0),
    m_height(0),
    m_use_aspect_ratio(false)
{
    // Remove frame and margins
    setFrameShape( QFrame::NoFrame );
    setContentsMargins(0,0,0,0);
    setBackgroundBrush(QBrush(Qt::black));

    // Create graphics scene
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

    // Ensure selection rect is in front of image
    m_view_rect->setZValue( 1 );
    m_view_pixmap->setZValue( 0 );
    m_view_rect->hide();
}

/**
 * @brief MandelbrotViewer destructor
 */
MandelbrotViewer::~MandelbrotViewer()
{}

/**
 * @brief Retrieves the displayed image
 * @return Image object
 */
QImage
MandelbrotViewer::getImage()
{
    return m_view_pixmap->pixmap().toImage();
}

/**
 * @brief Sets the image to display
 * @param a_image - Image to display
 */
void
MandelbrotViewer::setImage(  const QImage & a_image )
{
    m_view_pixmap->setPixmap( QPixmap::fromImage( a_image ));

    m_width = a_image.width();
    m_height = a_image.height();

    // Adjust scene size to fix scrollbar extent if image gets smaller
    scene()->setSceneRect(scene()->itemsBoundingRect());
}

/**
 * @brief Sets desired aspect ratio for zooming
 * @param a_major - Major axis of ratio (i.e. 16)
 * @param a_minor - Minor axis of ratio (i.e. 9)
 */
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

/**
 * @brief Handler of mouse press events
 * @param a_event - Mouse event
 *
 * This method initiates panning and zooming.
 */
void
MandelbrotViewer::mousePressEvent( QMouseEvent *a_event )
{
    m_buttons = a_event->buttons();

    if ( m_buttons == Qt::LeftButton )
    {
        if ( a_event->modifiers() == Qt::NoModifier )
        {
            // Start panning on left mouse button down (no modifiers) event
            m_panning = true;
            m_origin = a_event->pos();
            m_hscroll_val = horizontalScrollBar()->value();
            m_vscroll_val = verticalScrollBar()->value();
        }
        else if ( a_event->modifiers() == Qt::ShiftModifier )
        {
            // Start zoom window on shift + left mouse button down event
            QPointF pos = mapToScene(a_event->pos());
            m_zooming = true;
            m_origin = mapToScene(a_event->pos());
            m_sel_rect.setWidth(0);
            m_sel_rect.setHeight(0);

            if ( inBounds( pos ))
            {
                m_sel_rect.setRect( pos.x(), pos.y(), 0, 0 );
                m_view_rect->setRect( m_sel_rect );
                m_view_rect->show();
            }
        }
    }

    QGraphicsView::mousePressEvent( a_event );
}

/**
 * @brief Handler of mouse move events
 * @param a_event - Mouse event
 *
 * This method processes panning and zooming.
 */
void
MandelbrotViewer::mouseMoveEvent( QMouseEvent *a_event )
{
    if ( m_zooming )
    {
        QPointF pos = mapToScene(a_event->pos());

        // Zoom by difference in position from initial origin
        // Make sure window intersects with image

        if ( selectRectIntersect( m_origin, pos ))
        {
            m_view_rect->setRect( m_sel_rect );
            m_view_rect->show();
        }
    }
    else if ( m_panning )
    {
        // Scroll by difference in position from initial origin

        horizontalScrollBar()->setValue( m_hscroll_val + (m_origin.x() - a_event->pos().x()));
        verticalScrollBar()->setValue( m_vscroll_val + (m_origin.y() - a_event->pos().y()));
    }

    QGraphicsView::mousePressEvent( a_event );
}

/**
 * @brief Handler of mouse release events
 * @param a_event - Mouse event
 *
 * This method terminates image panning and zooming, and also triggers re-centering.
 */
void
MandelbrotViewer::mouseReleaseEvent( QMouseEvent *a_event )
{
    if ( m_zooming )
    {
        // Finish zooming process
        m_zooming = false;
        m_view_rect->hide();

        // Notify observer if zoom rect was valid
        if ( m_sel_rect.width() && m_sel_rect.height() )
        {
            m_observer.imageZoomIn( m_sel_rect );
        }
    }
    else if ( m_panning )
    {
        // Finish panning process
        m_panning = false;
    }
    else
    {
        // Notify observer of image recenter on control + left button release
        if ( m_buttons == Qt::LeftButton && a_event->modifiers() == Qt::ControlModifier )
        {
            m_observer.imageRecenter( mapToScene( a_event->pos() ));
        }
    }

    QGraphicsView::mousePressEvent( a_event );
}

/**
 * @brief Handler of key release events
 * @param a_event - Key event
 *
 * This method terminates image zooming if shift key is released befor mouse button.
 */
void
MandelbrotViewer::keyReleaseEvent( QKeyEvent *event )
{
    if ( m_zooming && event->key() == Qt::Key_Shift )
    {
        m_zooming = false;
        m_view_rect->hide();
    }
}

/**
 * @brief Checks if specified point is within bounds of displayed image
 * @param a_point - Point to test
 * @return True if point is in-bounds; false otherwise
 */
bool
MandelbrotViewer::inBounds( const QPointF &a_point )
{
    if ( a_point.x() < 0 || a_point.x() >= m_width || a_point.y() < 0 || a_point.y() >= m_height )
    {
        return false;
    }

    return true;
}

/**
 * @brief Checks if selection rect intersects with displayed image
 * @param a_origin - Start position of selection
 * @param a_cursor - Current position of selection
 * @return True if rect intersects with image; false otherwise
 *
 * This check is needed if the window size is larger than the displayed image,
 * and user begins or ends selection in the margins of the image.
 */
bool
MandelbrotViewer::selectRectIntersect( const QPointF &a_origin, const QPointF &a_cursor )
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

