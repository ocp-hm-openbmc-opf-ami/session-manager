#include "web-session-manager.hpp"

bool WebSessionManager::webSessionRegister(
    uint8_t sessionId, std::string ipAdress, std::string userName,
    uint8_t sessionType, uint8_t previlage, uint8_t userId)
{
    if (!isAuthorizedCaller({bmcwebExecutable}) || sessionId != 0 ||
        sessionType != sessionType::WEB ||
        (validPriv.find(previlage) == validPriv.end()))
    {
        return false;
    }

    if (uid == maxSessionId)
    {
        return false;
    }
    uid++;
    webInfo newSession;
    newSession = std::make_tuple(uid, ipAdress, userName, sessionType,
                                 previlage, userId);
    std::vector<webInfo> webSession = webSessionInfo();
    webSession.push_back(newSession);
    webSessionInfo(webSession);
    return true;
}

bool WebSessionManager::webSessionUnregister(
    uint8_t sessionId, uint8_t sessionType, uint8_t reason)
{
    bool status = false;
    if (!isAuthorizedCaller({bmcwebExecutable}) ||
        reasonUnregister.find(reason) == reasonUnregister.end() ||
        sessionType != sessionType::WEB)
    {
        return status;
    }
    std::vector<webInfo> webSession = webSessionInfo();

    for (auto itr = webSession.begin(); itr != webSession.end(); ++itr)
    {
        if (std::get<0>(*itr) == sessionId)
        {
            itr = webSession.erase(itr);
            webSessionInfo(webSession);
            status = true;
            break;
        }
    }
    return status;
}

void WebSessionManager::crashMonitor(sdbusplus::message_t& msg)
{
    uint32_t jobID{};
    sdbusplus::message::object_path jobPath;
    std::string jobUnit{};
    std::string jobResult{};
    msg.read(jobID, jobPath, jobUnit, jobResult);

    if (jobResult == "failed" && jobUnit == webService)
    {
        std::vector<webInfo> webSession = webSessionInfo();
        webSession.clear();
        webSessionInfo(webSession);
    }

    return;
}
