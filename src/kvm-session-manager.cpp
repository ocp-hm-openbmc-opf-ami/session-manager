#include "kvm-session-manager.hpp"

bool KvmSessionManager::kvmSessionRegister(
    uint8_t sessionId, std::string ipAdress, std::string userName,
    uint8_t sessionType, uint8_t previlage, uint8_t userId)
{
    if (!isAuthorizedCaller({obmcIkvmExecutable, bmcwebExecutable}) ||
        sessionId != 0 || sessionType != sessionType::KVM ||
        (validPriv.find(previlage) == validPriv.end()))
    {
        return false;
    }

    if (uid == maxSessionId)
    {
        return false;
    }
    uid++;
    kvmInfo newSession;
    newSession = std::make_tuple(uid, ipAdress, userName, sessionType,
                                 previlage, userId);
    std::vector<kvmInfo> kvmSession = kvmSessionInfo();
    kvmSession.push_back(newSession);
    kvmSessionInfo(kvmSession);
    return true;
}

bool KvmSessionManager::kvmSessionUnregister(
    uint8_t sessionId, uint8_t sessionType, uint8_t reason)
{
    bool status = false;
    if (!isAuthorizedCaller({obmcIkvmExecutable, bmcwebExecutable}) ||
        reasonUnregister.find(reason) == reasonUnregister.end() ||
        sessionType != sessionType::KVM)
    {
        return status;
    }
    std::vector<kvmInfo> kvmSession = kvmSessionInfo();

    for (auto itr = kvmSession.begin(); itr != kvmSession.end(); ++itr)
    {
        if (std::get<0>(*itr) == sessionId)
        {
            itr = kvmSession.erase(itr);
            kvmSessionInfo(kvmSession);
            status = true;
            break;
        }
    }
    return status;
}

void KvmSessionManager::crashMonitor(sdbusplus::message_t& msg)
{
    uint32_t jobID{};
    sdbusplus::message::object_path jobPath;
    std::string jobUnit{};
    std::string jobResult{};
    msg.read(jobID, jobPath, jobUnit, jobResult);

    if (jobResult == "failed" && jobUnit == kvmService)
    {
        std::vector<kvmInfo> kvmSession = kvmSessionInfo();
        kvmSession.clear();
        kvmSessionInfo(kvmSession);
    }

    return;
}

bool KvmSessionManager::clearAll()
{
    std::vector<kvmInfo> kvmSession = kvmSessionInfo();

    // Check if any session is available.
    if (kvmSession.size() == 0)
    {
        return false;
    }
    kvmSession.clear();
    kvmSessionInfo(kvmSession);
    return true;
}
