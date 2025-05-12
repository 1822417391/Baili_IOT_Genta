#include "RTCController.h"



// void RTCSetClock(char year, char month, char day, char hour, char minute, char second, char minisecond) {
//     BLYEAR = year; //Y:2021
//     BLMONTH = month; //M:12
//     BLDAY = day; //D:31
//     BLHOUR = hour; //H:23
//     BLMIN = minute; //M:59
//     BLSEC = second; //S:50
//     BLMINISEC = minisecond;
// }

// 判断是否为闰年
char is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// 获取某个月的天数
int days_in_month(int year, int month) {
    extern char xdata DAYS[12];
    if (month == 2 && is_leap_year(year)) {
        return 29;
    }
    return DAYS[month - 1];
}

// 初始化时间
void rtc_init(DateTime *dt, int year, int month, int day, int hour, int minute, int second) {
    dt->year = year;
    dt->month = month;
    dt->day = day;
    dt->hour = hour;
    dt->minute = minute;
    dt->second = second;
}

void rtc_init_str(DateTime *dt, char *dat, char offset) {
    dt->year = dat[offset] & 0x00ff;
    dt->month = dat[offset + 1] & 0x00ff;
    dt->day = dat[offset + 2] & 0x00ff;
    dt->hour = dat[offset + 3] & 0x00ff;
    dt->minute = dat[offset + 4] & 0x00ff;
    dt->second = dat[offset + 5] & 0x00ff;
}

// 规范化时间（处理溢出）
void normalize_datetime(DateTime *dt) {
    // 处理秒溢出
    while (dt->second >= 60) {
        dt->second -= 60;
        dt->minute++;
    }
    while (dt->second < 0) {
        dt->second += 60;
        dt->minute--;
    }
    
    // 处理分钟溢出
    while (dt->minute >= 60) {
        dt->minute -= 60;
        dt->hour++;
    }
    while (dt->minute < 0) {
        dt->minute += 60;
        dt->hour--;
    }
    
    // 处理小时溢出
    while (dt->hour >= 24) {
        dt->hour -= 24;
        dt->day++;
    }
    while (dt->hour < 0) {
        dt->hour += 24;
        dt->day--;
    }
    
    // 处理日溢出
    while (dt->day > days_in_month(dt->year, dt->month)) {
        dt->day -= days_in_month(dt->year, dt->month);
        dt->month++;
        if (dt->month > 12) {
            dt->month = 1;
            dt->year++;
        }
    }
    while (dt->day < 1) {
        dt->month--;
        if (dt->month < 1) {
            dt->month = 12;
            dt->year--;
        }
        dt->day += days_in_month(dt->year, dt->month);
    }
    
    // 处理月溢出
    while (dt->month > 12) {
        dt->month -= 12;
        dt->year++;
    }
    while (dt->month < 1) {
        dt->month += 12;
        dt->year--;
    }
}

// 添加秒
void rtc_add_seconds(DateTime *dt, int seconds) {
    dt->second += seconds;
    normalize_datetime(dt);
}

// long days_since_epoch(const DateTime *dt) {
//     int a,b;
//     int year = dt->year;
//     int month = dt->month;
    
//     if (month <= 2) {
//         year--;
//         month += 12;
//     }
    
//     a = year / 100;
//     b = 2 - a + a / 4;
    
//     return (long)(365.25 * year) + (long)(30.6001 * (month + 1)) + 
//            dt->day + b - 719562;
// }


// // DateTime转Unix时间戳（毫秒）
// long rtc_to_unixtime_ms(const DateTime *dt) {
//     long xdata days, hours, minutes, seconds, milliseconds;
//     days = days_since_epoch(dt);
//     hours = days * 24 + dt->hour;
//     minutes = hours * 60 + dt->minute;
//     seconds = minutes * 60 + dt->second;
//     milliseconds = seconds * 1000 ;
    
//     return milliseconds;
// }

// // Unix时间戳（毫秒）转DateTime
// void rtc_from_unixtime_ms(DateTime *dt, long unix_time_ms) {
//     long xdata seconds_total,days_total,z,era, doe, yoe, y, doy, mp;
//     // 计算毫秒部分
//     //dt->millisecond = unix_time_ms % 1000;
//      seconds_total = unix_time_ms / 1000;
    
//     // 计算时分秒
//     dt->second = seconds_total % 60;
//     seconds_total /= 60;
    
//     dt->minute = seconds_total % 60;
//     seconds_total /= 60;
    
//     dt->hour = seconds_total % 24;
//      days_total = seconds_total / 24;
    
//     // 计算年月日（使用Zeller公式的逆运算）
//     z = days_total + 719468;
//     era = (z >= 0 ? z : z - 146096) / 146097;
//     doe = z - era * 146097;
//     yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
//     y = yoe + era * 400;
//     doy = doe - (365*yoe + yoe/4 - yoe/100);
//     mp = (5*doy + 2)/153;
//     dt->day = doy - (153*mp+2)/5 + 1;
//     dt->month = mp + (mp < 10 ? 3 : -9);
//     dt->year = y + (dt->month <= 2 ? 1 : 0);
    
//     // 处理2月29日的情况
//     if (dt->month == 2 && dt->day == 29 && !is_leap_year(dt->year)) {
//         dt->day = 28;
//     }
// }