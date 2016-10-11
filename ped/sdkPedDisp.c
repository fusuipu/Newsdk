#include "sdkGlobal.h"


// 上下位机通讯定义
#define     PED_SOH                         0x01
#define     PED_ETX                          0x03
#define     PED_EOT                          0x04
#define     PED_TEXT                        0x10                // 正文
#define     PED_ORDER                     0x11               // 命令
#define     PED_KEY_TEXT                0X12               // 键盘的正文
#define     PED_KEY_ORDER            0X13               // 键盘的命令
#define     PED_EXT_TEXT                0x14               // 报文超过248 的时候的协议
#define     PED_KEY_EXTEXT           0x15

#define STEP_SOH                    0               // 接收命令头/标识
#define STEP_ORDER               1            // 接收命令字
#define STEP_LEN1                  2               // 接收长度第一位
#define STEP_LEN2                  3         // 接收长度第二位
#define STEP_DATA                  4               // 接收数据
#define STEP_ETX                     5               // 接收结束符
#define STEP_CS                       6               // 接收校验符

#define STATE_NULL            0       // 没接收到数据
#define STATE_OK                1       // 接收到正确的数据
#define STATE_CSERR          2       // 接收数据校验错

typedef struct
{
    u8 Step;                  // 接收步数
    u8 HeadLen;           // 接收0x01长度
    u8 Text;                // 贴标识
    u8 Comm;               // 接收到的命令
    u8 Ret;                    // 数据状态 0:正常等待 1:接收到正确的数据 2:接收数据错误
    u8 Cs;                      // 校验和
    u8 Buf[512];            // 数据
    u16 Len;                 // 数据长度
    u16 LenCun;           // 数据接收到的长度
} UartData;

typedef struct
{
    UartData *RecvUartData;           // 接收串口数据结构体
    u8 Cmd;                                             // 发送的命令
    u8 const *pDataIn;                                  // 发送的数据
    u16 Len;                                            // 发送数据的长度
} UartSendCfg;

#define         PINPAD_ZIKU_FILE1 "/res/hz1616.bin"
#define         PINPAD_ZIKU_FILE2 "mtd0/res/hz1616.bin"


#define PEDFDISP    0x00   //正显(默认正显)
#define PEDNOFDISP  0x01   //反显
#define PEDINCOL    0x02   //插入一列
#define PEDLDISP    0x04   //左对齐
#define PEDCDISP    0x08   //居中
#define PEDRDISP    0x10   //右对齐

#define OVERLINE 0x20   //上面有标记横线
#define DOWNLINE 0x40   //下面有标记横线
#define SIDELINE 0x80   //左右有框线


#define LCDWORDWIDTH 16 // 12 //显示字列数
#define PEDLCDCHARWIDTH 8       // 6  //显示字符列数


#if ((LCDWORDWIDTH == 16) && (PEDLCDCHARWIDTH == 8))
#define DOT_TABEL ASCII_8_16
#endif

#define PEDMAXCOL  128     //最大列数
#define PEDMAXROW  2    //5       //最大行数	K3102小屏 G101D
#define MAX_PAGE DISPAGE3 //5 K3102 小屏 32*128

#define MAXCHAR (PEDMAXCOL / PEDLCDCHARWIDTH)   //21      //每行最大字符数

#define         LCD_COL_OFFSET  4 //显示内容缓冲在LCD上的列偏移

#define MAX_DOT_COL             (PEDMAXCOL + LCD_COL_OFFSET)

#define MAX_DOT_ROW             32 //64		K3102小屏 32*128

#define MAX_BUF_ROW             (MAX_DOT_ROW / 8)

u8 gG101DispRam[MAX_BUF_ROW][MAX_DOT_COL]; //显示缓存


//--------------------------------------------------------
/*	字体          :	宋体12                                      */
/*  宽×高（像素）: 8 ×16										*/
/*  字模格式/大小 : 单色点阵液晶字模，纵向取模，字节倒序/16字节 */
u8 const ASCII_8_16[] =         //8*16 ASCII                                                    //zhuoquan 1107 黄学佳提供
{
    /*	 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* ! */
    0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x33, 0x30, 0x00, 0x00, 0x00,
    /* " */
    0x00, 0x10, 0x0C, 0x06, 0x10, 0x0C, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* # */
    0x40, 0xC0, 0x78, 0x40, 0xC0, 0x78, 0x40, 0x00, 0x04, 0x3F, 0x04, 0x04, 0x3F, 0x04, 0x04, 0x00,
    /* $ */
    0x00, 0x70, 0x88, 0xFC, 0x08, 0x30, 0x00, 0x00, 0x00, 0x18, 0x20, 0xFF, 0x21, 0x1E, 0x00, 0x00,
    /* % */
    0xF0, 0x08, 0xF0, 0x00, 0xE0, 0x18, 0x00, 0x00, 0x00, 0x21, 0x1C, 0x03, 0x1E, 0x21, 0x1E, 0x00,
    /* & */
    0x00, 0xF0, 0x08, 0x88, 0x70, 0x00, 0x00, 0x00, 0x1E, 0x21, 0x23, 0x24, 0x19, 0x27, 0x21, 0x10,
    /* ' */
    0x10, 0x16, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* ( */
    0x00, 0x00, 0x00, 0xE0, 0x18, 0x04, 0x02, 0x00, 0x00, 0x00, 0x00, 0x07, 0x18, 0x20, 0x40, 0x00,
    /* ) */
    0x00, 0x02, 0x04, 0x18, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x40, 0x20, 0x18, 0x07, 0x00, 0x00, 0x00,
    /* * */
    0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x02, 0x0F, 0x02, 0x05, 0x00, 0x00, 0x00,
    /* + */
    0x00, 0x00, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x1F, 0x01, 0x01, 0x01, 0x00,
    /* , */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xB0, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* - */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    /* . */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* / */
    0x00, 0x00, 0x00, 0x00, 0x80, 0x60, 0x18, 0x04, 0x00, 0x60, 0x18, 0x06, 0x01, 0x00, 0x00, 0x00,
    /* 0 */
    0x00, 0xE0, 0x10, 0x08, 0x08, 0x10, 0xE0, 0x00, 0x00, 0x0F, 0x10, 0x20, 0x20, 0x10, 0x0F, 0x00,
    /* 1 */
    0x00, 0x10, 0x10, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x3F, 0x20, 0x20, 0x00, 0x00,
    /* 2 */
    0x00, 0x70, 0x08, 0x08, 0x08, 0x88, 0x70, 0x00, 0x00, 0x30, 0x28, 0x24, 0x22, 0x21, 0x30, 0x00,
    /* 3 */
    0x00, 0x30, 0x08, 0x88, 0x88, 0x48, 0x30, 0x00, 0x00, 0x18, 0x20, 0x20, 0x20, 0x11, 0x0E, 0x00,
    /* 4 */
    0x00, 0x00, 0xC0, 0x20, 0x10, 0xF8, 0x00, 0x00, 0x00, 0x07, 0x04, 0x24, 0x24, 0x3F, 0x24, 0x00,
    /* 5 */
    0x00, 0xF8, 0x08, 0x88, 0x88, 0x08, 0x08, 0x00, 0x00, 0x19, 0x21, 0x20, 0x20, 0x11, 0x0E, 0x00,
    /* 6 */
    0x00, 0xE0, 0x10, 0x88, 0x88, 0x18, 0x00, 0x00, 0x00, 0x0F, 0x11, 0x20, 0x20, 0x11, 0x0E, 0x00,
    /* 7 */
    0x00, 0x38, 0x08, 0x08, 0xC8, 0x38, 0x08, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00,
    /* 8 */
    0x00, 0x70, 0x88, 0x08, 0x08, 0x88, 0x70, 0x00, 0x00, 0x1C, 0x22, 0x21, 0x21, 0x22, 0x1C, 0x00,
    /* 9 */
    0x00, 0xE0, 0x10, 0x08, 0x08, 0x10, 0xE0, 0x00, 0x00, 0x00, 0x31, 0x22, 0x22, 0x11, 0x0F, 0x00,
    /* : */
    0x00, 0x00, 0x00, 0xC0, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x00,
    /* ; */
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x60, 0x00, 0x00, 0x00, 0x00,
    /* < */
    0x00, 0x00, 0x80, 0x40, 0x20, 0x10, 0x08, 0x00, 0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x00,
    /* = */
    0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00,
    /* > */
    0x00, 0x08, 0x10, 0x20, 0x40, 0x80, 0x00, 0x00, 0x00, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, 0x00,
    /* ? */
    0x00, 0x70, 0x48, 0x08, 0x08, 0x08, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x30, 0x36, 0x01, 0x00, 0x00,
    /* @ */
    0xC0, 0x30, 0xC8, 0x28, 0xE8, 0x10, 0xE0, 0x00, 0x07, 0x18, 0x27, 0x24, 0x23, 0x14, 0x0B, 0x00,
    /* A */
    0x00, 0x00, 0xC0, 0x38, 0xE0, 0x00, 0x00, 0x00, 0x20, 0x3C, 0x23, 0x02, 0x02, 0x27, 0x38, 0x20,
    /* B */
    0x08, 0xF8, 0x88, 0x88, 0x88, 0x70, 0x00, 0x00, 0x20, 0x3F, 0x20, 0x20, 0x20, 0x11, 0x0E, 0x00,
    /* C */
    0xC0, 0x30, 0x08, 0x08, 0x08, 0x08, 0x38, 0x00, 0x07, 0x18, 0x20, 0x20, 0x20, 0x10, 0x08, 0x00,
    /* D */
    0x08, 0xF8, 0x08, 0x08, 0x08, 0x10, 0xE0, 0x00, 0x20, 0x3F, 0x20, 0x20, 0x20, 0x10, 0x0F, 0x00,
    /* E */
    0x08, 0xF8, 0x88, 0x88, 0xE8, 0x08, 0x10, 0x00, 0x20, 0x3F, 0x20, 0x20, 0x23, 0x20, 0x18, 0x00,
    /* F */
    0x08, 0xF8, 0x88, 0x88, 0xE8, 0x08, 0x10, 0x00, 0x20, 0x3F, 0x20, 0x00, 0x03, 0x00, 0x00, 0x00,
    /* G */
    0xC0, 0x30, 0x08, 0x08, 0x08, 0x38, 0x00, 0x00, 0x07, 0x18, 0x20, 0x20, 0x22, 0x1E, 0x02, 0x00,
    /* H */
    0x08, 0xF8, 0x08, 0x00, 0x00, 0x08, 0xF8, 0x08, 0x20, 0x3F, 0x21, 0x01, 0x01, 0x21, 0x3F, 0x20,
    /* I */
    0x00, 0x08, 0x08, 0xF8, 0x08, 0x08, 0x00, 0x00, 0x00, 0x20, 0x20, 0x3F, 0x20, 0x20, 0x00, 0x00,
    /* J */
    0x00, 0x00, 0x08, 0x08, 0xF8, 0x08, 0x08, 0x00, 0xC0, 0x80, 0x80, 0x80, 0x7F, 0x00, 0x00, 0x00,
    /* K */
    0x08, 0xF8, 0x88, 0xC0, 0x28, 0x18, 0x08, 0x00, 0x20, 0x3F, 0x20, 0x01, 0x26, 0x38, 0x20, 0x00,
    /* L */
    0x08, 0xF8, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x3F, 0x20, 0x20, 0x20, 0x20, 0x30, 0x00,
    /* M */
    0x08, 0xF8, 0xF8, 0x00, 0xF8, 0xF8, 0x08, 0x00, 0x20, 0x3F, 0x00, 0x3F, 0x00, 0x3F, 0x20, 0x00,
    /* N */
    0x08, 0xF8, 0x30, 0xC0, 0x00, 0x08, 0xF8, 0x08, 0x20, 0x3F, 0x20, 0x00, 0x07, 0x18, 0x3F, 0x00,
    /* O */
    0xE0, 0x10, 0x08, 0x08, 0x08, 0x10, 0xE0, 0x00, 0x0F, 0x10, 0x20, 0x20, 0x20, 0x10, 0x0F, 0x00,
    /* P */
    0x08, 0xF8, 0x08, 0x08, 0x08, 0x08, 0xF0, 0x00, 0x20, 0x3F, 0x21, 0x01, 0x01, 0x01, 0x00, 0x00,
    /* Q */
    0xE0, 0x10, 0x08, 0x08, 0x08, 0x10, 0xE0, 0x00, 0x0F, 0x18, 0x24, 0x24, 0x38, 0x50, 0x4F, 0x00,
    /* R */
    0x08, 0xF8, 0x88, 0x88, 0x88, 0x88, 0x70, 0x00, 0x20, 0x3F, 0x20, 0x00, 0x03, 0x0C, 0x30, 0x20,
    /* S */
    0x00, 0x70, 0x88, 0x08, 0x08, 0x08, 0x38, 0x00, 0x00, 0x38, 0x20, 0x21, 0x21, 0x22, 0x1C, 0x00,
    /* T */
    0x18, 0x08, 0x08, 0xF8, 0x08, 0x08, 0x18, 0x00, 0x00, 0x00, 0x20, 0x3F, 0x20, 0x00, 0x00, 0x00,
    /* U */
    0x08, 0xF8, 0x08, 0x00, 0x00, 0x08, 0xF8, 0x08, 0x00, 0x1F, 0x20, 0x20, 0x20, 0x20, 0x1F, 0x00,
    /* V */
    0x08, 0x78, 0x88, 0x00, 0x00, 0xC8, 0x38, 0x08, 0x00, 0x00, 0x07, 0x38, 0x0E, 0x01, 0x00, 0x00,
    /* W */
    0xF8, 0x08, 0x00, 0xF8, 0x00, 0x08, 0xF8, 0x00, 0x03, 0x3C, 0x07, 0x00, 0x07, 0x3C, 0x03, 0x00,
    /* X */
    0x08, 0x18, 0x68, 0x80, 0x80, 0x68, 0x18, 0x08, 0x20, 0x30, 0x2C, 0x03, 0x03, 0x2C, 0x30, 0x20,
    /* Y */
    0x08, 0x38, 0xC8, 0x00, 0xC8, 0x38, 0x08, 0x00, 0x00, 0x00, 0x20, 0x3F, 0x20, 0x00, 0x00, 0x00,
    /* Z */
    0x10, 0x08, 0x08, 0x08, 0xC8, 0x38, 0x08, 0x00, 0x20, 0x38, 0x26, 0x21, 0x20, 0x20, 0x18, 0x00,
    /* [ */
    0x00, 0x00, 0x00, 0xFE, 0x02, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x7F, 0x40, 0x40, 0x40, 0x00,
    /* \ */
    0x00, 0x0C, 0x30, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x06, 0x38, 0xC0, 0x00,
    /* ] */
    0x00, 0x02, 0x02, 0x02, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x7F, 0x00, 0x00, 0x00,
    /* ^ */
    0x00, 0x00, 0x04, 0x02, 0x02, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* _ */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
    /* ` */
    0x00, 0x02, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    /* a */
    0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x19, 0x24, 0x22, 0x22, 0x22, 0x3F, 0x20,
    /* b */
    0x08, 0xF8, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x11, 0x20, 0x20, 0x11, 0x0E, 0x00,
    /* c */
    0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x0E, 0x11, 0x20, 0x20, 0x20, 0x11, 0x00,
    /* d */
    0x00, 0x00, 0x00, 0x80, 0x80, 0x88, 0xF8, 0x00, 0x00, 0x0E, 0x11, 0x20, 0x20, 0x10, 0x3F, 0x20,
    /* e */
    0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x1F, 0x22, 0x22, 0x22, 0x22, 0x13, 0x00,
    /* f */
    0x00, 0x80, 0x80, 0xF0, 0x88, 0x88, 0x88, 0x18, 0x00, 0x20, 0x20, 0x3F, 0x20, 0x20, 0x00, 0x00,
    /* g */
    0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x6B, 0x94, 0x94, 0x94, 0x93, 0x60, 0x00,
    /* h */
    0x08, 0xF8, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x20, 0x3F, 0x21, 0x00, 0x00, 0x20, 0x3F, 0x20,
    /* i */
    0x00, 0x80, 0x98, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x3F, 0x20, 0x20, 0x00, 0x00,
    /* j */
    0x00, 0x00, 0x00, 0x80, 0x98, 0x98, 0x00, 0x00, 0x00, 0xC0, 0x80, 0x80, 0x80, 0x7F, 0x00, 0x00,
    /* k */
    0x08, 0xF8, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0x20, 0x3F, 0x24, 0x02, 0x2D, 0x30, 0x20, 0x00,
    /* l */
    0x00, 0x08, 0x08, 0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x20, 0x3F, 0x20, 0x20, 0x00, 0x00,
    /* m */
    0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x20, 0x3F, 0x20, 0x00, 0x3F, 0x20, 0x00, 0x3F,
    /* n */
    0x80, 0x80, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x20, 0x3F, 0x21, 0x00, 0x00, 0x20, 0x3F, 0x20,
    /* o */
    0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x00, 0x1F, 0x20, 0x20, 0x20, 0x20, 0x1F, 0x00,
    /* p */
    0x80, 0x80, 0x00, 0x80, 0x80, 0x00, 0x00, 0x00, 0x80, 0xFF, 0xA1, 0x20, 0x20, 0x11, 0x0E, 0x00,
    /* q */
    0x00, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x0E, 0x11, 0x20, 0x20, 0xA0, 0xFF, 0x80,
    /* r */
    0x80, 0x80, 0x80, 0x00, 0x80, 0x80, 0x80, 0x00, 0x20, 0x20, 0x3F, 0x21, 0x20, 0x00, 0x01, 0x00,
    /* s */
    0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x33, 0x24, 0x24, 0x24, 0x24, 0x19, 0x00,
    /* t */
    0x00, 0x80, 0x80, 0xE0, 0x80, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F, 0x20, 0x20, 0x00, 0x00,
    /* u */
    0x80, 0x80, 0x00, 0x00, 0x00, 0x80, 0x80, 0x00, 0x00, 0x1F, 0x20, 0x20, 0x20, 0x10, 0x3F, 0x20,
    /* v */
    0x80, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x00, 0x01, 0x0E, 0x30, 0x08, 0x06, 0x01, 0x00,
    /* w */
    0x80, 0x80, 0x00, 0x80, 0x00, 0x80, 0x80, 0x80, 0x0F, 0x30, 0x0C, 0x03, 0x0C, 0x30, 0x0F, 0x00,
    /* x */
    0x00, 0x80, 0x80, 0x00, 0x80, 0x80, 0x80, 0x00, 0x00, 0x20, 0x31, 0x2E, 0x0E, 0x31, 0x20, 0x00,
    /* y */
    0x80, 0x80, 0x80, 0x00, 0x00, 0x80, 0x80, 0x80, 0x80, 0x81, 0x8E, 0x70, 0x18, 0x06, 0x01, 0x00,
    /* z */
    0x00, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x00, 0x00, 0x21, 0x30, 0x2C, 0x22, 0x21, 0x30, 0x00,
    /* { */
    0x00, 0x00, 0x00, 0x00, 0x80, 0x7C, 0x02, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x40, 0x40,
    /* | */
    0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
    /* } */
    0x00, 0x02, 0x02, 0x7C, 0x80, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0x3F, 0x00, 0x00, 0x00, 0x00,
    /* ~ */
    0x00, 0x06, 0x01, 0x01, 0x02, 0x02, 0x04, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
//
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static bool IsWithPinpad()
{
    return sdk_dev_get_pinpadstate();
}
/*******************************************************************
   版          权: 新国都
   作          者: 郭泽贤
   函数名称: FormSendDataToPed
   函数功能: 组织发往串口的01 协议数据
   输入参数:
                          1. u8 order: 命令字
                          2. const u8 *buf: 输入的数据
                          3. uLen: 输入的数据长度
   输出参数:
                          1. u8 *pDataOut: 输出的数据
                          2. u16 *pLen: 输出的数据长度
   返   回  值:
   修改备注:
   日期时间:2012.05.11 13:38:33
*******************************************************************/
static void FormSendDataToPed(u8 order, const u8 *buf, u16 uLen, u8 *pDataOut, u16 *pLen)
{
    u8 bcc;
    u8 SendData[1024];
    u16 SendLen, len;
    u32 i;

    SendLen = 0;
    len = uLen;

    for (i = 0; i < 4; i++)
    {
        SendData[SendLen++] = 0x01;
    }

    bcc = PED_SOH;

    if (len == 0)
    {
        SendData[SendLen++] = PED_KEY_ORDER;
        bcc += PED_KEY_ORDER;
    }
    else if ((len > 0) && (len < 249))
    {
        SendData[SendLen++] = PED_KEY_TEXT;
        bcc += PED_KEY_TEXT;
    }
    else
    {
        SendData[SendLen++] = PED_KEY_EXTEXT;
        bcc += PED_KEY_EXTEXT;
    }
    SendData[SendLen++] = order;
    bcc += order;

    if (len > 0)
    {
        if (len < 249)
        {
            SendData[SendLen++] = (u8)len;                          // 数据长度
            bcc += (u8)len;
        }
        else
        {
            SendData[SendLen++] = (u8)(len / 0x100);                   // 数据长度
            bcc += (u8)(len / 0x100);
            SendData[SendLen++] = len % 0x100;                   // 数据长度
            bcc += len % 0x100;
        }

        for (i = 0; i < uLen; i++)
        {
            SendData[SendLen++] = buf[i];
            bcc += buf[i];
        }

        SendData[SendLen++] = PED_ETX;
        bcc += PED_ETX;
    }
    SendData[SendLen++] = bcc;
    memcpy(pDataOut, SendData, SendLen);
    *pLen = SendLen;
}

/*******************************************************************
   版          权: 新国都
   作          者: 郭泽贤
   函数名称: ParseDataFromPed
   函数功能: 解析串口数据
   输入参数: 1. QUEUE *queue: 要解析数据的队列
   输出参数: 1. UartData *RecvUartData: 输出解析的结果
   返   回  值:
                            SDK_OK: 解析正确
                            SDK_ERR: 解析错误
                            SDK_ESC: 解析退出
   修改备注:
   日期时间:2012.05.11 13:39:05
*******************************************************************/
static s32 ParseDataFromPed(UartData *RecvUartData, SDK_QUEUE *queue)
{
    u8 data;
    u32 head;

//    memset(RecvUartData, 0, sizeof(UartData));    //zxx 20130131 11:14 这里不初始化
    head = queue->siHead;                                                             // 赋值初始化查询头

    while (sdkTryQueueData(queue, head, &data))                        // 取队列数据
    {
        head = (head + 1) % QUEUE_SIZE;                                                   // 查询头前滚
        Trace("ped", "data = %02x Step = %02x\r\n", data, RecvUartData->Step);
        switch (RecvUartData->Step)
        {
            case STEP_SOH:
                RecvUartData->Cs = PED_SOH;

                if (data == PED_SOH)
                {
                    RecvUartData->HeadLen++;
                }
                else if ((data == PED_KEY_TEXT || data == PED_KEY_ORDER || data == PED_KEY_EXTEXT)
                         && (RecvUartData->HeadLen > 2))
                {
                    RecvUartData->Cs += data;
                    RecvUartData->Step = STEP_ORDER;
                    RecvUartData->Text = data;
                }
                else
                {
                    memset(RecvUartData, 0, sizeof(UartData));
                    RecvUartData->Step = STEP_SOH;
                    // RecvUartData->HeadLen = 0;
                    sdkSetQueueHead(queue, head);                            // 重新设置队列头
                }
                RecvUartData->Ret = STATE_NULL;
                break;

            case STEP_ORDER:
                RecvUartData->Cs += data;
                RecvUartData->Comm = data;                              // 命令字

                if (RecvUartData->Text == PED_KEY_TEXT || RecvUartData->Text == PED_KEY_EXTEXT)
                {
                    RecvUartData->Step = STEP_LEN1;
                }
                else if (RecvUartData->Text == PED_KEY_ORDER)
                {
                    // RecvUartData->Step = STEP_ETX;
                    RecvUartData->Step = STEP_CS;
                }
                RecvUartData->LenCun = 0;                            // 已接收的数据长度
                break;

            case STEP_LEN1:                                    // 接收长度第一位
                RecvUartData->Cs += data;
                RecvUartData->Len = (u16)data;                          // 应接收的数据长度

                if (RecvUartData->Text == PED_KEY_TEXT)
                {
                    RecvUartData->Step = STEP_DATA;                              // 接收数据
                }
                else if (RecvUartData->Text == PED_KEY_EXTEXT)
                {
                    RecvUartData->Step = STEP_LEN2;                              // 接收长度
                }
                break;

            case STEP_LEN2:                                     // 接收长度第二位
                RecvUartData->Cs += data;
                RecvUartData->Len = (u16)(RecvUartData->Len * 0x100 + data);                         // 应接收的数据长度

                if (RecvUartData->Text == PED_KEY_EXTEXT)
                {
                    RecvUartData->Step = STEP_DATA;                               // 接收长度
                }
                break;

            case STEP_DATA:                                       // 接收数据
                RecvUartData->Cs += data;
                RecvUartData->Buf[RecvUartData->LenCun++] = data;

                if (RecvUartData->LenCun == RecvUartData->Len)
                {
                    RecvUartData->Step = STEP_ETX;
                }
                break;

            case STEP_ETX:
                RecvUartData->Cs += data;

                if (data == PED_ETX)
                {
                    RecvUartData->Step = STEP_CS;
                }
                else
                {
                    memset(RecvUartData, 0, sizeof(UartData));
                    RecvUartData->Step = STEP_SOH;
                    sdkSetQueueHead(queue, head);                            // 重新设置队列头
                }
                break;

            case STEP_CS:
                sdkSetQueueHead(queue, head);                        // 重新设置队列头

                if (RecvUartData->Cs == data)
                {
                    RecvUartData->Ret = STATE_OK;                        // 数据正确
                    //------------------////zxx 3.0 20130301 16:32
                    if (!sdkIsQueueEmpty(queue) && RecvUartData->Comm == 0x20)
                    {
                        memset(RecvUartData, 0, sizeof(UartData));
                        continue;
                    }
                    //-------------------//
                    return SDK_OK;
                }
                else
                {
                    RecvUartData->Ret = STATE_CSERR;                       // 数据错误
                    Assert(0);
                    return SDK_ERR;
                }
            default:
                break;
        }
    }
    return SDK_ESC;
}

/*******************************************************************
   版          权: 新国都
   作          者: 郭泽贤
   函数名称: RecvDataFromPed
   函数功能: 串口接收数据
   输入参数: 1. u8 uSendCmd: 发送的命令字
   输出参数: 1.UartData *pRecvData: 解析得到的数据
   返   回  值:
                            SDK_OK: 接收数据正确
                            SDK_ERR: 接收数据错误
                            SDK_TIME_OUT: 接收超时
   修改备注:
   日期时间:2012.05.11 13:40:58
*******************************************************************/
static s32 RecvDataFromPed(u8 uSendCmd, UartData *pRecvData)
{
    FIFO fifo;
    s32 rslt;
    SDK_QUEUE ComQueue;
    u32 timerID = sdkTimerGetId();

    memset(&ComQueue, 0, sizeof(ComQueue));
    memset(pRecvData, 0, sizeof(UartData)); //zxx 20130131 14:13
    while (1)
    {
        if (uSendCmd == 0x8d || uSendCmd == 0xdd)
        {
            if (sdkTimerIsEnd(timerID, SDK_PED_TIMEOUT * 15) == 1)           // 输入金额超时时间60秒到
            {
                return SDK_TIME_OUT;
            }
        }
        else
        {
            if (sdkTimerIsEnd(timerID, SDK_PED_TIMEOUT) == 1) // 超时时间4秒到
            {
                return SDK_TIME_OUT;
            }
        }

        if (sdk_dev_read_fifo(&fifo, 0))
        {
            if (fifo.Cmd == FIFO_RECCOMTRANSP)
            {
                sdkInsertQueue(&ComQueue, fifo.Data, fifo.Len);                    // 将数据压入队列
                TraceHex("ped", "receive data from PED", fifo.Data, fifo.Len);
                rslt = ParseDataFromPed(pRecvData, &ComQueue);
                //------------------------------//zxx 20130131 11:22
                if (rslt == SDK_ESC)
                {
                    memset(&ComQueue, 0, sizeof(ComQueue));     //继续读取，清之前的队列
                    continue;          //返回sdk_esc说明数据未收完，继续读取解析
                }
                //------------------------------//
                if (rslt == SDK_OK)
                {
                    //zxx 3.0 20130301 16:24移动ParseDataFromPed里面处理，这里不知道到底是已经收到下一条了还是没有收到，是否还要读fifo不确定
                    if (pRecvData->Comm == 0x20)        //表示密码键盘已经成功接收指令
                    {
                        Trace("ped", "ped dataok,continue recv\r\n");
                        memset(pRecvData, 0, sizeof(UartData)); //zxx 20130131 14:9
                        continue;
                    }

//                    else if (pRecvData->Comm == 0xf7 // 初始化密钥芯片
//                             || pRecvData->Comm == 0x83 //回传金额(针对3306H和G101)
//                             || pRecvData->Comm == 0xdd) //回传金额(针对3306R)
//                    {
//                        return SDK_OK;
//                    }
//                    else        //正常情况不应该走到这里
//                    {
//                        return SDK_ERR;
//                    }

                }
//                else
//                {
//                    return SDK_ERR;
//                }
                return rslt;
            }
        }
    }
}

/*******************************************************************
   版          权: 新国都
   作          者: 郭泽贤
   函数名称: SendDataToPed
   函数功能: 发送数据给密码键盘
   输入参数: 1. const UartSendCfg *UartSend: 需要发送的数据
   输出参数: 无
   返   回  值:
                            SDK_OK: 接收数据正确
                            SDK_ERR: 接收数据错误
                            SDK_TIME_OUT: 接收超时
   修改备注:
   日期时间:2012.05.11 13:41:04
*******************************************************************/
s32 SendDataToPed(const UartSendCfg *UartSend)
{
    s32 rslt;
    u8 buf[1024];
    u16 len;
    u8 ped_flag = (IsWithPinpad()) ? WITHPINPAD : WITHOUTPINPAD;
    SDK_COMM_STUARTPARAM UartCfg;

    memset(&UartCfg, 0, sizeof(UartCfg));

    if (ped_flag == WITHPINPAD)
    {
        UartCfg.ucCom = sdkSysGetComNo(SDK_SYS_COM_PIN_PAD);
    }
    else
    {
        UartCfg.ucCom = sdkSysGetComNo(SDK_SYS_COM_PC);
    }
    Trace("ped", "\r\nCom NO = %d\r\n", UartCfg.ucCom);

    UartCfg.uiBaudrate = 38400;
    UartCfg.ucByteLen = '8';
    UartCfg.ucCheckBit = 'n';
    UartCfg.ucStopBitLen = '1';
    UartCfg.ucFlowControl = '0';

    memset(buf, 0, sizeof(buf));
    FormSendDataToPed(UartSend->Cmd, UartSend->pDataIn, UartSend->Len, buf, &len);
    sdkCommOpenUart(&UartCfg);
    sdkCommUartSendData(UartCfg.ucCom, buf, len);
    rslt = RecvDataFromPed(UartSend->Cmd, UartSend->RecvUartData);
    // rslt = sdkCommRecvDataFromUart(UartCfg.Com , const u8 * pRecvData , const u16 nRecvMaxLen , const u8 nTimeOut , const u8 nProtocol , s32 (* pParseFunc)(const u8 * pData , u8 * len));
    sdkCommCloseUart(UartCfg.ucCom);
    return rslt;
}

/*******************************************************************
   函数名称:
   函数功能: 获取汉字点阵数据
   相关调用:
   入口参数:    buf:取到的点阵数据
                        offset:偏移量
   返 回 值:	0 成功
                        非0 失败
   备    注:
   创建信息:   卓铨
   修改信息:

                黄学佳，2012.11.02 测试读取文件，获取点阵传给密码键盘
 ********************************************************************/
s32  G101_get_arr(u8 *buf, u32 offset)
{
    int fontfd = -1;
    u32 xlen = 16;
    u32 yhigh = 16;
    u32 fontsize = 0;

    //打开当前汉字库文件


    //打开文件
    fontfd = open(PINPAD_ZIKU_FILE1, O_RDONLY);

    if (fontfd == -1)
    {
        //  printf("open hz1616 ziku failed\n");
        Trace("ped", "open hz1616 ziku failed\n"); //shiweisong 2013.01.08 19:21
        Assert(0);
        fontfd = open(PINPAD_ZIKU_FILE2, O_RDONLY);
        if(fontfd == -1)
        {
           return -1; 
        }
    }
    //一个汉字所占字节数
    fontsize = ((xlen + 7) / 8) * yhigh;

    lseek(fontfd, offset, SEEK_SET);

    if (read(fontfd, buf, fontsize) == fontsize)
    {
        close(fontfd);
        return 0;
    }
    else
    {
        close(fontfd);
        return -1;
    }
}

/************************************************************
        支持12点阵到48点阵转换

   参数：
        width 字宽
        height 字高
        h2v  =1 H->V; 横库,左高右低，左上->右上->左下->右下==>纵库,下高上低，左上-->右上-->左下-->右下
                 =0 V->H  纵库,下高上低，左上-->右上-->左下-->右下==>横库，左高右低，左上->右上->左下->右下
                 =2 H->H;(横库,左高右低，左上左下，右上右下)==>横库，左高右低，左上->右上->左下->右下  宽度不足1Byte,补0
        dat 转换数据
   RETURN:
        =0 FAIL
        =1 SUCCESS
   算法：先求出点的坐标，再转换

        黄学佳，2012.11.05 从打印机程序中拷贝过来，用于处理传给G101的点阵
 ****************************************************************/
int  G101_LCD_FontHVChange(int width, int height, int h2v, char *dat)
{
    int old_widthbyte, old_heightbyte, old_mask = 0;
    int new_widthbyte, new_heightbyte;
    int i, j, k, m, n;
    int a;
    u8 data;

    char srcbuf[512];

    if (width > 48 || height > 48)
    {
        return 0;                                  //限制只支持不高于48X48点阵
    }
    memset(srcbuf, 0, sizeof(srcbuf));

    //dot = (char*)malloc(512);
    //横库，左高右低，左上->右上->左下->右下  宽度不足1Byte,补0
    //纵库,下高上低，左上-->右上-->左下-->右下
    if (1 == h2v)        //H->V;  横转纵
    {
        //未经验证  ????
        //横库
        old_widthbyte  = (width + 7) >> 3;
        old_heightbyte = height;
        //纵库
        new_widthbyte = width;
        new_heightbyte = (height + 7) >> 3;
        //补齐
        n = (old_heightbyte + 7) & 0xf8;
        k = old_widthbyte * old_heightbyte;
        memcpy(srcbuf, dat, k);
        memset(&srcbuf[k], 0, (n - old_heightbyte) * old_widthbyte);
        a = n;
        n = 0;

        //m=0;
        for (i = 0; i < new_heightbyte; i++)
        {
            for (j = 0; j < new_widthbyte; j++)
            {
                if (0 == (j & 0x07))
                {
                    old_mask = 0x80;
                }
                else
                {
                    old_mask >>= 1;
                }
                data = 0;
                m = (i << 3) * old_widthbyte + (j >> 3);

                for (k = 0; k < 8; k++)
                {
                    data >>= 1;

                    if (srcbuf[m] & old_mask)
                    {
                        data |= 0x80;
                    }
                    m += old_widthbyte;
                }

                dat[n++] = data;
            }
        }
    }
#if 0 //黄学佳 2012.11.05
    else if (0 == h2v)               //V->H;  纵转横
    {
        //纵库
        old_widthbyte = width;
        old_heightbyte = (height + 7) >> 3;
        //横库
        new_widthbyte = (width + 7) >> 3;
        new_heightbyte = height;
        //补齐
        n = (old_widthbyte + 7) & 0xf8;
        j = 0;
        k = 0;

        for (i = 0; i < old_heightbyte; i++)
        {
            memcpy(&srcbuf[j], &dat[k], old_widthbyte);
            j += n;
            k += old_widthbyte;
        }

        a = n;
        n = 0;

        //m = 0;
        for (i = 0; i < new_heightbyte; i++)
        {
            m = (i >> 3) * a;

            if (0 == (i & 0x07))
            {
                old_mask = 0x01;
            }
            else
            {
                old_mask <<= 1;                //上低下高
            }

            for (j = 0; j < new_widthbyte; j++)
            {
                //new_mask = 0x80;
                data = 0;
                l = (j << 3);

                for (k = 0; k < 8; k++)
                {
//	                if(l>=old_widthbyte){ //l=(0,(old_widthbyte-1))
//	                    data <<= (8-k);
//	                    break;
//	                }
                    data <<= 1;

                    if (srcbuf[m + l + k] & old_mask)
                    {
                        data += 1;
                    }
                }

                dat[n++] = data;
            }
        }
    }
    else if (2 == h2v)              //横库转横库
    {
        //横库
        old_widthbyte  = (width + 7) >> 3;
        old_heightbyte = height;
        //补齐
        //n = (old_heightbyte+7)&0xf8;
        k = old_widthbyte * old_heightbyte;
        memcpy(srcbuf, dat, k);
        n = 0;

        for (i = 0; i < height; i++)
        {
            for (j = 0; j < old_widthbyte; j++)
            {
                dat[n++] = srcbuf[j * height + i];
            }
        }
    }
#endif
    else
    {
        return -1;
    }
    return 1;
}

/****************************************************************************
  **Description:
  **Input parameters:
  **Output parameters:
**
******Returned value:
**
******Created by:			zhuoquan                11/07
******--------------------------------------------------------------------------
******Modified by:
******Modified by:
****************************************************************************/
s32 G101_get_dot(u8 *ch, u8 *DataPtr)
{
    u32 the_dot_addr;
    u32 i;
    u32 offset;

    if (ch[0] < 0x80)           // ASCII字符
    {
        // 计算存储地址
        the_dot_addr = (ch[0] - 0x20) * 2 * PEDLCDCHARWIDTH;         //+ ZIKU12ADD;

        for (i = 0; i < 2 * PEDLCDCHARWIDTH; i++)
        {
            *DataPtr++ = DOT_TABEL[the_dot_addr + i];
        }
    }
    else        // 中文字符
    {
        //计算出偏移
        offset = 0x00;
        offset = *ch - 0xa1;
        offset = offset * 94;
        offset = offset + *(ch + 1) - 0xa1;
        offset = offset * 32;

        if (0 == G101_get_arr(DataPtr, offset))
        {
            G101_LCD_FontHVChange(16, 16, 1, DataPtr);            //转成左上，右上，左下，右下的纵库
            return 1;
        }
        else
        {
            return 0;
        }
    }
    return 1;
}

/****************************************************************************
  **Description:
  **Input parameters:
  **Output parameters:
**
******Returned value:
**
******Created by:			zhuoquan                11/07
******--------------------------------------------------------------------------
******Modified by:
******Modified by:
****************************************************************************/
void G101_lcd_write_line(u8 row, u8 head, u8 tail)
{
    u8 i;

    switch (row)
    {
        case 0:

            for (i = head; i < tail; i++)
            {
                gG101DispRam[row][i] |= 0x01;
            }

            break;

        case 1:

            for (i = head; i < tail; i++)
            {
                gG101DispRam[row][i] |= 0x10;
            }

            break;

        case 2:

            for (i = head; i < tail; i++)
            {
                gG101DispRam[row][i] |= 0x02;
            }

            break;

        case 3:

            for (i = head; i < tail; i++)
            {
                gG101DispRam[row][i] |= 0x40;
            }

            break;

#if (MAX_PAGE == (DISPAGE5 + 1))
        case 4:

            for (i = head; i < tail; i++)
            {
                gG101DispRam[row][i] |= 0x08;
            }

            break;

        case 5:

            for (i = head; i < tail; i++)
            {
                gG101DispRam[row][i] |= 0x80;
            }

            break;

#endif
        default:
            break;
    }
}

/*******************************************************************
   函数名称: void dev_lcd_clear_ram(void)
   函数功能: 清除显示内存
   相关调用: gDispRam 显示缓存
   入口参数: 无
   返 回 值: 无
   备    注:
   创建信息:	zhuoquan                11/07
   修改信息:
 ********************************************************************/
void G101_lcd_clear_ram(void)
{
    u8 i, j;

    for (i = 0; i < MAX_BUF_ROW; i++)
    {
        for (j = 0; j < MAX_DOT_COL; j++)
        {
            gG101DispRam[i][j] = 0;
        }
    }
}

/****************************************************************************
  **Description:
  **Input parameters:
  **Output parameters:
**
******Returned value:
**
******Created by:			zhuoquan                11/07
******--------------------------------------------------------------------------
******Modified by:
******Modified by:
****************************************************************************/
void G101_lcd_fill_rowram(u8 rowid, u8 colid, u8 *str, u8 atr)
{
    s32 i, digcount;
    u8 tmpdisram[2][MAX_DOT_COL], dot_col;
    s32 j;

    union ConverData
    {
        u8 s[4 * PEDLCDCHARWIDTH];
        u16 z[2 * PEDLCDCHARWIDTH];
    } dot;

    u8 no, t_atr;
    u8 Hzlen;

    if (colid >= PEDMAXCOL)
    {
        colid = 0;                        //纠错128
    }

    if (rowid >= PEDMAXROW)
    {
        rowid = 0;                        //5
    }
    no = strlen((const char *)str);    //

    if (no > MAXCHAR)
    {
        no = MAXCHAR;         //21
    }
    //Hzlen = CaluHanzi(str);

    dot_col = colid;                       //列数
    //temp1 = 0;		   //列数
    Hzlen = 0;                             //汉字个数

    for (i = 0; i < no;)
    {
        if (str[i] < 0x80)               //ASCII码表16*8
        {
            dot_col += PEDLCDCHARWIDTH;

            if (dot_col > PEDMAXCOL)
            {
                break;
            }
        }
        else                                     //汉字，16*16
        {
            dot_col += PEDLCDCHARWIDTH;
            i++;

            if (dot_col > PEDMAXCOL)             // 2012.04.10 hxj add
            {
                break;
            }

            if (atr & PEDINCOL)
            {
                if (i < 16)
                {
                    dot_col += PEDLCDCHARWIDTH + 1;
                }
                else
                {
                    dot_col += PEDLCDCHARWIDTH;
                }
            }
            else
            {
                dot_col += PEDLCDCHARWIDTH;
            }

            if (dot_col >= PEDMAXCOL)
            {
                break;
            }
            Hzlen++;
        }
        i++;
    }

    no = i;

    if (atr & PEDINCOL)
    {
        if (colid + no * PEDLCDCHARWIDTH + Hzlen > PEDMAXCOL)
        {
            colid = 0;            //纠错
        }
    }
    else
    {
        if (colid + no * PEDLCDCHARWIDTH > PEDMAXCOL)
        {
            colid = 0;            //纠错
        }
    }
    t_atr = atr & 0x1C;    //(LDISP | CDISP | RDISP)

    switch (t_atr)
    {
        case PEDCDISP:       //居中

            if (atr & PEDINCOL)
            {
                if (colid + no * PEDLCDCHARWIDTH + Hzlen > PEDMAXCOL)
                {
                    colid = 0;
                }
                else
                {
                    colid += (PEDMAXCOL - (colid + no * PEDLCDCHARWIDTH + Hzlen)) / 2;
                }
            }
            else
            {
                colid += (PEDMAXCOL - (colid + no * PEDLCDCHARWIDTH)) / 2;
            }
            break;

        case PEDRDISP:       //右对齐

            if (atr & PEDINCOL)
            {
                colid = PEDMAXCOL       - (no * PEDLCDCHARWIDTH + Hzlen);
            }
            else
            {
                colid = PEDMAXCOL       - no * PEDLCDCHARWIDTH;
            }
            break;

        default:       //左对齐
            break;
    }

    colid += LCD_COL_OFFSET;

    j = (rowid * LCDWORDWIDTH);   // LCD屏行点阵序号
    i = (j >> 3);

    for (digcount = 0; digcount < MAX_DOT_COL; digcount++)
    {
        j &= 7;

        if (j == 0)
        {
            tmpdisram[0][digcount] = gG101DispRam[i][digcount];
            tmpdisram[1][digcount] = gG101DispRam[i + 1][digcount];
        }
        else
        {
            tmpdisram[0][digcount] = gG101DispRam[i][digcount] >> j;
            tmpdisram[0][digcount] |= gG101DispRam[i + 1][digcount] << (8 - j);
            tmpdisram[1][digcount] = gG101DispRam[i + 1][digcount] >> j;
            tmpdisram[1][digcount] |= gG101DispRam[i + 2][digcount] << (8 - j);
        }
    }

    //Uart_Printf("no = %x\r\n",no);
    dot_col = colid;

    for (digcount = 0; digcount < no; digcount++)
    {
        if (str[digcount] < 0x80)        //asc 字符
        {
            //黄学佳 2012.07.04 星号使用小点阵

            // 取字库点阵
            G101_get_dot(&str[digcount], dot.s);

            if ((atr & PEDNOFDISP) != PEDNOFDISP)
            {
                for (i = 0; i < PEDLCDCHARWIDTH; i++)
                {
                    tmpdisram[0][dot_col + i] = dot.s[i];                                               // dot.s[i];//
                    tmpdisram[1][dot_col + i] &= 0xff << (LCDWORDWIDTH - 8);                            //0xf0;
                    tmpdisram[1][dot_col + i] |= dot.s[i + PEDLCDCHARWIDTH] & (0xff >> (16 - LCDWORDWIDTH));                                    // dot.s[6 + i];//
                }
            }
            else
            {
                for (i = 0; i < PEDLCDCHARWIDTH; i++)
                {
                    tmpdisram[0][dot_col + i] = 0xff - dot.s[i];                                // dot.s[i];//
                    tmpdisram[1][dot_col + i] &= 0xff << (LCDWORDWIDTH - 8);
                    tmpdisram[1][dot_col + i] |= (0xff << (16 - LCDWORDWIDTH)) - dot.s[i + PEDLCDCHARWIDTH];                    // dot.s[6 + i];//
                }
            }
            dot_col += PEDLCDCHARWIDTH;
        }
        else
        {
            if (dot_col > LCD_COL_OFFSET + PEDMAXCOL - LCDWORDWIDTH)             //if (digcount > (MAXCHAR - 2))
            {
                ;
            }
            else
            {
                G101_get_dot(&str[digcount], dot.s);

                if ((atr & PEDNOFDISP) != PEDNOFDISP)                              //正显
                {
                    for (i = 0; i < 2 * PEDLCDCHARWIDTH; i++)
                    {
                        tmpdisram[0][dot_col + i] = dot.s[i];
                        tmpdisram[1][dot_col + i] &= 0xff << (LCDWORDWIDTH - 8);
                        tmpdisram[1][dot_col + i] |= dot.s[2 * PEDLCDCHARWIDTH + i] & (0xff >> (16 - LCDWORDWIDTH));
                    }
                }
                else
                {
                    for (i = 0; i < 2 * PEDLCDCHARWIDTH; i++)
                    {
                        tmpdisram[0][dot_col + i] = 0xff - dot.s[i];
                        tmpdisram[1][dot_col + i] &= 0xff << (LCDWORDWIDTH - 8);
                        tmpdisram[1][dot_col + i] |= (0xff >> (16 - LCDWORDWIDTH)) - dot.s[2 * PEDLCDCHARWIDTH + i];
                    }
                }

                if ((atr & PEDINCOL) == PEDINCOL)
                {
                    if (digcount < 16)
                    {
                        if ((atr & PEDNOFDISP) == PEDNOFDISP)                                   //反显
                        {
                            tmpdisram[0][dot_col + 2 * PEDLCDCHARWIDTH] = 0xff;
                            //tmpdisram[1][dot_col + 2*LCDCHARWIDTH] &= 0xff<<(LCDWORDWIDTH-8);
                            tmpdisram[1][dot_col + 2 * PEDLCDCHARWIDTH] |= 0xff >> (16 - LCDWORDWIDTH);
                        }
                        dot_col += 2 * PEDLCDCHARWIDTH + 1;
                    }
                    else
                    {
                        dot_col += 2 * PEDLCDCHARWIDTH;
                    }
                }
                else
                {
                    dot_col += 2 * PEDLCDCHARWIDTH;
                }
            }
            digcount++;
        }
    }

    //这里添加边框的处理
    if (atr & SIDELINE)   //有左右框
    {
        dot_col += 2;
        tmpdisram[0][dot_col - 1] = 0xff;
        tmpdisram[1][dot_col - 1] |= 0xff >> (16 - LCDWORDWIDTH);

        for (i = dot_col - 2; i > colid; i--)
        {
            tmpdisram[0][i] = tmpdisram[0][i - 1];
            tmpdisram[1][i] = tmpdisram[1][i - 1];
        }

        tmpdisram[0][colid] = 0xff;
        tmpdisram[1][colid] |= 0xff >> (16 - LCDWORDWIDTH);
    }
    j = rowid * LCDWORDWIDTH;
    i = (j >> 3);

    for (digcount = 0; digcount < MAX_DOT_COL; digcount++)
    {
        j &= 7;

        if (j == 0)
        {
            gG101DispRam[i][digcount] = tmpdisram[0][digcount];
            gG101DispRam[i + 1][digcount] = tmpdisram[1][digcount];
        }
        else
        {
            gG101DispRam[i][digcount] &= (0xff >> (8 - j));
            gG101DispRam[i][digcount] |= tmpdisram[0][digcount] << j;
            gG101DispRam[i + 1][digcount] = tmpdisram[0][digcount] >> (8 - j);
            gG101DispRam[i + 1][digcount] |= tmpdisram[1][digcount] << j;
            gG101DispRam[i + 2][digcount] &= 0xff << (8 - j);
            gG101DispRam[i + 2][digcount] |= tmpdisram[1][digcount] >> (8 - j);
        }
    }

    //画上下线
    if (atr & OVERLINE)
    {
        G101_lcd_write_line(rowid, colid, dot_col);
    }

    if (atr & DOWNLINE)
    {
        G101_lcd_write_line(rowid + 1, colid, dot_col);
    }
}

/****************************************************************************
  **Description:		获取使用DD 76命令发送给密码键盘的正文内容
  **Input parameters:
  **Output parameters:
**
******Returned value:
**
******Created by:			zhuoquan                11/07
******--------------------------------------------------------------------------
******Modified by:
******Modified by:
****************************************************************************/
s32 G101_lcd_brush_screen_m(u8 *rbuf)
{
    s32 i = 0;

    rbuf[0] = (0x03);       //清除
    rbuf[1] = 0;
    i = 2;
    memcpy(&rbuf[i], gG101DispRam[0], 128);
    i += 128;
    memcpy(&rbuf[i], gG101DispRam[1], 128);
    i += 128;
    rbuf[i++] = 1;
    memcpy(&rbuf[i], gG101DispRam[2], 128);
    i += 128;
    memcpy(&rbuf[i], gG101DispRam[3], 128);
    i += 128;

    return i;
}

/*****************************************************************************
** Descriptions:	        控制键盘显示
** Parameters:          str: 显示的字符串
                                line: 显示行数
                                col : 显示列数
                                atr:
** Returned value:
** Created By:		卓铨  2012.11.07
** Remarks:
*****************************************************************************/
s32 PedDisplayStr(u8 *str, u8 row, u8 col, u8 atr)
{
    u16 ret = 0;//shijianglong 2013.01.09 15:54去掉用不到的i
    u8 DotData[1024];
    u8 Sendbuf[1024];
    s32 DotLen = 0;
    s32 len = 0;
    u32 timerID;
    FIFO fifo;

    if (NULL == str)
    {
        return SDK_PARA_ERR;
    }
    memset(DotData, 0, sizeof(DotData));
    memset(Sendbuf, 0, sizeof(Sendbuf));
    G101_lcd_clear_ram();                                                            //清空显示内存
    G101_lcd_fill_rowram(row, col, str, atr);                                //填充RAM
    DotLen = G101_lcd_brush_screen_m(DotData);                       //得到点阵

//    for(i = 0; i < DotLen; i++)
//    {
//        printf("%02x ", DotData[i]);
//    }
    TraceHex("ped", "pedData", DotData, DotLen); //shiweisong 2013.01.08 19:22
    sdk_dev_pack_fifo(FIFO_BRIDGEENA, "", 0);                                            //打开桥接
    Sendbuf[len++] = TOPINPAD;
    Sendbuf[len++] = 0xDB;                                                  //DB指令
    Sendbuf[len++] = 0x76;
    memcpy(&Sendbuf[len], DotData, DotLen);
    len += DotLen;
    sdk_dev_pack_fifo(FIFO_BRIDGETEXT, Sendbuf, len);
    timerID = sdkTimerGetId();

    while (1)
    {
        if (sdk_dev_read_fifo(&fifo, 0))
        {
            if (fifo.Cmd == FIFO_BRIDGETEXT)
            {
                break;
            }
        }

        if (sdkTimerIsEnd(timerID, SDK_PED_TIMEOUT) == 1)
        {
            ret = 1;
            break;
        }
    }

    sdk_dev_pack_fifo(FIFO_BRIDGEDIS, "", 0);                                                    //关闭桥接

    if (ret)
    {
        return SDK_TIME_OUT;
    }
    return SDK_OK;
}


/*****************************************************************************
** Descriptions:	        密码键盘信息显示
** Parameters:          str: 显示的字符串
                                row: 显示行数
                                col : 显示列数
                                atr:
** Returned value:
** Created By:		//shijianglong 2013.01.14 20:15
** Remarks:
*****************************************************************************/
s32 sdkPedRDispStr(u8 *str, u8 row, u8 col, u8 atr)
{
    u8 temp[512], buf[256];
    u16 le = 0, len = 0;
    s32 timer = 0;
    FIFO fifo;

    if (NULL == str)
    {
        return SDK_PARA_ERR;
    }

    if (row > 1)
    {
        row = 1;
    }
    memset(temp, 0, sizeof(temp));
    memset(buf, 0, sizeof(buf));                                        //清临时缓冲区
    len = strlen(str);
    buf[le++] = TOPINPAD;
    buf[le++] = 0xA0;
    buf[le++] = row;                                                  //行号0-第一行
    buf[le++] = col;                                                 // 列号
//    buf[le++] = 0x08;                                                 //左对齐
    buf[le++] = atr;
    buf[le++] = 0x01;                                                //背光灯0:不亮1:亮
    memcpy(&buf[le], str, len);
    le += len;
    buf[le++] = 0x00;           //结束符防止乱码

    sdk_dev_pack_fifo(FIFO_POSTKEYPADDB, buf, le);
    timer = sdkTimerGetId();

    while (1)
    {
        if (sdk_dev_read_fifo(&fifo, 0))
        {
            if ((fifo.Cmd == FIFO_POSTKEYPADDB) && (fifo.Data[2] == 0))  //返回0表示成功，非0失败
            {
                TraceHex("ped", "data is ", fifo.Data, fifo.Len);
                return SDK_OK;
            }
        }

        if (sdkTimerIsEnd(timer, SDK_PED_TIMEOUT) == 1)
        {
            Trace("ped", "timeout!\r\n");
            return SDK_TIME_OUT;
        }
    }

    return SDK_ERR;
}


/*******************************************************************
   版          权: 新国都
   作          者: 郭泽贤
   函数名称: sdkPEDInputAmt
   函数功能: 外置密码键盘输入金额。
   输入参数:
   输出参数: 1. u8 *pDataOut: 输入的金额(8字节右对齐BCD)
   返   回  值:
                          SDK_OK: 成功
                          SDK_PARA_ERR: 参数错误
                          SDK_TIME_OUT: 密码键盘响应超时(单位: ms)
   修改备注: 密码键盘输入金额位数最大为10位。
   日期时间:2012.05.13 16:58:51
*******************************************************************/
s32 PedInputAmt(u8 *pbcDataOut)
{
    s32 rslt;
    UartSendCfg UartSend;
    UartData RecvData;
    u8 ped_ver[40] = {0};
    u8 ped_flag, i;
    u8 temp[10] = {0};
    u16 len;

    if (NULL == pbcDataOut)
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
    memset(&UartSend, 0, sizeof(UartSend));
    memset(&RecvData, 0, sizeof(RecvData));

    UartSend.RecvUartData = &RecvData;

    rslt = sdkPEDGetPedVersion(ped_ver);

    if (rslt != SDK_PARA_ERR && rslt != SDK_TIME_OUT)
    {
        if ((!memcmp(ped_ver, "V22", 3)) || (!memcmp(ped_ver, "K3306R", 6)))
        {
            UartSend.Cmd = 0xdd;
            UartSend.pDataIn = "\xB0";
            UartSend.Len = 1;       //  K3306R是文本帧
            ped_flag = 0;
        }
        else
        {
            UartSend.Cmd = 0x8d;
            UartSend.Len = 0;       // K3306H和G101是命令帧
            ped_flag = 1;
        }
    }
    else
    {
        return rslt;
    }
    rslt = SendDataToPed(&UartSend);

    if (rslt == SDK_OK)
    {
        if (ped_flag == 1)
        {
            TraceHex("ped", "3306H or G101 RecvData", RecvData.Buf, 8);

            for (i = 7; i >= 6; i--)       //G101输入小于1元的金额，会补F, 如: 0.01元，后两个字节为0F F1; 0.11元，后两个字节为0F 11; 1.11元，后两个字节为01 11
            {
                if ((RecvData.Buf[i] & 0x0f) == 0x0f)
                {
                    RecvData.Buf[i] &= 0xf0;
                }

                if (((RecvData.Buf[i] >> 4) & 0x0f) == 0x0f)
                {
                    RecvData.Buf[i] &= 0x0f;
                }
            }

            memcpy(pbcDataOut, RecvData.Buf, 8);
        }
        else
        {
            TraceHex("ped", "3306R RecvData", &RecvData.Buf[2], RecvData.Len - 2);
            sdkAscToBcdR(temp, &RecvData.Buf[2], (RecvData.Len - 2 + 1) / 2);  //sjl 2012.08.09 14:54 //07 b0 00 35 35 35 35 35 表示输入金额555.55
            len = ((RecvData.Len - 2) + 1) / 2;
            memcpy(&pbcDataOut[8 - len], temp, len);
        }
    }
    return rslt;
}


/*******************************************************************
   版          权: 新国都
   作          者: 郭泽贤
   函数名称: sdkPEDInitKey
   函数功能: 初始化密钥芯片
   输入参数: 无
   输出参数: 无
   返   回  值:
                        SDK_OK: 成功
                        SDK_ERR: 失败
                        SDK_TIME_OUT: 密码键盘响应超时(单位: ms)
   修改备注:
   日期时间:2012.05.10 14:52:40
*******************************************************************/
s32 PedInitKeyIC(void)
{
    s32 rslt;
    UartSendCfg UartSend;
    UartData RecvData;
    u8 ped_ver[40] = {0};
    u8 ped_flag = (IsWithPinpad()) ? WITHPINPAD : WITHOUTPINPAD;

    memset(&UartSend , 0 , sizeof(UartSend));

    UartSend.RecvUartData = &RecvData;
    UartSend.Cmd = 0xfd;

    if (ped_flag == WITHPINPAD)
    {
        rslt = sdkPEDGetPedVersion(ped_ver);

        if (rslt != SDK_PARA_ERR && rslt != SDK_TIME_OUT)
        {
            if ((!memcmp(ped_ver , "V22" , 3)) || (!memcmp(ped_ver , "K3306R" , 6)))
            {
                UartSend.pDataIn = "\x01";
                UartSend.Len = 1;   //  K3306R是文本帧
            }
            else
            {
                UartSend.Len = 0;   // K3306H和G101是命令帧
            }
        }
        else
        {
            return rslt;
        }
    }
    rslt = SendDataToPed(&UartSend);

    if (rslt == SDK_OK)
    {
        if (RecvData.Comm == 0xf7)
        {
            Trace("ped" , "initial key IC successfully\r\n");
            return SDK_OK;
        }
        else
        {
            return SDK_ERR;
        }
    }
    return rslt;
}
s32 Private_sdkPEDSetSN(const u8 *pDataIn)
{
    UartSendCfg UartSend;
    UartData RecvData;
//    u8 tmplen = 0;//shijianglong 2013.04.17 14:39
    u8 tmpdata[64];
    if (NULL == pDataIn || (strlen(pDataIn) != 11)||!IsWithPinpad())////shijianglong 2013.04.24 17:24
    {
        Assert(0);
        return SDK_PARA_ERR;
    }

    memset(&UartSend, 0, sizeof(UartSend));
    memset(&RecvData, 0, sizeof(RecvData));

    memset(tmpdata, 0, sizeof(tmpdata));
    memcpy(tmpdata, pDataIn, 11);
    UartSend.RecvUartData = &RecvData;
    UartSend.Cmd = 0x9f;    // 命令字
    UartSend.pDataIn = tmpdata;
    UartSend.Len = 11;//zxx 20130305 10:25黄学佳说固定11个字节

    return SendDataToPed(&UartSend);
}

