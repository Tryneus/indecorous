#include <ncurses.h>
#include <unistd.h>

#include <deque>
#include <vector>
#include <sstream>

#include "rpc/handler.hpp"

using namespace indecorous;

struct record_t {
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
        m_height(0), m_width(0), m_window(nullptr) { }

    void resize(size_t height, size_t width, size_t top, size_t left) {
        if (m_window != nullptr) {
            wborder(m_window,' ',' ',' ',' ',' ',' ',' ',' ');
            wrefresh(m_window);
            delwin(m_window);
        }

        m_height = height;
        m_width = width;
        m_window = newwin(m_height, m_width, top, left);
        wborder(m_window,'|','|','-','-','+','+','+','+');
    }

    virtual void refresh() {
        wnoutrefresh(m_window);
    }

protected:
    size_t m_height;
    size_t m_width;
    WINDOW *m_window;
};

class thread_graph_t final : public windowed_t {
public:
    thread_graph_t() : windowed_t(), m_cache() { }

    void add_frame(const record_t &data) {
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
    std::deque<record_t> m_cache;
};

const double thread_graph_t::s_cache_time = 10.0;

class thread_stats_t final : public windowed_t {
public:
    thread_stats_t() :
        windowed_t(), m_cache() { }

    void add_frames(std::vector<record_t> data) {
        m_cache = std::move(data);
    }

    std::string truncate(const std::string s) {
        return (s.size() > m_width - 2) ? s.substr(0, m_width - 2) : s;
    }

    void refresh() override final {
        std::string header(" Thread |  Sent  |  Recv  |  Coro  |  Swap  \n");
        std::string divider("--------------------------------------------\n");

        mvwaddstr(m_window, 1, 1, truncate(header).c_str());
        mvwaddstr(m_window, 2, 1, truncate(divider).c_str());
        windowed_t::refresh();
    }

    std::vector<record_t> m_cache;

};

class coro_stats_t {
public:
    coro_stats_t() { }

    void add_frames(std::vector<record_t> frames) {
        if (frames.size() != m_thread_graphs.size()) {
            m_thread_graphs.resize(frames.size());
        }
        for (size_t i = 0; i < frames.size(); ++i) {
            m_thread_graphs[i].add_frame(frames[i]);
        }
        m_thread_stats.add_frames(std::move(frames));
        this->redraw();
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

class coro_stats_t {
    DECLARE_STATIC_RPC(coro_stats_t::main)(std::string, uint16_t) -> void;
}

IMPL_STATIC_RPC(coro_stats_t::main)(std::string host, uint16_t port) -> void {
    event_t done_event;
    interruptor_t done_interruptor(&done_event);

    file_wait_t input = file_wait_t::in(STDIN_FILENO);

    coro_stats_t stats;

    coro_t::spawn([&] {
            // TODO: handle disconnections
            tcp_conn_t conn(host, port);
            while (true) {
                record_t record;
                conn.read(&record, sizeof(record)),
                stats.insert(record);
            }
        });

    while (true) {
        input.wait();
        switch (getch()) {
        case KEY_RESIZE: coro_stats.redraw(); break;
        case 'Q':
        case 'q': done_event.set(); break;
        default: break;
        }
    }
}

void print_usage() {
    printf("Usage: coro_stat HOST PORT");
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Error: Expected 2 arguments.");
        print_usage();
        exit(1);
    }

    std::string host(argv[1]);
    int raw_port = atoi(argv[2]);

    if (host.empty()) {
        printf("Error: invalid hostname.");
        print_usage();
        exit(1);
    }

    if (raw_port == 0 || raw_port > std::numeric_limits<uint16_t>::max()) {
        printf("Error: invalid port, expected a number between 1 and %d.",
               std::numeric_limits<uint16_t>::max());
        print_usage();
        exit(1);
    }
    uint16_t port = static_cast<uint16_t>(raw_port);

    initscr(); // Start curses mode
    cbreak(); // Line buffering disabled, Pass on everything to me
    noecho();
    nodelay();

    scheduler_t sched(1, shutdown_policy_t::Eager);
    sched.target(0)->call_noreply<coro_stats_t::main>(host, port);
    sched.run();

    endwin(); // End curses mode
    return 0;
}
