#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "../Project5/BookingScheduler.cpp"
#include<iostream>
using namespace std;
using namespace testing;

class SundayBookingScheduler : public BookingScheduler
{
public:
	SundayBookingScheduler(int capacityPerHour) :
		BookingScheduler{ capacityPerHour }
	{

	}
	time_t getNow() override
	{
		return getTime(2021, 3, 28, 17, 0);
	}
	time_t getTime(int year, int mon, int day, int hour, int min)
	{
		tm result = { 0,min,hour,day,mon - 1,year - 1900,0,0,-1 };
		return mktime(&result);
	}
};
class MondayBookingScheduler : public BookingScheduler
{
public:
	MondayBookingScheduler(int capacityPerHour) :
		BookingScheduler{ capacityPerHour }
	{

	}
	time_t getNow() override
	{
		return getTime(2024, 6, 3, 17, 0);
	}
	time_t getTime(int year, int mon, int day, int hour, int min)
	{
		tm result = { 0,min,hour,day,mon - 1,year - 1900,0,0,-1 };
		return mktime(&result);
	}
};
class TestableSmsSender : public SmsSender
{
public:
	void send(Schedule* schedule) override
	{
		cout << "테스트용 smssender class의 send 메서드 실행" << endl;
		sendMethodIsCalled = true;
	}
	bool isSendMethodIsCalled()
	{
		return sendMethodIsCalled;
	}
private:
	bool sendMethodIsCalled;

};
class TestableMailSender : public MailSender
{
public:
	void sendMail(Schedule* schedule) override
	{
		countSendMailMethodIsCalled++;
	}
	int getCountSendMailMethodIsCalled()
	{
		return countSendMailMethodIsCalled;
	}
private:
	bool countSendMailMethodIsCalled = 0;

};
class BookingItem : public testing::Test
{
protected:
	void SetUp() override
	{
		NOT_ON_THE_HOUR = getTIme(2021, 3, 26, 9, 5);
		ON_THE_HOUR = getTIme(2021, 3, 26, 9, 0);
		bookingScheduler.setSmsSender(&testableSmsSender);
		bookingScheduler.setMailSender(&testableMailSender);

	}
public:
	tm getTIme(int year, int mon, int day, int hour, int min)
	{
		tm result = { 0,min,hour,day,mon - 1,year - 1900,0,0,-1 };
		mktime(&result);
		return result;
	}
	tm plusHour(tm base, int hour)
	{
		base.tm_hour += hour;
		mktime(&base);
		return base;

	}
	tm NOT_ON_THE_HOUR;
	tm ON_THE_HOUR;
	Customer CUSTOMER{ "Fake name", "010-1234-5678" };
	Customer CUSTOMER_WITH_MAIL{ "Fake name", "010-1234-5678", "test@test.com"};
	const int UNDER_CAPACITY = 1;
	const int CAPACITY_PER_HOUR = 3;
	BookingScheduler bookingScheduler{ CAPACITY_PER_HOUR };
	TestableSmsSender testableSmsSender;
	TestableMailSender testableMailSender;

};
TEST_F(BookingItem, 예약은_정시에만_가능하다_정시가_아닌경우_예약불가)
{
	//arrange
	Schedule* schedule = new Schedule{ NOT_ON_THE_HOUR, UNDER_CAPACITY,  CUSTOMER };

	//act
	EXPECT_THROW({
		bookingScheduler.addSchedule(schedule);
		}, std::runtime_error);
	//assert

}

TEST_F(BookingItem, 예약은_정시에만_가능하다_정시인_경우_예약가능)
{
	//arrange
	Schedule* schedule = new Schedule{ ON_THE_HOUR, UNDER_CAPACITY, CUSTOMER };

	//act
	bookingScheduler.addSchedule(schedule);

	//assert
	EXPECT_EQ(true, bookingScheduler.hasSchedule(schedule));
}

TEST_F(BookingItem, 시간대별_인원제한이_있다_같은_시간대에_Capacity_초과할_경우_예외발생)
{
	//arrange
	Schedule* schedule = new Schedule{ ON_THE_HOUR, CAPACITY_PER_HOUR, CUSTOMER };
	bookingScheduler.addSchedule(schedule);

	try
	{
		Schedule* newSchedule = new Schedule{ ON_THE_HOUR, UNDER_CAPACITY, CUSTOMER };
		bookingScheduler.addSchedule(newSchedule);
		FAIL();
	}
	catch (std::runtime_error& e)
	{
		EXPECT_EQ(string{ e.what() }, string{ "Number of people is over restaurant capacity per hour" });
	}

}

TEST_F(BookingItem, 시간대별_인원제한이_있다_같은_시간대가_다르면_Capacity_차있어도_스케쥴_추가_성공)
{
	Schedule* schedule = new Schedule{ ON_THE_HOUR, CAPACITY_PER_HOUR, CUSTOMER };
	bookingScheduler.addSchedule(schedule);

	tm differentHour = plusHour(ON_THE_HOUR,1);
	mktime(&differentHour);
	Schedule* newSchedule = new Schedule{ differentHour, UNDER_CAPACITY,CUSTOMER };
	bookingScheduler.addSchedule(newSchedule);

	EXPECT_EQ(true, bookingScheduler.hasSchedule(schedule));
	EXPECT_EQ(true, bookingScheduler.hasSchedule(newSchedule));


}

TEST_F(BookingItem, 예약완료시_SMS는_무조건_발송)
{
	//arragne
	Schedule* schedule = new Schedule{ ON_THE_HOUR,CAPACITY_PER_HOUR,CUSTOMER };
	//act
	bookingScheduler.addSchedule(schedule);
	//assert
	EXPECT_EQ(true, testableSmsSender.isSendMethodIsCalled());

}

TEST_F(BookingItem, 이메일이_없는_경우에는_이메일_미발송)
{
	//arragne
	Schedule* schedule = new Schedule{ ON_THE_HOUR,CAPACITY_PER_HOUR,CUSTOMER };
	//act
	bookingScheduler.addSchedule(schedule);
	//assert
	EXPECT_EQ(0, testableMailSender.getCountSendMailMethodIsCalled());

}

TEST_F(BookingItem, 이메일이_있는_경우에는_이메일_발송)
{
	//arragne
	Schedule* schedule = new Schedule{ON_THE_HOUR,CAPACITY_PER_HOUR,CUSTOMER_WITH_MAIL};
	//act
	bookingScheduler.addSchedule(schedule);
	//assert
	EXPECT_EQ(1, testableMailSender.getCountSendMailMethodIsCalled());
}

TEST_F(BookingItem, 현재날짜가_일요일인_경우_예약불가_예외처리)
{
	BookingScheduler* bookingScheduler = new SundayBookingScheduler{ CAPACITY_PER_HOUR };
	try
	{
		Schedule* schedule = new Schedule{ ON_THE_HOUR,CAPACITY_PER_HOUR,CUSTOMER_WITH_MAIL };
		//act
		bookingScheduler->addSchedule(schedule);
		FAIL();
	}
	catch(runtime_error& e)
	{
		EXPECT_EQ(string{ e.what() }, string("Booking system is not available on sunday"));
	}
}

TEST_F(BookingItem, 현재날짜가_일요일이_아닌경우_예약가능)
{
	BookingScheduler* bookingScheduler = new MondayBookingScheduler{ CAPACITY_PER_HOUR };
	Schedule* schedule = new Schedule{ ON_THE_HOUR,CAPACITY_PER_HOUR,CUSTOMER_WITH_MAIL };
	//act
	bookingScheduler->addSchedule(schedule);
	EXPECT_EQ(true, bookingScheduler->hasSchedule(schedule));
}