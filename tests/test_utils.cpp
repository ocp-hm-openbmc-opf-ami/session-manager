#include "utils.hpp"

#include <gtest/gtest.h>

TEST(UtilsTest, SessionTypeEnum_Values_MatchSpec)
{
    EXPECT_EQ(sessionType::KVM, 0);
    EXPECT_EQ(sessionType::WEB, 1);
    EXPECT_EQ(sessionType::VMEDIA, 2);
    EXPECT_EQ(sessionType::SSH, 3);
}

TEST(UtilsTest, ValidPrivMap_ContainsAllFiveLevels)
{
    EXPECT_EQ(validPriv.size(), 5U);
    EXPECT_TRUE(validPriv.count(0x1)); // Callback
    EXPECT_TRUE(validPriv.count(0x2)); // User
    EXPECT_TRUE(validPriv.count(0x3)); // Operator
    EXPECT_TRUE(validPriv.count(0x4)); // Administrator
    EXPECT_TRUE(validPriv.count(0x5)); // OEM Proprietary
    EXPECT_FALSE(validPriv.count(0x0));
    EXPECT_FALSE(validPriv.count(0x6));
}

TEST(UtilsTest, ValidPrivMap_Names_MatchSpec)
{
    EXPECT_EQ(validPriv.at(0x1), "Callback");
    EXPECT_EQ(validPriv.at(0x2), "User");
    EXPECT_EQ(validPriv.at(0x3), "Operator");
    EXPECT_EQ(validPriv.at(0x4), "Administrator");
    EXPECT_EQ(validPriv.at(0x5), "OEM Proprietary");
}

TEST(UtilsTest, ReasonUnregisterMap_ContainsThreeReasons)
{
    EXPECT_EQ(reasonUnregister.size(), 3U);
    EXPECT_TRUE(reasonUnregister.count(0x01)); // Logout
    EXPECT_TRUE(reasonUnregister.count(0x02)); // Expired
    EXPECT_TRUE(reasonUnregister.count(0x03)); // Unknown
    EXPECT_FALSE(reasonUnregister.count(0x00));
    EXPECT_FALSE(reasonUnregister.count(0x04));
}

TEST(UtilsTest, ReasonUnregisterMap_Names_MatchSpec)
{
    EXPECT_EQ(reasonUnregister.at(0x01), "Logout");
    EXPECT_EQ(reasonUnregister.at(0x02), "Expired");
    EXPECT_EQ(reasonUnregister.at(0x03), "Unknown");
}

TEST(UtilsTest, SessionMgrObj_Path_MatchesSpec)
{
    EXPECT_STREQ(sessionMgrObj, "/xyz/openbmc_project/SessionManager");
}

TEST(UtilsTest, SessionDbusName_MatchesSpec)
{
    EXPECT_STREQ(sessionDbusNmae, "xyz.openbmc_project.SessionManager");
}
