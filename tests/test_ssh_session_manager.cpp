#include "ssh-session-manager.hpp"

#include <sdbusplus/test/sdbus_mock.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class SshSessionManagerTest : public ::testing::Test
{
  protected:
    testing::NiceMock<sdbusplus::SdBusMock> sdbusMock;
    sdbusplus::bus_t bus{sdbusplus::get_mocked_new(&sdbusMock)};
    SshSessionManager mgr{bus, sshsessionMgrObj};
};

TEST_F(SshSessionManagerTest, Register_ValidParams_ReturnsTrueAndStoresSession)
{
    EXPECT_TRUE(mgr.sshSessionRegister(0, "192.168.1.1", "admin",
                                       sessionType::SSH, 0x4, 1));
    auto sessions = mgr.sshSessionInfo();
    ASSERT_EQ(sessions.size(), 1U);
    EXPECT_EQ(std::get<1>(sessions[0]), "192.168.1.1");
    EXPECT_EQ(std::get<2>(sessions[0]), "admin");
    EXPECT_EQ(std::get<3>(sessions[0]), static_cast<uint8_t>(sessionType::SSH));
}

TEST_F(SshSessionManagerTest, Register_SessionIdNotZero_ReturnsFalse)
{
    EXPECT_FALSE(mgr.sshSessionRegister(5, "192.168.1.1", "admin",
                                        sessionType::SSH, 0x4, 1));
    EXPECT_TRUE(mgr.sshSessionInfo().empty());
}

TEST_F(SshSessionManagerTest, Register_WrongSessionType_ReturnsFalse)
{
    EXPECT_FALSE(mgr.sshSessionRegister(0, "192.168.1.1", "admin",
                                        sessionType::KVM, 0x4, 1));
    EXPECT_TRUE(mgr.sshSessionInfo().empty());
}

TEST_F(SshSessionManagerTest, Register_InvalidPrivilege_ReturnsFalse)
{
    EXPECT_FALSE(mgr.sshSessionRegister(0, "192.168.1.1", "admin",
                                        sessionType::SSH, 0xFF, 1));
    EXPECT_TRUE(mgr.sshSessionInfo().empty());
}

TEST_F(SshSessionManagerTest, Register_PrivilegeZero_ReturnsFalse)
{
    EXPECT_FALSE(mgr.sshSessionRegister(0, "10.0.0.1", "user", sessionType::SSH,
                                        0x0, 1));
    EXPECT_TRUE(mgr.sshSessionInfo().empty());
}

TEST_F(SshSessionManagerTest, Register_AllValidPrivileges_AllSucceed)
{
    for (const auto& [priv, name] : validPriv)
    {
        EXPECT_TRUE(mgr.sshSessionRegister(0, "10.0.0.1", "user",
                                           sessionType::SSH, priv, 1));
    }
    EXPECT_EQ(mgr.sshSessionInfo().size(), validPriv.size());
}

TEST_F(SshSessionManagerTest, Register_MultipleSessionsUidIncrementsCorrectly)
{
    mgr.sshSessionRegister(0, "192.168.1.1", "user1", sessionType::SSH, 0x2, 1);
    mgr.sshSessionRegister(0, "192.168.1.2", "user2", sessionType::SSH, 0x3, 2);
    auto sessions = mgr.sshSessionInfo();
    ASSERT_EQ(sessions.size(), 2U);
    EXPECT_EQ(std::get<0>(sessions[0]), 1U);
    EXPECT_EQ(std::get<0>(sessions[1]), 2U);
}

TEST_F(SshSessionManagerTest, Unregister_ValidParams_ReturnsTrueAndRemoves)
{
    mgr.sshSessionRegister(0, "192.168.1.1", "admin", sessionType::SSH, 0x4, 1);
    uint8_t uid = std::get<0>(mgr.sshSessionInfo()[0]);
    EXPECT_TRUE(mgr.sshSessionUnregister(uid, sessionType::SSH, 0x01));
    EXPECT_TRUE(mgr.sshSessionInfo().empty());
}

TEST_F(SshSessionManagerTest, Unregister_AllValidReasons_Succeed)
{
    for (const auto& [reason, name] : reasonUnregister)
    {
        mgr.sshSessionRegister(0, "10.0.0.1", "user", sessionType::SSH, 0x2, 1);
        uint8_t uid = std::get<0>(mgr.sshSessionInfo().back());
        EXPECT_TRUE(mgr.sshSessionUnregister(uid, sessionType::SSH, reason));
    }
    EXPECT_TRUE(mgr.sshSessionInfo().empty());
}

TEST_F(SshSessionManagerTest, Unregister_InvalidReason_ReturnsFalse)
{
    mgr.sshSessionRegister(0, "192.168.1.1", "admin", sessionType::SSH, 0x4, 1);
    uint8_t uid = std::get<0>(mgr.sshSessionInfo()[0]);
    EXPECT_FALSE(mgr.sshSessionUnregister(uid, sessionType::SSH, 0xFF));
    EXPECT_EQ(mgr.sshSessionInfo().size(), 1U);
}

TEST_F(SshSessionManagerTest, Unregister_WrongSessionType_ReturnsFalse)
{
    mgr.sshSessionRegister(0, "192.168.1.1", "admin", sessionType::SSH, 0x4, 1);
    uint8_t uid = std::get<0>(mgr.sshSessionInfo()[0]);
    EXPECT_FALSE(mgr.sshSessionUnregister(uid, sessionType::KVM, 0x01));
    EXPECT_EQ(mgr.sshSessionInfo().size(), 1U);
}

TEST_F(SshSessionManagerTest, Unregister_SessionNotFound_ReturnsFalse)
{
    mgr.sshSessionRegister(0, "192.168.1.1", "admin", sessionType::SSH, 0x4, 1);
    EXPECT_FALSE(mgr.sshSessionUnregister(99, sessionType::SSH, 0x01));
    EXPECT_EQ(mgr.sshSessionInfo().size(), 1U);
}

TEST_F(SshSessionManagerTest, Unregister_EmptyList_ReturnsFalse)
{
    EXPECT_FALSE(mgr.sshSessionUnregister(1, sessionType::SSH, 0x01));
}

TEST_F(SshSessionManagerTest, Register_UnregisterMiddleSession_OthersRemain)
{
    mgr.sshSessionRegister(0, "10.0.0.1", "u1", sessionType::SSH, 0x2, 1);
    mgr.sshSessionRegister(0, "10.0.0.2", "u2", sessionType::SSH, 0x3, 2);
    mgr.sshSessionRegister(0, "10.0.0.3", "u3", sessionType::SSH, 0x4, 3);
    auto sessions = mgr.sshSessionInfo();
    uint8_t midUid = std::get<0>(sessions[1]);
    EXPECT_TRUE(mgr.sshSessionUnregister(midUid, sessionType::SSH, 0x01));
    auto remaining = mgr.sshSessionInfo();
    ASSERT_EQ(remaining.size(), 2U);
    EXPECT_EQ(std::get<2>(remaining[0]), "u1");
    EXPECT_EQ(std::get<2>(remaining[1]), "u3");
}

TEST_F(SshSessionManagerTest, Unregister_FirstOfMultiple_SecondRemains)
{
    mgr.sshSessionRegister(0, "10.0.0.1", "u1", sessionType::SSH, 0x2, 1);
    mgr.sshSessionRegister(0, "10.0.0.2", "u2", sessionType::SSH, 0x4, 2);
    uint8_t firstUid = std::get<0>(mgr.sshSessionInfo()[0]);
    EXPECT_TRUE(mgr.sshSessionUnregister(firstUid, sessionType::SSH, 0x02));
    auto remaining = mgr.sshSessionInfo();
    ASSERT_EQ(remaining.size(), 1U);
    EXPECT_EQ(std::get<2>(remaining[0]), "u2");
}

TEST_F(SshSessionManagerTest, Unregister_LastOfMultiple_FirstRemains)
{
    mgr.sshSessionRegister(0, "10.0.0.1", "u1", sessionType::SSH, 0x2, 1);
    mgr.sshSessionRegister(0, "10.0.0.2", "u2", sessionType::SSH, 0x4, 2);
    uint8_t lastUid = std::get<0>(mgr.sshSessionInfo().back());
    EXPECT_TRUE(mgr.sshSessionUnregister(lastUid, sessionType::SSH, 0x03));
    auto remaining = mgr.sshSessionInfo();
    ASSERT_EQ(remaining.size(), 1U);
    EXPECT_EQ(std::get<2>(remaining[0]), "u1");
}

TEST_F(SshSessionManagerTest, Register_AllTupleFields_StoredCorrectly)
{
    mgr.sshSessionRegister(0, "172.16.0.1", "tester", sessionType::SSH, 0x3,
                           99);
    auto sessions = mgr.sshSessionInfo();
    ASSERT_EQ(sessions.size(), 1U);
    EXPECT_EQ(std::get<0>(sessions[0]), 1U);
    EXPECT_EQ(std::get<1>(sessions[0]), "172.16.0.1");
    EXPECT_EQ(std::get<2>(sessions[0]), "tester");
    EXPECT_EQ(std::get<3>(sessions[0]), static_cast<uint8_t>(sessionType::SSH));
    EXPECT_EQ(std::get<4>(sessions[0]), static_cast<uint8_t>(0x3));
    EXPECT_EQ(std::get<5>(sessions[0]), static_cast<uint8_t>(99));
}

TEST_F(SshSessionManagerTest,
       Register_ThenUnregister_ThenRegisterAgain_UidContinues)
{
    mgr.sshSessionRegister(0, "10.0.0.1", "u1", sessionType::SSH, 0x2, 1);
    uint8_t uid1 = std::get<0>(mgr.sshSessionInfo()[0]);
    mgr.sshSessionUnregister(uid1, sessionType::SSH, 0x01);
    mgr.sshSessionRegister(0, "10.0.0.2", "u2", sessionType::SSH, 0x4, 2);
    uint8_t uid2 = std::get<0>(mgr.sshSessionInfo()[0]);
    EXPECT_GT(uid2, uid1);
}
