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
    m_use_thread_pool( a_use_thread_pool ),
    m_worker_count(0),
    m_data_cur_size(0),
    m_data(0),
    m_main_waiting(false)
{
    // Indicate no work to do by setting negative current image line (Y-axis)
    atomic_store( &m_y_cur, -1 );

    // Create initial thread pool if requested
    if ( m_use_thread_pool )
    {
        if ( a_initial_pool_size )
        {
            // Spin up worker threads in pool
            unique_lock lock(m_worker_mutex);

            m_worker_count = a_initial_pool_size;
            m_workers.reserve( m_worker_count );

            for ( uint8_t i = 0; i < m_worker_count; i++ )
            {
                m_workers.push_back( new thread( &MandelbrotCalc::workerThread, this, i ));
            }
        }
    }
}

/**
 * @brief MandelbrotCalc destructor
 */
MandelbrotCalc::~MandelbrotCalc()
{
    // Stop threads
    stop();

    // Delete image buffer
    if ( m_data )
    {
        delete[] m_data;
    }
}

/**
 * @brief Stops all worker threads
 *
 * This method is called by the destructor, but may also be called by the
 * client app if desired. Threads will be recreated on next calculation
 * call.
 */
void
MandelbrotCalc::stop()
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
 * @brief Calculates the Mandelbrot set based on given parameters
 * @param a_params - Calculation parameters
 * @return A CalcResult instance containing internal image buffer pointer and various metrics
 */
MandelbrotCalc::CalcResult
MandelbrotCalc::calculate( CalcParams & a_params )
{
    // Start timer
    auto t1 = Clock::now();

    CalcResult result;

    // Validate parameters

    if ( a_params.res == 0 )
    {
        throw out_of_range("Invalid image resolution parameter: must be greater than zero.");
    }

    if ( a_params.iter_mx == 0 )
    {
        throw out_of_range("Invalid max iterations parameter: must be greater than zero.");
    }

    result.x1 = a_params.x1;
    result.y1 = a_params.y1;
    result.x2 = a_params.x2;
    result.y2 = a_params.y2;
    result.th_cnt = a_params.th_cnt;
    result.iter_mx = a_params.iter_mx;

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
        m_delta = w/(a_params.res - 1);
        result.img_width = a_params.res;
        result.img_height = (uint16_t)(floor(h/m_delta) + 1);
    }
    else
    {
        m_delta = h/(a_params.res - 1);
        result.img_width = (uint16_t)(floor(w/m_delta) + 1);
        result.img_height = a_params.res;
    }

    unique_lock lock( m_worker_mutex );

    // Prepare internal parameters
    m_x1 = result.x1;
    m_y1 = result.y1;
    m_mxi = a_params.iter_mx;
    m_w = result.img_width;

    // Resize internal buffer if size changes
    uint32_t data_sz = result.img_width  * result.img_height;
    if ( m_data_cur_size != data_sz )
    {
        if ( m_data )
        {
            delete[] m_data;
        }
        m_data = new uint16_t[data_sz];
        m_data_cur_size = data_sz;
    }

    // Note: most threads will be waiting on their cvar, but new threads may not make it to
    // the cvar before the "start work" notify is signalled, thus new workers will check if
    // there is work to do BEFORE waiting on initial notify. This avoids the race condition
    // that could cause them to become stuck on the wait after the signal is sent. Thus the
    // work (m_y_cur) must be set before any workers are signalled.

    // Set work remaining (first line to process)
    atomic_store( &m_y_cur, result.img_height - 1 );

    // Adjust worker threads if needed
    if( a_params.th_cnt > m_workers.size() )
    {
        uint8_t i = m_worker_count;

        m_worker_count = a_params.th_cnt;
        m_workers.reserve( a_params.th_cnt );

        for ( ; i < a_params.th_cnt; i++ )
        {
            m_workers.push_back( new thread( &MandelbrotCalc::workerThread, this, i ));
        }
    }
    else if ( a_params.th_cnt < m_workers.size() )
    {
        vector<thread*>::iterator t = m_workers.begin() + a_params.th_cnt;

        // Set new desired worker count, notify workers
        m_worker_count = a_params.th_cnt;
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

    // Wait for last worker to complete
    // Note that for small/simple images, some workers may not contribute to the calculation
    m_main_waiting = true;
    while( atomic_load( &m_y_cur ) > -m_worker_count )
    {
        m_worker_cvar.wait(lock);
    }
    m_main_waiting = false;

    lock.unlock();

    // Stop timer
    auto t2 = Clock::now();

    result.img_data = m_data;
    result.time_ms = chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count(); // time msec;

    return result;
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
            }

            // Last worker to complete will see -N in line count where N is the number of worker threads
            // When this is detected, notify the main thread through the cvar
            // This will wake other workers, but they will see there is no work and go back to waiting.
            if ( line <= -m_worker_count )
            {
                m_worker_cvar.notify_all();
                //cout << "TN" << (int)a_id << endl;
            }
        }
        else if ( m_main_waiting )
        {
            // If a worker gets here, it means other workers completed calculation before
            // this thread was able to get any work (lines), thus proceed with termination.
            line = atomic_fetch_sub( &m_y_cur, 1 );
            if ( line <= -m_worker_count )
            {
                m_worker_cvar.notify_all();
                //cout << "TN(x)" << (int)a_id << endl;
            }
        }
    }

    //cout << "TX" << (int)a_id << endl;
}
