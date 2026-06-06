#pragma once

#include <sdbusplus/asio/object_server.hpp>

#include <iostream>

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
