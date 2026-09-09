#include "vmedia-session-manager.hpp"

bool VmediaSessionManager::vmediaSessionRegister(
    uint8_t sessionId, std::string ipAdress, std::string userName,
    uint8_t sessionType, uint8_t previlage, uint8_t userId,
    std::string mountType, std::string slotId)
{
    if (!isAuthorizedCaller({virtualMediaExecutable, bmcwebExecutable}) ||
        sessionId != 0 || sessionType != sessionType::VMEDIA ||
        (validPriv.find(previlage) == validPriv.end()))
    {
        return false;
    }

    if (uid == maxSessionId)
    {
        return false;
    }
    uid++;
    vmediaInfo newSession;
    newSession = std::make_tuple(uid, ipAdress, userName, sessionType,
                                 previlage, userId, mountType, slotId);
    std::vector<vmediaInfo> vmediaSession = vmediaSessionInfo();
    vmediaSession.push_back(newSession);
    vmediaSessionInfo(vmediaSession);
    return true;
}

bool VmediaSessionManager::vmediaSessionUnregister(
    uint8_t sessionId, uint8_t sessionType, uint8_t reason)
{
    if (!isAuthorizedCaller({virtualMediaExecutable, bmcwebExecutable}) ||
        sessionType != sessionType::VMEDIA)
    {
        return false;
    }
    bool status = false;
    if (reasonUnregister.find(reason) == reasonUnregister.end())
    {
        return status;
    }
    std::vector<vmediaInfo> vmediaSession = vmediaSessionInfo();

    for (auto itr = vmediaSession.begin(); itr != vmediaSession.end(); ++itr)
    {
        if (std::get<0>(*itr) == sessionId)
        {
            itr = vmediaSession.erase(itr);
            vmediaSessionInfo(vmediaSession);
            status = true;
            break;
        }
    }
    return status;
}

void VmediaSessionManager::crashMonitor(sdbusplus::message_t& msg)
{
    uint32_t jobID{};
    sdbusplus::message::object_path jobPath;
    std::string jobUnit{};
    std::string jobResult{};
    msg.read(jobID, jobPath, jobUnit, jobResult);

    if (jobResult == "failed" && jobUnit == vmediaService)
    {
        std::vector<vmediaInfo> vmediaSession = vmediaSessionInfo();
        vmediaSession.clear();
        vmediaSessionInfo(vmediaSession);
    }

    return;
}
