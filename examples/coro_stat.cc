#include <ncurses.h>
#include <unistd.h>

#include <atomic>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>

struct stats_frame_t {
    double timestamp;
    size_t thread_id;
    size_t messages_sent;
    size_t messages_received;
    size_t coroutines_alive;
    size_t coroutine_swaps;
};

class windowed_t {
public:
    windowed_t() :
        m_height(0), m_width(0), m_window(newwin(m_height, m_width, 0, 0)) { }

    void resize(size_t height, size_t width, size_t top, size_t left) {
        wborder(m_window, ' ', ' ', ' ',' ',' ',' ',' ',' ');
        wrefresh(m_window);
        delwin(m_window);

        m_height = height;
        m_width = width;
        m_window = newwin(m_height, m_width, top, left);
        wborder(m_window, '|', '|', '-','-','.','.','.','.');
    }

    virtual void refresh() {
        wnoutrefresh(m_window);
    }

private:
    size_t m_height;
    size_t m_width;
    WINDOW *m_window;
};

class thread_graph_t final : public windowed_t {
public:
    thread_graph_t() : windowed_t(), m_cache() { }

    void add_frame(const stats_frame_t &data) {
        m_cache.push_back(data);
        while (m_cache.front().timestamp < data.timestamp - s_cache_time) {
            m_cache.pop_front();
        }
    }

    void refresh() override final {
        // Write the new state of the window
        // ...
        windowed_t::refresh();
    }

    static const double s_cache_time;
    std::deque<stats_frame_t> m_cache;
};

const double thread_graph_t::s_cache_time = 10.0;

class thread_stats_t final : public windowed_t {
public:
    thread_stats_t() :
        windowed_t(), m_cache() { }

    void add_frames(std::vector<stats_frame_t> data) {
        m_cache = std::move(data);
    }

    void refresh() override final {
        // Write the new state of the window
        // ...
        windowed_t::refresh();
    }

    std::vector<stats_frame_t> m_cache;

};

class coro_stats_t {
public:
    coro_stats_t() { }

    void add_frames(std::vector<stats_frame_t> frames) {
        if (frames.size() != m_thread_graphs.size()) {
            m_thread_graphs.resize(frames.size());
        }
        for (size_t i = 0; i < frames.size(); ++i) {
            m_thread_graphs[i].add_frame(frames[i]);
        }
        m_thread_stats.add_frames(std::move(frames));
    }

    void redraw() {
        // recalculate sub-window bounding boxes
        size_t column_div = COLS / 2;

        size_t graph_height = LINES / m_thread_graphs.size();
        size_t graph_width = column_div;
        for (size_t i = 0; i < m_thread_graphs.size(); ++i) {
            m_thread_graphs[i].resize(graph_height, graph_width, i * graph_height, 0);
        }

        size_t stats_height = LINES;
        size_t stats_width = COLS - graph_width;
        m_thread_stats.resize(stats_height, stats_width, 0, graph_width);

        refresh();
    }

    void refresh() {
        for (auto graph : m_thread_graphs) {
            graph.refresh();
        }
        m_thread_stats.refresh();
        doupdate();
    }

private:
    std::vector<thread_graph_t> m_thread_graphs;
    thread_stats_t m_thread_stats;
};

// TODO: implement a real connection to the server
class dummy_conn_t {
public:
    dummy_conn_t() : m_last_timestamp(0.0) { }   
    
    std::vector<stats_frame_t> get_frames() {
        sleep(1);
        m_last_timestamp += 1.0;
        std::vector<stats_frame_t> frames;
        for (size_t i = 0; i < 10; ++i) {
            stats_frame_t frame;
            frame.timestamp = m_last_timestamp;
            frame.thread_id = i;
            frame.messages_sent = (int)m_last_timestamp % 100;
            frame.messages_received = ((int)m_last_timestamp + 25) % 100;
            frame.coroutines_alive = ((int)m_last_timestamp + 50) % 100;
            frame.coroutine_swaps = ((int)m_last_timestamp + 75) % 100;
            frames.push_back(frame);
        }
        return frames;
    }

    double m_last_timestamp;
};

int main(int argc, char *argv[]) {
    initscr(); // Start curses mode
    cbreak(); // Line buffering disabled, Pass on everything to me
    noecho();

    coro_stats_t coro_stats;

    std::mutex ncurses_mutex;
    std::atomic<bool> shutdown_flag(false);

    std::thread update_thread([&] {
            while (!shutdown_flag.load()) {
                dummy_conn_t dummy_conn;
                std::vector<stats_frame_t> frames;
                while (!(frames = dummy_conn.get_frames()).empty()) {
                    coro_stats.add_frames(std::move(frames));
                    std::lock_guard<std::mutex> lock(ncurses_mutex);
                    printw("writing frame");
                    coro_stats.refresh();
                }
            }
        });

    char ch;
    while((ch = getch()) != 'q') {
        switch(ch) {
        case KEY_RESIZE: {
            std::lock_guard<std::mutex> lock(ncurses_mutex);
            coro_stats.redraw();
        } break;
        default:
            break;
        }
    }

    shutdown_flag.store(true);
    update_thread.join();

    endwin(); // End curses mode
    return 0;
}
