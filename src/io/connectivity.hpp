#ifndef IO_CONNECTIVITY_HPP_
#define IO_CONNECTIVITY_HPP_

class cluster_registry_t {
public:
    cluster_registry_t();
    ~cluster_registry_t();

    void add_member();
    void remove_member();
    

private:
    spinlock_t m_spinlock;
    drainer_t m_drainer;
};

class cluster_connection_t {
public:
private:
};

#endif // IO_CONNECTIVITY_HPP_
