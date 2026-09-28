#include "web-session-manager.hpp"

#include <sdbusplus/test/sdbus_mock.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class WebSessionManagerTest : public ::testing::Test
{
  protected:
    testing::NiceMock<sdbusplus::SdBusMock> sdbusMock;
    sdbusplus::bus_t bus{sdbusplus::get_mocked_new(&sdbusMock)};
    WebSessionManager mgr{bus, websessionMgrObj};
};

TEST_F(WebSessionManagerTest, Register_ValidParams_ReturnsTrueAndStoresSession)
{
    EXPECT_TRUE(mgr.webSessionRegister(0, "192.168.1.20", "webuser",
                                       sessionType::WEB, 0x4, 1));
    auto sessions = mgr.webSessionInfo();
    ASSERT_EQ(sessions.size(), 1U);
    EXPECT_EQ(std::get<1>(sessions[0]), "192.168.1.20");
    EXPECT_EQ(std::get<2>(sessions[0]), "webuser");
    EXPECT_EQ(std::get<3>(sessions[0]), static_cast<uint8_t>(sessionType::WEB));
}

TEST_F(WebSessionManagerTest, Register_SessionIdNotZero_ReturnsFalse)
{
    EXPECT_FALSE(mgr.webSessionRegister(7, "192.168.1.20", "webuser",
                                        sessionType::WEB, 0x4, 1));
    EXPECT_TRUE(mgr.webSessionInfo().empty());
}

TEST_F(WebSessionManagerTest, Register_WrongSessionType_ReturnsFalse)
{
    EXPECT_FALSE(mgr.webSessionRegister(0, "192.168.1.20", "webuser",
                                        sessionType::VMEDIA, 0x4, 1));
    EXPECT_TRUE(mgr.webSessionInfo().empty());
}

TEST_F(WebSessionManagerTest, Register_InvalidPrivilege_ReturnsFalse)
{
    EXPECT_FALSE(mgr.webSessionRegister(0, "192.168.1.20", "webuser",
                                        sessionType::WEB, 0x99, 1));
    EXPECT_TRUE(mgr.webSessionInfo().empty());
}

TEST_F(WebSessionManagerTest, Register_AllValidPrivileges_AllSucceed)
{
    for (const auto& [priv, name] : validPriv)
    {
        EXPECT_TRUE(mgr.webSessionRegister(0, "10.0.0.1", "user",
                                           sessionType::WEB, priv, 1));
    }
    EXPECT_EQ(mgr.webSessionInfo().size(), validPriv.size());
}

TEST_F(WebSessionManagerTest, Register_MultipleSessionsUidIncrementsCorrectly)
{
    mgr.webSessionRegister(0, "10.0.0.1", "u1", sessionType::WEB, 0x2, 1);
    mgr.webSessionRegister(0, "10.0.0.2", "u2", sessionType::WEB, 0x4, 2);
    auto sessions = mgr.webSessionInfo();
    ASSERT_EQ(sessions.size(), 2U);
    EXPECT_EQ(std::get<0>(sessions[0]), 1U);
    EXPECT_EQ(std::get<0>(sessions[1]), 2U);
}

TEST_F(WebSessionManagerTest, Unregister_ValidParams_ReturnsTrueAndRemoves)
{
    mgr.webSessionRegister(0, "192.168.1.20", "webuser", sessionType::WEB, 0x4,
                           1);
    uint8_t uid = std::get<0>(mgr.webSessionInfo()[0]);
    EXPECT_TRUE(mgr.webSessionUnregister(uid, sessionType::WEB, 0x01));
    EXPECT_TRUE(mgr.webSessionInfo().empty());
}

TEST_F(WebSessionManagerTest, Unregister_InvalidReason_ReturnsFalse)
{
    mgr.webSessionRegister(0, "192.168.1.20", "webuser", sessionType::WEB, 0x4,
                           1);
    uint8_t uid = std::get<0>(mgr.webSessionInfo()[0]);
    EXPECT_FALSE(mgr.webSessionUnregister(uid, sessionType::WEB, 0x00));
    EXPECT_EQ(mgr.webSessionInfo().size(), 1U);
}

TEST_F(WebSessionManagerTest, Unregister_WrongSessionType_ReturnsFalse)
{
    mgr.webSessionRegister(0, "192.168.1.20", "webuser", sessionType::WEB, 0x4,
                           1);
    uint8_t uid = std::get<0>(mgr.webSessionInfo()[0]);
    EXPECT_FALSE(mgr.webSessionUnregister(uid, sessionType::SSH, 0x01));
    EXPECT_EQ(mgr.webSessionInfo().size(), 1U);
}

TEST_F(WebSessionManagerTest, Unregister_SessionNotFound_ReturnsFalse)
{
    mgr.webSessionRegister(0, "192.168.1.20", "webuser", sessionType::WEB, 0x4,
                           1);
    EXPECT_FALSE(mgr.webSessionUnregister(99, sessionType::WEB, 0x01));
    EXPECT_EQ(mgr.webSessionInfo().size(), 1U);
}

TEST_F(WebSessionManagerTest, Unregister_AllValidReasons_Succeed)
{
    for (const auto& [reason, name] : reasonUnregister)
    {
        mgr.webSessionRegister(0, "10.0.0.1", "user", sessionType::WEB, 0x2, 1);
        uint8_t uid = std::get<0>(mgr.webSessionInfo().back());
        EXPECT_TRUE(mgr.webSessionUnregister(uid, sessionType::WEB, reason));
    }
    EXPECT_TRUE(mgr.webSessionInfo().empty());
}

TEST_F(WebSessionManagerTest, Unregister_EmptyList_ReturnsFalse)
{
    EXPECT_FALSE(mgr.webSessionUnregister(1, sessionType::WEB, 0x01));
}

TEST_F(WebSessionManagerTest, Unregister_FirstOfMultiple_SecondRemains)
{
    mgr.webSessionRegister(0, "10.0.0.1", "u1", sessionType::WEB, 0x2, 1);
    mgr.webSessionRegister(0, "10.0.0.2", "u2", sessionType::WEB, 0x4, 2);
    uint8_t firstUid = std::get<0>(mgr.webSessionInfo()[0]);
    EXPECT_TRUE(mgr.webSessionUnregister(firstUid, sessionType::WEB, 0x02));
    auto remaining = mgr.webSessionInfo();
    ASSERT_EQ(remaining.size(), 1U);
    EXPECT_EQ(std::get<2>(remaining[0]), "u2");
}

TEST_F(WebSessionManagerTest, Unregister_LastOfMultiple_FirstRemains)
{
    mgr.webSessionRegister(0, "10.0.0.1", "u1", sessionType::WEB, 0x2, 1);
    mgr.webSessionRegister(0, "10.0.0.2", "u2", sessionType::WEB, 0x4, 2);
    uint8_t lastUid = std::get<0>(mgr.webSessionInfo().back());
    EXPECT_TRUE(mgr.webSessionUnregister(lastUid, sessionType::WEB, 0x03));
    auto remaining = mgr.webSessionInfo();
    ASSERT_EQ(remaining.size(), 1U);
    EXPECT_EQ(std::get<2>(remaining[0]), "u1");
}

TEST_F(WebSessionManagerTest, Register_AllTupleFields_StoredCorrectly)
{
    mgr.webSessionRegister(0, "192.168.5.5", "webadmin", sessionType::WEB, 0x5,
                           10);
    auto sessions = mgr.webSessionInfo();
    ASSERT_EQ(sessions.size(), 1U);
    EXPECT_EQ(std::get<0>(sessions[0]), 1U);
    EXPECT_EQ(std::get<1>(sessions[0]), "192.168.5.5");
    EXPECT_EQ(std::get<2>(sessions[0]), "webadmin");
    EXPECT_EQ(std::get<3>(sessions[0]), static_cast<uint8_t>(sessionType::WEB));
    EXPECT_EQ(std::get<4>(sessions[0]), static_cast<uint8_t>(0x5));
    EXPECT_EQ(std::get<5>(sessions[0]), static_cast<uint8_t>(10));
}
