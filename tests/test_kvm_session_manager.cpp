#include "kvm-session-manager.hpp"

#include <sdbusplus/test/sdbus_mock.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

class KvmSessionManagerTest : public ::testing::Test
{
  protected:
    testing::NiceMock<sdbusplus::SdBusMock> sdbusMock;
    sdbusplus::bus_t bus{sdbusplus::get_mocked_new(&sdbusMock)};
    KvmSessionManager mgr{bus, kvmsessionMgrObj};
};

TEST_F(KvmSessionManagerTest, Register_ValidParams_ReturnsTrueAndStoresSession)
{
    EXPECT_TRUE(mgr.kvmSessionRegister(0, "192.168.1.10", "admin",
                                       sessionType::KVM, 0x4, 1));
    auto sessions = mgr.kvmSessionInfo();
    ASSERT_EQ(sessions.size(), 1U);
    EXPECT_EQ(std::get<1>(sessions[0]), "192.168.1.10");
    EXPECT_EQ(std::get<2>(sessions[0]), "admin");
    EXPECT_EQ(std::get<3>(sessions[0]), static_cast<uint8_t>(sessionType::KVM));
}

TEST_F(KvmSessionManagerTest, Register_SessionIdNotZero_ReturnsFalse)
{
    EXPECT_FALSE(mgr.kvmSessionRegister(1, "192.168.1.10", "admin",
                                        sessionType::KVM, 0x4, 1));
    EXPECT_TRUE(mgr.kvmSessionInfo().empty());
}

TEST_F(KvmSessionManagerTest, Register_WrongSessionType_ReturnsFalse)
{
    EXPECT_FALSE(mgr.kvmSessionRegister(0, "192.168.1.10", "admin",
                                        sessionType::SSH, 0x4, 1));
    EXPECT_TRUE(mgr.kvmSessionInfo().empty());
}

TEST_F(KvmSessionManagerTest, Register_InvalidPrivilege_ReturnsFalse)
{
    EXPECT_FALSE(mgr.kvmSessionRegister(0, "192.168.1.10", "admin",
                                        sessionType::KVM, 0x00, 1));
    EXPECT_TRUE(mgr.kvmSessionInfo().empty());
}

TEST_F(KvmSessionManagerTest, Register_MultipleSessionsUidIncrementsCorrectly)
{
    mgr.kvmSessionRegister(0, "10.0.0.1", "u1", sessionType::KVM, 0x2, 1);
    mgr.kvmSessionRegister(0, "10.0.0.2", "u2", sessionType::KVM, 0x4, 2);
    auto sessions = mgr.kvmSessionInfo();
    ASSERT_EQ(sessions.size(), 2U);
    EXPECT_EQ(std::get<0>(sessions[0]), 1U);
    EXPECT_EQ(std::get<0>(sessions[1]), 2U);
}

TEST_F(KvmSessionManagerTest, Unregister_ValidParams_ReturnsTrueAndRemoves)
{
    mgr.kvmSessionRegister(0, "192.168.1.10", "admin", sessionType::KVM, 0x4,
                           1);
    uint8_t uid = std::get<0>(mgr.kvmSessionInfo()[0]);
    EXPECT_TRUE(mgr.kvmSessionUnregister(uid, sessionType::KVM, 0x01));
    EXPECT_TRUE(mgr.kvmSessionInfo().empty());
}

TEST_F(KvmSessionManagerTest, Unregister_InvalidReason_ReturnsFalse)
{
    mgr.kvmSessionRegister(0, "192.168.1.10", "admin", sessionType::KVM, 0x4,
                           1);
    uint8_t uid = std::get<0>(mgr.kvmSessionInfo()[0]);
    EXPECT_FALSE(mgr.kvmSessionUnregister(uid, sessionType::KVM, 0x00));
    EXPECT_EQ(mgr.kvmSessionInfo().size(), 1U);
}

TEST_F(KvmSessionManagerTest, Unregister_WrongSessionType_ReturnsFalse)
{
    mgr.kvmSessionRegister(0, "192.168.1.10", "admin", sessionType::KVM, 0x4,
                           1);
    uint8_t uid = std::get<0>(mgr.kvmSessionInfo()[0]);
    EXPECT_FALSE(mgr.kvmSessionUnregister(uid, sessionType::WEB, 0x01));
    EXPECT_EQ(mgr.kvmSessionInfo().size(), 1U);
}

TEST_F(KvmSessionManagerTest, Unregister_SessionNotFound_ReturnsFalse)
{
    mgr.kvmSessionRegister(0, "192.168.1.10", "admin", sessionType::KVM, 0x4,
                           1);
    EXPECT_FALSE(mgr.kvmSessionUnregister(99, sessionType::KVM, 0x01));
    EXPECT_EQ(mgr.kvmSessionInfo().size(), 1U);
}

TEST_F(KvmSessionManagerTest, ClearAll_NoSessions_ReturnsFalse)
{
    EXPECT_FALSE(mgr.clearAll());
}

TEST_F(KvmSessionManagerTest, ClearAll_WithSessions_ReturnsTrueAndClearsAll)
{
    mgr.kvmSessionRegister(0, "10.0.0.1", "u1", sessionType::KVM, 0x2, 1);
    mgr.kvmSessionRegister(0, "10.0.0.2", "u2", sessionType::KVM, 0x4, 2);
    EXPECT_EQ(mgr.kvmSessionInfo().size(), 2U);
    EXPECT_TRUE(mgr.clearAll());
    EXPECT_TRUE(mgr.kvmSessionInfo().empty());
}

TEST_F(KvmSessionManagerTest, ClearAll_CalledTwice_SecondReturnsFalse)
{
    mgr.kvmSessionRegister(0, "10.0.0.1", "u1", sessionType::KVM, 0x2, 1);
    EXPECT_TRUE(mgr.clearAll());
    EXPECT_FALSE(mgr.clearAll());
}

TEST_F(KvmSessionManagerTest, Unregister_EmptyList_ReturnsFalse)
{
    EXPECT_FALSE(mgr.kvmSessionUnregister(1, sessionType::KVM, 0x01));
}

TEST_F(KvmSessionManagerTest, Unregister_FirstOfMultiple_SecondRemains)
{
    mgr.kvmSessionRegister(0, "10.0.0.1", "u1", sessionType::KVM, 0x2, 1);
    mgr.kvmSessionRegister(0, "10.0.0.2", "u2", sessionType::KVM, 0x4, 2);
    uint8_t firstUid = std::get<0>(mgr.kvmSessionInfo()[0]);
    EXPECT_TRUE(mgr.kvmSessionUnregister(firstUid, sessionType::KVM, 0x02));
    auto remaining = mgr.kvmSessionInfo();
    ASSERT_EQ(remaining.size(), 1U);
    EXPECT_EQ(std::get<2>(remaining[0]), "u2");
}

TEST_F(KvmSessionManagerTest, Unregister_LastOfMultiple_FirstRemains)
{
    mgr.kvmSessionRegister(0, "10.0.0.1", "u1", sessionType::KVM, 0x2, 1);
    mgr.kvmSessionRegister(0, "10.0.0.2", "u2", sessionType::KVM, 0x4, 2);
    uint8_t lastUid = std::get<0>(mgr.kvmSessionInfo().back());
    EXPECT_TRUE(mgr.kvmSessionUnregister(lastUid, sessionType::KVM, 0x03));
    auto remaining = mgr.kvmSessionInfo();
    ASSERT_EQ(remaining.size(), 1U);
    EXPECT_EQ(std::get<2>(remaining[0]), "u1");
}

TEST_F(KvmSessionManagerTest, ClearAll_ThenRegister_SessionAdded)
{
    mgr.kvmSessionRegister(0, "10.0.0.1", "u1", sessionType::KVM, 0x2, 1);
    mgr.clearAll();
    EXPECT_TRUE(
        mgr.kvmSessionRegister(0, "10.0.0.2", "u2", sessionType::KVM, 0x4, 2));
    EXPECT_EQ(mgr.kvmSessionInfo().size(), 1U);
    EXPECT_EQ(std::get<2>(mgr.kvmSessionInfo()[0]), "u2");
}

TEST_F(KvmSessionManagerTest, Register_AllTupleFields_StoredCorrectly)
{
    mgr.kvmSessionRegister(0, "172.16.0.1", "kvmuser", sessionType::KVM, 0x4,
                           7);
    auto sessions = mgr.kvmSessionInfo();
    ASSERT_EQ(sessions.size(), 1U);
    EXPECT_EQ(std::get<0>(sessions[0]), 1U);
    EXPECT_EQ(std::get<1>(sessions[0]), "172.16.0.1");
    EXPECT_EQ(std::get<2>(sessions[0]), "kvmuser");
    EXPECT_EQ(std::get<3>(sessions[0]), static_cast<uint8_t>(sessionType::KVM));
    EXPECT_EQ(std::get<4>(sessions[0]), static_cast<uint8_t>(0x4));
    EXPECT_EQ(std::get<5>(sessions[0]), static_cast<uint8_t>(7));
}
