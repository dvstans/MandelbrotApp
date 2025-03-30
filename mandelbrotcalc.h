#ifndef MANDELBROTCALC_H
#define MANDELBROTCALC_H

#include <cstdint>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

/**
 * @brief The MandelbrotCalc class implements parallel calculation of the Mandelbrot set
 *
 * The Mandelbrot set is calculated using an optional/configurable pool of worker
 * threads. An internal image buffer is used to avoid memory reallocation
 * unless required by calculation parameter changes.
 *
 * The concurrency approach utilizes lock-free atomics to minimize thread contention.
 * Available work is assessed independently by worker threads and consists of individual
 * image lines (y-axis). This avoids memory contention and is simple to implement.
 *
 * If a worker thread pool is utilized, the pool is maintained across multiple
 * calculation calls, and it's size is adjusted as needed based in calculation
 * parameters.
 *
 * The calculated image is a buffer containing iteration counts per pixel (i.e.
 * not a rendered image for display).
 */
class MandelbrotCalc
{
public:
    /**
     * @brief The CalcParams class contains required calculation parameters
     */
    struct CalcParams{
        uint16_t            res;        // Image resolution in pixels on major axis
        double              x1;         // x coordinate bounding point 1
        double              y1;         // y coordinate bounding point 1
        double              x2;         // x coordinate bounding point 2
        double              y2;         // y coordinate bounding point 2
        uint16_t            iter_mx;    // Max iterations
        uint8_t             th_cnt;     // Thread count
    };

    /**
     * @brief The CalcResult class contains the produced image and various metrics
     */
    struct CalcResult{
        CalcResult(): img_data(0)
        {}

        double              x1;         // x coordinate bounding point 1 (adjusted)
        double              y1;         // y coordinate bounding point 1 (adjusted)
        double              x2;         // x coordinate bounding point 2 (adjusted)
        double              y2;         // y coordinate bounding point 2 (adjusted)
        uint16_t            iter_mx;    // Max iterations
        uint8_t             th_cnt;     // Thread count used
        uint16_t            img_width;  // Image width
        uint16_t            img_height; // Imahe height
        const uint16_t *    img_data;   // Image data (internal buffer)
        uint64_t            time_ms;    // Calc time in milliseconds
    };

    MandelbrotCalc( bool use_thread_pool = false, uint8_t initial_pool_size = 0 );
    ~MandelbrotCalc();

    CalcResult  calculate( CalcParams & a_params );
    void        stop();

private:
    bool                        m_use_thread_pool;  // Use thread pool flag
    uint8_t                     m_worker_count;     // Current desired number (target) of running threads
    std::vector<std::thread*>   m_workers;          // Worker thread container
    std::mutex                  m_worker_mutex;     // Mutex used to protect worker cvar
    std::condition_variable     m_worker_cvar;      // Cvar used to signal workers
    std::atomic<int32_t>        m_y_cur;            // Current image line to process (negative means no work)
    uint32_t                    m_data_cur_size;    // Current image size
    uint16_t *                  m_data;             // Image buffer
    uint16_t                    m_mxi;              // Max iterations
    uint16_t                    m_w;                // Image width
    double                      m_x1;               // Initial X value
    double                      m_y1;               // Initial Y value
    double                      m_delta;            // Real delta between pixels
    bool                        m_main_waiting;     // Flag to indicate main thread is waiting to be signalled

    void workerThread( uint8_t id );
};

#endif // MANDELBROTCALC_H
