#pragma once

#include <systemd/sd-bus.h>

#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <string>

static constexpr const char* sessionMgrObj =
    "/xyz/openbmc_project/SessionManager";
static constexpr const char* sessionDbusNmae =
    "xyz.openbmc_project.SessionManager";

using namespace sdbusplus::bus::match::rules;
constexpr auto matchString =
    "type='signal',interface='org.freedesktop.systemd1.Manager',"
    "member='JobRemoved'";

// Session Information
using SessionId = uint8_t;
using IpAdress = std::string;
using UserName = std::string;
using SessionType = uint8_t;
using Previlage = uint8_t;
using UserId = uint8_t;
using MountType = std::string;
using SlotId = std::string;

enum sessionType
{
    KVM = 0,
    WEB = 1,
    VMEDIA = 2,
    SSH = 3
};

const std::map<uint8_t, std::string> validPriv = {
    {0x1, "Callback"},
    {0x2, "User"},
    {0x3, "Operator"},
    {0x4, "Administrator"},
    {0x5, "OEM Proprietary"}};

// Reason to Unregister the session
const std::map<uint8_t, std::string> reasonUnregister = {
    {0x01, "Logout"}, {0x02, "Expired"}, {0x03, "Unknown"}};

// Centralized authorized D-Bus caller executable paths, shared by all
// session manager types to avoid duplicated/drifting literals per header.
constexpr auto bmcwebExecutable = "/usr/libexec/bmcwebd";
constexpr auto obmcIkvmExecutable = "/usr/bin/obmc-ikvm";
constexpr auto dropbearManagerExecutable = "/usr/bin/dropbear-manager";
constexpr auto virtualMediaExecutable = "/usr/libexec/virtual-media";

inline bool isAuthorizedCaller(
    std::initializer_list<const char*> expectedExecutables)
{
#ifdef SESSION_MANAGER_TESTS
    // Unit tests call the D-Bus methods directly with no sender context.
    (void)expectedExecutables;
    return true;
#else
    // Method calls are dispatched on the process default bus (new_default()).
    sd_bus* nativeBus = nullptr;
    if (sd_bus_default(&nativeBus) < 0 || nativeBus == nullptr)
    {
        return false;
    }

    // Borrowed reference, valid only while the call is being dispatched.
    sd_bus_message* message = sd_bus_get_current_message(nativeBus);

    // Fail closed when there is no D-Bus sender context.
    if (message == nullptr)
    {
        sd_bus_unref(nativeBus);
        return false;
    }

    sd_bus_creds* credentials = nullptr;
    // SD_BUS_CREDS_EXE is never sent on the wire; AUGMENT lets sd-bus resolve
    // it via /proc/<pid>/exe as a fallback.
    int rc = sd_bus_query_sender_creds(
        message, SD_BUS_CREDS_EXE | SD_BUS_CREDS_AUGMENT, &credentials);
    if (rc < 0 || credentials == nullptr)
    {
        sd_bus_unref(nativeBus);
        return false;
    }

    const char* executable = nullptr;
    bool haveExe = sd_bus_creds_get_exe(credentials, &executable) >= 0 &&
                   executable != nullptr;

    bool authorized =
        haveExe &&
        std::any_of(expectedExecutables.begin(), expectedExecutables.end(),
                    [executable](const char* expected) {
                        return std::strcmp(executable, expected) == 0;
                    });

    sd_bus_creds_unref(credentials);
    sd_bus_unref(nativeBus);
    return authorized;
#endif
}

constexpr uint8_t maxSessionId = 0xff;
