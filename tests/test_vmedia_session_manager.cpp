#include "vmedia-session-manager.hpp"

#include <sdbusplus/test/sdbus_mock.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class VmediaSessionManagerTest : public ::testing::Test
{
  protected:
    testing::NiceMock<sdbusplus::SdBusMock> sdbusMock;
    sdbusplus::bus_t bus{sdbusplus::get_mocked_new(&sdbusMock)};
    VmediaSessionManager mgr{bus, vmediasessionMgrObj};
};

TEST_F(VmediaSessionManagerTest,
       Register_ValidParams_ReturnsTrueAndStoresSession)
{
    EXPECT_TRUE(mgr.vmediaSessionRegister(
        0, "192.168.1.30", "vuser", sessionType::VMEDIA, 0x4, 1, "remote",
        "slot0"));
    auto sessions = mgr.vmediaSessionInfo();
    ASSERT_EQ(sessions.size(), 1U);
    EXPECT_EQ(std::get<1>(sessions[0]), "192.168.1.30");
    EXPECT_EQ(std::get<2>(sessions[0]), "vuser");
    EXPECT_EQ(std::get<3>(sessions[0]),
              static_cast<uint8_t>(sessionType::VMEDIA));
    EXPECT_EQ(std::get<6>(sessions[0]), "remote");
    EXPECT_EQ(std::get<7>(sessions[0]), "slot0");
}

TEST_F(VmediaSessionManagerTest, Register_SessionIdNotZero_ReturnsFalse)
{
    EXPECT_FALSE(mgr.vmediaSessionRegister(
        3, "192.168.1.30", "vuser", sessionType::VMEDIA, 0x4, 1, "remote",
        "slot0"));
    EXPECT_TRUE(mgr.vmediaSessionInfo().empty());
}

TEST_F(VmediaSessionManagerTest, Register_WrongSessionType_ReturnsFalse)
{
    EXPECT_FALSE(
        mgr.vmediaSessionRegister(0, "192.168.1.30", "vuser", sessionType::WEB,
                                  0x4, 1, "remote", "slot0"));
    EXPECT_TRUE(mgr.vmediaSessionInfo().empty());
}

TEST_F(VmediaSessionManagerTest, Register_InvalidPrivilege_ReturnsFalse)
{
    EXPECT_FALSE(mgr.vmediaSessionRegister(
        0, "192.168.1.30", "vuser", sessionType::VMEDIA, 0x00, 1, "remote",
        "slot0"));
    EXPECT_TRUE(mgr.vmediaSessionInfo().empty());
}

TEST_F(VmediaSessionManagerTest, Register_DifferentMountTypes_StoredCorrectly)
{
    mgr.vmediaSessionRegister(0, "10.0.0.1", "u1", sessionType::VMEDIA, 0x3, 1,
                              "console", "slot1");
    mgr.vmediaSessionRegister(0, "10.0.0.2", "u2", sessionType::VMEDIA, 0x4, 2,
                              "remote", "slot2");
    auto sessions = mgr.vmediaSessionInfo();
    ASSERT_EQ(sessions.size(), 2U);
    EXPECT_EQ(std::get<6>(sessions[0]), "console");
    EXPECT_EQ(std::get<7>(sessions[0]), "slot1");
    EXPECT_EQ(std::get<6>(sessions[1]), "remote");
    EXPECT_EQ(std::get<7>(sessions[1]), "slot2");
}

TEST_F(VmediaSessionManagerTest,
       Register_MultipleSessionsUidIncrementsCorrectly)
{
    mgr.vmediaSessionRegister(0, "10.0.0.1", "u1", sessionType::VMEDIA, 0x2, 1,
                              "remote", "s0");
    mgr.vmediaSessionRegister(0, "10.0.0.2", "u2", sessionType::VMEDIA, 0x4, 2,
                              "remote", "s1");
    auto sessions = mgr.vmediaSessionInfo();
    ASSERT_EQ(sessions.size(), 2U);
    EXPECT_EQ(std::get<0>(sessions[0]), 1U);
    EXPECT_EQ(std::get<0>(sessions[1]), 2U);
}

TEST_F(VmediaSessionManagerTest, Unregister_ValidParams_ReturnsTrueAndRemoves)
{
    mgr.vmediaSessionRegister(0, "192.168.1.30", "vuser", sessionType::VMEDIA,
                              0x4, 1, "remote", "s0");
    uint8_t uid = std::get<0>(mgr.vmediaSessionInfo()[0]);
    EXPECT_TRUE(mgr.vmediaSessionUnregister(uid, sessionType::VMEDIA, 0x01));
    EXPECT_TRUE(mgr.vmediaSessionInfo().empty());
}

TEST_F(VmediaSessionManagerTest, Unregister_WrongSessionType_ReturnsFalse)
{
    mgr.vmediaSessionRegister(0, "192.168.1.30", "vuser", sessionType::VMEDIA,
                              0x4, 1, "remote", "s0");
    uint8_t uid = std::get<0>(mgr.vmediaSessionInfo()[0]);
    EXPECT_FALSE(mgr.vmediaSessionUnregister(uid, sessionType::KVM, 0x01));
    EXPECT_EQ(mgr.vmediaSessionInfo().size(), 1U);
}

TEST_F(VmediaSessionManagerTest, Unregister_InvalidReason_ReturnsFalse)
{
    mgr.vmediaSessionRegister(0, "192.168.1.30", "vuser", sessionType::VMEDIA,
                              0x4, 1, "remote", "s0");
    uint8_t uid = std::get<0>(mgr.vmediaSessionInfo()[0]);
    EXPECT_FALSE(mgr.vmediaSessionUnregister(uid, sessionType::VMEDIA, 0x99));
    EXPECT_EQ(mgr.vmediaSessionInfo().size(), 1U);
}

TEST_F(VmediaSessionManagerTest, Unregister_SessionNotFound_ReturnsFalse)
{
    mgr.vmediaSessionRegister(0, "192.168.1.30", "vuser", sessionType::VMEDIA,
                              0x4, 1, "remote", "s0");
    EXPECT_FALSE(mgr.vmediaSessionUnregister(99, sessionType::VMEDIA, 0x01));
    EXPECT_EQ(mgr.vmediaSessionInfo().size(), 1U);
}

TEST_F(VmediaSessionManagerTest, Unregister_AllValidReasons_Succeed)
{
    for (const auto& [reason, name] : reasonUnregister)
    {
        mgr.vmediaSessionRegister(0, "10.0.0.1", "user", sessionType::VMEDIA,
                                  0x2, 1, "r", "s");
        uint8_t uid = std::get<0>(mgr.vmediaSessionInfo().back());
        EXPECT_TRUE(
            mgr.vmediaSessionUnregister(uid, sessionType::VMEDIA, reason));
    }
    EXPECT_TRUE(mgr.vmediaSessionInfo().empty());
}

TEST_F(VmediaSessionManagerTest, Unregister_EmptyList_ReturnsFalse)
{
    EXPECT_FALSE(mgr.vmediaSessionUnregister(1, sessionType::VMEDIA, 0x01));
}

TEST_F(VmediaSessionManagerTest, Unregister_FirstOfMultiple_SecondRemains)
{
    mgr.vmediaSessionRegister(0, "10.0.0.1", "u1", sessionType::VMEDIA, 0x2, 1,
                              "r", "s0");
    mgr.vmediaSessionRegister(0, "10.0.0.2", "u2", sessionType::VMEDIA, 0x4, 2,
                              "r", "s1");
    uint8_t firstUid = std::get<0>(mgr.vmediaSessionInfo()[0]);
    EXPECT_TRUE(
        mgr.vmediaSessionUnregister(firstUid, sessionType::VMEDIA, 0x02));
    auto remaining = mgr.vmediaSessionInfo();
    ASSERT_EQ(remaining.size(), 1U);
    EXPECT_EQ(std::get<2>(remaining[0]), "u2");
}

TEST_F(VmediaSessionManagerTest, Unregister_LastOfMultiple_FirstRemains)
{
    mgr.vmediaSessionRegister(0, "10.0.0.1", "u1", sessionType::VMEDIA, 0x2, 1,
                              "r", "s0");
    mgr.vmediaSessionRegister(0, "10.0.0.2", "u2", sessionType::VMEDIA, 0x4, 2,
                              "r", "s1");
    uint8_t lastUid = std::get<0>(mgr.vmediaSessionInfo().back());
    EXPECT_TRUE(
        mgr.vmediaSessionUnregister(lastUid, sessionType::VMEDIA, 0x03));
    auto remaining = mgr.vmediaSessionInfo();
    ASSERT_EQ(remaining.size(), 1U);
    EXPECT_EQ(std::get<2>(remaining[0]), "u1");
}

TEST_F(VmediaSessionManagerTest,
       Unregister_BothWrongTypeAndReason_TypeCheckFires)
{
    // vmediaSessionUnregister checks sessionType before reason
    mgr.vmediaSessionRegister(0, "10.0.0.1", "u1", sessionType::VMEDIA, 0x2, 1,
                              "r", "s");
    uint8_t uid = std::get<0>(mgr.vmediaSessionInfo()[0]);
    EXPECT_FALSE(mgr.vmediaSessionUnregister(uid, sessionType::SSH, 0x99));
    EXPECT_EQ(mgr.vmediaSessionInfo().size(), 1U);
}

TEST_F(VmediaSessionManagerTest, Register_AllTupleFields_StoredCorrectly)
{
    mgr.vmediaSessionRegister(0, "10.1.2.3", "vmuser", sessionType::VMEDIA, 0x3,
                              55, "console", "slot5");
    auto sessions = mgr.vmediaSessionInfo();
    ASSERT_EQ(sessions.size(), 1U);
    EXPECT_EQ(std::get<0>(sessions[0]), 1U);
    EXPECT_EQ(std::get<1>(sessions[0]), "10.1.2.3");
    EXPECT_EQ(std::get<2>(sessions[0]), "vmuser");
    EXPECT_EQ(std::get<3>(sessions[0]),
              static_cast<uint8_t>(sessionType::VMEDIA));
    EXPECT_EQ(std::get<4>(sessions[0]), static_cast<uint8_t>(0x3));
    EXPECT_EQ(std::get<5>(sessions[0]), static_cast<uint8_t>(55));
    EXPECT_EQ(std::get<6>(sessions[0]), "console");
    EXPECT_EQ(std::get<7>(sessions[0]), "slot5");
}
