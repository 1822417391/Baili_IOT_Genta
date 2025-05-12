#include "romhandler.h"

extern char xdata WAKE_MODE;  // 当前唤醒模式
extern char xdata LAST_WAKE_MIN;         // 上次唤醒的分钟数

void InitROM() {
    IAP_CONTR = 0x80; //使能 IAP
    IAP_TPS = 11; //设置等待参数 12MHz
}

void IapIdle() {
    IAP_CONTR = 0; //关闭 IAP 功能
    IAP_CMD = 0; //清除命令寄存器
    IAP_TRIG = 0; //清除触发寄存器
    IAP_ADDRH = 0x80; //将地址设置到非 IAP 区域
    IAP_ADDRL = 0;
}



void IapReadWithSize(int addr, int size, char* res) {
    int tmpAddr;
    int i;

    InitROM();
    i = 0;
    while(i < size) {
        tmpAddr = addr + i;
        IAP_CMD = 1;
        IAP_ADDRL = tmpAddr; //设置 IAP 低地址
        IAP_ADDRH = tmpAddr >> 8; //设置 IAP 高地址
        IAP_TRIG = 0x5a; //写触发命令(0x5a)
        IAP_TRIG = 0xa5; //写触发命令(0xa5)
        _nop_();
        _nop_();
        res[i] =  IAP_DATA;
        i++;
    }
    IapIdle(); //关闭 IAP 功能
}



void IapMultiWrite(int addr, char *dat, char offset, char size) {
    char i;
    int tmpAddr;

    InitROM();
    i = 0;
    while(i < size) {
        tmpAddr = addr + i;
        IAP_CMD = 2; //设置 IAP 写命令
        IAP_ADDRL = tmpAddr; //设置 IAP 低地址
        IAP_ADDRH = tmpAddr >> 8; //设置 IAP 高地址
        IAP_DATA = dat[i + offset]; //写 IAP 数据
        IAP_TRIG = 0x5a; //写触发命令(0x5a)
        IAP_TRIG = 0xa5; //写触发命令(0xa5)
        _nop_();
        _nop_();
        i++;
    }
    IapIdle(); //关闭 IAP 功能
}

void IapErase(int addr) {
    InitROM();
    IAP_CMD =3; //设置 IAP 擦除命令
    IAP_ADDRL = addr; //设置 IAP 低地址
    IAP_ADDRH = addr >> 8; //设置 IAP 高地址
    IAP_TRIG = 0x5a;
    //写触发命令(0x5a)
    IAP_TRIG = 0xa5; //写触发命令(0xa5)
    _nop_(); //
    IapIdle(); //关闭 IAP 功能
}

/**
 * @brief 从ROM读取2字节数据并转换为int类型
 * @param addr 要读取的起始地址
 * @param res 存储结果的指针
 */
void IapGet2byteData(int addr, int *res) {
    char dat[2];

    // 读取2字节数据
    IapReadWithSize(addr, 2, dat);

    // 将2字节数据组合为int
    *res = dat[0] & 0x00ff;
    *res = *res << 8;
    *res = *res + (dat[1] & 0x00ff);
}


/**
 * Genta_New
 * @brief 设置唤醒和休眠选项（四功能）
 * @param dat 包含配置数据的数组
 * @param offset 数据在数组中的偏移量
 * @note 数据格式:
 *       byte[offset]: 唤醒模式(0=间隔,1=整点,2=半点,3=15分钟)
 *       byte[offset+1-2]: 间隔唤醒时间(仅模式0有效)
 */
void setWakeupSleepOptions(char *dat, char offset) {
    // 保存新唤醒模式
    WAKE_MODE = dat[offset];

    // 擦除原有数据
    IapErase(IapWakeupInterval2bAddr);

    // 只对间隔唤醒模式保存时间参数
    if(WAKE_MODE == WAKE_INTERVAL) {
        IapMultiWrite(IapWakeupInterval2bAddr, dat, offset, 3); // 模式+时间
    } else {
        // 其他模式只保存模式字节
        IapMultiWrite(IapWakeupInterval2bAddr, dat, offset, 1);
    }

    // 重置上次唤醒时间
    LAST_WAKE_MIN = -1;
}

/**
 * @brief 设置工作选项
 * @param dat 包含配置数据的数组
 * @param offset 数据在数组中的偏移量
 */
void setWorkingOptions(char *dat, char offset) {
    // 先擦除原有数据
    IapErase(IapWorkEnable2bAddr);
    // 写入新数据(8字节)
    IapMultiWrite(IapWorkEnable2bAddr, dat, offset, 8);
}


/**
 * @brief 检查水阀启动条件
 * @param now 当前时间(时间戳)
 * @param startTime 计划启动时间
 * @param duration 工作持续时间(秒)
 * @param workEnable 工作使能标志
 * @param gateStatus 当前水阀状态
 * @param countdown 工作倒计时
 * @return 检查结果:
 *         0:待机
 *         1:需要启动
 *         2:工作中
 *         3:需要停止
 */
char GateStartChecker(long now, long startTime, int duration, char workEnable, char gateStatus, int countdown) {
    char res = 0;

    // 如果工作未使能
    if(workEnable == 0) {
        if(gateStatus == 1) {
            return 3; // 如果水阀是开的，则需要关闭
        }
        return 0; // 否则保持待机
    }

    // 如果水阀正在工作
    if(gateStatus == 1) {
        if(countdown <= 0) {
            return 3; // 倒计时结束则停止
        }
        return 2; // 否则保持工作状态
    }

    // 如果未到启动时间
    if(now < startTime) {
        if(gateStatus == 1) {
            return 3; // 如果水阀是开的，则需要关闭
        }
        return 0; // 否则保持待机
    }

    // 如果到达启动时间窗口(60秒内)且水阀关闭
    if(now > startTime && (now - startTime) < 60 && gateStatus == 0) {
        return 1; // 需要启动水阀
    }

    return res;
}



/**
 * @brief 将长整型数据转换为字节数组
 * @param value 要转换的长整型值
 * @param buffer 存储结果的缓冲区
 * @param offset 缓冲区中的起始偏移
 */
void longToByteArray(long value, char *buffer, char offset) {
    buffer[offset + 0] = (unsigned char)(value >> 24);  // 最高字节
    buffer[offset + 1] = (unsigned char)(value >> 16);
    buffer[offset + 2] = (unsigned char)(value >> 8);
    buffer[offset + 3] = (unsigned char)(value);        // 最低字节
}

/**
 * @brief 将字节数组转换为长整型数据
 * @param buf 源字节数组
 * @param offset 数组中的起始偏移
 * @param res 存储结果的指针
 * @return 转换后的长整型值
 */
long byteArrayToLong(char *buf, char offset, long *res) {
    long tmp = 0;

    // 将4字节数据组合为long
    tmp = buf[offset + 0] & 0xff;
    tmp <<= 8;
    tmp += buf[offset + 1] & 0xff;
    tmp <<= 8;
    tmp += buf[offset + 2] & 0xff;
    tmp <<= 8;
    tmp += buf[offset + 3] & 0xff;

    *res = tmp; // 存储结果
    return tmp;
}






// int getWakeupInterval() {
//     int res;
//     char dat[2];

//     IapReadWithSize(IapWakeupInterval2bAddr, 2, dat);
//     res = dat[0];
//     res = res << 8;
//     res = res + dat[1];

//     return res;
// }

// int getSleepTimeout() {
//     int res;
//     char dat[2];

//     IapReadWithSize(IapSleepTimeout2bAddr, 2, dat);
//     res = dat[0];
//     res = res << 8;
//     res = res + dat[1];

//     return res;
// }

// int getGateStartTime() {
//     int res;
//     char dat[2];

//     IapReadWithSize(IapStartTime2bAddr, 2, dat);
//     res = dat[0];
//     res = res << 8;
//     res = res + dat[1];

//     return res;
// }

// int getGateStopTime() {
//     int res;
//     char dat[2];

//     IapReadWithSize(IapStopTime2bAddr, 2, dat);
//     res = dat[0];
//     res = res << 8;
//     res = res + dat[1];

//     return res;
// }

// int getGateWorkEnable() {
//     int res;
//     char dat[2];

//     IapReadWithSize(IapWorkEnable1bAddr, 2, dat);
//     res = dat[0];
//     res = res << 8;
//     res = res + dat[1];

//     return res;
// }

// void setAllOptions(char *dat, int size) {
//     IapErase(IapSector1Addr);

//     IapMultiWrite(IapSector1Addr, dat, size);
// }
