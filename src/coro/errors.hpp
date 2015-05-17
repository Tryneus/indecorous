#ifndef CORO_ERRORS_HPP_
#define CORO_ERRORS_HPP_

#include <exception>
#include <string>

class coro_exc_t : public std::exception {
public:
  coro_exc_t(const std::string& info) :
    m_info(info) { }

  virtual ~coro_exc_t() throw() { }

  virtual const char* what() const throw() {
    return m_info.c_str();
  }
private:
  const std::string m_info;
};

class parameter_exc_t : public coro_exc_t {
public:
  parameter_exc_t(const std::string& info) :
    coro_exc_t(info) { }
  virtual ~parameter_exc_t() throw() { }
};

class wait_error_exc_t : public coro_exc_t {
public:
  wait_error_exc_t(const std::string& info) :
    coro_exc_t(info) { }
  virtual ~wait_error_exc_t() throw() { }
};

class wait_interrupted_exc_t : public wait_error_exc_t {
public:
  wait_interrupted_exc_t() :
    wait_error_exc_t("wait object interrupted during wait") { }
  virtual ~wait_interrupted_exc_t() throw() { }
};

class wait_object_lost_exc_t : public wait_error_exc_t {
public:
  wait_object_lost_exc_t() :
    wait_error_exc_t("wait object destroyed during wait") { }
  virtual ~wait_object_lost_exc_t() throw() { }
};

#endif
