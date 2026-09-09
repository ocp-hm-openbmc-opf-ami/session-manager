#include "ssh-session-manager.hpp"

bool SshSessionManager::sshSessionRegister(
    uint8_t sessionId, std::string ipAdress, std::string userName,
    uint8_t sessionType, uint8_t previlage, uint8_t userId)

{
    if (!isAuthorizedCaller({dropbearManagerExecutable, bmcwebExecutable}) ||
        sessionId != 0 ||
        sessionType != sessionType::SSH ||
        (validPriv.find(previlage) == validPriv.end()))
    {
        return false;
    }

    if (uid == maxSessionId)
    {
        return false;
    }
    uid++;
    sshInfo newSession;
    newSession = std::make_tuple(uid, ipAdress, userName, sessionType,
                                 previlage, userId);
    std::vector<sshInfo> sshSession = sshSessionInfo();
    sshSession.push_back(newSession);
    sshSessionInfo(sshSession);
    return true;
}

bool SshSessionManager::sshSessionUnregister(
    uint8_t sessionId, uint8_t sessionType, uint8_t reason)
{
    bool status = false;
    if (!isAuthorizedCaller({dropbearManagerExecutable, bmcwebExecutable}) ||
        reasonUnregister.find(reason) == reasonUnregister.end() ||
        sessionType != sessionType::SSH)
    {
        return status;
    }
    std::vector<sshInfo> sshSession = sshSessionInfo();

    for (auto itr = sshSession.begin(); itr != sshSession.end(); ++itr)
    {
        if (std::get<0>(*itr) == sessionId)
        {
            itr = sshSession.erase(itr);
            sshSessionInfo(sshSession);
            status = true;
            break;
        }
    }
    return status;
}
