// #include <stc8h.h>

// 定义日期时间结构体
typedef struct {
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
} DateTime;

// 判断是否为闰年
char is_leap_year(int year);

// 获取某个月的天数
int days_in_month(int year, int month);

// 初始化时间
void rtc_init(DateTime *dt, int year, int month, int day, 
              int hour, int minute, int second);

// 初始化时间
void rtc_init_str(DateTime *dt, char *dat, char offset);

// 添加秒
void rtc_add_seconds(DateTime *dt, int seconds);


void normalize_datetime(DateTime *dt);

long rtc_to_unixtime_ms(const DateTime *dt);

void rtc_from_unixtime_ms(DateTime *dt, long unix_time_ms);