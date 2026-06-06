
/* ****************************************************************
 *
 * Session Management
 * Filename : main.cpp
 *
 * @brief Session Management contains dbus methods and properties for
 *  storing the session information.
 *
 * Author: Krishna Raj krishnar@ami.com
 *
 *****************************************************************/

#include "kvm-session-manager.hpp"
#include "ssh-session-manager.hpp"
#include "utils.hpp"
#include "vmedia-session-manager.hpp"
#include "web-session-manager.hpp"

#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/connection.hpp>

#include <iostream>
#include <tuple>
#include <vector>
int main()
{
    auto bus = sdbusplus::bus::new_default();
    // Claim the bus now
    bus.request_name(sessionDbusNmae);

    sdbusplus::server::manager_t objManager(bus, sessionMgrObj);

    KvmSessionManager KvmSession(bus, kvmsessionMgrObj);
    WebSessionManager WebSession(bus, websessionMgrObj);
    VmediaSessionManager VmediaSession(bus, vmediasessionMgrObj);
    SshSessionManager SshSession(bus, sshsessionMgrObj);

    // Wait for client request
    bus.process_loop();

    return 0;
}
