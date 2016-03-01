#ifndef _VEE_NET_H_
#define _VEE_NET_H_

#include <vee/stream.h>
#include <vee/platform.h>

namespace vee {
   
namespace net {

using port_t = unsigned short;
#if VEE_PLATFORM_X32
using socketfd_t = uint32_t;
#elif VEE_PLATFORM_X64
using socketfd_t = uint64_t;
#endif // VEE_PLATFORM

// Forward declaration
class net_stream;

using session_t = ::std::shared_ptr<net_stream>;

struct async_connect_info
{
    using shared_ptr = ::std::shared_ptr<async_connect_info>;
    bool is_success;
    session_t session;
};

class net_stream abstract: public io_stream
{
public:
    using this_t = net_stream;
    using ref_t  = this_t&;
    using rref_t = this_t&&;
    using shared_ptr = ::std::shared_ptr<this_t>;
    using unique_ptr = ::std::unique_ptr<this_t>;
    using async_connect_delegate = delegate<void(async_connect_info::shared_ptr)>;
    virtual ~net_stream() = default;
    virtual void connect(const char* ip, port_t port) __PURE;
    virtual void disconnect() __PURE;
    virtual void async_connect(async_connect_delegate::shared_ptr info) __noexcept __PURE;
    virtual socketfd_t native() __noexcept __PURE;
};

namespace tcp {

struct async_accept_info
{
    using shared_ptr = ::std::shared_ptr<async_accept_info>;
    bool is_success;
    session_t session;
};

class server abstract
{
public:
    using this_t = server;
    using ref_t = this_t&;
    using rref_t = this_t&&;
    using shared_ptr = ::std::shared_ptr<this_t>;
    using unique_ptr = ::std::unique_ptr<this_t>;
    using async_accept_delegate = delegate<void(async_accept_info::shared_ptr)>;
    virtual ~server() = default;
    virtual void open() __PURE;
    virtual void close() __PURE;
    virtual ::std::pair<bool, session_t> accept() __PURE;
    virtual void async_accept(async_accept_delegate::shared_ptr callback) __PURE;
};

session_t create_session();
server::shared_ptr create_server(port_t port);

} // !namespace tcp
    
namespace udp {

session_t create_stream();

} // !namespace udp

namespace rfc6455 {

session_t create_session();
tcp::server::shared_ptr create_server(port_t port);

} // !namespace rfc6455

} // !namespace net

} // !namespace vee

#endif // !_VEE_NET_H_