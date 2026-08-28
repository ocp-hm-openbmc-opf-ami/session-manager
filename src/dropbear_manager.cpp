#include <pwd.h>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>

#include <algorithm>
#include <chrono>
#include <csignal>
#include <iostream>
#include <map>
#include <mutex>
#include <regex>
#include <string>
#include <thread>
#include <variant>
#include <vector>

enum class UnregisterReason
{
    LOGOUT_SESSION = 1,
    LOGOUT_EXPIRY,
    TERMINATED_UNEXPECTEDLY
};

struct SessionInfo
{
    uint8_t sessionId;
    std::string ipAddress;
    std::string username;
    uint8_t sessionType;
    uint8_t privilege;
    uint8_t userId;
    bool pending = false;
};

std::map<std::string, SessionInfo> serviceSessions;
// Guards serviceSessions, shared between the monitorTriggers thread
// (register/deregisterService) and the monitorSshSessionInfo thread
// (compareSessionInfos).
std::mutex serviceSessionsMutex;
std::vector<SessionInfo> sshSessionInfoProperty;

constexpr int monitorTriggersDelay = 3;

using SshSessionInfoEntry =
    std::tuple<uint8_t, std::string, std::string, uint8_t, uint8_t, uint8_t>;
using SshSessionInfoType = std::vector<SshSessionInfoEntry>;

const char* systemdService = "org.freedesktop.systemd1";
const char* systemdPath = "/org/freedesktop/systemd1";
const char* managerInterface = "org.freedesktop.systemd1.Manager";

const char* dbusPropertyInterface = "org.freedesktop.DBus.Properties";
const char* serviceInterface = "org.freedesktop.systemd1.Service";

const char* userMgrService = "xyz.openbmc_project.User.Manager";
const char* userMgrIfc = "xyz.openbmc_project.User.Manager";
const char* userMgrObjpath = "/xyz/openbmc_project/user";

const char* sessionMgrservice = "xyz.openbmc_project.SessionManager";
const char* sessionMgrobjpath = "/xyz/openbmc_project/SessionManager/ssh";
const char* sessionMgrSSHInterface =
    "xyz.openbmc_project.SessionManager.SshSessionInfo";
SshSessionInfoType sessionInfoList;

uint8_t mapPrivilegeToLevel(const std::string& privilege)
{
    if (privilege == "priv-admin")
        return 4;
    if (privilege == "priv-operator")
        return 3;
    if (privilege == "priv-user")
        return 2;
    return 0;
}

using VariantType = std::variant<bool, std::string, std::vector<std::string>>;

void deregisterService(const std::string& serviceName);

bool stopService(const std::string& serviceName)
{
    try
    {
        auto bus = sdbusplus::bus::new_system();
        auto stopMethod = bus.new_method_call(systemdService, systemdPath,
                                              managerInterface, "StopUnit");
        stopMethod.append(serviceName, "replace");
        bus.call(stopMethod);
        return true;
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        std::cerr << "Failed to stop service " << serviceName << ": "
                  << ex.what() << std::endl;
        return false;
    }
}

std::string extractClientIP(const std::string& serviceName)
{
    std::regex ipRegex(
        R"(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}:\d{1,5}-(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}))");
    std::smatch match;

    if (std::regex_search(serviceName, match, ipRegex))
    {
        return match[1];
    }

    return "";
}

std::string extractServerIP(const std::string& serviceName) // 10.0.136.93
{
    std::regex ipRegex(
        R"((\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}):\d{1,5}-\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})");
    std::smatch match;

    if (std::regex_search(serviceName, match, ipRegex))
    {
        return match[1];
    }

    return "";
}

SessionInfo getServiceSessionInfo(const std::string& serviceName)
{
    SessionInfo sessionInfo = {0, "", "", 0, 0, 0};
    sdbusplus::message::object_path path;

    try
    {
        auto bus = sdbusplus::bus::new_system();

        auto method = bus.new_method_call(systemdService, systemdPath,
                                          managerInterface, "GetUnit");
        method.append(serviceName);
        auto reply = bus.call(method);

        reply.read(path);
        std::string uintpath = static_cast<std::string>(path);

        auto showMethod = bus.new_method_call(systemdService, uintpath.c_str(),
                                              dbusPropertyInterface, "Get");
        showMethod.append(serviceInterface, "MainPID");

        std::variant<uint32_t> mainPID;
        sdbusplus::message::message showResponse = bus.call(showMethod);
        showResponse.read(mainPID);
        uint32_t PID = std::get<uint32_t>(mainPID);

        std::string pidPath = "/proc/" + std::to_string(PID) + "/status";
        FILE* file = fopen(pidPath.c_str(), "r");
        if (file)
        {
            char line[256];
            while (fgets(line, sizeof(line), file))
            {
                if (strncmp(line, "Uid:", 4) == 0)
                {
                    unsigned int uid;
                    sscanf(line, "Uid:\t%u", &uid);
                    sessionInfo.userId = static_cast<uint8_t>(uid);
                    break;
                }
            }
            fclose(file);
        }

        struct passwd* pw = getpwuid(sessionInfo.userId);
        if (pw)
        {
            sessionInfo.username = pw->pw_name;
        }
        else
        {
            std::cerr << "Failed to get username for UID: "
                      << sessionInfo.userId << std::endl;
        }

        sessionInfo.ipAddress = extractClientIP(serviceName);
        std::string serverIPadd = extractServerIP(serviceName);

        auto getUserInfo = bus.new_method_call(userMgrService, userMgrObjpath,
                                               userMgrIfc, "GetUserInfo");
        getUserInfo.append(sessionInfo.username, serverIPadd);
        auto userInfo = bus.call(getUserInfo);
        std::map<std::string, VariantType> userInfoDetailes;
        userInfo.read(userInfoDetailes);

        auto it = userInfoDetailes.find("UserPrivilege");
        if (it != userInfoDetailes.end())
        {
            const auto& var = it->second;
            if (std::holds_alternative<std::string>(var))
            {
                std::string privilegeStr = std::get<std::string>(var);
                sessionInfo.privilege = mapPrivilegeToLevel(privilegeStr);
            }
            else
            {
                std::cerr << "Error in getting UserPrivilege" << std::endl;
            }
        }
        else
        {
            std::cerr << "Error in getting UserPrivilege" << std::endl;
        }
    }
    catch (const sdbusplus::exception::SdBusError& ex)
    {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

    return sessionInfo;
}

SshSessionInfoType getDbusProperty(sdbusplus::bus::bus& bus)
{
    SshSessionInfoType sessionInfoList;

    try
    {
        auto method = bus.new_method_call(sessionMgrservice, sessionMgrobjpath,
                                          dbusPropertyInterface, "Get");

        method.append(sessionMgrSSHInterface, "SshSessionInfo");

        sdbusplus::message::message response = bus.call(method);

        std::variant<SshSessionInfoType> result;
        response.read(result);

        sessionInfoList = std::get<SshSessionInfoType>(result);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error retrieving D-Bus property: " << e.what()
                  << std::endl;
    }

    return sessionInfoList;
}

void compareSessionInfos(SshSessionInfoType& sshSessionInfos)
{
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::string serviceName;
    if (sshSessionInfos == sessionInfoList)
    {
        std::cout << "Both session lists are the same. Skipping assignment."
                  << std::endl;
        return;
    }

    for (const auto& entry : sshSessionInfos)
    {
        bool valid = false;
        if (std::find(sessionInfoList.begin(), sessionInfoList.end(), entry) ==
            sessionInfoList.end())
        {
            {
                std::lock_guard<std::mutex> lock(serviceSessionsMutex);
                for (const auto& serviceEntry : serviceSessions)
                {
                    if (!serviceEntry.second.pending &&
                        serviceEntry.second.sessionId == std::get<0>(entry))
                    {
                        valid = true;
                        serviceName = serviceEntry.first;
                    }
                }
            }
            if (!valid)
            {
                try
                {
                    auto bus = sdbusplus::bus::new_default();
                    auto method = bus.new_method_call(
                        sessionMgrservice, sessionMgrobjpath,
                        sessionMgrSSHInterface, "SshSessionUnregister");

                    method.append(
                        std::get<0>(entry), std::get<3>(entry),
                        static_cast<int>(
                            UnregisterReason::TERMINATED_UNEXPECTEDLY));

                    sdbusplus::message::message response = bus.call(method);
                    bool success;
                    response.read(success);
                    if (success)
                    {
                        std::cout
                            << "successfully deregister the unrunning service"
                            << std::endl;
                    }
                }
                catch (const std::exception& e)
                {
                    std::cerr << "Error registering service " << serviceName
                              << ": " << e.what() << std::endl;
                }
            }
        }
    }

    for (const auto& entry : sessionInfoList)
    {
        if (std::find(sshSessionInfos.begin(), sshSessionInfos.end(), entry) ==
            sshSessionInfos.end())
        {
            std::vector<std::string> servicesToStop;
            {
                std::lock_guard<std::mutex> lock(serviceSessionsMutex);
                for (const auto& serviceEntry : serviceSessions)
                {
                    if (!serviceEntry.second.pending &&
                        serviceEntry.second.sessionId == std::get<0>(entry))
                    {
                        servicesToStop.push_back(serviceEntry.first);
                    }
                }
            }
            for (const auto& serviceToStop : servicesToStop)
            {
                stopService(serviceToStop);
            }
        }
    }

    sessionInfoList = sshSessionInfos;
}

void handleSshSessionInfoSignal(sdbusplus::message::message& msg)
{
    std::string interface;
    std::map<std::string, std::variant<SshSessionInfoType>> properties;

    msg.read(interface, properties);
    SshSessionInfoType sshSessionInfos;
    if (properties.find("SshSessionInfo") != properties.end())
    {
        sshSessionInfos =
            std::get<SshSessionInfoType>(properties["SshSessionInfo"]);
    }
    else
    {
        sshSessionInfos.clear();
    }
    compareSessionInfos(sshSessionInfos);
}

void monitorSshSessionInfo(sdbusplus::bus::bus& bus)
{
    auto match = sdbusplus::bus::match::match(
        bus,
        "type='signal',interface='org.freedesktop.DBus.Properties',"
        "member='PropertiesChanged',path='/xyz/openbmc_project/SessionManager/ssh',"
        "arg0='xyz.openbmc_project.SessionManager.SshSessionInfo'",
        handleSshSessionInfoSignal);

    while (true)
    {
        bus.process_discard();
        bus.wait();
    }
}

void registerService(const std::string& serviceName)
{
    std::cout << "Registering service: " << serviceName << std::endl;

    {
        std::lock_guard<std::mutex> lock(serviceSessionsMutex);
        bool inserted =
            serviceSessions
                .try_emplace(serviceName, SessionInfo{.pending = true})
                .second;
        if (!inserted)
        {
            std::cout << "Service " << serviceName << " is already registered."
                      << std::endl;
            return;
        }
    }

    SessionInfo sessionInfo = getServiceSessionInfo(serviceName);
    sessionInfo.sessionType = 3;

    auto bus = sdbusplus::bus::new_default();
    try
    {
        auto method =
            bus.new_method_call(sessionMgrservice, sessionMgrobjpath,
                                sessionMgrSSHInterface, "SshSessionRegister");

        method.append(sessionInfo.sessionId, sessionInfo.ipAddress,
                      sessionInfo.username, sessionInfo.sessionType,
                      sessionInfo.privilege, sessionInfo.userId);

        sdbusplus::message::message response = bus.call(method);
        bool success;
        response.read(success);

        if (success)
        {
            auto method2 =
                bus.new_method_call(sessionMgrservice, sessionMgrobjpath,
                                    dbusPropertyInterface, "Get");

            method2.append(sessionMgrSSHInterface, "SshSessionInfo");

            sdbusplus::message::message response2 = bus.call(method2);

            std::variant<std::vector<std::tuple<
                uint8_t, std::string, std::string, uint8_t, uint8_t, uint8_t>>>
                result;
            response2.read(result);

            const auto& sessionData = std::get<std::vector<std::tuple<
                uint8_t, std::string, std::string, uint8_t, uint8_t, uint8_t>>>(
                result);

            if (!sessionData.empty())
            {
                const auto& lastEntry = sessionData.back();
                sessionInfo.sessionId = std::get<0>(
                    lastEntry); // Update session ID from the last entry
            }
            else
            {
                std::cerr
                    << "Error: No session data available to update sessionId for service "
                    << serviceName << std::endl;
            }

            {
                std::lock_guard<std::mutex> lock(serviceSessionsMutex);
                serviceSessions[serviceName] = sessionInfo;
            }
            std::cout << "successfully register service " << serviceName
                      << std::endl;
            return;
        }
        else
        {
            std::cout << "Service " << serviceName << " registration failed."
                      << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error registering service " << serviceName << ": "
                  << e.what() << std::endl;
    }

    {
        std::lock_guard<std::mutex> lock(serviceSessionsMutex);
        serviceSessions.erase(serviceName);
    }
    return;
}

void deregisterService(const std::string& serviceName)
{
    SessionInfo sessionInfo;
    {
        std::lock_guard<std::mutex> lock(serviceSessionsMutex);
        auto it = serviceSessions.find(serviceName);
        if (it == serviceSessions.end() || it->second.pending)
        {
            std::cerr << "Service " << serviceName << " is not registered."
                      << std::endl;
            return;
        }
        sessionInfo = it->second;
    }

    UnregisterReason reason = UnregisterReason::LOGOUT_SESSION;
    auto bus = sdbusplus::bus::new_default();
    try
    {
        auto method =
            bus.new_method_call(sessionMgrservice, sessionMgrobjpath,
                                sessionMgrSSHInterface, "SshSessionUnregister");

        method.append(sessionInfo.sessionId, sessionInfo.sessionType,
                      static_cast<uint8_t>(reason));

        sdbusplus::message::message response = bus.call(method);
        bool success;
        response.read(success);

        if (success)
        {
            std::cout << "Service " << serviceName
                      << " deregistered successfully for reason: ";
            {
                std::lock_guard<std::mutex> lock(serviceSessionsMutex);
                serviceSessions.erase(serviceName);
            }
            switch (reason)
            {
                case UnregisterReason::LOGOUT_SESSION:
                    std::cout << "Logout session" << std::endl;
                    break;
                case UnregisterReason::LOGOUT_EXPIRY:
                    std::cout << "Logout expiry" << std::endl;
                    break;
                case UnregisterReason::TERMINATED_UNEXPECTEDLY:
                    std::cout << "Session terminated unexpectedly" << std::endl;
                    break;
            }
            return;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error deregistering service " << serviceName << ": "
                  << e.what() << std::endl;
    }
    return;
}

std::vector<std::string> getTriggersProperty(sdbusplus::bus::bus& bus)
{
    std::vector<std::string> triggers;
    auto method = bus.new_method_call(
        systemdService, "/org/freedesktop/systemd1/unit/dropbear_2esocket",
        dbusPropertyInterface, "Get");
    method.append("org.freedesktop.systemd1.Unit", "Triggers");

    try
    {
        sdbusplus::message::message reply = bus.call(method);

        std::variant<std::vector<std::string>> result;

        reply.read(result);

        triggers = std::get<std::vector<std::string>>(result);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Failed to retrieve Triggers property: " << e.what()
                  << std::endl;
    }
    return triggers;
}

void monitorTriggers()
{
    auto bus = sdbusplus::bus::new_default();
    std::vector<std::string> variable1 = getTriggersProperty(bus);
    std::vector<std::string> variable2;

    for (const auto& service : variable1)
    {
        registerService(service);
    }

    while (true)
    {
        variable2 = getTriggersProperty(bus);

        for (const auto& service : variable1)
        {
            if (std::find(variable2.begin(), variable2.end(), service) ==
                variable2.end())
            {
                deregisterService(service);
                std::cout << "Deregistered service: " << service << std::endl;
            }
        }

        for (const auto& service : variable2)
        {
            if (std::find(variable1.begin(), variable1.end(), service) ==
                variable1.end())
            {
                registerService(service);
                std::cout << "Registered service: " << service << std::endl;
            }
        }

        variable1 = variable2;
        std::this_thread::sleep_for(std::chrono::seconds(monitorTriggersDelay));
    }
}

int main()
{
    auto bus = sdbusplus::bus::new_default();

    sessionInfoList = getDbusProperty(bus);

    std::thread monitorThread(monitorTriggers);
    std::thread monitorThread1(monitorSshSessionInfo, std::ref(bus));

    monitorThread.join();
    monitorThread1.join();

    return 0;
}
