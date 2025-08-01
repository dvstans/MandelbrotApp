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
    struct Params{
        uint16_t            res;        // Image resolution in pixels on major axis
        double              x1;         // x coordinate bounding point 1
        double              y1;         // y coordinate bounding point 1
        double              x2;         // x coordinate bounding point 2
        double              y2;         // y coordinate bounding point 2
        uint32_t            iter_mx;    // Max iterations
        uint16_t            th_cnt;     // Thread count
    };

    /**
     * @brief The CalcResult class contains the produced image and various metrics
     */
    struct Result{
        Result()
        {}

        double                  x1;         // x coordinate bounding point 1 (adjusted)
        double                  y1;         // y coordinate bounding point 1 (adjusted)
        double                  x2;         // x coordinate bounding point 2 (adjusted)
        double                  y2;         // y coordinate bounding point 2 (adjusted)
        uint32_t                iter_mx;    // Max iterations
        uint16_t                th_cnt;     // Thread count used
        uint16_t                img_width;  // Image width
        uint16_t                img_height; // Imahe height
        std::vector<uint32_t>   img_data;   // Image data (internal buffer)
        uint64_t                time_ms;    // Calc time in milliseconds
    };

    class IObserver
    {
    public:
        virtual void cbCalcProgress( int ) = 0;
        virtual void cbCalcCompleted( Result ) = 0;
        virtual void cbCalcCancelled() = 0;
    };

    MandelbrotCalc( bool use_thread_pool = false, uint8_t initial_pool_size = 0 );
    ~MandelbrotCalc();

    //CalcResult  calculate( CalcParams & a_params );
    void        calculate( IObserver & a_observer, Params & a_params );
    bool        isCalculating();
    void        stopCalculation();
    void        stopWorkerThreads();

private:
    std::thread*                m_control_thread;   // Control thread to manage workers
    std::mutex                  m_control_mutex;    // Mutex used to protect control thread cvar
    std::condition_variable     m_control_cvar;     // Cvar used to signal control thread to start
    Params                      m_params;
    IObserver *                 m_observer;
    bool                        m_use_thread_pool;  // Use thread pool flag
    uint16_t                    m_worker_count;     // Current desired number (target) of running threads
    std::vector<std::thread*>   m_workers;          // Worker thread container
    std::mutex                  m_worker_mutex;     // Mutex used to protect worker cvar
    std::condition_variable     m_worker_cvar;      // Cvar used to signal workers
    std::atomic<int32_t>        m_y_cur;            // Current image line to process (negative means no work)
    std::atomic<int32_t>        m_y_done;           // Completed image lines
    int32_t                     m_y_upd;
    uint32_t *                  m_data;             // Image buffer
    uint32_t                    m_mxi;              // Max iterations
    uint16_t                    m_w;                // Image width
    uint16_t                    m_h;                // Image height
    double                      m_x1;               // Initial X value
    double                      m_y1;               // Initial Y value
    double                      m_delta;            // Real delta between pixels
    bool                        m_cancel;
    bool                        m_exit;

    void controlThread();
    void workerThread( uint16_t id );
};

#endif // MANDELBROTCALC_H
