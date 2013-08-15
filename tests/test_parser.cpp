#include "gtest/gtest.h"
#include <mongo/client/dbclient.h>
#include <mongo/bson/bsonobjbuilder.h>
#include "robomongo/core/utils/BsonUtils.h"
#include "robomongo/shell/db/ptimeutil.h"

using namespace Robomongo;

namespace
{
    void expectIsoDate(const std::string &expectedIsoDate, long long actualMilliseconds)
    {
        boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
        boost::posix_time::ptime actualTime = epoch + boost::posix_time::millisec(actualMilliseconds);

        std::string actualIsoDate = miutil::isotimeString(actualTime, true, false);
        EXPECT_EQ(expectedIsoDate, actualIsoDate);
    }

    void expectMilliseconds(long long expectedMilliseconds, const std::string &actualIsoDate)
    {
        boost::posix_time::ptime actualTime = miutil::ptimeFromIsoString(actualIsoDate);

        boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
        boost::posix_time::ptime expectedTime = epoch + boost::posix_time::millisec(expectedMilliseconds);
        EXPECT_EQ(expectedTime, actualTime);
    }

    void expectEqualIsoDate(const std::string &firstIsoDate, const std::string &secondIsoDate)
    {
        boost::posix_time::ptime first = miutil::ptimeFromIsoString(firstIsoDate);
        boost::posix_time::ptime second = miutil::ptimeFromIsoString(secondIsoDate);
        EXPECT_EQ(first, second);
    }

    void testDateParsing(long long milliseconds, const std::string &isoDate)
    {
        expectIsoDate(isoDate, milliseconds);
        expectMilliseconds(milliseconds, isoDate);
    }
}

TEST(JsonString, BSONObjConversion) 
{
    mongo::BSONObj obj;
    std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1, DefaultEncoding,Utc);
    EXPECT_EQ("{}", str);
}

TEST(JsonString, DateConversion)
{
    mongo::BSONObjBuilder toCheck;
    toCheck.append("Date",mongo::Date_t(0));
    mongo::BSONObj obj = toCheck.obj();
    std::string str = BsonUtils::jsonString(obj, mongo::TenGen, 1, DefaultEncoding,LocalTime);
    EXPECT_EQ("{\n    \"Date\" : ISODate(\"1970-01-01T00:00:00.000Z\")\n}", str);
}

TEST(DateTests, DateConversionEpoch)
{
    expectMilliseconds(0, "1970-01-01T00:00:00.000Z");
    expectMilliseconds(0, "1970-01-01T00:00:00.000+00:00");
    expectMilliseconds(0, "1970-01-01T03:00:00.000+03:00");
    expectMilliseconds(0, "1969-12-31T21:00:00.000-03:00");
}

TEST(DateTests, DateConversionOne)
{
    testDateParsing(-2177452800000, "1901-01-01T00:00:00.000Z");
}

TEST(DateTests, DateZero)
{
    testDateParsing(-2208988800000, "1900-01-01T00:00:00.000Z");
}

TEST(DateTests, DateConversionTwo)
{
    testDateParsing(6977452800000, "2191-02-08T13:20:00.000Z");
}

TEST(DateTests, DateMilliseconds)
{
    testDateParsing(978307200127, "2001-01-01T00:00:00.127Z");
}

TEST(DateTests, DateConversionThree)
{
    testDateParsing(1312291712320, "2011-08-02T13:28:32.320Z");
}

TEST(DateTests, DateTimezone)
{
    expectMilliseconds(1376347174411, "2013-08-13T01:39:34.411+03:00");
    expectMilliseconds(1376407774411, "2013-08-13T01:39:34.411-13:50");
}

TEST(DateTests, DateRandomTest)
{
    expectEqualIsoDate("2013-08-12T22:39:34.411Z", "2013-08-13T01:39:34.411+03:00");
    expectEqualIsoDate("2013-08-13T09:09:34.001Z", "2013-08-13T01:39:34.001-07:30");
}

/**
 * Brute-force test that takes some time to complete (~30 seconds).
 * We are moving by 16 minutes from "1900-01-01T00:00:00.000Z"
 * to "2191-02-08T13:20:00.000Z" and rendering ISO date string.
 * Then we are trying to parse this date string back to time.
 */
TEST(DateTests, LongBruteForceTest)
{
    const long long from = -2208988800000; // "1900-01-01T00:00:00.000Z"
    const long long to   = 6977452800000;  // "2191-02-08T13:20:00.000Z"
    const long long step = 60000 * 16;     // 16 minutes

    boost::posix_time::ptime epoch(boost::gregorian::date(1970, 1, 1));
    boost::posix_time::ptime temp = epoch + boost::posix_time::millisec(from);

    for (long long i = from + step; i <= to; i+= step) {
        boost::posix_time::ptime expectedTime = epoch + boost::posix_time::millisec(i);

        // The following 4 lines are used to protect us from
        // possible integer value overflow. We are ensuring,
        // that we are monotonically growing by specified number
        // of minutes (16 minutes by default)
        boost::posix_time::time_duration duration(expectedTime - temp);
        ASSERT_EQ(16, duration.minutes());
        ASSERT_GT(expectedTime, temp);
        temp = expectedTime;

        std::string actualIsoDate = miutil::isotimeString(expectedTime, true, false);
        boost::posix_time::ptime actualTime = miutil::ptimeFromIsoString(actualIsoDate);
        ASSERT_EQ(expectedTime, actualTime);
    }
}
