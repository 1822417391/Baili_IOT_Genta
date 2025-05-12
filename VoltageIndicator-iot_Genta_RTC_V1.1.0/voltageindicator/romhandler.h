
#include <stc8h.h>
#include <intrins.h>

//#define IapSector1Addr                  0x0000
#define IapWakeupInterval2bAddr             0x0C00
#define IapSleepTimeout2bAddr               0x0C02

#define IapWorkEnable2bAddr                 0x0E00
// #define IapWorkIntervalType2bAddr           0x0202  //0：每天，1：每周，2：每月，3：固定间隔，4：按时间执行
#define IapStartDateTime4bAddr              0x0E02
// #define IapStartDuration2bAddr              0x0210
#define IapWorkDuration2bAddr               0x0E06

/*(Genta New) 唤醒类别定义 */
#define WAKE_INTERVAL     0     // 间隔唤醒(现有功能)
#define WAKE_HOURLY       1     // 整点唤醒(新增)
#define WAKE_HALF_HOUR    2     // 半点唤醒(新增)
#define WAKE_QUARTER_HOUR 3     // 每15分钟唤醒(新增)

void InitROM();
void IapIdle();

void IapReadWithSize(int, int, char *);

void IapMultiWrite(int, char *, char, char);
void IapErase(int);
void IapGet2byteData(int, int *);

//void setWakeupInterval(int);
// int getWakeupInterval();

// int getSleepTimeout();

// //void setStartTime(int);
// int getGateStartTime();

// //void setStopTime(int);
// int getGateStopTime();

// //void setWorkEnable(char);
// int getGateWorkEnable();

void setWakeupSleepOptions(char *, char);
void setWorkingOptions(char *, char);
//char[] getAllOptions();

char GateStartChecker(long, long, int, char, char, int);

void longToByteArray(long, char *, char);
long byteArrayToLong(char *, char, long *);
char checkWakeupTime(long currentTime);
