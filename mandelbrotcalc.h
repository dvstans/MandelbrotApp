#ifndef MANDELBROTCALC_H
#define MANDELBROTCALC_H

#include <cstdint>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>


class MandelbrotCalc
{
public:
    struct CalcParams{
        uint16_t            res;        // Image resolution in pixels on major axis
        double              x1;         // x coordinate bounding point 1
        double              y1;         // y coordinate bounding point 1
        double              x2;         // x coordinate bounding point 2
        double              y2;         // y coordinate bounding point 2
        uint16_t            iter_mx;    // Max iterations
        uint8_t             th_cnt;     // Thread count (0 = auto)
    };

    struct CalcResult{
        CalcResult(): img_data(0)
        {}

        double              x1;         // x coordinate bounding point 1 (adjusted)
        double              y1;         // y coordinate bounding point 1 (adjusted)
        double              x2;         // x coordinate bounding point 2 (adjusted)
        double              y2;         // y coordinate bounding point 2 (adjusted)
        uint16_t            iter_mx;    // Max iterations
        uint8_t             th_cnt;     // Thread count used
        uint16_t            img_width;
        uint16_t            img_height;
        const uint16_t *    img_data;
        uint64_t            time_ms;
    };

    MandelbrotCalc( bool use_thread_pool = false, uint8_t initial_pool_size = 0 );
    ~MandelbrotCalc();

    CalcResult  calculate( CalcParams & a_params );
    void        stop();

private:
    bool                        m_use_thread_pool;
    uint8_t                     m_worker_count; // Current desired number of running threads
    std::vector<std::thread*>   m_workers;
    std::mutex                  m_worker_mutex;
    std::condition_variable     m_worker_cvar;
    std::atomic<int32_t>        m_y_cur;
    uint32_t                    m_data_cur_size;
    uint16_t *                  m_data;
    uint16_t                    m_mxi;
    uint16_t                    m_w;
    double                      m_x1;
    double                      m_y1;
    double                      m_delta;

    void workerThread( uint8_t id );
};

#endif // MANDELBROTCALC_H
