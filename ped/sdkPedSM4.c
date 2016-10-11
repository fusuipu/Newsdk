#include "sdkGlobal.h"

/*=======BEGIN: zhouzhihua 2013.07.25  15:2 modify===========*/
#define DC_CMD_CTRL_RES "\x11"  //dc指令中的控制域的数据
//DC指令中子命令
#define DC_CMD_EX_DUKPT_INIT  0x1011     //dukpt初始化
#define DC_CMD_EX_DUKPT_GET_PIN  0x1012  //获得密码
#define DC_CMD_EX_DUKPT_GET_MAC  0x1013  //获得mac
#define SDK_DUKPT_KSN_LEN 10
#define SDK_DUKPT_KEY_LEN 16

/*****************************************************************************
** Descriptions:	内置DUKPT初始化
** Parameters: nMode：初始化模式，该参数暂未使用，目前调用函数时该参数可传入0
          uInitKeyType： 0 --- mackey
                         1 --- pinkey
                         2 --- dkey
          uKeyIndex：pinpad中保存的初始密钥索引号，范围0~9
          upInitKey：传入的初始密钥数据
          uInitKeyLen：初始密钥数据长度，规定为16字节
          uKsnIndex：pinpad中保存的初始KSN索引号，范围0~9
          upInitKsn：传入的初始KSN数据
          uInitKsnLen：初始KSN数据长度，规定为10字节
          siTimerOut：超时时间（单位ms），该参数暂未使用，目前调用函数时该参数可传入0
   output：无
   return： 0 --- 成功
   其他值 --- 失败
   注意：目前nMode和siTimerOut参数都未使用到，调用函数时都可传入0。而且目前来说uKsnIndex和uKeyIndex传入的参数值都是一样的。
** Created By:		fusuipu  2014.06.13
** Remarks:             外置DUKTP初始化使用FIFO指令，内置直接调用libdev.so的接口
*****************************************************************************/
static s32 sdkPedDukptInnerLoadInitKey(u8 nMode, u8 uInitKeyType, u8 * uKeyIndex, u8 * upInitKey, u8 uInitKeyLen, u8 * uKsnIndex, u8 * upInitKsn, u8 uInitKsnLen, s32 siTimerOut)
{
    static s32 (*p_fun)(u8 nMode, u8 uInitKeyType, u8 *uKeyIndex, u8 *upInitKey, u8 uInitKeyLen, u8 *uKsnIndex, u8 *upInitKsn, u8 uInitKsnLen, s32 siTimerOut) = NULL;
    s32 ret = 0;

    if(p_fun == NULL)
    {
        p_fun =  (s32 (*)(u8, u8, u8 *, u8 *, u8, u8 *, u8 *, u8, s32))sdk_dev_get_libdev_fun_hand("PedDukptLoadInitKey");
    }

    if(p_fun != NULL)
    {
        ret =  p_fun(nMode, uInitKeyType, uKeyIndex, upInitKey, uInitKeyLen, uKsnIndex, upInitKsn, uInitKsnLen, siTimerOut);

        if(0 == ret)
        {
            return SDK_OK;
        }
        else
        {
            return SDK_ERR;
        }
    }
    else
    {
        Trace("dev", "warming: PedDukptLoadInitKey");
        return SDK_ERR;
    }
}

/*****************************************************************************
** Descriptions:使用dukpt密钥getpin
** input：      ucMode：0 --- 正常getpin模式
                        1 --- 测试模式，只需输入pin一次
                upPan：panblock，长度6字节
                upPwdLen：输入pin长度范围，两字节upPwdLen[0] --- 最小长度
                upPwdLen[1] --- 最大长度
                uKsnIndx：KSN索引号
                siTimerOut：超时时间，单位ms
** output：     upPinBlock：getpin结果，长度8字节
                upKsn：KSN，长度10字节
                upKsnLen：KSN长度
** return：     0 --- 成功
                其他值 --- 失败
   注意：目前参数siTimerOut暂未使用，调用函数时该参数可传入0。

** Created By:		fusuipu  2014.06.13
** Remarks:
*****************************************************************************/
static s32 sdkPedDukptInnerGetPinFun(u8 ucMode, u8 * upPan, u8 * upPwdLen, u8 * upPinBlock, u8 uKsnIndx, u8 * upKsn, u8 * upKsnLen, s32 siTimerOut)
{
    static s32 (*p_fun)(u8 ucMode, u8 *upPan, u8 *upPwdLen, u8 *upPinBlock, u8 uKsnIndx, u8 *upKsn, u8 *upKsnLen, s32 siTimerOut) = NULL;

    if(p_fun == NULL)
    {
        p_fun =  (s32 (*)(u8, u8 *, u8 *, u8 *, u8, u8 *, u8 *, s32))sdk_dev_get_libdev_fun_hand("PedDukptGetPin");
    }

    if(p_fun != NULL)
    {
        return p_fun(ucMode, upPan, upPwdLen, upPinBlock, uKsnIndx, upKsn, upKsnLen, siTimerOut);
    }
    else
    {
        Trace("dev", "warming: PedDukptGetPin");
        return SDK_ERR;
    }
}

/*****************************************************************************

** Descriptions:        内置使用dukpt密钥getmac
** Parameters:          upData：要加密的数据
                        uspDatalen：要加密的数据长度
                        upDukptMode：   0 --- 如果调用该接口getmac之前，有过用dukpt密钥getpin，则使用之前getpin的dukpt密钥来getmac；如果没有，则使用新的dukpt密钥来getmac；
                                        1 --- 使用新的dukpt密钥getmac
                        uKsnIndx： KSN索引号
                        siTimerOut：超时时间，单位ms
   output：                upMac：getmac结果，长度8字节
                        upKsn：KSN，长度10字节
                        upKsnLen：KSN长度
   return:                 0 --- 成功
                        其他值 --- 失败
   注意：目前参数siTimerOut暂未使用，调用函数时该参数可传入0。

** Returned value:
** Created By:		fusuipu  2014.06.13
** Remarks:
*****************************************************************************/
static s32 sdkPedDukptInnerGetMac(u8 * upData, u16 uspDatalen, u8 * upDukptMode, u8 * upMac, u8 uKsnIndx, u8 * upKsn, u8 * upKsnLen, s32 siTimerOut)
{
    static s32 (*p_fun)(u8 *, u16, u8 *, u8 *, u8, u8 *, u8 *, s32) = NULL;

    if(p_fun == NULL)
    {
        p_fun =  (s32 (*)(u8 *, u16, u8 *, u8 *, u8, u8 *, u8 *, s32))sdk_dev_get_libdev_fun_hand("PedDukptGetMac");
    }

    if(p_fun != NULL)
    {
        return p_fun(upData, uspDatalen, upDukptMode, upMac, uKsnIndx, upKsn, upKsnLen, siTimerOut);
    }
    else
    {
        Trace("dev", "warming: PedDukptGetPin");
        return SDK_ERR;
    }
}
/*****************************************************************************

** Descriptions:        内置使用dukpt密钥getmac
** Parameters:          upData：要加密的数据
                        uspDatalen：要加密的数据长度
                        upDukptMode：   0 --- 如果调用该接口getmac之前，有过用dukpt密钥getpin，则使用之前getpin的dukpt密钥来getmac；如果没有，则使用新的dukpt密钥来getmac；
                                        1 --- 使用新的dukpt密钥getmac
                        ucAlgo		扩展算法类型
                        uKsnIndx： KSN索引号
                        siTimerOut：超时时间，单位ms
   output：                upMac：getmac结果，长度8字节
                        upKsn：KSN，长度10字节
                        upKsnLen：KSN长度
   return:                 0 --- 成功
                        其他值 --- 失败
   注意：目前参数siTimerOut暂未使用，调用函数时该参数可传入0。

** Returned value:
** Created By:		fusuipu  2014.06.13
** Remarks:
*****************************************************************************/

static s32 sdkPedDukptInnerGetMacEx(u8 * upData, u16 uspDatalen, u8 * upDukptMode, u8 ucAlgo, u8 * upMac, u8 uKsnIndx, u8 * upKsn, u8 * upKsnLen, s32 siTimerOut)
{
    static s32 (*p_fun)(u8 *, u16, u8 *, u8, u8 *, u8, u8 *, u8 *, s32) = NULL;

    if(p_fun == NULL)
    {
        p_fun =  (s32 (*)(u8 *, u16, u8 *, u8, u8 *, u8, u8 *, u8 *, s32))sdk_dev_get_libdev_fun_hand("PedDukptGetMacExt");
    }

    if(p_fun != NULL)
    {
        return p_fun(upData, uspDatalen, upDukptMode, ucAlgo, upMac, uKsnIndx, upKsn, upKsnLen, siTimerOut);
    }
    else
    {
        Trace("dev", "warming: PedDukptGetMacExt");
        return SDK_ERR;
    }
}

/*****************************************************************************
** Descriptions:	获取DUKPT内置密码键盘
** Parameters:          const SDK_PED_PIN_CFG * pstPinCfg
** Returned value:
** Created By:		fusuipu  2014.06.13
** Remarks:
*****************************************************************************/
extern s32 sdkKbTransKey(s32 siKey);
static s32 sdkPEDDukptGetInnerKey(const SDK_PED_PIN_CFG * pstPinCfg)
{
    s32 i = 0, key = 0, pinlen = 0;
    u32 timer_id = 0, flash_id = 0;
    u8 disppin[32] = {0};
    u8 flash_flag = 0;                //添加一个闪烁箭头，进行提示

    memset(disppin, 0, sizeof(disppin));
    timer_id = sdkTimerGetId();
    flash_id = timer_id;
    pinlen = (pstPinCfg->ePinMode == SDK_PED_MAG_PIN) ? 6 : 12;

//    sdkDispClearScreen();
//    sdkDispFillRowRam(SDK_DISP_LINE1, 0, "请输入密码", SDK_DISP_DEFAULT);
//    sdkDispBrushScreen();

    while(1)
    {
        if(sdkTimerIsEnd(timer_id, (u32)pstPinCfg->iTimeOut))
        {
            return SDK_TIME_OUT;
        }

//空字符串时添加闪烁提示
        if(0 == i)
        {
            if(true == sdkTimerIsEnd(flash_id, 500))
            {
                flash_flag = (flash_flag + 1) % 2;
                flash_id = sdkTimerGetId();
            }

            if(flash_flag == 0)
            {
                sdkDispRow(pstPinCfg->ucRow, 0, " ", SDK_DISP_RIGHT_DEFAULT);
            }
            else
            {
                sdkDispRow(pstPinCfg->ucRow, 0, "_", SDK_DISP_RIGHT_DEFAULT);
            }
        }
//处理按键
        key = Private_sdkGetKeyValue();

        if(key <= 0)
        {
            continue;
        }
        timer_id = sdkTimerGetId();

        if( key == '*')
        {
        	if(i >= pinlen)
        	{
				sdkSysBeep(SDK_SYS_BEEP_ERR);
				continue;				
			}
            sdkSysBeep(SDK_SYS_BEEP_OK);
            disppin[i++] = (u8)key;
            sdkDispRow(pstPinCfg->ucRow, 0, disppin, SDK_DISP_RIGHT_DEFAULT);
        }
        else
        {
            key = sdkKbTransKey(key);

            if(key == SDK_KEY_CLEAR )
            {
                if(i != 0)
                {
                    sdkSysBeep(SDK_SYS_BEEP_OK);;
                }
                else
                {
                    sdkSysBeep(SDK_SYS_BEEP_ERR);
                }
                i = 0;
                memset(disppin, 0, sizeof(disppin));
            }
            else if(key == SDK_KEY_BACKSPACE)
            {
                if(i == 0)
                {
                    sdkSysBeep(SDK_SYS_BEEP_ERR);
                    memset(disppin, 0, sizeof(disppin));
                }
                else
                {
                    i--;
                    sdkSysBeep(SDK_SYS_BEEP_OK);
                    i = (0 >= i) ? (0) : (i);                                    //防止出错
                    disppin[i] = 0;
                    sdkDispRow(pstPinCfg->ucRow, 0, disppin, SDK_DISP_RIGHT_DEFAULT);
                }
            }
            else if(SDK_KEY_ESC == key)
            {
                sdkSysBeep(SDK_SYS_BEEP_OK);
                return SDK_ESC;
            }
            else if(key == SDK_KEY_ENTER )
            {
                sdkSysBeep(SDK_SYS_BEEP_OK);
                break;
            }
        }
    }

    if (i > 0 && i < 4)                                        // 密码长度小于4
    {
        return SDK_ERR;
    }
    else
    {
        if (!i)
        {
            return SDK_ERR;
        }
        else
        {
            return SDK_OK;
        }
    }
}

/*****************************************************************************
** Descriptions:
** Descriptions:    内置密码键盘输入密码
** Parameters:      u8 ucMode  这个为DUKPT的模式 SDK_PED_DUKPT_GET_PIN_MODE
                    const SDK_PED_PIN_CFG *pstPinCfg
                    u8 *pucOutData 加密后的数据
                    u8 *pucKsn ksn的值
** Returned value:
                    大于0	操作成功，并返回KSN的长度（针对DUKPT）
                    SDK_PED_PIN_FORMAT_ERR	密码格式错
                    SDK_TIME_OUT	密码键盘响应超时(单位: ms)
                    SDK_ERR	操作失败
                    SDK_ESC	用户取消操作
                    SDK_PED_NOPIN	无密码输入
** Created By:		fusuipu  2014.06.13
** Remarks:
*****************************************************************************/
static s32 sdkPEDDukptInsideInput(u8 ucMode,  const SDK_PED_PIN_CFG * pstPinCfg, u8 * upPwdLen, u8 * pucOutData, u8 ucIndex, u8 * pucKsn, u8 * pucKsnLen, s32 siTimer)
{
    s32 ret = 0;

    if(SDK_PED_DUKPT_GET_PIN_RELEASE == ucMode)
    {
        sdk_dev_key_set_filitermode(1, upPwdLen[0], upPwdLen[1], NULL, 0);

        if((ret = sdkPEDDukptGetInnerKey(pstPinCfg)) != SDK_OK)
        {
            sdk_dev_key_set_filitermode(0, upPwdLen[0], upPwdLen[1], NULL, 0);
            return ret;
        }
    }

    if(pstPinCfg->eKeyType == SDK_PED_DES_DUKPT)
    {
        ret =  sdkPedDukptInnerGetPinFun(ucMode, &pstPinCfg->hePan[2], upPwdLen, pucOutData, ucIndex, pucKsn, pucKsnLen, siTimer);
        sdk_dev_key_set_filitermode(0, upPwdLen[0], upPwdLen[1], NULL, 0);

        if(0 == ret)
        {
            return SDK_OK;
        }
        else
        {
            Assert(0);
            Trace("libsdkped", "ret = %d\r\n", ret);
            return SDK_ERR;
        }
    }
    else                                        //这里暂时没有数据
    {
        sdk_dev_key_set_filitermode(0, upPwdLen[0], upPwdLen[1], NULL, 0);
        Trace("libsdkped", "pstPinCfg->eKeyType != SDK_PED_DES_DUKPT quite\r\n");
        return SDK_ERR;
    }
}

/*****************************************************************************
** Descriptions:	用来组DC控制域和子命令部分
** Parameters:                      u16 uiLen 控制域长度(该字节现在的数据长度不能超过256,具体的可以参考文档)
                               u8 *pucData 控制域数据
                               u16 uiCmd
                               u8 *pucOutData
** Returned value:	输出数据长度,返回值是大于0 的。
** Created By:		zhouzhihua  2013.07.26
** Remarks:
*****************************************************************************/
static s32 sdkPedFormDCReserved(u16 uiLen, const u8 * pucData, u16 uiCmd, u8 * pucOutData)
{
    u8 temp[512]  = {0};
    u16 i;

    if(pucData == NULL || NULL == pucOutData)
    {
        Assert(0);
        return SDK_PARA_ERR;
    }

    if(uiLen > 0xFF)
    {
        Assert(0);
    }
    i = 0;
    memset(temp, 0, sizeof(temp) );
    temp[i++] = (u8)uiLen;
    memcpy(&temp[i], pucData, (u8)uiLen);
    i += (u8)uiLen;
    temp[i++] = (uiCmd & 0xFF00) >> 8;
    temp[i++] = (u8)uiCmd;
    memcpy(pucOutData, temp, i);

    return i;
}

/*****************************************************************************
** Descriptions:	 DUKPT发送 数据至密码键盘
** Parameters:          u16 usCmd发送命令字
                               u8 *pucData发送数据
                               u16 usDataLen数据长度
** Returned value:
** Created By:
** Remarks:         DUKPT指令的内外置和其他指令有些不一样
                    如果上层方向为0（有密码键盘），发往内置密码键盘；
                    如果上层方向是1（没有密码键盘），发往外部密码键盘
*****************************************************************************/
static s32 sdkPedDCSend(u16 usCmd, const u8 * pucSend, u16 usSendLen)
{
    FIFO fifo;
    u8 dest = 0;

    memset(&fifo, 0, sizeof(FIFO));
    fifo.Cmd = usCmd;

    /*=======BEGIN: fusuipu 2014.10.13  16:48 modify对于0x11指令，当方向等于0，表示
       发往内置密码键盘；当方向等于1时，表示发往外置密码键盘。对于0x12指令,当方向等于0，
       表示发往外置非接；当方向等于1，表示发往内置非接。对于0x10指令，当方向等于0时，表
       示发往密码键盘，方向为1时，发往内部。===========*/
    dest = ((true == sdkPEDIsWithPinpad()) ? TOPINPAD : TOAUX51);

    if(FIFO_POSTDCCMD == usCmd && 0X11 == pucSend[1])
    {
        dest = (dest == TOAUX51 ? TOPINPAD : TOAUX51);
    }
    /*====================== END======================== */

    fifo.Data[0] = dest;                        //内置或外置

    Trace("sdkped", ">>>>>>>>>>send to %s\r\n", (sdkPEDIsWithPinpad()) ? "TOPINPAD " : " TOAUX51");

    if (pucSend != NULL && usSendLen > 0)
    {
        memcpy(&fifo.Data[1], pucSend, usSendLen);
    }
    fifo.Len = usSendLen + 1;
    TraceHex("sdkped", "ped send fifo data", fifo.Data, fifo.Len);
    sdk_dev_clear_app_read_fifo();                        //zxx 20130131 16://shijianglong 2013.07.12 17:9
    sdk_dev_write_fifo(&fifo);
    return SDK_OK;
}

/*****************************************************************************
** Descriptions:   导入初始参数
** Parameters:
                   u8 ucIndex 索引
                   u8 *pucKey 初始key DK
                   u16 uiKeyLen 可以长度
                   u8 *pucKsn ksn
                   u16 uiKsnLen ksn的长度
                   s32 siTimer  超时时间
** Returned value:
                    大于0	操作成功
                    SDK_ESC	用户取消
                    SDK_PARA_ERR	参数错误
                    SDK_ESC	用户取消
                    SDK_TIME_OUT	超时退出
                    SDK_ERR	操作失败
** Created By:		zhouzhihua  2013.07.24
** Remarks:
*****************************************************************************/
s32 sdkPEDDukptLoadAndInit(u8 ucIndex, const u8 * pucKey, u16 uiKeyLen, const u8 * pucKsn, u16 uiKsnLen, s32 siTimer)
{
    u8 ucksn[20] = {0}, ucbuf[512] = {0}, temp[1024] = {0};
    s32 rslt, i;
    u16 ucCmd[1] = {0}, len = 0;

    //UARTCFG stUartCfg;

    siTimer  = (siTimer < 500) ? 500 : siTimer;

    if(pucKey == NULL || NULL == pucKsn || uiKsnLen > SDK_DUKPT_KSN_LEN )
    {
        Assert(0);
        return SDK_PARA_ERR;
    }

    if(!sdkPEDIsWithPinpad())
    {
        rslt = sdkPedDukptInnerLoadInitKey(0, 2, &ucIndex, pucKey, uiKeyLen, &ucIndex, pucKsn, uiKsnLen, 0);

        if(SDK_OK == rslt)
        {
            return SDK_OK;
        }
        else
        {
            return SDK_ERR;
        }
    }
    else
    {
        memset(ucksn, 0xFF, sizeof(ucksn));

        if( uiKsnLen <= SDK_DUKPT_KSN_LEN )
        {
            memcpy(&ucksn[SDK_DUKPT_KSN_LEN - uiKsnLen], pucKsn, uiKsnLen);
        }

        if( uiKeyLen != SDK_DUKPT_KEY_LEN )                                    //目前key的长度必须是16字节
        {
            Assert(0);
            return SDK_PARA_ERR;
        }
        memset(ucbuf, 0, sizeof(ucbuf));
        i = sdkPedFormDCReserved(1, DC_CMD_CTRL_RES, DC_CMD_EX_DUKPT_INIT, ucbuf);

        if( i < 0 )
        {
            Assert(0);
            return SDK_PARA_ERR;
        }
        ucbuf[i++] = SDK_PED_DUKPT_UPDATE_ALL;                                    //更新模式 0:同是更新ksn和key;1:只更新key，2:只更新ksn
        ucbuf[i++] = ucIndex;
        ucbuf[i++] = SDK_DUKPT_KEY_LEN;
        memcpy(&ucbuf[i], pucKey, SDK_DUKPT_KEY_LEN);
        i += SDK_DUKPT_KEY_LEN;
        ucbuf[i++] = SDK_DUKPT_KSN_LEN;
        memcpy(&ucbuf[i], ucksn, SDK_DUKPT_KSN_LEN);
        i += SDK_DUKPT_KSN_LEN;
        ucCmd[0] = (u16)FIFO_POSTDCCMD;
        TraceHex("zhouzhihua", "ucbuf  ", ucbuf, i);
#if 0 /*Modify by zhouzhihua at 2014.05.08  11:4 */
        sSetUartCfg(&stUartCfg, POS_DC_COMMAND, ucbuf, (u16)i, ucCmd, 1, siTimer);
        rslt = sSendDataToPed(&stUartCfg);
#else
        sdkPedDCSend(FIFO_POSTDCCMD, ucbuf, (u16)i);
        memset(temp, 0, sizeof(temp));
        rslt = sdk_ped_receive(ucCmd, 1, temp, &len, SDK_PED_TIMEOUT, false);
        TraceHex("zhouzhihua", "temp  ", temp, len);
#endif /* if 0 */

//得到的数据组成控制域长度1Byte + 控制域数据(nBytes)+cmd(2Bytes)+ret(1Byte)
        if( rslt == SDK_OK )
        {
            memset(ucbuf, 0, sizeof(ucbuf));
            rslt = SDK_ERR;

            if( (temp[2] == ((DC_CMD_EX_DUKPT_INIT >> 8) & 0xFF)) && (temp[3] == (u8)DC_CMD_EX_DUKPT_INIT) && (temp[4] == 0))
            {
                Trace("zhouzhihua", "chenggong+++++++++ \r\n");
                rslt = SDK_OK;
            }
        }
        return rslt;
    }
}

/*****************************************************************************
** Descriptions:

** Parameters:          u8 ucMode 00 正常模式，0x01 测试模式
                        u8 ucIndex 索引
                        u8 *pucPan 6Bytes BCD 带主帐号加密
                        u16 uiMinLen 密码最小长度
                        u16 uiMaxLen 密码最大长度
                        u8 *pucPinBlock 加密后的密码
                        s32 siTimer 定时器
** Returned value:	大于0 ksn的长度，小于等于0 失败
** Created By:		zhouzhihua  2013.07.29
** Remarks:             目前必须带主帐号加密
*****************************************************************************/
s32 sdkPEDDukptGetPin(u8 ucMode, u8 ucIndex, const SDK_PED_PIN_CFG * pstPinCfg, u8 uiMinLen, u8 uiMaxLen, u8 * pucPinBlock, u8 * pucKsn, u8 * pucKsnLen, s32 siTimer)
{
    u8 ucbuf[1024] = {0}, temp[1024] = {0};
    s32 rslt = 0, i = 0;
    u16 len = 0;
    u16 ucCmd[1];
    u8 pucPan[128] = {0};
    u8 len_buf[3] = {0};

    //UARTCFG stUartCfg;

    if( (uiMinLen < 4) || (uiMaxLen > 12) || (uiMinLen > uiMaxLen) || (NULL == pucPinBlock) || (pucKsn == NULL))
    {
        return SDK_PARA_ERR;
    }

    if( siTimer < 1000 )
    {
        Assert(0);
        return SDK_PARA_ERR;
    }

    if(!sdkPEDIsWithPinpad())
    {
        len_buf[0] = uiMinLen;
        len_buf[1] = uiMaxLen;
        return sdkPEDDukptInsideInput(ucMode, pstPinCfg, len_buf, pucPinBlock, ucIndex, pucKsn, pucKsnLen, siTimer);
    }
    else
    {
        siTimer -= 500;                                        //去掉500ms
        memset(ucbuf, 0, sizeof(ucbuf));
        memset(pucPan, 0, sizeof(pucPan));
        memcpy(pucPan, &pstPinCfg->hePan[2], 6);
        i = sdkPedFormDCReserved(1, DC_CMD_CTRL_RES, DC_CMD_EX_DUKPT_GET_PIN, ucbuf);

        if( i < 0 )
        {
            Assert(0);
            return SDK_PARA_ERR;
        }
        ucbuf[i++] = ucMode;
        ucbuf[i++] = ucIndex;
        ucbuf[i++] = uiMinLen;
        ucbuf[i++] = uiMaxLen;

        if( pucPan == NULL )                                    //如果需要不带主帐号的加密可以如此处理
        {
            memset(&ucbuf[i], 0, 6);
        }
        else
        {
            memcpy(&ucbuf[i], pucPan, 6);
        }
        i += 6;
        ucbuf[i++] = (u8)(((u32)siTimer & 0xFF000000) >> 24);
        ucbuf[i++] = (u8)((siTimer & 0x00FF0000) >> 16);
        ucbuf[i++] = (u8)((siTimer & 0x0000FF00) >> 8);
        ucbuf[i++] = (u8)((siTimer & 0x000000FF));
        ucCmd[0] = FIFO_POSTDCCMD;

        TraceHex("zhouzhihua", "ucbuf  ", ucbuf, i);

        // sSetUartCfg(&stUartCfg, POS_DC_COMMAND, ucbuf, (u16)i, ucCmd, 1, siTimer);
        // rslt = sSendDataToPed(&stUartCfg);

        sdkPedDCSend(FIFO_POSTDCCMD, ucbuf, (u16)i);
        memset(temp, 0, sizeof(temp));
        rslt = sdk_ped_receive(ucCmd, 1, temp, &pucKsnLen, SDK_PED_TIMEOUT, false);
        len = *pucKsnLen;

        //得到的数据组成子域长度1Byte + 子域数据(nBytes)+cmd(2Bytes)+ret(1Byte)+数据
        //data 是由pinblock长度(1Byte)+pinblock数据(nByte)+ksn长度(1Byte)+ksn数据(nByte)
        if(rslt == SDK_OK)
        {
            memset(ucbuf, 0, sizeof(ucbuf));
            rslt = SDK_ERR;
            i = 1 + temp[0] + 2;                                    //返回结果的位置

            if( temp[i] == 0x00 )
            {
                len = temp[i + 1];                                    //pinblock 长度
                memcpy(pucPinBlock, &temp[i + 1 + 1], 8);
                memcpy(pucKsn, &temp[i + 1 + 1 + len + 1], SDK_DUKPT_KSN_LEN);
                TraceHex("zhouzhihua", "pucKsn  ", pucKsn, 10);
                TraceHex("zhouzhihua", "pucPinBlock  ", pucPinBlock, 8);
                return SDK_OK;
            }
        }
        return rslt;
    }
}

/*****************************************************************************
** Descriptions:    DUKPT获取mac
** Parameters:          u8 ucMode 0x00 旧key ， 0x01新key, 0x10 旧key ， 0x01新key
                               u8 ucIndex  索引
                               u8 *pucInData 数据必须为 ASCII
                               u16 uiDataLen 数据长度
                               u8 *pucMac 计算的mac
                               u8 *pucKsn
                               s32 siTimer
** Returned value:	大于0 ksn的长度 正确，小于0:失败（失败，超时，用户取消，参数错误）
** Created By:		zhouzhihua  2013.07.30
** Remarks:
*****************************************************************************/
s32 sdkPEDDukptGetMac(u8 ucMode, u8 ucIndex, const u8 * puascInData, u16 uiDataLen, u8 * pucMac, u8 * pucKsn, u8 * pucKsnLen, s32 siTimer)
{
    u8 ucbuf[512 + 512 + 64] = {0}, temp[1024] = {0};
    s32 rslt, i;
    u16 len = 0;
    u16 ucCmd[1];
	u8 alog = 0;
    //UARTCFG stUartCfg;

    if( (NULL == puascInData) || (NULL == pucMac) )
    {
        return SDK_PARA_ERR;
    }

    if(uiDataLen > 1024)
    {
        Assert(0);
        return SDK_PARA_ERR;
    }

    if(!sdkPEDIsWithPinpad())
    {
    	if(1 == (ucMode >> 4 & 0x0F) || 2 == (ucMode >> 4 & 0x0F))
    	{
			alog = (ucMode >> 4 & 0x0F);	
			ucMode = ucMode & 0x0F;
			rslt = sdkPedDukptInnerGetMacEx(puascInData, uiDataLen, &ucMode, alog, pucMac, ucIndex, pucKsn, pucKsnLen, 0);
		}
		else
		{			
			rslt = sdkPedDukptInnerGetMac(puascInData, uiDataLen, &ucMode, pucMac, ucIndex, pucKsn, pucKsnLen, 0);
		}
        
        if(0 == rslt)
        {
            return SDK_OK;
        }
        else
        {
            return SDK_ERR;
        }
    }
    else
    {
        memset(ucbuf, 0, sizeof(ucbuf));
        i = sdkPedFormDCReserved(1, DC_CMD_CTRL_RES, DC_CMD_EX_DUKPT_GET_MAC, ucbuf);

        if( i < 0 )
        {
            Assert(0);
            return SDK_PARA_ERR;
        }
        ucbuf[i++] = ucMode;
        ucbuf[i++] = ucIndex;
        ucbuf[i++] = (uiDataLen & 0xFF00) >> 8;
        ucbuf[i++] = (u8)uiDataLen;
        memcpy(&ucbuf[i], puascInData, uiDataLen);
        i += uiDataLen;
        ucbuf[i++] = (u8)(((u32)siTimer & 0xFF000000) >> 24);
        ucbuf[i++] = (u8)((siTimer & 0x00FF0000) >> 16);
        ucbuf[i++] = (u8)((siTimer & 0x0000FF00) >> 8);
        ucbuf[i++] = (u8)((siTimer & 0x000000FF));
        ucCmd[0] = FIFO_POSTDCCMD;

        TraceHex("zhouzhihua", "ucbuf  ", ucbuf, i);

        //sSetUartCfg(&stUartCfg, POS_DC_COMMAND, ucbuf, (u16)i, ucCmd, 1, siTimer);
        // rslt = sSendDataToPed(&stUartCfg);

        sdkPedDCSend(FIFO_POSTDCCMD, ucbuf, (u16)i);
        memset(temp, 0, sizeof(temp));
        rslt = sdk_ped_receive(ucCmd, 1, temp, pucKsnLen, SDK_PED_TIMEOUT, false);
        len = *pucKsnLen;

        //得到的数据组成子域长度1Byte + 子域数据(nBytes)+cmd(2Bytes)+ret(1Byte)+data
        //data 是由mac长度(1Byte)+mac数据(nByte)+ksn长度(1Byte)+ksn数据(nByte)
        if(rslt == SDK_OK)
        {
            memset(ucbuf, 0, sizeof(ucbuf));
            rslt = SDK_ERR;
            i = 1 + temp[0] + 2;                                    //返回结果的位置

            if( temp[i] == 0x00 )
            {
                len = temp[i + 1];
                memcpy(pucMac, &temp[i + 1 + 1], 8);
                memcpy(pucKsn, &temp[i + 1 + 1 + len + 1], SDK_DUKPT_KSN_LEN);
                TraceHex("zhouzhihua", "pucKsn  ", pucKsn, 10);
                TraceHex("zhouzhihua", "pucMac  ", pucMac, 8);
                return SDK_DUKPT_KSN_LEN;
            }
        }
        return rslt;
    }
}

/*******************************************************************
   作    者: 付遂普
   版    权: Xinguodu Ltd,Co.
   函数名称: void XOR8(u8 *Dest, u8 *Scr1, u8 *Scr2)
   函数功能: 8个字节异或
   入口参数: 1.目的地址; 2.源地址; 3.源地址2
   返 回 值: 无
   相关调用:
   备    注: 8个字节的异或
   修改信息:
 ********************************************************************/
static void XORN(u8 *Dest, u8 *Scr1, u8 *Scr2, u32 Len)
{
    u8 i;

    for (i = 0; i < Len; i++)
    {
        *(Dest + i) = *(Scr1 + i) ^ *(Scr2 + i);
    }
}

/*****************************************************************************
** Descriptions:	使用SM4算法加密工作密钥
** Parameters:          Input: None
** Returned value:
** Created By:		fusuipu  2014.09.22
** Remarks:
*****************************************************************************/
static s32 sdkPedDCSendDateToPad(u8 *psrDate, u32 uiLen, u8 *pCmd, u8 uiCmdLen)
{
    u8 *pbuf = NULL;
    u32 i = 0;

    if(NULL == psrDate || NULL == pCmd)
    {
        return SDK_PARA_ERR;
    }

    if(NULL == (pbuf = sdkGetMem(uiLen + uiCmdLen + 1))) //1byte 是否带密码键盘，预留1byte防止溢出
    {
        return SDK_ERR;
    }
    memset(pbuf, 0, uiLen + uiCmdLen + 1);
    memcpy(pbuf, pCmd, uiCmdLen);
    i += uiCmdLen;

    memcpy(pbuf + i, psrDate, uiLen);
    i += uiLen;

    sdkPedDCSend(FIFO_POSTDCCMD, pbuf, i);
    sdkFreeMem(pbuf);
    pbuf = NULL;
    return SDK_OK;
}

/*****************************************************************************
** Descriptions:	对DC指令返回值的判断
** Parameters:          void
** Returned value:
** Created By:		fusuipu  2015.05.05
** Remarks:             不包括对数据的拷贝和
*****************************************************************************/
static s32 sdkPedDCReceviceJudge(bool bIsRetDate, u8 *lpOut, u8 *lpCmd, s32 siCmdLen, s32 siTimeOut)
{
    FIFO fifo;
    u32 time_id = 0;
    s32 key = 0;

    time_id = sdkTimerGetId();
    memset(&fifo, 0, sizeof(FIFO));

    while(1)
    {
        if(sdk_dev_read_fifo(&fifo, 100))
        {
            if(fifo.Cmd == FIFO_POSTDCCMD)
            {
                if((0 == memcmp(fifo.Data, lpCmd, siCmdLen)) && (0 == fifo.Data[4])) //0X00表示成功；0X01表示失败
                {
                    if(true == bIsRetDate)
                    {
                        memcpy(lpOut, &fifo.Data[5], fifo.Len - 5);
                    }
                    return SDK_OK;
                }
            }
        }

        if (0 != siTimeOut && sdkTimerIsEnd(time_id, siTimeOut)) //查询时间是否到
        {
            return SDK_TIME_OUT;
        }
        key = sdkKbGetKey(); //shijianglong 2012.12.27 14:47修改使用方式

        if (key == SDK_KEY_ESC)
        {
            sdkSysBeep(SDK_SYS_BEEP_OK);
            return SDK_ERR;
        }
        else if (key != 0)
        {
            sdkSysBeep(SDK_SYS_BEEP_ERR);
        }
    }

    return SDK_ERR;
}

/*****************************************************************************
** Descriptions:	使用SM4算法更新工作密钥
** Parameters:          u8 *pChectOutDate   加密工作密钥后返回的校验数据
                                        1B	                16B	            1B	                16 B
                                        第一组暂存索引号	第一组校验数据	第二组暂存索引号	第二组校验数据

                    u8 *pucWkDate       需要更新的原始工作密钥数据
                                        第一组数据:1B	        1B	    16 B	1B	        16 B	1 B
                                               主密钥索引号	密文长度	密文	明文长度	明文	暂存索引号
                                        第二组数据:1B	        1B	    16 B	1B	        16 B	1 B
                                               主密钥索引号	密文长度	密文	明文长度	明文	暂存索引号

                    u32 uiLen           需要更新的工作密钥数据长度
                    u32 uiTimeOut       当超时时间为0时，使用默认设置的等待接受密码键盘的超时时间
                                        大于0时，超时时间为uiTimeOut。
                    VTable *pVTable
** Returned value:	SDK_OK             操作成功
                    SDK_ESC            操作失败
                    RSLT_OVERTIME       当超时时间为0时，使用默认设置的等待接受密码键盘的超时时间
                                        大于0时，超时时间为uiTimeOut
** Created By:		fusuipu  2014.09.23
** Remarks:
*****************************************************************************/
s32 sdk_ped_derypt_wk_sm4(u8 *pChectOutDate, u8 *pucWkDate, u32 uiLen, u32 uiTimeOut)
{
    u8 cmd[5] = {0X01, 0X11, 0X10, 0x20, 0x00};
    u8 rst = 0;

    if(SDK_OK != (rst = sdkPedDCSendDateToPad(pucWkDate, uiLen, cmd, strlen(cmd))))
    {
        return rst;
    }
    return sdkPedDCReceviceJudge(true, pChectOutDate, cmd, 4,  uiTimeOut);
}

/*****************************************************************************
** Descriptions:	使用SM4算法更新工作密钥
** Parameters:          bool bEchoTak       是否保存更新的工作密钥，目前保留
                    u8 nTempTpkSNo      第一组工作密钥暂存索引号
                    u8 nTpkSNo          第一组工作密钥索引
                    u8 nTempTakSNo      第二组暂存索引号
                    u8 nTakSNo          第二组工作密钥索引
                    VTable *pVTable
** Returned value:	SDK_OK             操作成功
                    SDK_ESC            操作失败
                    RSLT_OVERTIME       超时时间使用默认的密码键盘的fifo接收超时时间
** Created By:		fusuipu  2014.09.23
** Remarks:
*****************************************************************************/
s32 sdk_ped_flush_wk_sm4(bool bEchoTak, u8 nTempTpkSNo, u8 nTpkSNo, u8 nTempTakSNo, u8 nTakSNo, u32 uiTimeOut)
{
    u8 temp[10] = {0};
    FIFO fifo;
    u8 cmd[5] = {0X01, 0X11, 0X10, 0x21, 0x00};
    u8 rst = 0;
    u32 i = 0;
    u32 time_id = 0;

    memset(temp, 0, sizeof(temp));
    i = 0;
    temp[i++] = nTempTpkSNo;
    temp[i++] =  nTpkSNo;
    temp[i++] = nTempTakSNo;
    temp[i++] =  nTakSNo;

    if(SDK_OK != (rst = sdkPedDCSendDateToPad(temp, i, cmd, strlen(cmd))))
    {
        return rst;
    }
    memset(&fifo, 0, sizeof(FIFO));
    time_id = sdkTimerGetId();

    while(1)
    {
        if(sdk_dev_read_fifo(&fifo, 100))
        {
            if(fifo.Cmd == FIFO_POSTDCCMD)
            {
                if((0 == memcmp(fifo.Data, cmd, 4)) && (0 == fifo.Data[4]))
                {
                    return SDK_OK;
                }
            }
        }

        if (0 != uiTimeOut && sdkTimerIsEnd(time_id, uiTimeOut)) //查询时间是否到
        {
            return SDK_TIME_OUT;
        }
    }

    return SDK_ERR;
}

/*******************************************************************
   作	  者: 付遂普
   版	  权: 深圳新国都股份有限公司
   函数功能: 基于SM4算法的ECB算法
   入口参数: 1:存放的地址;2.被Mac数据指针;3.被Mac数据长度
   返 回 值: 返回迭代后的16个字节ASCII数据
   备	  注:
 ********************************************************************/
static void sdkEcbSM4(u8 *lpOut, u8 *lpIn, u16 nLen)
{
    u16 i;
    u8 temp[256], A1[256];

    memset(A1, 0, sizeof(A1));

    for(i = 0; i < (nLen / 16); i++)
    {
        XORN(A1, A1, &lpIn[i * 16], 16);
    }

    if((nLen % 16) != 0)
    {
        memset(temp, 0, sizeof(temp));
        memcpy(temp, &lpIn[(nLen / 16) * 16], nLen - (nLen / 16) * 16);
        XORN(A1, A1, temp, 16);
    }
    sdkBcdToAsc(lpOut, A1, 16);
}

typedef enum
{
    SDK_SM4_ENCRYPT = 0x00,                                     //加密
    SDK_SM4_DECRYPT = 0x01,                                     //解密
}SDK_SM4_DEC_ENC_MODE;
/*****************************************************************************
** Descriptions:	基于SM4的加解密算法
** Parameters:          u8 uiAlogMode
                                        0x00 SM4加密，只可使用双倍长密钥（128bit）
                                        0x01 SM4解密，只可使用双倍长密钥（128bit）
                                        0x02 张成利提供山西农信算法
                    u8 uiKeyMode
                                        0x00 主密钥
                                        0x01 工作密钥
                    u8 *lpOut           加密后的数据
                    u8 *lpIn            算法类型(1B)	密钥类型(1B)	密钥索引号(1B)	待计算数据(NB)
                    u16 nLen            待加密数据长度
                    u32 uiTimeOut       当超时时间为0时，使用默认设置的等待接受密码键盘的超时时间
                                        大于0时，超时时间为uiTimeOut。
                    VTable *pVTable
** Returned value:	SDK_OK             操作成功
                    SDK_ESC            操作失败
                    RSLT_OVERTIME       当超时时间为0时，使用默认设置的等待接受密码键盘的超时时间
                                        大于0时，超时时间为uiTimeOut
** Created By:		fusuipu  2014.09.23
** Remarks:
*****************************************************************************/
s32 sdk_ped_wk_des_sm4(u8 uiAlogMode, u8 uiKeyMode, u8 nWkIndex, u8 *lpOut, u8 *lpIn, u16 nLen, u32 uiTimeOut)
{
    u8 *temp = NULL;
    u16 i = 0;
    u8 cmd[5] = {0X01, 0X11, 0X10, 0x22, 0x00};

    if(NULL == lpOut || NULL == lpIn)
    {
        return SDK_PARA_ERR;
    }

    if(NULL == (temp = sdkGetMem(3 + nLen + 1)))
    {
        return SDK_ERR;
    }
    memset(temp, 0, 3 + nLen + 1);
    i = 0;
    temp[i++] = uiAlogMode;                             //算法类型
    temp[i++] = uiKeyMode;                              //密钥类型
    temp[i++] = nWkIndex;
    memcpy(&temp[i], lpIn, nLen);
    i += nLen;
    sdkPedDCSendDateToPad(temp, i, cmd, strlen(cmd));
    sdkFreeMem(temp);
    temp = NULL;

    return sdkPedDCReceviceJudge(true, lpOut, cmd, 4, uiTimeOut);
}

/*****************************************************************************
** Descriptions:	实现基于SM4算法的E90算法Mac
** Parameters:          u8 *lpOut           加密后返回的结果数据
                    u8 *lpIn            需要计算的MAC数据的PAN
                    u16 nLen            需要计算的MAC数据PAN长度
                    u8 nTakSno          密钥索引号
                    VTable *pVTable
** Returned value:	SDK_OK             操作成功
                    SDK_ESC            操作失败
                    RSLT_OVERTIME       当超时时间为0时，使用默认设置的等待接受密码键盘的超时时间
                                        大于0时，超时时间为uiTimeOut
** Created By:		fusuipu  2014.09.23
** Remarks:
*****************************************************************************/
s32 sdk_ped_calc_mac_sm4(u8 *lpOut, u8 *lpIn, u16 nLen, u8 nTakSno)
{
    u8 mac[256] = {0}, temp[256] = {0};
    u8 rslt = 0;

    memset(mac, 0, sizeof(mac));
    sdkEcbSM4(mac, lpIn, nLen);
    memset(temp, 0, sizeof(temp));

    rslt = sdk_ped_wk_des_sm4(SDK_SM4_ENCRYPT, 0x01, nTakSno, temp, mac, 16, 10000);

    if(rslt != SDK_OK)
    {
        return rslt;
    }
    XORN(mac, temp, &mac[16], 16);

    rslt = sdk_ped_wk_des_sm4(SDK_SM4_ENCRYPT, 0x01, nTakSno, temp, mac, 16, 10000);

    if(rslt != SDK_OK)
    {
        return rslt;
    }
    sdkBcdToAsc(lpOut, temp, 8);

    return SDK_OK;
}

/*****************************************************************************
** Descriptions:	实现基于SM4算法的CBC算法Mac
** Parameters:          u8 *lpOut           加密后返回的结果数据
                    u8 *lpIn            需要计算的MAC数据的PAN
                    u16 nLen            需要计算的MAC数据PAN长度
                    u8 nTakSno          密钥索引号
                    VTable *pVTable
** Returned value:	SDK_OK             操作成功
                    SDK_ESC            操作失败
                    RSLT_OVERTIME       当超时时间为0时，使用默认设置的等待接受密码键盘的超时时间
                                        大于0时，超时时间为uiTimeOut
** Created By:		fusuipu  2014.09.23
** Remarks:
*****************************************************************************/
s32 sdk_ped_calc_mac_cbc_sm4(u8 *lpOut, u8 *lpIn, u16 nLen, u8 nTakSno)
{
    u8 temp[256] = {0}, A1[256] = {0};
    u8 rslt = 0;
    u16 i;
       
    memset(A1, 0, sizeof(A1));

    for(i = 0; i < (nLen / 16); i++)
    {
        XORN(A1, A1, &lpIn[i * 16], 16);
        if(SDK_OK != (rslt = sdk_ped_wk_des_sm4(SDK_SM4_ENCRYPT, 0x01, nTakSno, temp, A1, 16, 10000)))
        {
            return SDK_ERR;
        }
        memcpy(A1, temp, 16);
    }

    if((nLen % 16) != 0)
    {
        memset(temp, 0, sizeof(temp));
        memcpy(temp, &lpIn[(nLen / 16) * 16], nLen - (nLen / 16) * 16);
        XORN(A1, A1, temp, 16);
        if(SDK_OK != (rslt = sdk_ped_wk_des_sm4(SDK_SM4_ENCRYPT, 0x01, nTakSno, temp, A1, 16, 10000)))
        {
            return SDK_ERR;
        }
    }
    memcpy(lpOut, temp, 16);
    return SDK_OK;
}
/*****************************************************************************
** Descriptions:	密码键盘字符串播放
** Parameters:      u8 uiMode           :目前暂支持0x01
                    u8 *pVoiceStr       :{0, "0"},
                                         {1, "1"},
                                         {2, "2"},
                                         {3, "3"},
                                         {4, "4"},
                                         {5, "5"},
                                         {6, "6"},
                                         {7, "7"},
                                         {8, "8"},
                                         {9, "9"},
                                         {10, "."},
                                         {11, "*"},
                                         {12, "#"},
                                         {13, "元整"},
                                         {14, "请"},//语音有问题，不清晰，尽量不使用
                                         {15, "刷卡"},////语音有问题，不清晰，尽量不使用
                                         {16, "再次"},////语音有问题，不清晰，尽量不使用
                                         {17, "刷卡"},////语音有问题，不清晰，尽量不使用
                                         {18, "输入金"},////语音有问题，不清晰，尽量不使用
                                         {19, "额再"},////语音有问题，不清晰，尽量不使用
                                         {20, "次输入"},////语音有问题，不清晰，尽量不使用
                                         {21, "入金额"},////语音有问题，不清晰，尽量不使用
                                         {22, "额输入"},////语音有问题，不清晰，尽量不使用
                                         {23, "重新挥卡"},
                                         {24, "请"},
                                         {25, "刷卡"},
                                         {26, "再次刷卡"},
                                         {27, "输入金额"},
                                         {28, "再次输入金额"},
                                         {29, "输入密码"},
                                         {30, "确认"},
                                         {31, "选择"},
                                         {32, "插卡"},
                                         {33, "BEEP(OK)"},
                                         {34, "BEEP(ERR)"},
                                         {35, "请勿重复"},
                                         {36, "交易"},
                                         {37, "成功"},
                                         {38, "失败"},
                                         {39, "挥卡"},


                    s32 siStrLen         :声音字符串长度
                    s32 siTimeOut        :超时时间
** Returned value:  SDK_OK               :播放成功
** Created By:		fusuipu  2015.05.05
** Remarks:         如"请勿重复刷卡": 0x23,0x19
                      "挥卡失败"    : 0x27,0x26
*****************************************************************************/
s32 sdkPEDVoicePlay(u8 uiMode, u8 *pVoiceStr, s32 siStrLen, s32 siTimeOut)
{
    s32 mode = 0x01, len = 0;
    u8 cmd[5] = {0X01, 0X10, 0X10, 0x12, 0x00};     //播放语音
    u8 date[512] = {0};
    s32 i = 0;

    if(NULL == pVoiceStr || siStrLen <= 0 || siTimeOut < 0)
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
    date[i++] = mode;
    len = (siStrLen > 480 ? 480 : siStrLen);        //防止数据过长，把密码键盘撑死了
    date[i++] = len;
    memcpy(&date[i], pVoiceStr, len);
    i += len;
    sdkPedDCSendDateToPad(date, i, cmd, 4);
    return sdkPedDCReceviceJudge(false, NULL, cmd, 4, siTimeOut);
}

/*****************************************************************************
** Descriptions:	密码键盘声音大小设置
** Parameters:          u8 uiVolume
                               s32 siTimeOut
** Returned value:
** Created By:		fusuipu  2015.05.05
** Remarks:
*****************************************************************************/
s32 sdk_ped_volume_set(u8 uiVolume, s32 siTimeOut)
{
    u8 cmd[5] = {0X01, 0X10, 0X10, 0x13, 0x00};     //播放语音
    u8 date[128] = {0};
    s32 i = 0;
    s32 volume = 0;

    if(siTimeOut < 0)
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
    volume = (uiVolume > 7 ? 7 : uiVolume);
    date[i++] = volume;
    sdkPedDCSendDateToPad(date, i, cmd, 4);
    return sdkPedDCReceviceJudge(false, NULL, cmd, 4, siTimeOut);
}

/*****************************************************************************
** Descriptions:	显示密码键盘欢迎界面
** Parameters:          SDK_PED_WELCOME_STR *pWelStr
                               s32 siTimeOut
** Returned value:
** Created By:		fusuipu  2015.05.06
** Remarks:
*****************************************************************************/
s32 sdkPEDDispWelcomStr(SDK_PED_WELCOME_STR *pWelStr, s32 siTimeOut)
{
    u8 cmd[5] = {0X01, 0X10, 0X10, 0x11, 0x00};     //播放语音
    u8 date[512] = {0};
    s32 i = 0;
    s32 len = 0;

    if(NULL == pWelStr || siTimeOut < 0)
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
    date[i++] = 0;                                  //长度位预留
    date[i++] = 0x02;                               //显示节点个数
    date[i++] = 0x34;                               //第一个节点个数tag
    len = strlen(pWelStr->ucLine1Str);
    date[i++] = len;

    if(len > 0)
    {
        strcpy(&date[i], pWelStr->ucLine1Str);
        i += len;
    }
    date[i++] = 0x35;                               //第一个节点个数tag
    len = strlen(pWelStr->ucLine2Str);
    date[i++] = len;

    if(len > 0)
    {
        strcpy(&date[i], pWelStr->ucLine2Str);
        i += len;
    }
    date[0] = i;
    sdkPedDCSendDateToPad(date, i, cmd, 4);
    return sdkPedDCReceviceJudge(false, NULL, cmd, 4, siTimeOut);
}
/*****************************************************************************
** Descriptions:	mode 扩展质量，走DC指令下发
** Parameters:          SDK_PED_WELCOME_STR *pWelStr
                               s32 siTimeOut
** Returned value:
** Created By:		fusuipu  2015.05.06
** Remarks:
*****************************************************************************/
s32 sdk_ped_mode_ex(u8 *pucWkDate, u32 uiLen)
{
    u8 cmd[5] = {0X01, 0X14, 0x00, 0X10};
    u8 rst = 0;
	TraceHex("sdkExtLib", "pucWkDate", pucWkDate, uiLen);
    return sdkPedDCSendDateToPad(pucWkDate, uiLen, cmd, 4);
}

