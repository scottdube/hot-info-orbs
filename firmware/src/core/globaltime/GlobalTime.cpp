#include "GlobalTime.h"
#include "TimeZones.h"

#include "config_helper.h"
#include <TimeLib.h>

GlobalTime *GlobalTime::m_instance = nullptr;

GlobalTime::GlobalTime() {
    m_timeClient = new NTPClient(m_udp);
    m_timeClient->begin();
    m_timeClient->setPoolServerName(NTP_SERVER);
    // The zone rule is applied once: the settings page restarts the orb on save.
    // NTP stays at offset 0 (UTC); the C library turns UTC into local time from
    // the rule, daylight saving included, with no web service and no key.
    const std::string &zone = Settings::get().tz;
    if (tzdb::find(zone) == nullptr) {
        Serial.printf("Time zone \"%s\" is not in the table (TimeZones.h); showing UTC\n", zone.c_str());
    }
    setenv("TZ", tzdb::ruleFor(zone), 1);
    tzset();
}

GlobalTime::~GlobalTime() {
    delete m_timeClient;
}

GlobalTime *GlobalTime::getInstance() {
    if (m_instance == nullptr) {
        m_instance = new GlobalTime();
    }
    return m_instance;
}

void GlobalTime::updateTime() {
    if (millis() - m_updateTimer > m_oneSecond) {
        m_timeClient->update();
        const time_t utc = m_timeClient->getEpochTime();
        m_timeZoneOffset = tzdb::offsetAt(utc);
        // Local time kept as an epoch: the widgets read it through TimeLib's
        // hour()/day(), which do no zone math of their own.
        m_unixEpoch = utc + m_timeZoneOffset;
        m_updateTimer = millis();
        m_minute = minute(m_unixEpoch);
        if (m_format24hour) {
            m_hour = hour(m_unixEpoch);
        } else {
            m_hour = hourFormat12(m_unixEpoch);
        }
        m_hour24 = hour(m_unixEpoch);
        m_second = second(m_unixEpoch);

        m_day = day(m_unixEpoch);
        m_month = month(m_unixEpoch);
        m_monthName = LOC_MONTH[m_month - 1];
        m_year = year(m_unixEpoch);
        m_weekday = LOC_WEEKDAY[(weekday(m_unixEpoch)) -1];
        m_time = String(m_hour) + ":" + (m_minute < 10 ? "0" + String(m_minute) : String(m_minute));
    }
}

void GlobalTime::getHourAndMinute(int &hour, int &minute) {
    hour = m_hour;
    minute = m_minute;
}

int GlobalTime::getHour() {
    return m_hour;
}

int GlobalTime::getHour24() {
    return m_hour24;
}

// Two digits in 24-hour mode only ("09:05"); 12-hour mode gives "9:05", the
// way ClockWidget blanks its first digit. Used by the weather and Parqet clocks.
String GlobalTime::getHourPadded() {
    if (m_format24hour && m_hour < 10) {
        return "0" + String(m_hour);
    } else {
        return String(m_hour);
    }
}

int GlobalTime::getMinute() {
    return m_minute;
}

String GlobalTime::getMinutePadded() {
    if (m_minute < 10) {
        return "0" + String(m_minute);
    } else {
        return String(m_minute);
    }
}

int GlobalTime::getSecond() {
    return m_second;
}

time_t GlobalTime::getUnixEpoch() {
    return m_unixEpoch;
}

int GlobalTime::getDay() {
    return m_day;
}

int GlobalTime::getMonth() {
    return m_month;
}

String GlobalTime::getMonthName() {
    return m_monthName;
}

int GlobalTime::getYear() {
    return m_year;
}

String GlobalTime::getTime() {
    return m_time;
}

String GlobalTime::getWeekday() {
    return m_weekday;
}

String GlobalTime::getDayAndMonth() {
    if (Settings::get().wxmetric) {
        String retVal = LOC_FORMAT_DAYMONTH;
        retVal.replace("%d", String(m_day));
        retVal.replace("%B", m_monthName);
        return retVal;
    }
    return m_monthName + " " + String(m_day);
}

bool GlobalTime::isPM() {
    return hour(m_unixEpoch) >= 12;
}

int GlobalTime::getTimeZoneOffset() {
    return m_timeZoneOffset;
}

bool GlobalTime::getFormat24Hour() {
    return m_format24hour;
}

bool GlobalTime::setFormat24Hour(bool format24hour) {
    m_format24hour = format24hour;
    return m_format24hour;
}