#include <iostream>
#include <stdexcept>
#include <chrono>
#include <algorithm>
#include <cmath>
#include "mandelbrotcalc.h"

using namespace std;

typedef std::chrono::high_resolution_clock Clock;

/**
 * @brief MandelbrotCalc constructor
 * @param a_use_thread_pool - If true, requests worker thread pool be maintained across calculations
 * @param a_initial_pool_size - Initial thread pool size
 */
MandelbrotCalc::MandelbrotCalc( bool a_use_thread_pool, uint8_t a_initial_pool_size ):
    m_observer(0),
    m_use_thread_pool( a_use_thread_pool ),
    m_worker_count(0),
    //m_data_cur_size(0),
    //m_data(0),
    m_exit(false)
{
    // Indicate no work to do by setting negative current image line (Y-axis)
    atomic_store( &m_y_cur, -1 );

    // Create initial thread pool if requested
    if ( m_use_thread_pool )
    {
        if ( a_initial_pool_size )
        {
            // Spin up worker threads in pool
            unique_lock lock( m_worker_mutex );

            m_worker_count = a_initial_pool_size;
            m_workers.reserve( m_worker_count );

            for ( uint8_t i = 0; i < m_worker_count; i++ )
            {
                m_workers.push_back( new thread( &MandelbrotCalc::workerThread, this, i ));
            }
        }
    }

    unique_lock lock( m_control_mutex );
    m_control_thread = new thread( &MandelbrotCalc::controlThread, this );
}

/**
 * @brief MandelbrotCalc destructor
 */
MandelbrotCalc::~MandelbrotCalc()
{
    // Stop threads
    stopWorkerThreads();

    unique_lock ctrl_lock( m_control_mutex );
    m_exit = true;
    m_control_cvar.notify_one();
    ctrl_lock.unlock();

    m_control_thread->join();
    delete m_control_thread;

    // Delete image buffer
    //if ( m_data )
    //{
    //    delete[] m_data;
    //}
}


/**
 * @brief Starts the Mandelbrot set calculation based on given parameters
 * @param a_observer - Object to receive progress notifications
 * @param a_params - Calculation parameters
 *
 * This method validate parameters and starts the calculation process asynchronously.
 * The observer will receive notification of progress, cancellation, or completion.
 */
void
MandelbrotCalc::calculate( IObserver & a_observer, Params & a_params )
{
    // Validate parameters

    if ( a_params.res == 0 )
    {
        throw out_of_range("Invalid image resolution parameter: must be greater than zero.");
    }

    if ( a_params.iter_mx == 0 )
    {
        throw out_of_range("Invalid max iterations parameter: must be greater than zero.");
    }

    // Control thread holds this mutex for duration of calculation
    lock_guard lock( m_control_mutex );

    m_params = a_params;
    m_observer = &a_observer;
    m_control_cvar.notify_one();
}

void
MandelbrotCalc::controlThread()
{
    unique_lock ctrl_lock( m_control_mutex, defer_lock );

    while( 1 )
    {
        ctrl_lock.lock();
        m_cancel = false;

        while( m_observer == 0 )
        {
            m_control_cvar.wait(ctrl_lock);
            if ( m_exit )
                return;

        }

        // Start timer
        auto t1 = Clock::now();

        Result result;

        result.x1 = m_params.x1;
        result.y1 = m_params.y1;
        result.x2 = m_params.x2;
        result.y2 = m_params.y2;
        result.th_cnt = m_params.th_cnt;
        result.iter_mx = m_params.iter_mx;

        // Adjust bounding rect if needed
        if ( result.x1 > result.x2 )
        {
            swap( result.x1, result.x2 );
        }

        if ( result.y1 > result.y2 )
        {
            swap( result.y1, result.y2 );
        }

        // Calculate actual image size
        double w = result.x2 - result.x1;
        double h = result.y2 - result.y1;

        if ( w > h )
        {
            m_delta = w/(m_params.res - 1);
            result.img_width = m_params.res;
            result.img_height = (uint16_t)(floor(h/m_delta) + 1);
        }
        else
        {
            m_delta = h/(m_params.res - 1);
            result.img_width = (uint16_t)(floor(w/m_delta) + 1);
            result.img_height = m_params.res;
        }

        unique_lock lock( m_worker_mutex );

        // Prepare internal parameters
        m_x1 = result.x1;
        m_y1 = result.y1;
        m_mxi = m_params.iter_mx;
        m_w = result.img_width;
        m_h = result.img_height;

        // Resize image data buffer
        result.img_data.resize( result.img_width  * result.img_height );
        // m_data points to beginning of data buffer
        m_data = &result.img_data[0];

        /*if ( m_data_cur_size != data_sz )
        {
            if ( m_data )
            {
                delete[] m_data;
            }
            m_data = new uint16_t[data_sz];
            m_data_cur_size = data_sz;
        }*/

        // Note: most threads will be waiting on their cvar, but new threads may not make it to
        // the cvar before the "start work" notify is signalled, thus new workers will check if
        // there is work to do BEFORE waiting on initial notify. This avoids the race condition
        // that could cause them to become stuck on the wait after the signal is sent. Thus the
        // work (m_y_cur) must be set before any workers are signalled.

        // Set work done and remaining (first line to process)
        atomic_store( &m_y_done, result.img_height );
        atomic_store( &m_y_cur, result.img_height - 1 );

        m_y_upd = 0.99*m_h;

        // Adjust worker threads if needed
        if( m_params.th_cnt > m_workers.size() )
        {
            uint8_t i = m_worker_count;

            m_worker_count = m_params.th_cnt;
            m_workers.reserve( m_params.th_cnt );

            for ( ; i < m_params.th_cnt; i++ )
            {
                m_workers.push_back( new thread( &MandelbrotCalc::workerThread, this, i ));
            }
        }
        else if ( m_params.th_cnt < m_workers.size() )
        {
            vector<thread*>::iterator t = m_workers.begin() + m_params.th_cnt;

            // Set new desired worker count, notify workers
            m_worker_count = m_params.th_cnt;
            m_worker_cvar.notify_all();
            lock.unlock();

            // Wait for pruned worker threads to stop, then delete
            for ( ; t != m_workers.end(); t++ )
            {
                (*t)->join();
                delete *t;
            }

            lock.lock();
            m_workers.resize(m_worker_count);
        }

        // Start workers
        m_worker_cvar.notify_all();
        lock.unlock();

        // Wait for all work to be completed
        // Note that for small/simple images, some workers may not contribute to the calculation
        while( atomic_load( &m_y_done ) > 0 && !m_cancel )
        {
            m_control_cvar.wait( ctrl_lock );
        }

        // Stop timer
        auto t2 = Clock::now();

        if ( m_cancel )
        {
            m_observer->cbCalcCancelled();
        }
        else
        {
            //result.img_data = m_data;
            result.time_ms = chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count(); // time msec;

            // Notify observer of completion
            m_observer->cbCalcCompleted( result );
        }

        // Clear observer and release ctrl lock
        m_observer = 0;
        ctrl_lock.unlock();
    }
}


/**
 * @brief Determine if calculation is in progress
 * @return True if calculation running; false otherwise
 */
bool
MandelbrotCalc::isCalculating()
{
    return atomic_load( &m_y_cur ) >= 0;
}


void
MandelbrotCalc::stopCalculation()
{
    // Set no work indicated
    atomic_store( &m_y_cur, -1 );

    lock_guard ctrl_lock( m_control_mutex );
    m_cancel = true;
    m_control_cvar.notify_all();
}

/**
 * @brief Stops all worker threads
 *
 * This method is called by the destructor, but may also be called by the
 * client app if desired. Threads will be recreated on next calculation
 * call.
 */
void
MandelbrotCalc::stopWorkerThreads()
{
    if ( m_workers.size() )
    {
        unique_lock lock(m_worker_mutex);

        // Set no work indicated
        atomic_store( &m_y_cur, -1 );

        // Signal waiting workers to exit
        m_worker_count = 0;
        m_worker_cvar.notify_all();
        lock.unlock();

        // Ensure threads are stopped, then delete
        for ( vector<thread*>::iterator t = m_workers.begin(); t != m_workers.end(); t++ )
        {
            (*t)->join();
            delete *t;
        }

        m_workers.resize(0);
    }
}


/**
 * @brief The worker thread method
 * @param a_id - A monotonically increasing ID assigned to workers
 *
 * Worker threads run until signalled to stop by the main thread. While
 * waiting for work, a mutex-protected cvar is used to for efficiency.
 * Once work is triggered, lock-free atomics are used to acquire work
 * until none is left - at which point the workers go back to the
 * cvar wait state. Workers may be pruned by reducing the "m_worker_count"
 * attribute - all threads with IDs beyond this value exit.
 */
void
MandelbrotCalc::workerThread( uint8_t a_id )
{
    //cout << "TS" << (int)a_id << endl;

    unique_lock lock( m_worker_mutex, defer_lock );
    uint16_t    x;
    int32_t     line;
    uint16_t *  dat;
    double      xr, yr, zx, zy, zx2, zy2, tmp;
    uint16_t    i, mxi;

    // Note: workers will run until all work is exhausted then check for exit conditions
    // This means worker count cannot be adjusted during a calculation

    while( 1 )
    {
        // Wait for run notification ONLY if there is no work to do. Note: notifications may be spurious
        if ( atomic_load( &m_y_cur ) < 0 )
        {
            lock.lock();
            m_worker_cvar.wait(lock);
            lock.unlock();
        }

        // Prune this thread if its ID is out of bounds with target worker count
        if ( a_id >= m_worker_count )
        {
            break;
        }

        // Check if there is any work to do
        if ( atomic_load( &m_y_cur ) > -1 )
        {
            mxi = m_mxi;

            // Process remaining work until no more left
            while (( line = atomic_fetch_sub( &m_y_cur, 1 )) > -1 )
            {
                // Move to position in image to be calculated
                dat = m_data + line*m_w;

                xr = m_x1;
                yr = m_y1 + line*m_delta;

                // Iterate over current line's X-axis
                for ( x = 0; x < m_w; x++, xr += m_delta )
                {
                    // Perform calculation: Z => Z^2 + C

                    i = 0;
                    zx2 = zx = xr;
                    zy2 = zy = yr;
                    zx2 *= zx;
                    zy2 *= zy;

                    while ( i++ <= mxi && (( zx2 + zy2 ) < 4 )) {
                        tmp = zx;
                        zx2 = zx = zx2 - zy2 + xr;
                        zy2 = zy = 2*tmp*zy + yr;
                        zx2 *= zx;
                        zy2 *= zy;
                    };

                    if ( i > mxi ) {
                        i = 0;
                    }

                    *dat++ = i;
                }

                // Update work completed
                line = atomic_fetch_sub( &m_y_done, 1 );
                if ( line == 1 )
                {
                    // When m_y_done reaches 0, all work has been completed
                    // Use worker cvar to wake control thread
                    // Taking control mutex ensure main thread is waiting on cvar
                    lock_guard ctrl_lock( m_control_mutex );
                    m_control_cvar.notify_all();
                }
                else if ( line == m_y_upd )
                {
                    m_y_upd = (m_h - line)*100/m_h;
                    m_observer->cbCalcProgress( m_y_upd );
                    m_y_upd = (99 - m_y_upd)*m_h/100;
                }
            }
        }
    }

    //cout << "TX" << (int)a_id << endl;
}
