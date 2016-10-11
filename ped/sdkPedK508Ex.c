#include "sdkGlobal.h"
/*为了实现改造后的电话POS主密钥下载方案，sdkped.c里边需要添加和修改的接口如下：

（注：为了实现01协议的解析，SDK使用到了sdk0102Frame.c中的接口sdkCommRev01And02Fram）


一、需要添加的接口
*/
 
//=================================================================//
/****************************电话POS下载主密钥相关******************/
//=================================================================//

#define USERDATEMAXNUM     88 //用户自定义区大小
#define USERDATEFILE       "SafeModeNoFile"  //用户自定义区文件名

/********************************************************************
函数功能: 保存用户自定义区数据--内置键盘
输入参数:
输出参数:
返 回 值:
修改信息:
********************************************************************/
s32 sdk_ped_save_pinpad_user_date(u8 *pasIn, s32 uiInLen)
{
    u8 fn[64],gasCurAppDir[64];
    u8 FileData[128];

    if(NULL == pasIn || uiInLen > USERDATEMAXNUM || uiInLen<=0)
    {
        return SDK_ERR;
    }

    memset(fn,0,sizeof(fn));
    memset(gasCurAppDir,0,sizeof(fn));
    sdkSysGetCurAppDir(gasCurAppDir);
    strcpy(fn,gasCurAppDir);
    strcat(fn,USERDATEFILE);
    memset(FileData,0,sizeof(FileData));
    memcpy(FileData,pasIn,uiInLen);

    if(sdkWriteFile(fn,FileData,uiInLen) == SDK_FILE_OK)
    {
        return SDK_OK;
    }
    return SDK_ERR;
}

/********************************************************************
函数功能: 读取自定义区内容--内置键盘
输入参数:
输出参数:
返 回 值:
修改信息:
********************************************************************/
s32 sdk_ped_read_pinpad_user_date(u8 *pasOut,s32 *uiOutLen)
{
    u8 fn[64],gasCurAppDir[64];

    if(NULL == pasOut || *uiOutLen<=0 ||*uiOutLen>USERDATEMAXNUM)
    {
        return SDK_ERR;
    }

    memset(fn,0,sizeof(fn));
    memset(gasCurAppDir,0,sizeof(fn));
    sdkSysGetCurAppDir(gasCurAppDir);
    strcpy(fn,gasCurAppDir);
    strcat(fn,USERDATEFILE);

    if(sdkReadFile(fn, pasOut,0,uiOutLen) == SDK_FILE_OK)
    {
        TraceHex("AppDebug", "读取内置自定义区的内容", pasOut, *uiOutLen);
        return SDK_OK;
    }
    else
    {
        *uiOutLen=0;
        return SDK_ERR;
    }
}

/********************************************************************
函数功能: OK帧
输入参数:
输出参数:
返 回 值:
修改信息: 2015-08-21  卢裕燕  创建
********************************************************************/
static void SendDataOK(void)
{
    sdkCommUartSendData(sdkSysGetComNo(SDK_SYS_COM_PIN_PAD),"\x01\x01\x01\x01\x01\x13\x20\x34",8);
}

/********************************************************************
函数功能: 失败帧
输入参数:
输出参数:
返 回 值:
修改信息: 2015-08-21  卢裕燕  创建
********************************************************************/
static void SendDataErr(void)
{
    sdkCommUartSendData(sdkSysGetComNo(SDK_SYS_COM_PIN_PAD),"\x01\x01\x01\x01\x01\x13\x2f\x43",8);
}

/********************************************************************
函数功能:发送数据到密钥POS
输入参数:
输出参数:
返 回 值:
修改信息: 2015-04-14  张文晖  创建
********************************************************************/
static s32 SendDataToKeyPos(u8 *heBuf,u16 unLen)
{
    u8 heRecvBuf[128]= {0};
    u8 len=0;

    memset(heRecvBuf,0x00,sizeof(heRecvBuf));
    memcpy(&heRecvBuf[len],"\x01\x01\x01\x01",4);//同步头
    len+=4;
    heRecvBuf[len++] = 0x01;//01协议
    memcpy(&heRecvBuf[len],heBuf,unLen);
    len+=unLen;
    heRecvBuf[len++] = 0x03;//结束标志字节
    heRecvBuf[len] = sdkCalcCUSUM(&heRecvBuf[4],len-4);
    len+=1;

    sdkCommUartSendData(sdkSysGetComNo(SDK_SYS_COM_PIN_PAD),heRecvBuf,len);
    return SDK_OK;
}

/********************************************************************
函数功能: 解析密钥POS命令数据
输入参数:
输出参数:
返 回 值:
备    注:主要处理8C、EE、FB、DB01、DB03、DB04指令
修改信息: 2015-04-08  张文晖  创建
********************************************************************/
static s32 PraseKeyPosData(u8 ucFrameId,u16 ucCmd,u8 *heRecvBuf,s32 unRecvLen)
{
    const u8 StaticKey[8]= {0x36,0x38,0x34,0x38,0x32,0x35,0x36,0x39};//68482569 固定密钥,用于主密钥加密传输解密的情况
    u8 Bufferkey[8]= {0};
    static u8 EncryMode = 0xff;
    u8 temp[128]= {0},SData[128]= {0};
    u8 heTmk[16];
    s32 ret=0;
    u32 uiTmkIndex = 0;
    s32 rlen = 0;

    memset(heTmk,0x00,sizeof(heTmk));
    switch(ucCmd)
    {
        case 0x8C: //取消密码键盘指令
        {
            Trace("AppDebug","PraseKeyPosData() case 0x8C\r\n");
            sdkSysBeep(SDK_SYS_BEEP_OK);
            SendDataOK();
            break;
        }
        case 0xEE: //主密钥更新方式
        {
            Trace("AppDebug","PraseKeyPosData() case 0xEE\r\n传输模式:%x\r\n",heRecvBuf[0]);
            TraceHex("AppDebug","传输密钥\r\n",&heRecvBuf[1],8);
            memcpy(Bufferkey,&heRecvBuf[1],8);
            EncryMode = heRecvBuf[0];
            sdkSysBeep(SDK_SYS_BEEP_OK);
            SendDataOK();
            break;
        }
        case 0xFB://更新单个主密钥
        {
            Trace("AppDebug","PraseKeyPosData() case 0xFB\r\n");
            Trace("AppDebug","TMK 索引:%d\r\n",heRecvBuf[0]);
            TraceHex("AppDebug","TMK 密文:%d\r\n",&heRecvBuf[1],unRecvLen-1);

            uiTmkIndex = heRecvBuf[0];
            if(uiTmkIndex > 101)
            {
                //母POS下发的是3DES偏移量，需减去该偏移量
                uiTmkIndex = uiTmkIndex - 102;
            }
            memset(heTmk,0,sizeof(heTmk));
            if(unRecvLen == 0x09)
            {
                memcpy(heTmk, &heRecvBuf[1],8);

                //如果主密钥是加密传统
                if(EncryMode == 0x03)
                {
                    sdkDesS(0, heTmk, StaticKey);
                }
            }
            else if(unRecvLen == 0x11)
            {
                memcpy(heTmk, &heRecvBuf[1],16);

                //如果主密钥是加密传输
                if(EncryMode == 0x03)
                {
                    sdkDesS(0, heTmk, StaticKey);
                    sdkDesS(0, &heTmk[8], StaticKey);
                }
            }
            Trace("AppDebug","TMK uiTmkIndex:%d\r\n",uiTmkIndex);
            TraceHex("AppDebug","TMK 明文:%d\r\n",heTmk,unRecvLen-1);

            //更新主密钥
            ret = sdkPEDUpdateTmk(uiTmkIndex, SDK_PED_DES_TRIPLE, heTmk, SDK_PED_TIMEOUT);
            if(ret == SDK_OK)
            {
                SendDataOK();
                sdkSysBeep(SDK_SYS_BEEP_OK);
                Trace("AppDebug","更新TMK成功!\r\n");
                return SDK_OK;
            }
            else
            {
                SendDataErr();
                sdkSysBeep(SDK_SYS_BEEP_ERR);
                Trace("AppDebug","更新TMK失败!\r\n");
                return SDK_ERR;
            }
            break;
        }

        //====================DB指令======================/
        case 0x01: //读取序列号,用于读取用户自定义区的内容
        {
            SendDataOK();
            memset(temp,0,sizeof(temp));
            memset(SData,0,sizeof(SData));
            memcpy(SData,"\x12\xDB\x12\x01",4);

            //获取读取自定义区的长度
            rlen = heRecvBuf[1];

            //读取用户自定义区的内容
            if(sdk_ped_read_pinpad_user_date(temp,&rlen)==SDK_OK)
            {
                SData[4]=0x00;
            }
            else
            {
                SData[4]=0x01; //失败
                memset(temp,0,sizeof(temp));
            }
            memcpy(&SData[5],temp,rlen);
            SendDataToKeyPos(SData, 5+rlen);
            break;
        }
        case 0x03://下载序列号，用于写用户自定义区
        {
            SendDataOK();
            memset(SData,0,sizeof(SData));
            memcpy(SData,"\x12\xDB\x02\x03",4);
            if(sdk_ped_save_pinpad_user_date(&heRecvBuf[1], unRecvLen-1)==SDK_OK)
            {
                SData[4]=0x00;
            }
            else
            {
                SData[4]=0x01;
            }
            SendDataToKeyPos(SData, 5);
            break;
        }
        case 0x04://读取机器出厂序列号(SN)
        {
            SendDataOK();
            memset(temp,0,sizeof(temp));
            memset(SData,0,sizeof(SData));
            memcpy(SData,"\x12\xDB\x12\x04",4);
            if(sdkReadPosSn(temp)==SDK_OK)
            {
                SData[4]=0x00;
            }
            else
            {
                SData[4]=0x01;
            }
            memcpy(&SData[5],temp,11);
            SendDataToKeyPos(SData, 16);
            break;
        }
        default:
             break;
    }
    return SDK_OK;
}

/********************************************************************
函数功能:接收母POS的指令--01协议
输入参数:
            u8 *ucFrameId   帧ID
            u8 *uiCmd       指令
            u32 iTimeOut    超时时间
输出参数:
            u8 *heRecvBuf   有效数据
            s32 *usLen      有效数据长度
返 回 值:
修改信息: 2014-11-26  张文晖  创建
********************************************************************/
static s32 ResvDataFromKeyPos(u8 *ucFrameId,u16 *ucCmd,u8 *heRecvBuf,u16 *usLen)
{
    u8 rbuf[1024] = {0};
    s32 rlen = 0;
    u16 dccmdaddress = 0;

    *ucFrameId = 0;
    *ucCmd = 0;

    //设置为true会吃掉按键
    rlen = sdkCommRev01And02Fram(SDK_SYS_COM_PIN_PAD, rbuf, sizeof(rbuf), 1, false);

    if(rlen > 0)
    {
        TraceHex("zwh", "resv ok \r\n", rbuf, rlen);
        if(rbuf[0] == 0x01)     //判断是01协议
        {
            *ucFrameId = rbuf[1];
            *ucCmd = rbuf[2];

            if(*ucFrameId == 0x15)//扩展帧
            {
                *usLen = rbuf[3] * 256 + rbuf[4];

                if( *ucCmd == 0xDC)
                {
                    //协议头1个字节+帧标识1个字节 +DC命令字1个字节 +长度2个字节+ 控制域长度1个字节+控制域数据n字节+1
                    dccmdaddress = 5 + rbuf[5] + 1;
                    *ucCmd = ((rbuf[dccmdaddress] & 0xFF) << 8) | (rbuf[dccmdaddress + 1]);
                }
                else
                {
                    if(*ucCmd == 0xDD || *ucCmd == 0xDB )
                    {
                        *ucCmd = rbuf[5];        //二级Ext_cmd命令
                    }
                }
                memcpy(heRecvBuf, &rbuf[5], *usLen);
            }
            else if(*ucFrameId == 0x13) //上帧命令执行正确，不处理，有时候就需要返回这个
            {
                //命令帧
            }
            else
            {
                //正文帧
                *usLen = rbuf[3];

                if(*ucCmd == 0xDC )
                {
                    //协议头1个字节+帧标识1个字节 +DC命令字1个字节 +长度1个字节+ 控制域长度n个字节+1
                    dccmdaddress = 4 + rbuf[4] + 1;
                    *ucCmd = ((rbuf[dccmdaddress] & 0xFF) << 8) | (rbuf[dccmdaddress + 1]);
                }
                else
                {
                    if(*ucCmd == 0xDD || *ucCmd == 0xDB ) //
                    {
                        *ucCmd = rbuf[4];      //二级Ext_cmd命令
                    }
                }
                memcpy(heRecvBuf, &rbuf[4], *usLen);//带2级命令出去
            }
            TraceHex("zwh", "heRecvBuf", heRecvBuf, *usLen);
            Trace("zwh", "*usLen = %d\r\n", *usLen);
            Trace("zwh", "*uiCmd = %02x\r\n", *ucCmd);
            Trace("zwh", "数据正确--0101010101132034\r\n");
            return SDK_OK;
        }
    }

    return SDK_ESC;
}

/*******************************************************************
作	  者: 钟辉
版	  权: 深圳新国都股份有限公司
函数功能: 下载主密钥
入口参数:
返 回 值:
备	  注:
********************************************************************/
s32 sdkPEDImportTMKData(void)
{
    u8 rbuf[1024] = {0};
    s32 rlen = 0;
    u16 ucCmd=0;
    u8 ucFrameId = 0;
    s32 ret=0;

    memset(rbuf,0x00,sizeof(rbuf));
    ret = ResvDataFromKeyPos(&ucFrameId, &ucCmd, rbuf, &rlen);
    if(ret == SDK_OK)
    {
        TraceHex("AppDebug","队列里的所有接收的数据:\r\n",rbuf,rlen);
        Trace("AppDebug","命令uiCmd:%x\r\n",ucCmd);

        ret=PraseKeyPosData(ucFrameId,ucCmd, rbuf, rlen);
        Trace("AppDebug","PraseData--i_rslt:%d\r\n",ret);
    }
    return ret;
}
 