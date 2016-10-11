#include "sdkGlobal.h"


// ����λ��ͨѶ����
#define     PED_SOH                         0x01
#define     PED_ETX                          0x03
#define     PED_EOT                          0x04
#define     PED_TEXT                        0x10                // ����
#define     PED_ORDER                     0x11               // ����
#define     PED_KEY_TEXT                0X12               // ���̵�����
#define     PED_KEY_ORDER            0X13               // ���̵�����
#define     PED_EXT_TEXT                0x14               // ���ĳ���248 ��ʱ���Э��
#define     PED_KEY_EXTEXT           0x15

#define STEP_SOH                    0               // ��������ͷ/��ʶ
#define STEP_ORDER               1            // ����������
#define STEP_LEN1                  2               // ���ճ��ȵ�һλ
#define STEP_LEN2                  3         // ���ճ��ȵڶ�λ
#define STEP_DATA                  4               // ��������
#define STEP_ETX                     5               // ���ս�����
#define STEP_CS                       6               // ����У���

#define STATE_NULL            0       // û���յ�����
#define STATE_OK                1       // ���յ���ȷ������
#define STATE_CSERR          2       // ��������У���

typedef struct
{
    u8 Step;                  // ���ղ���
    u8 HeadLen;           // ����0x01����
    u8 Text;                // ����ʶ
    u8 Comm;               // ���յ�������
    u8 Ret;                    // ����״̬ 0:�����ȴ� 1:���յ���ȷ������ 2:�������ݴ���
    u8 Cs;                      // У���
    u8 Buf[512];            // ����
    u16 Len;                 // ���ݳ���
    u16 LenCun;           // ���ݽ��յ��ĳ���
} UartData;

typedef struct
{
    UartData *RecvUartData;           // ���մ������ݽṹ��
    u8 Cmd;                                             // ���͵�����
    u8 const *pDataIn;                                  // ���͵�����
    u16 Len;                                            // �������ݵĳ���
} UartSendCfg;

#define         PINPAD_ZIKU_FILE1 "/res/hz1616.bin"
#define         PINPAD_ZIKU_FILE2 "mtd0/res/hz1616.bin"


#define PEDFDISP    0x00   //����(Ĭ������)
#define PEDNOFDISP  0x01   //����
#define PEDINCOL    0x02   //����һ��
#define PEDLDISP    0x04   //�����
#define PEDCDISP    0x08   //����
#define PEDRDISP    0x10   //�Ҷ���

#define OVERLINE 0x20   //�����б�Ǻ���
#define DOWNLINE 0x40   //�����б�Ǻ���
#define SIDELINE 0x80   //�����п���


#define LCDWORDWIDTH 16 // 12 //��ʾ������
#define PEDLCDCHARWIDTH 8       // 6  //��ʾ�ַ�����


#if ((LCDWORDWIDTH == 16) && (PEDLCDCHARWIDTH == 8))
#define DOT_TABEL ASCII_8_16
#endif

#define PEDMAXCOL  128     //�������
#define PEDMAXROW  2    //5       //�������	K3102С�� G101D
#define MAX_PAGE DISPAGE3 //5 K3102 С�� 32*128

#define MAXCHAR (PEDMAXCOL / PEDLCDCHARWIDTH)   //21      //ÿ������ַ���

#define         LCD_COL_OFFSET  4 //��ʾ���ݻ�����LCD�ϵ���ƫ��

#define MAX_DOT_COL             (PEDMAXCOL + LCD_COL_OFFSET)

#define MAX_DOT_ROW             32 //64		K3102С�� 32*128

#define MAX_BUF_ROW             (MAX_DOT_ROW / 8)

u8 gG101DispRam[MAX_BUF_ROW][MAX_DOT_COL]; //��ʾ����


//--------------------------------------------------------
/*	����          :	����12                                      */
/*  ����ߣ����أ�: 8 ��16										*/
/*  ��ģ��ʽ/��С : ��ɫ����Һ����ģ������ȡģ���ֽڵ���/16�ֽ� */
u8 const ASCII_8_16[] =         //8*16 ASCII                                                    //zhuoquan 1107 ��ѧ���ṩ
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
   ��          Ȩ: �¹���
   ��          ��: ������
   ��������: FormSendDataToPed
   ��������: ��֯�������ڵ�01 Э������
   �������:
                          1. u8 order: ������
                          2. const u8 *buf: ���������
                          3. uLen: ��������ݳ���
   �������:
                          1. u8 *pDataOut: ���������
                          2. u16 *pLen: ��������ݳ���
   ��   ��  ֵ:
   �޸ı�ע:
   ����ʱ��:2012.05.11 13:38:33
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
            SendData[SendLen++] = (u8)len;                          // ���ݳ���
            bcc += (u8)len;
        }
        else
        {
            SendData[SendLen++] = (u8)(len / 0x100);                   // ���ݳ���
            bcc += (u8)(len / 0x100);
            SendData[SendLen++] = len % 0x100;                   // ���ݳ���
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
   ��          Ȩ: �¹���
   ��          ��: ������
   ��������: ParseDataFromPed
   ��������: ������������
   �������: 1. QUEUE *queue: Ҫ�������ݵĶ���
   �������: 1. UartData *RecvUartData: ��������Ľ��
   ��   ��  ֵ:
                            SDK_OK: ������ȷ
                            SDK_ERR: ��������
                            SDK_ESC: �����˳�
   �޸ı�ע:
   ����ʱ��:2012.05.11 13:39:05
*******************************************************************/
static s32 ParseDataFromPed(UartData *RecvUartData, SDK_QUEUE *queue)
{
    u8 data;
    u32 head;

//    memset(RecvUartData, 0, sizeof(UartData));    //zxx 20130131 11:14 ���ﲻ��ʼ��
    head = queue->siHead;                                                             // ��ֵ��ʼ����ѯͷ

    while (sdkTryQueueData(queue, head, &data))                        // ȡ��������
    {
        head = (head + 1) % QUEUE_SIZE;                                                   // ��ѯͷǰ��
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
                    sdkSetQueueHead(queue, head);                            // �������ö���ͷ
                }
                RecvUartData->Ret = STATE_NULL;
                break;

            case STEP_ORDER:
                RecvUartData->Cs += data;
                RecvUartData->Comm = data;                              // ������

                if (RecvUartData->Text == PED_KEY_TEXT || RecvUartData->Text == PED_KEY_EXTEXT)
                {
                    RecvUartData->Step = STEP_LEN1;
                }
                else if (RecvUartData->Text == PED_KEY_ORDER)
                {
                    // RecvUartData->Step = STEP_ETX;
                    RecvUartData->Step = STEP_CS;
                }
                RecvUartData->LenCun = 0;                            // �ѽ��յ����ݳ���
                break;

            case STEP_LEN1:                                    // ���ճ��ȵ�һλ
                RecvUartData->Cs += data;
                RecvUartData->Len = (u16)data;                          // Ӧ���յ����ݳ���

                if (RecvUartData->Text == PED_KEY_TEXT)
                {
                    RecvUartData->Step = STEP_DATA;                              // ��������
                }
                else if (RecvUartData->Text == PED_KEY_EXTEXT)
                {
                    RecvUartData->Step = STEP_LEN2;                              // ���ճ���
                }
                break;

            case STEP_LEN2:                                     // ���ճ��ȵڶ�λ
                RecvUartData->Cs += data;
                RecvUartData->Len = (u16)(RecvUartData->Len * 0x100 + data);                         // Ӧ���յ����ݳ���

                if (RecvUartData->Text == PED_KEY_EXTEXT)
                {
                    RecvUartData->Step = STEP_DATA;                               // ���ճ���
                }
                break;

            case STEP_DATA:                                       // ��������
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
                    sdkSetQueueHead(queue, head);                            // �������ö���ͷ
                }
                break;

            case STEP_CS:
                sdkSetQueueHead(queue, head);                        // �������ö���ͷ

                if (RecvUartData->Cs == data)
                {
                    RecvUartData->Ret = STATE_OK;                        // ������ȷ
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
                    RecvUartData->Ret = STATE_CSERR;                       // ���ݴ���
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
   ��          Ȩ: �¹���
   ��          ��: ������
   ��������: RecvDataFromPed
   ��������: ���ڽ�������
   �������: 1. u8 uSendCmd: ���͵�������
   �������: 1.UartData *pRecvData: �����õ�������
   ��   ��  ֵ:
                            SDK_OK: ����������ȷ
                            SDK_ERR: �������ݴ���
                            SDK_TIME_OUT: ���ճ�ʱ
   �޸ı�ע:
   ����ʱ��:2012.05.11 13:40:58
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
            if (sdkTimerIsEnd(timerID, SDK_PED_TIMEOUT * 15) == 1)           // �����ʱʱ��60�뵽
            {
                return SDK_TIME_OUT;
            }
        }
        else
        {
            if (sdkTimerIsEnd(timerID, SDK_PED_TIMEOUT) == 1) // ��ʱʱ��4�뵽
            {
                return SDK_TIME_OUT;
            }
        }

        if (sdk_dev_read_fifo(&fifo, 0))
        {
            if (fifo.Cmd == FIFO_RECCOMTRANSP)
            {
                sdkInsertQueue(&ComQueue, fifo.Data, fifo.Len);                    // ������ѹ�����
                TraceHex("ped", "receive data from PED", fifo.Data, fifo.Len);
                rslt = ParseDataFromPed(pRecvData, &ComQueue);
                //------------------------------//zxx 20130131 11:22
                if (rslt == SDK_ESC)
                {
                    memset(&ComQueue, 0, sizeof(ComQueue));     //������ȡ����֮ǰ�Ķ���
                    continue;          //����sdk_esc˵������δ���꣬������ȡ����
                }
                //------------------------------//
                if (rslt == SDK_OK)
                {
                    //zxx 3.0 20130301 16:24�ƶ�ParseDataFromPed���洦�����ﲻ֪���������Ѿ��յ���һ���˻���û���յ����Ƿ�Ҫ��fifo��ȷ��
                    if (pRecvData->Comm == 0x20)        //��ʾ��������Ѿ��ɹ�����ָ��
                    {
                        Trace("ped", "ped dataok,continue recv\r\n");
                        memset(pRecvData, 0, sizeof(UartData)); //zxx 20130131 14:9
                        continue;
                    }

//                    else if (pRecvData->Comm == 0xf7 // ��ʼ����ԿоƬ
//                             || pRecvData->Comm == 0x83 //�ش����(���3306H��G101)
//                             || pRecvData->Comm == 0xdd) //�ش����(���3306R)
//                    {
//                        return SDK_OK;
//                    }
//                    else        //���������Ӧ���ߵ�����
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
   ��          Ȩ: �¹���
   ��          ��: ������
   ��������: SendDataToPed
   ��������: �������ݸ��������
   �������: 1. const UartSendCfg *UartSend: ��Ҫ���͵�����
   �������: ��
   ��   ��  ֵ:
                            SDK_OK: ����������ȷ
                            SDK_ERR: �������ݴ���
                            SDK_TIME_OUT: ���ճ�ʱ
   �޸ı�ע:
   ����ʱ��:2012.05.11 13:41:04
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
   ��������:
   ��������: ��ȡ���ֵ�������
   ��ص���:
   ��ڲ���:    buf:ȡ���ĵ�������
                        offset:ƫ����
   �� �� ֵ:	0 �ɹ�
                        ��0 ʧ��
   ��    ע:
   ������Ϣ:   ׿��
   �޸���Ϣ:

                ��ѧ�ѣ�2012.11.02 ���Զ�ȡ�ļ�����ȡ���󴫸��������
 ********************************************************************/
s32  G101_get_arr(u8 *buf, u32 offset)
{
    int fontfd = -1;
    u32 xlen = 16;
    u32 yhigh = 16;
    u32 fontsize = 0;

    //�򿪵�ǰ���ֿ��ļ�


    //���ļ�
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
    //һ��������ռ�ֽ���
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
        ֧��12����48����ת��

   ������
        width �ֿ�
        height �ָ�
        h2v  =1 H->V; ���,����ҵͣ�����->����->����->����==>�ݿ�,�¸��ϵͣ�����-->����-->����-->����
                 =0 V->H  �ݿ�,�¸��ϵͣ�����-->����-->����-->����==>��⣬����ҵͣ�����->����->����->����
                 =2 H->H;(���,����ҵͣ��������£���������)==>��⣬����ҵͣ�����->����->����->����  ��Ȳ���1Byte,��0
        dat ת������
   RETURN:
        =0 FAIL
        =1 SUCCESS
   �㷨�������������꣬��ת��

        ��ѧ�ѣ�2012.11.05 �Ӵ�ӡ�������п������������ڴ�����G101�ĵ���
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
        return 0;                                  //����ֻ֧�ֲ�����48X48����
    }
    memset(srcbuf, 0, sizeof(srcbuf));

    //dot = (char*)malloc(512);
    //��⣬����ҵͣ�����->����->����->����  ��Ȳ���1Byte,��0
    //�ݿ�,�¸��ϵͣ�����-->����-->����-->����
    if (1 == h2v)        //H->V;  ��ת��
    {
        //δ����֤  ????
        //���
        old_widthbyte  = (width + 7) >> 3;
        old_heightbyte = height;
        //�ݿ�
        new_widthbyte = width;
        new_heightbyte = (height + 7) >> 3;
        //����
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
#if 0 //��ѧ�� 2012.11.05
    else if (0 == h2v)               //V->H;  ��ת��
    {
        //�ݿ�
        old_widthbyte = width;
        old_heightbyte = (height + 7) >> 3;
        //���
        new_widthbyte = (width + 7) >> 3;
        new_heightbyte = height;
        //����
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
                old_mask <<= 1;                //�ϵ��¸�
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
    else if (2 == h2v)              //���ת���
    {
        //���
        old_widthbyte  = (width + 7) >> 3;
        old_heightbyte = height;
        //����
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

    if (ch[0] < 0x80)           // ASCII�ַ�
    {
        // ����洢��ַ
        the_dot_addr = (ch[0] - 0x20) * 2 * PEDLCDCHARWIDTH;         //+ ZIKU12ADD;

        for (i = 0; i < 2 * PEDLCDCHARWIDTH; i++)
        {
            *DataPtr++ = DOT_TABEL[the_dot_addr + i];
        }
    }
    else        // �����ַ�
    {
        //�����ƫ��
        offset = 0x00;
        offset = *ch - 0xa1;
        offset = offset * 94;
        offset = offset + *(ch + 1) - 0xa1;
        offset = offset * 32;

        if (0 == G101_get_arr(DataPtr, offset))
        {
            G101_LCD_FontHVChange(16, 16, 1, DataPtr);            //ת�����ϣ����ϣ����£����µ��ݿ�
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
   ��������: void dev_lcd_clear_ram(void)
   ��������: �����ʾ�ڴ�
   ��ص���: gDispRam ��ʾ����
   ��ڲ���: ��
   �� �� ֵ: ��
   ��    ע:
   ������Ϣ:	zhuoquan                11/07
   �޸���Ϣ:
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
        colid = 0;                        //����128
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

    dot_col = colid;                       //����
    //temp1 = 0;		   //����
    Hzlen = 0;                             //���ָ���

    for (i = 0; i < no;)
    {
        if (str[i] < 0x80)               //ASCII���16*8
        {
            dot_col += PEDLCDCHARWIDTH;

            if (dot_col > PEDMAXCOL)
            {
                break;
            }
        }
        else                                     //���֣�16*16
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
            colid = 0;            //����
        }
    }
    else
    {
        if (colid + no * PEDLCDCHARWIDTH > PEDMAXCOL)
        {
            colid = 0;            //����
        }
    }
    t_atr = atr & 0x1C;    //(LDISP | CDISP | RDISP)

    switch (t_atr)
    {
        case PEDCDISP:       //����

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

        case PEDRDISP:       //�Ҷ���

            if (atr & PEDINCOL)
            {
                colid = PEDMAXCOL       - (no * PEDLCDCHARWIDTH + Hzlen);
            }
            else
            {
                colid = PEDMAXCOL       - no * PEDLCDCHARWIDTH;
            }
            break;

        default:       //�����
            break;
    }

    colid += LCD_COL_OFFSET;

    j = (rowid * LCDWORDWIDTH);   // LCD���е������
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
        if (str[digcount] < 0x80)        //asc �ַ�
        {
            //��ѧ�� 2012.07.04 �Ǻ�ʹ��С����

            // ȡ�ֿ����
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

                if ((atr & PEDNOFDISP) != PEDNOFDISP)                              //����
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
                        if ((atr & PEDNOFDISP) == PEDNOFDISP)                                   //����
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

    //������ӱ߿�Ĵ���
    if (atr & SIDELINE)   //�����ҿ�
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

    //��������
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
  **Description:		��ȡʹ��DD 76����͸�������̵���������
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

    rbuf[0] = (0x03);       //���
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
** Descriptions:	        ���Ƽ�����ʾ
** Parameters:          str: ��ʾ���ַ���
                                line: ��ʾ����
                                col : ��ʾ����
                                atr:
** Returned value:
** Created By:		׿��  2012.11.07
** Remarks:
*****************************************************************************/
s32 PedDisplayStr(u8 *str, u8 row, u8 col, u8 atr)
{
    u16 ret = 0;//shijianglong 2013.01.09 15:54ȥ���ò�����i
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
    G101_lcd_clear_ram();                                                            //�����ʾ�ڴ�
    G101_lcd_fill_rowram(row, col, str, atr);                                //���RAM
    DotLen = G101_lcd_brush_screen_m(DotData);                       //�õ�����

//    for(i = 0; i < DotLen; i++)
//    {
//        printf("%02x ", DotData[i]);
//    }
    TraceHex("ped", "pedData", DotData, DotLen); //shiweisong 2013.01.08 19:22
    sdk_dev_pack_fifo(FIFO_BRIDGEENA, "", 0);                                            //���Ž�
    Sendbuf[len++] = TOPINPAD;
    Sendbuf[len++] = 0xDB;                                                  //DBָ��
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

    sdk_dev_pack_fifo(FIFO_BRIDGEDIS, "", 0);                                                    //�ر��Ž�

    if (ret)
    {
        return SDK_TIME_OUT;
    }
    return SDK_OK;
}


/*****************************************************************************
** Descriptions:	        ���������Ϣ��ʾ
** Parameters:          str: ��ʾ���ַ���
                                row: ��ʾ����
                                col : ��ʾ����
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
    memset(buf, 0, sizeof(buf));                                        //����ʱ������
    len = strlen(str);
    buf[le++] = TOPINPAD;
    buf[le++] = 0xA0;
    buf[le++] = row;                                                  //�к�0-��һ��
    buf[le++] = col;                                                 // �к�
//    buf[le++] = 0x08;                                                 //�����
    buf[le++] = atr;
    buf[le++] = 0x01;                                                //�����0:����1:��
    memcpy(&buf[le], str, len);
    le += len;
    buf[le++] = 0x00;           //��������ֹ����

    sdk_dev_pack_fifo(FIFO_POSTKEYPADDB, buf, le);
    timer = sdkTimerGetId();

    while (1)
    {
        if (sdk_dev_read_fifo(&fifo, 0))
        {
            if ((fifo.Cmd == FIFO_POSTKEYPADDB) && (fifo.Data[2] == 0))  //����0��ʾ�ɹ�����0ʧ��
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
   ��          Ȩ: �¹���
   ��          ��: ������
   ��������: sdkPEDInputAmt
   ��������: ����������������
   �������:
   �������: 1. u8 *pDataOut: ����Ľ��(8�ֽ��Ҷ���BCD)
   ��   ��  ֵ:
                          SDK_OK: �ɹ�
                          SDK_PARA_ERR: ��������
                          SDK_TIME_OUT: ���������Ӧ��ʱ(��λ: ms)
   �޸ı�ע: �������������λ�����Ϊ10λ��
   ����ʱ��:2012.05.13 16:58:51
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
            UartSend.Len = 1;       //  K3306R���ı�֡
            ped_flag = 0;
        }
        else
        {
            UartSend.Cmd = 0x8d;
            UartSend.Len = 0;       // K3306H��G101������֡
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

            for (i = 7; i >= 6; i--)       //G101����С��1Ԫ�Ľ��ᲹF, ��: 0.01Ԫ���������ֽ�Ϊ0F F1; 0.11Ԫ���������ֽ�Ϊ0F 11; 1.11Ԫ���������ֽ�Ϊ01 11
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
            sdkAscToBcdR(temp, &RecvData.Buf[2], (RecvData.Len - 2 + 1) / 2);  //sjl 2012.08.09 14:54 //07 b0 00 35 35 35 35 35 ��ʾ������555.55
            len = ((RecvData.Len - 2) + 1) / 2;
            memcpy(&pbcDataOut[8 - len], temp, len);
        }
    }
    return rslt;
}


/*******************************************************************
   ��          Ȩ: �¹���
   ��          ��: ������
   ��������: sdkPEDInitKey
   ��������: ��ʼ����ԿоƬ
   �������: ��
   �������: ��
   ��   ��  ֵ:
                        SDK_OK: �ɹ�
                        SDK_ERR: ʧ��
                        SDK_TIME_OUT: ���������Ӧ��ʱ(��λ: ms)
   �޸ı�ע:
   ����ʱ��:2012.05.10 14:52:40
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
                UartSend.Len = 1;   //  K3306R���ı�֡
            }
            else
            {
                UartSend.Len = 0;   // K3306H��G101������֡
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
    UartSend.Cmd = 0x9f;    // ������
    UartSend.pDataIn = tmpdata;
    UartSend.Len = 11;//zxx 20130305 10:25��ѧ��˵�̶�11���ֽ�

    return SendDataToPed(&UartSend);
}

