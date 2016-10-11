#include "sdkGlobal.h"
//ped add
/*****************************************************************************
** Descriptions:	是否带密码键盘
** Parameters:          void
** Returned value:	true 带
                                false 不带
** Created By:		lqq2012.11.26
** Remarks:
*****************************************************************************/
bool sdkPEDIsWithPinpad(void)
{
    if(SDK_SYS_MACHINE_K301FZ == sdkSysGetMachineCode(NULL) ||
       SDK_SYS_MACHINE_K301FZE == sdkSysGetMachineCode(NULL) ||
       SDK_SYS_MACHINE_K350 == sdkSysGetMachineCode(NULL) )
    {
        if(false == sdk_dev_get_pinpadstate())
        {
            sdk_dev_set_pinpadstate(true);        //UC平台的机器自动的转为外置密码键盘
        }
    }
    return sdk_dev_get_pinpadstate();
}

/*****************************************************************************
** Descriptions:	 发送 数据至密码键盘
** Parameters:          u16 usCmd发送命令字
                               u8 *pucData发送数据
                               u16 usDataLen数据长度
** Returned value:
** Created By:		lqq2012.11.25
** Remarks:             //用来将来扩展使用，只有sPinpadSend和sPinpadRecv会调用fifo
*****************************************************************************/
s32 sdk_ped_send(u16 usCmd, const u8 *pucSend, u16 usSendLen)
{
    FIFO fifo;

    memset(&fifo, 0, sizeof(FIFO));
    fifo.Cmd = usCmd;
    fifo.Data[0] = (sdkPEDIsWithPinpad()) ?  TOPINPAD : TOAUX51; //内置或外置
    //fifo.Data[0] = (sdkPEDIsWithPinpad()) ? TOAUX51: TOPINPAD; //内置或外置

    Trace("sdkped", ">>>>>>>>>>send to %s\r\n", (sdkPEDIsWithPinpad()) ? "TOPINPAD " : " TOAUX51");

    if (pucSend != NULL && usSendLen > 0)
    {
        memcpy(&fifo.Data[1], pucSend, usSendLen);
    }
    fifo.Len = usSendLen + 1;
    TraceHex("sdkped", "ped send fifo data", fifo.Data, fifo.Len);
    sdk_dev_clear_app_read_fifo();     //zxx 20130131 16://shijianglong 2013.07.12 17:9
    sdk_dev_write_fifo(&fifo);
    return SDK_OK;
}

/*****************************************************************************
** Descriptions:	接收密码键盘数据
** Parameters:          u16 usCmd 待接收的命令字数组
                                u8 usCmdNum待接收的命令字个数
                               u8 *pucRecv输出数据
                               u16 pucRecvLen输出长度
** Returned value:
** Created By:		lqq2012.11.25
** Remarks:             用来将来扩展使用，只有sPinpadSend和sPinpadRecv会调用fifo
*****************************************************************************/
s32 sdk_ped_receive(u16 *pusCmd, u8 usCmdNum, u8 *pucRecv, u16 *pucRecvLen, s32 iTimeOut, bool bEnableEsc)
{
    FIFO fifo;
    s32 timer = 0, key = 0;
    u8 i = 0;

    timer = sdkTimerGetId();

//    ClearAppReadFifo();     //zxx 20130131 16:8
    //之所以这里清是因为存在回两条指令，第一条被读取了，第二条没有，紧接着发同样的指令，会把第二条直接读走
    //但是会不会有隐患?
    while (1)
    {
        if (sdk_dev_read_fifo(&fifo, 50)) //shijianglong 2013.07.12 17:4，时间从0改为50
        {
            for (i = 0; i < usCmdNum; i++) //遍历命令字
            {
                if (fifo.Cmd == pusCmd[i])
                {
                    pusCmd[0] = fifo.Cmd; //回传给命令字

                    if (pucRecvLen && pucRecv)
                    {
                        memcpy(pucRecv, fifo.Data, fifo.Len);
                        *pucRecvLen = fifo.Len;
                    }
                    return SDK_OK; //shijianglong 2012.12.06 16:29
                }
            }
        }

        if (sdkTimerIsEnd(timer, iTimeOut)) //查询时间是否到
        {
            return SDK_TIME_OUT;
        }
        key = sdkKbGetKey(); //shijianglong 2012.12.27 14:47修改使用方式

        if (key == SDK_KEY_ESC && bEnableEsc)
        {
            sdkSysBeep(SDK_SYS_BEEP_OK);
            return SDK_ESC;
        }
        else if (key != 0)
        {
            sdkSysBeep(SDK_SYS_BEEP_ERR);
        }
        sdkmSleep(10);
    }

    return SDK_OK;
}

//获取主密钥实际索引
/*****************************************************************************
** Descriptions:
** Parameters:          u8 ucVirtualTmkIndex 主密钥索引0-99
                               SDK_PED_DES_TYPE eTmkType主密钥类型
** Returned value:
** Created By:		lqq2012.11.26
** Remarks:
*****************************************************************************/
static u8 sPedGetFactTmkIndex(u8 ucVirtualTmkIndex, SDK_PED_DES_TYPE eTmkType)
{
    u8 tmkindex = ucVirtualTmkIndex;

    if (eTmkType == SDK_PED_DES_TRIPLE ||
        eTmkType == SDK_PED_SM4 )
    {
        tmkindex += MK_3DESTHRESHOLD;
    }
    return tmkindex;
}

/*****************************************************************************
** Descriptions:
** Parameters:          1. iTmkIndex: 存储主密钥的索引
                               0-99
                        2. eKeyType: 密钥类型
                                SDK_PED_DES_TYPE:DES密钥	   [8字节]
                                SDK_PED_DES_TRIPLE:3DES密钥   [16字节]
                        3. pheKey: 密钥值
                        4. iTimeout: 超时时间 不低于100ms
** Returned value:
                        SDK_OK: 成功
                        SDK_PARA_ERR: 参数错误
                        SDK_TIME_OUT: 密码键盘响应超时(单位: ms)
** Created By:		lqq2012.11.27
** Remarks:
*****************************************************************************/
s32 sdkPEDUpdateTmk(s32 iTmkIndex, SDK_PED_DES_TYPE eKeyType, const u8 *pheKey, s32 iTimeout)
{
    u8 temp[256], temp_key[256], i = 0;
    s32 rslt = 0;
    u16 cmd[5];
    u8 tran_key[20] = {0x36, 0x38, 0x34, 0x38, 0x32, 0x35, 0x36, 0x39}; //密码键盘固定传输密钥

    if ((pheKey == NULL) || (iTmkIndex < 0) || (iTmkIndex >= 100) || (iTimeout < 100)) //shijianglong 2013.01.28 15:10
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
    // 更新主密钥的模式
    i = 0;
    memset(temp, 0, sizeof(temp));

    /*=======BEGIN: fusuipu 2014.11.14  10:30 modify===========*/
    //引用黄炳鑫中间件的修改原文
    //20140514会议讨论，以后更新密钥不允许明文传输
    //在应用程序设置更新密钥模式为0xFF（明文）时，强制更改为0x03（使用传输密钥3des加密）
    //if(SDK_SYS_MACHINE_K360 != sdkSysGetMachineCode(NULL))

    temp[i++] = 0x03;
    memcpy(&temp[i], tran_key, 8);
    i += 8;

    sdk_ped_send(FIFO_UPDATETMKMODE, temp, i);
    cmd[0] = FIFO_PINPADOK;
    rslt = sdk_ped_receive(cmd, 1, NULL, NULL, iTimeout, false);

    if (SDK_OK != rslt)
    {
        return rslt;
    }
    // 更新单个主密钥
    i = 0;
    memset(temp, 0, sizeof(temp));
    temp[i++] = sPedGetFactTmkIndex((u8)iTmkIndex, eKeyType); //密钥索引

    memset(temp_key, 0, sizeof(temp_key));

    if (eKeyType == SDK_PED_DES_SINGLE)
    {
        memcpy(temp_key, pheKey, 8);
        sdkDesS(1, temp_key, tran_key);
        memcpy(&temp[i], temp_key, 8);
        i += 8;
    }
    else
    {
        memcpy(temp_key, pheKey, 16);
        sdkDesS(1, temp_key, tran_key);
        memcpy(&temp[i], temp_key, 8);
        i += 8;
        sdkDesS(1, &temp_key[8], tran_key);
        memcpy(&temp[i], &temp_key[8], 8);
        i += 8;
    }
    sdk_ped_send(FIFO_UPDATEONETMK, temp, i);
    cmd[0] = FIFO_PINPADOK;
    return sdk_ped_receive(cmd, 1, NULL, NULL, iTimeout, false);
}

/*****************************************************************************
** Descriptions:
** Parameters:          1. iTmkIndex: 存储主密钥的索引
                               0-99
                        2. eKeyType: 密钥类型
                                SDK_PED_DES_TYPE:DES密钥	   [8字节]
                                SDK_PED_DES_TRIPLE:3DES密钥   [16字节]
                        3. pheKey: 密钥值
                        4. iTimeout: 超时时间 不低于100ms
** Returned value:
                        SDK_OK: 成功
                        SDK_PARA_ERR: 参数错误
                        SDK_TIME_OUT: 密码键盘响应超时(单位: ms)
** Created By:		lqq2012.11.27
** Remarks:
*****************************************************************************/
s32 sdkPEDUpdateTmkJL(s32 iTmkIndex, SDK_PED_DES_TYPE eKeyType, const u8 *pheKey, s32 iTimeout)
{
    u8 temp[256], temp_key[256], i = 0;
    s32 rslt = 0;
    u16 cmd[5];
    u8 tran_key[20] = {0x36, 0x38, 0x34, 0x38, 0x32, 0x35, 0x36, 0x39}; //密码键盘固定传输密钥

    if ((pheKey == NULL) || (iTmkIndex < 0) || (iTmkIndex >= 100) || (iTimeout < 100)) //shijianglong 2013.01.28 15:10
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
	if(false == sdk_dev_is_panel_new_enough("247"))
	{
		return sdkPEDUpdateTmk(iTmkIndex, eKeyType, pheKey, iTimeout);
	}
    // 更新主密钥的模式
    i = 0;
    memset(temp, 0, sizeof(temp));

    /*=======BEGIN: fusuipu 2014.11.14  10:30 modify===========*/
    //引用黄炳鑫中间件的修改原文
    //20140514会议讨论，以后更新密钥不允许明文传输
    //在应用程序设置更新密钥模式为0xFF（明文）时，强制更改为0x03（使用传输密钥3des加密）
    //if(SDK_SYS_MACHINE_K360 != sdkSysGetMachineCode(NULL))

    temp[i++] = 0x03;
    memcpy(&temp[i], tran_key, 8);
    i += 8;

    sdk_ped_send(FIFO_UPDATETMKMODE, temp, i);
    cmd[0] = FIFO_PINPADOK;
    rslt = sdk_ped_receive(cmd, 1, NULL, NULL, iTimeout, false);

    if (SDK_OK != rslt)
    {
        return rslt;
    }
    // 更新单个主密钥
    i = 0;
    memset(temp, 0, sizeof(temp));
    temp[i++] = sPedGetFactTmkIndex((u8)iTmkIndex, eKeyType); //密钥索引

    memset(temp_key, 0, sizeof(temp_key));

    if (eKeyType == SDK_PED_DES_SINGLE)
    {
        memcpy(temp_key, pheKey, 8);
        sdkDesS(1, temp_key, tran_key);
        memcpy(&temp[i], temp_key, 8);
        i += 8;
    }
    else
    {
        memcpy(temp_key, pheKey, 16);
        sdkDesS(1, temp_key, tran_key);
        memcpy(&temp[i], temp_key, 8);
        i += 8;
        sdkDesS(1, &temp_key[8], tran_key);
        memcpy(&temp[i], &temp_key[8], 8);
        i += 8;
    }
    sdk_ped_send(FIFO_UPDATEONETMK_JL, temp, i);
    cmd[0] = FIFO_PINPADOK;
    return sdk_ped_receive(cmd, 1, NULL, NULL, iTimeout, false);
}

//计算实际工作密钥索引位置
/*****************************************************************************
** Descriptions:
** Parameters:          u8 ucVirtualWkIndex  工作密钥对外索引0-124
                               SDK_PED_DES_TYPE eWkType 工作密钥类型
** Returned value:
** Created By:		lqq2012.11.26
** Remarks:
*****************************************************************************/
static u8 sPedGetFactWkIndex(u8 ucVirtualWkIndex, SDK_PED_DES_TYPE eWkType)
{
    u8 wkindex = ucVirtualWkIndex;

    if (wkindex > 19) //20组之后索引区
    {
        if (eWkType == SDK_PED_DES_SINGLE)
        {
            wkindex += (WK_DESTHRESHOLDEX - 20);
        }
        else                        //3DES和SM4都使用双倍长索引
        {
            wkindex += (WK_3DESTHRESHOLDEX - 20);
        }
    }
    else //前20组
    {
        if (eWkType == SDK_PED_DES_SINGLE)
        {
            wkindex += WK_DESTHRESHOLD;
        }
        else                        //3DES和SM4都使用双倍长索引
        {
            wkindex += WK_3DESTHRESHOLD;
        }
    }
    return wkindex;
}

/*****************************************************************************
** Descriptions:	扩展更新工作密钥，不用传校验数据，解完密就灌入
** Parameters:          pstWkCfg->eTmkType
                            pstWkCfg->ucTmkIndex
                            pstWkCfg->eWkType
                            pstWkCfg->ucWkIndex
                            pstWkCfg->ucEnWkLen;
                            pstWkCfg->heEnWk;
                            //结构体其它可不用传入
                            eCryptWay:用于解密的主密钥长度类型，如16字节，可只用前8字节
** Returned value:
** Created By:		zxx 20130513 17:43
** Remarks:
*****************************************************************************/
s32 sdkPEDUpdateWkEx(const SDK_PED_WK_CFG *pstWkCfg, SDK_PED_CRYPT_WAY eCryptWay)
{
    s32 rslt = 0;
    u8 temp[64] = {0};
    u16 cmd[5] = {0};
    u16 len = 0;

    if (pstWkCfg == NULL || pstWkCfg->ucTmkIndex >= 100 || pstWkCfg->ucWkIndex >= 125
        || pstWkCfg->ucEnWkLen == 0 || pstWkCfg->ucEnWkLen > 24 || pstWkCfg->ucEnWkLen % 8 != 0)
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
    rslt = sdkPEDDesEx(pstWkCfg->ucTmkIndex, SDK_PED_TMK, pstWkCfg->eTmkType, SDK_PED_DECRYPT, eCryptWay, pstWkCfg->heEnWk, pstWkCfg->ucEnWkLen, temp);

    if (rslt != SDK_OK)
    {
        return rslt;
    }
    memset(temp, 0, sizeof(temp));

    if (pstWkCfg->eTmkType == SDK_PED_DES_TRIPLE && eCryptWay == SDK_PED_DES_ALL)
    {
        temp[len++] = 0x10; // 3des缓存索引
    }
    else
    {
        temp[len++] = 0;    //des缓存索引
    }
    temp[len++] =  sPedGetFactWkIndex((u8)pstWkCfg->ucWkIndex, pstWkCfg->eWkType);

    sdk_ped_send(FIFO_PCIWKFLUSH, temp, len);
    cmd[0] = FIFO_PCIWKFLUSH;
    return sdk_ped_receive(cmd, 1, temp, &len, SDK_PED_TIMEOUT, false);
}

/*****************************************************************************
** Descriptions:	更新K370工作密钥
** Parameters:          void
** Returned value:
** Created By:		fusuipu  2014.07.16
** Remarks:
*****************************************************************************/
#define K370_TAK_KEY_FILE  "/mtd0/res/SpecialFile"
static s32 sdkPEDUpdatek370Tak(const u8 uiTakIdx, const u8 *ucTakKey, SDK_PED_WK_CFG *pstWkCfg)
{
    s32 i = 0;
    s32 ret = 0;
    s32 len = 0;
    u8 temp[1024] = {0};

    typedef struct
    {                                                //主密钥索引(0-100)  //master key index (1-100)
        SDK_PED_DES_TYPE eTakType;                                       //工作密钥类型，DES或3DES   //working key type, DES or 3DES
        u8 ucTakIndex;                                                   //存储wk的索引(0-124)  //the index of storing wk (0-124)
        u8 ucTakLen;                                                   //wk密文长度//wk ciphertext length
        u8 heTak[24];                                                  //wk密文//wk ciphertext
    }SDK_PED_TAK_CFG;
    SDK_PED_TAK_CFG tmp_tak;

    if(NULL == ucTakKey || NULL == pstWkCfg)
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
    memset(&tmp_tak, 0, sizeof(tmp_tak));

    tmp_tak.eTakType = pstWkCfg->eWkType;
    tmp_tak.ucTakIndex = uiTakIdx;
    tmp_tak.ucTakLen = pstWkCfg->ucCheckDataLen;

    memcpy(tmp_tak.heTak, ucTakKey, tmp_tak.ucTakLen);

    ret = sdkGetFileSize(K370_TAK_KEY_FILE);

    if(7 * 1024 != ret)
    {
        if(ret > 0) //tak密钥文件标准大小为7*1024,如果不是这么大，肯定出错了，整个文件删除重写
        {
            sdkDelFile(K370_TAK_KEY_FILE);
        }
        /*如果存储tak密钥的文件不存在，先创建并且先全部写0，相当于创建一个256*sizeof(read_tak)
            的芯片区，为接下来的密钥插入存储做好准备
         */
        memset(temp, 0, sizeof(temp));
        len = sizeof(temp);

        for(i = 0; i < 7; i++)
        {
            ret = sdkAppendFile(K370_TAK_KEY_FILE, temp, len);

            if(ret != SDK_FILE_OK)
            {
                Assert(0);
                sdkDelFile(K370_TAK_KEY_FILE);
                return SDK_ERR;
            }
        }
    }
    ret = sdkInsertFile(K370_TAK_KEY_FILE, (u8 *)&tmp_tak, uiTakIdx * sizeof(tmp_tak), sizeof(tmp_tak));

    if(SDK_FILE_OK == ret)
    {
        return SDK_OK;
    }
    Assert(0);
    return SDK_ERR;
}

/*****************************************************************************
** Descriptions:	更新工作密钥
** Parameters:          const SDK_WK_CFG *pstWkCfg 工作密钥配置结构
                               const u8 siWknum配置密钥个数
                               u32 siTimeout超时时间不低于100ms单位ms
** Returned value:
** Created By:		//lqq 2012.11.23 10:57
** Remarks:
   主密钥 0-99
   工作密钥 0-124
*****************************************************************************/
s32 sdkPEDUpdateWk(SDK_PED_WK_CFG *pstWkCfg, const s32 iWknum, s32 iTimeout)
{
    s32 i = 0, rslt = 0;
    u8 tmkindex = 0;
    u8 temptpk = 0, tpkindex = 0, temptak = 0, takindex = 0;
    u8 temp[2048] = {0};
    u16 j = 0, cmd[5];

    if ((pstWkCfg == NULL) || (iWknum <= 0) || (iTimeout < 100))     //shijianglong 2013.01.28 15:10
    {
        Assert(0);
        return SDK_PARA_ERR;
    }

    /*====================== END======================== */
    for (i = 0; i < iWknum; i++)
    {
        if ((i + 1) % 2)
        {
            j = 0;
            memset(temp, 0, sizeof(temp));
        }
        //主密钥索引
        Assert(pstWkCfg[i].ucTmkIndex < 100);
        tmkindex = sPedGetFactTmkIndex(pstWkCfg[i].ucTmkIndex, pstWkCfg[i].eTmkType);
        temp[j++] = tmkindex;

        /***********************增加对工作密钥长度的判定，底层设计不合理导致**************************/     //lqq 2012.12.18 15:1
        if (pstWkCfg[0].bOnlyCalcCheckValue)     //lqq 2012.12.18 15:0
        {
            pstWkCfg[i].ucCheckValueLen = pstWkCfg[i].ucCheckDataLen;
        }

        if (pstWkCfg[i].eWkType == SDK_PED_DES_SINGLE)
        {
            if (pstWkCfg[i].ucEnWkLen != 8 || pstWkCfg[i].ucCheckDataLen != 8 || pstWkCfg[i].ucCheckValueLen > 8)
            {
                return SDK_PARA_ERR;
            }
        }
        else if (pstWkCfg[i].eWkType == SDK_PED_DES_TRIPLE ||
                 SDK_PED_SM4 == pstWkCfg[i].eWkType)
        {
            if (pstWkCfg[i].ucEnWkLen != 16 || pstWkCfg[i].ucCheckDataLen != 16 || pstWkCfg[i].ucCheckValueLen > 16)
            {
                return SDK_PARA_ERR;
            }
        }
        /*************************************************************************************************************/
        //WK密文长度
        Assert(pstWkCfg[i].ucEnWkLen <= 32);
        temp[j++] = pstWkCfg[i].ucEnWkLen;
        memcpy(&temp[j], pstWkCfg[i].heEnWk, pstWkCfg[i].ucEnWkLen);
        j += pstWkCfg[i].ucEnWkLen;

        //WK效验密钥长度
        Assert(pstWkCfg[i].ucCheckDataLen <= 32);
        temp[j++] = pstWkCfg[i].ucCheckDataLen;
        memcpy(&temp[j], pstWkCfg[i].heCheckData, pstWkCfg[i].ucCheckDataLen);
        j += pstWkCfg[i].ucCheckDataLen;

        //工作密钥索引
        Assert(pstWkCfg[i].ucWkIndex < 125);     //最大工作索引判定

        if ((i + 1) % 2)
        {
            tpkindex = sPedGetFactWkIndex(pstWkCfg[i].ucWkIndex, pstWkCfg[i].eWkType);

            if (pstWkCfg[i].eWkType == SDK_PED_DES_SINGLE)
            {
                temptpk = TPK_TEMPSTORENO;
            }
            else                                //3DES和SM4都使用双倍长密钥
            {
                temptpk = TPK_3DESTEMPSTORENO;
            }
            temp[j++] = temptpk;
        }
        else
        {
            takindex = sPedGetFactWkIndex(pstWkCfg[i].ucWkIndex, pstWkCfg[i].eWkType);

            if (pstWkCfg[i].eWkType == SDK_PED_DES_SINGLE)
            {
                temptak = TAK_TEMPSTORENO;
            }
            else
            {
                temptak = TAK_3DESTEMPSTORENO;
            }
            temp[j++] = temptak;
        }

        if ((i + 1) == iWknum  && (iWknum % 2))     //奇数个密钥,且到最后一组
        {
            temptak = temptpk + 1;
            takindex  = tpkindex;
            memcpy(&temp[j], temp, j);
            j += j;
            temp[j - 1] = temptak;
            i++;     //进入更新工作密钥处理
        }

        if ((i + 1) % 2 == 0)            //如果2次就进行解析
        {
            /*=======BEGIN: fusuipu 2015.01.23  14:38 modify SM4更新工作密钥===========*/
            if (pstWkCfg[i - 1].eWkType == SDK_PED_SM4)                  /*不是只有一个密钥的情况*/
            {
                if (i < iWknum && pstWkCfg[i].eWkType != SDK_PED_SM4)   /*B保证Tak和tpk配对使用SM4算法*/
                {
                    return SDK_PARA_ERR;
                }
                rslt = sdk_ped_derypt_wk_sm4(temp, temp, j, iTimeout);
            }
            /*====================== END======================== */
            else
            {
                sdk_ped_send(FIFO_DECRYPTWK, temp, j);
                cmd[0] = FIFO_DECRYPTWKOK;
                rslt = sdk_ped_receive(cmd, 1, temp, &j, iTimeout, false);
            }

            if (SDK_OK != rslt)
            {
                Assert(0);
                Trace("libsdkped", "rslt = %d\r\n", rslt);
                return rslt;
            }
            else
            {
                if (pstWkCfg[0].bOnlyCalcCheckValue)
                {
                    memcpy(pstWkCfg[i - 1].heCheckValue, &temp[1], pstWkCfg[i - 1].ucCheckValueLen);

                    if (i < iWknum)     /*不是只有一个密钥的情况*/
                    {
                        memcpy(pstWkCfg[i].heCheckValue, &temp[pstWkCfg[i - 1].ucCheckDataLen + 2], pstWkCfg[i].ucCheckValueLen);
                    }
                }
                else
                {
                    if (memcmp(&temp[1], pstWkCfg[i - 1].heCheckValue, pstWkCfg[i - 1].ucCheckValueLen))
                    {
                        return SDK_PED_TPK_ERR;
                    }

                    if ((i < iWknum)     /*不是只有一个密钥的情况*/
                        &&  memcmp(&temp[pstWkCfg[i - 1].ucCheckDataLen + 2], pstWkCfg[i].heCheckValue, pstWkCfg[i].ucCheckValueLen))
                    {
                        return SDK_PED_TAK_ERR;
                    }
                }
            }

            if (pstWkCfg[i - 1].eWkType == SDK_PED_SM4)                     /*不是只有一个密钥的情况*/
            {
                if (i < iWknum && pstWkCfg[i].eWkType != SDK_PED_SM4)       /*保证Tak和tpk配对使用SM4算法*/
                {
                    return SDK_PARA_ERR;
                }

                if(SDK_OK != (rslt = sdk_ped_flush_wk_sm4(0, temptpk, tpkindex, temptak, takindex, iTimeout)))
                {
                    Assert(0);
                    return SDK_ERR;
                }
            }
            else
            {
                j = 0;
                memset(temp, 0, sizeof(temp));
                temp[j++] = temptpk;
                temp[j++] =  tpkindex;
                temp[j++] = temptak;

                if(SDK_SYS_MACHINE_K370 == sdkSysGetMachineCode(NULL))
                {
                    temp[j++] = 0xFF;
                }
                else
                {
                    temp[j++] =  takindex;
                }
                sdk_ped_send(FIFO_FLUSHWK, temp, j);
                cmd[0] = FIFO_FLUSHWKOK;
                cmd[1] = FIFO_FLUSHWKERROR;
                rslt = sdk_ped_receive(cmd, 2, temp, &j, iTimeout, false);            //FIFO_FLUSHWKERROR 情况不考虑

                if (SDK_OK != rslt)
                {
                    return rslt;
                }
                else
                {
                    if (cmd[0] == FIFO_FLUSHWKERROR)
                    {
                        Assert(0);
                        return SDK_ERR;
                    }
                }

                if(SDK_SYS_MACHINE_K370 == sdkSysGetMachineCode(NULL))
                {
                    sdkPEDUpdatek370Tak(takindex, temp, &pstWkCfg[i]);
                }
            }
        }
    }

    return SDK_OK;
}

/*****************************************************************************
** Descriptions:	 工作密钥解密数据
** Parameters:          u8 ucWkIndex 工作密钥实际索引
                               u8 *pheOut 解密后结果，8byte
                               u8 *pheIn 待解密数据，8byte
                               s32 siInLen待解密数据长度 目前没有实际作用
** Returned value:
** Created By:		lqq2012.11.23
** Remarks:
*****************************************************************************/
static s32 sPedWkDecryptData(u8 ucWkIndex, u8 * pheOut, u8 * pheIn, s32 siInLen)
{
    u8 temp[512];

    s32 rslt = 0;
    u16 i = 0, cmd[5];

    memset(temp, 0, sizeof(temp));
    i = 0;
    temp[i++] = 0;                        // 解密
    temp[i++] = ucWkIndex;                        // 密钥索引
    temp[i++] = 1;                        // 是否返回结果    1: 返回  0: 不返回
    temp[i++] = 0;                        // 临时密钥索引: 0-8
    memcpy(&temp[i], pheIn, 8);                        //暂时写死，目前不知道任意长度不知底层是否支持
    i += 8;

    sdk_ped_send(FIFO_PCIWKDES, temp, i);
    cmd[0] = FIFO_PCIWKDES;
    rslt = sdk_ped_receive(cmd, 1, temp, &i, SDK_PED_TIMEOUT, false);

    if (SDK_OK != rslt)
    {
        return rslt;
    }
    else
    {
        if (temp[1] == 0)                        //解密成功
        {
            memcpy(pheOut, &temp[2], 8);                        //
        }
        else
        {
            Assert(0);
            return SDK_ERR;                        //失败
        }
    }
    return SDK_OK;
}

/*****************************************************************************
** Descriptions:	工作密钥加密数据
** Parameters:          u8 ucWkIndex工作密钥实际索引
                               u8 *pheOut加密后结果8byte
                               u8 *pheIn待加密数据
                               u8 ucLen待加密数据长度(目前是不是只支持8byte)
** Returned value:
** Created By:		lqq2012.11.26
** Remarks:
*****************************************************************************/
static s32 sPedWkEncryptData(u8 ucWkIndex, u8 * pheOut, u8 * pheIn, u8 ucLen)
{
    u8 temp[1024];
    u16 i = 0, cmd[5];
    s32 rslt = 0;

    memset(temp, 0, sizeof(temp));
    i = 0;
    temp[i++] = ucWkIndex;
    memcpy(&temp[i], pheIn, ucLen);
    i += ucLen;

    sdk_ped_send(FIFO_DESENCRYPT, temp, i);
    cmd[0] = FIFO_DESENCRYPTOK;
    cmd[1] = FIFO_DESENCRYPTERROR;
    rslt = sdk_ped_receive(cmd, 2, temp, &i, SDK_PED_TIMEOUT, false);

    if (SDK_OK != rslt)
    {
        return rslt;
    }
    else
    {
        if (cmd[0] == FIFO_DESENCRYPTERROR)
        {
            Assert(0);
            return SDK_ERR;
        }
        memcpy(pheOut, &temp[1], 8);
        return SDK_OK;
    }
}

/*****************************************************************************
** Descriptions:	获取k370机型tak密钥
** Parameters:          u8 *pKeyBuf
** Returned value:
** Created By:		fusuipu  2014.07.23
** Remarks:
*****************************************************************************/
static s32 sdkGetK370TakKey(u8 ucTakIndex, SDK_PED_DES_TYPE eKeyType, u8 * pKeyBuf)
{
    s32 i = 0;
    s32 ret = 0;
    s32 len = 0;

    typedef struct
    {                                                //主密钥索引(0-100)  //master key index (1-100)
        SDK_PED_DES_TYPE eTakType;                                       //工作密钥类型，DES或3DES   //working key type, DES or 3DES
        u8 ucTakIndex;                                                   //存储wk的索引(0-124)  //the index of storing wk (0-124)
        u8 ucTakLen;                                                   //wk密文长度//wk ciphertext length
        u8 heTak[24];                                                  //wk密文//wk ciphertext
    }SDK_PED_TAK_CFG;
    SDK_PED_TAK_CFG read_tak;


    len = sizeof(read_tak);
    i = 0;
    memset(&read_tak, 0, sizeof(read_tak));
    ret = sdkReadFile(K370_TAK_KEY_FILE, (u8 *)&read_tak, ucTakIndex * sizeof(read_tak), &len);

    if(ret != SDK_FILE_OK && ret != SDK_FILE_EOF)
    {
        Assert(0);
        return ret;
    }

    if(eKeyType == read_tak.eTakType &&
       ucTakIndex == read_tak.ucTakIndex)
    {
        memcpy(pKeyBuf, read_tak.heTak, 8);
        sdkDesS(false, pKeyBuf, "\x33\x33\x30\x31\x35\x38\x36\x37");

        if(SDK_PED_DES_TRIPLE == eKeyType)
        {
            memcpy(&pKeyBuf[8], &read_tak.heTak[8], 8);
            sdkDesS(false, &pKeyBuf[8], "\x33\x33\x30\x31\x35\x38\x36\x37");
        }
        return SDK_OK;
    }
    Assert(0);
    return SDK_ERR;
}

/*****************************************************************************
** Descriptions:
** Parameters:          u8 ucTakIndex  tak实际索引
                               u8 *pheOut 加密后结果
                               u8 *pheIn待加密数据
                               s32 siInLen待加密数据长度
** Returned value:
** Created By:		lqq2012.11.23
** Remarks:
*****************************************************************************/
static s32 sPedE90ForK370(u8 ucTakIndex, SDK_PED_DES_TYPE eKeyType, u8 * pheOut, const u8 * pheIn, s32 siInLen)
{
    u8 temp[24], temp_rslt[8];
    s32 rslt = 0;
    u8 tak_key[16] = {0};

    memset(temp, 0, sizeof(temp));
    sdkEcb(temp, pheIn, siInLen);

    rslt = sdkGetK370TakKey(ucTakIndex, eKeyType, tak_key);

    if(SDK_PED_DES_SINGLE == eKeyType)
    {
        sdkDesS(true, temp, tak_key);
    }
    else
    {
        sdkDes3S(true, temp, tak_key);
    }
    memcpy(temp_rslt, temp, 8);

    if (rslt == SDK_OK)
    {
        Verify(sdkXOR8(temp, temp_rslt, &temp[8]) == SDK_OK);

        if(SDK_PED_DES_SINGLE == eKeyType)
        {
            sdkDesS(true, temp, tak_key);
        }
        else
        {
            sdkDes3S(true, temp, tak_key);
        }
        memcpy(temp_rslt, temp, 8);
        memset(temp, 0, sizeof(temp));
        Verify(sdkBcdToAsc(temp, temp_rslt, 4) == 8);
        memcpy(pheOut, temp, 8);
    }
    return rslt;
}

/*****************************************************************************
** Descriptions:
** Parameters:          u8 ucTakIndex  tak实际索引
                               u8 *pheOut 加密后结果
                               u8 *pheIn待加密数据
                               s32 siInLen待加密数据长度
** Returned value:
** Created By:		lqq2012.11.23
** Remarks:
*****************************************************************************/
static s32 sPedE90(u8 ucTakIndex, u8 * pheOut, const u8 * pheIn, s32 siInLen)
{
    u8 temp[24], temp_rslt[8];
    s32 rslt = 0;

    memset(temp, 0, sizeof(temp));

    sdkEcb(temp, pheIn, siInLen);

    rslt = sPedWkEncryptData(ucTakIndex, temp_rslt, temp, 8);


    if (rslt == SDK_OK)
    {
        Verify(sdkXOR8(temp, temp_rslt, &temp[8]) == SDK_OK);

        rslt = sPedWkEncryptData(ucTakIndex, temp_rslt, temp, 8);

        if (rslt == SDK_OK)
        {
            memset(temp, 0, sizeof(temp));
            Verify(sdkBcdToAsc(temp, temp_rslt, 4) == 8);
            memcpy(pheOut, temp, 8);
        }
    }
    return rslt;
}

/*****************************************************************************
** Descriptions:        密码键盘E99/E919加密
** Parameters:          u8 ucTakIndex  tak实际索引
                               u8 *pheOut 加密后结果
                               u8 *pheIn待加密数据
                               s32 siInLen待加密数据长度
                               bool bX919 是否是919，否是99cbc
** Returned value:
** Created By:		fusuipu
** Remarks:         根据产品软件部彭学斌指示，K370使用秘钥芯片，读不出来，不支持X919算法
                    所以自行使用软加密算法实现
*****************************************************************************/
#define SDK_PED_DATE_MAX_LEN 480            //每次每段数据下发到密码键盘的最大数据长度(8的倍数,大于8)；
static s32 sPedCbcForK370(u8 takindex, SDK_PED_DES_TYPE eKeyType, u8 * pheOut, const u8 * pheIn, s32 siInLen, bool bX919)
{
    u8 temp[512] = {0};
    u8 temp_rslt[10] = {0};
    u16 flag = 0;
    s32 len = 0;
    s32 i = 0, imax = 0;
    u8 tak_buf[64] = {0};

    if (NULL == pheOut || NULL == pheIn || siInLen < 0)
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
    imax = (siInLen + SDK_PED_DATE_MAX_LEN - 1) / SDK_PED_DATE_MAX_LEN;
    memset(temp, 0, sizeof(temp));
    memset(temp_rslt, 0, sizeof(temp_rslt));
    memset(tak_buf, 0, sizeof(tak_buf));

    sdkGetK370TakKey(takindex, eKeyType, tak_buf);

    if(SDK_PED_DES_SINGLE == eKeyType && true == bX919)
    {
        bX919 = false;
    }
    len = 0;

    for (i = 0; i < imax; i++)
    {
        if (i > 0)
        {
            len = 8;
        }
        memset(temp, 0, sizeof(temp));
        memcpy(temp, temp_rslt, len);                        //将上段数据的加密结果处理后放到下一段数据

        if ((i + 1 == imax)  && (siInLen % SDK_PED_DATE_MAX_LEN))                        //最后一笔，且不是480的整数倍
        {
            memcpy(temp + len, pheIn + SDK_PED_DATE_MAX_LEN * i, siInLen % SDK_PED_DATE_MAX_LEN);
            len += ((u16)(((siInLen % SDK_PED_DATE_MAX_LEN) + 7) / 8)) * 8;                        //最后不足8字节补0
        }
        else
        {
            memcpy(temp + len, pheIn + SDK_PED_DATE_MAX_LEN * i, SDK_PED_DATE_MAX_LEN);
            len += SDK_PED_DATE_MAX_LEN;
        }
        flag = FIFO_PCIWKE99;                             //E99和919的中间处理步骤是一样的，所以中间步骤统一使用E99算法

        if ((i + 1 == imax) && (true == bX919))
        {
            flag = FIFO_PCIWK919;                         //fusuipu 2013.09.23 18:27，E919最后一段数据下发，改用E919的处理流程
        }

        if(FIFO_PCIWKE99 == flag)
        {
            sdkE99S(temp_rslt, temp, len, tak_buf, SDK_DES);

            if(i + 1 != imax)
            {
                sdkDesS(false, temp_rslt, tak_buf);
            }
        }
        else
        {
            sdkE99S(temp_rslt, temp, len, tak_buf, SDK_DES);                        //fusuipu 2013.08.29 10:42
            sdkDesS(false, temp_rslt, &tak_buf[8]);
            sdkDesS(true, temp_rslt, tak_buf);
        }
    }

    TraceHex("libsdkped", "sPedCbcForK370, temp_rslt:", temp_rslt, strlen(temp_rslt));
    memcpy(pheOut, temp_rslt, 8);
    return SDK_OK;
}

/*****************************************************************************
** Descriptions:        密码键盘E99/E919加密
** Parameters:          u8 ucTakIndex  tak实际索引
                               u8 *pheOut 加密后结果
                               u8 *pheIn待加密数据
                               s32 siInLen待加密数据长度
                               bool bX919 是否是919，否是99cbc
** Returned value:
** Created By:		lqq2012.11.23
** Remarks:         根据产品软件部彭学斌指示，K370使用秘钥芯片，读不出来，不支持X919算法
*****************************************************************************/
static s32 sPedCbc(u8 ucTakIndex, SDK_PED_DES_TYPE eKeyType, u8 * pheOut, const u8 * pheIn, s32 siInLen, bool bX919)
{
    u8 temp[512] = {0};
    u8 temp_rslt[8] = {0};
    u8 temp_out[8] = {0};
    u16 cmd[5] = {0};
    u16 len = 0;
    s32 rslt = 0, i = 0, imax = 0;

    if (NULL == pheOut || NULL == pheIn || siInLen < 0)
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
    imax = (siInLen + SDK_PED_DATE_MAX_LEN - 1) / SDK_PED_DATE_MAX_LEN;

    memset(cmd, 0, sizeof(cmd));
    memset(temp, 0, sizeof(temp));
    memset(temp_rslt, 0, sizeof(temp_rslt));
    memset(temp_out, 0, sizeof(temp_out));

    for (i = 0; i < imax; i++)
    {
        if (i > 0)
        {
            len = 8;
        }
        temp[0] = ucTakIndex;
        memset(&temp[1], 0, sizeof(temp) - 1);
        memcpy(&temp[1], temp_rslt, len);                        //将上段数据的加密结果处理后放到下一段数据

        if ((i + 1 == imax)  && (siInLen % SDK_PED_DATE_MAX_LEN))                        //最后一笔，且不是480的整数倍
        {
            memcpy(&temp[1] + len, pheIn + SDK_PED_DATE_MAX_LEN * i, siInLen % SDK_PED_DATE_MAX_LEN);
            len += ((u16)(((siInLen % SDK_PED_DATE_MAX_LEN) + 7) / 8)) * 8;                        //最后不足8字节补0
        }
        else
        {
            memcpy(&temp[1] + len, pheIn + SDK_PED_DATE_MAX_LEN * i, SDK_PED_DATE_MAX_LEN);
            len += SDK_PED_DATE_MAX_LEN;
        }

        /*=======BEGIN: fusuipu 2014.07.25  17:31 modify 根据和彭学斌的确认
           在索引传入单des的情况下，X919算法默认执行E99动作===========*/
        if(SDK_PED_DES_SINGLE == eKeyType)
        {
            bX919 = false;
        }
        /*====================== END======================== */
        cmd[0] = FIFO_PCIWKE99;                                //E99和919的中间处理步骤是一样的，所以中间步骤统一使用E99算法

        if (true == bX919)
        {
            cmd[0] = FIFO_PCIWK919;                             //fusuipu 2013.09.23 18:27，E919最后一段数据下发，改用E919的处理流程
        }
        sdk_ped_send(cmd[0], temp, len + 1);
        rslt = sdk_ped_receive(cmd, 1, temp, &len, SDK_PED_TIMEOUT * 5, false);


        if (SDK_OK != rslt)
        {
            return rslt;
        }
        else
        {
            if (temp[1] == 0)                        //成功
            {
                memcpy(temp_out, &temp[2], 8);                        //拷备结果

                /*=======BEGIN: fusuipu 2013.09.23  20:46 modify 解决多段数据加密===========*/
                /*不管是E99还是E919的分段加密，每段数据最后一步都是需要经过一个des/3des加密，
                   如果将上一段数据加密的8个字节的结果直接放到下一段数据下发到密码键盘加密，上
                   一段数据的结果又会被进行一次des加密计算，所以处理多段数据加密时，不到最后一
                   次，将上一次的数据进行des解密消除重复des加密后再放到下一段数据进行加密*/
                if (i + 1 != imax)
                {
                    if (SDK_OK != (rslt = sPedWkDecryptData(ucTakIndex, temp_rslt, temp_out, 8)))
                    {
                        return rslt;
                    }
                }
                else
                {
                    memcpy(temp_rslt, temp_out, 8);
                }
                /*====================== END======================== */
            }
            else
            {
                Assert(0);
                return SDK_ERR;
            }
        }
    }

    memcpy(pheOut, temp_rslt, 8);
    TraceHex("libsdkped", "temp_rslt", temp_rslt, 8);
    return SDK_OK;
}

/*****************************************************************************
** Descriptions:	K370的MAC算法因为兼容CBC算法而和其他机型不一致
** Parameters:          s32 iKeyIndex
                               SDK_PED_MAC_MODE eMode
                               SDK_PED_DES_TYPE eKeyType
                               const u8 *pheIn
                               s32 iInLen
                               u8 *pheOut
** Returned value:
** Created By:		fusuipu  2014.07.23
** Remarks:
*****************************************************************************/
static s32 sdkPEDCalaMacForK370(s32 iKeyIndex, SDK_PED_MAC_MODE eMode, SDK_PED_DES_TYPE eKeyType, const u8 * pheIn, s32 iInLen, u8 * pheOut)
{
    if (eMode == SDK_PED_ECB)
    {
        return sPedE90ForK370(iKeyIndex, eKeyType, pheOut, pheIn, iInLen);
    }
// --------------ssssssssss--------------------CBC----------------------------------
    else if (eMode == SDK_PED_CBC)
    {
        return sPedCbcForK370(iKeyIndex, eKeyType, pheOut, pheIn, iInLen, 0);
    }
// ----------------------------------X9.19----------------------------------
    else if (eMode == SDK_PED_X919)
    {
        return sPedCbcForK370(iKeyIndex, eKeyType, pheOut, pheIn, iInLen, 1);
    }
    else
    {
        Assert(0);
        return SDK_ERR;
    }
}

/*****************************************************************************
** Descriptions:	计算mac
** Parameters:          s32 iKeyIndex tak索引 :0-124
                               SDK_PED_MAC_MODE eMode: ecb ,cbc,x919
                               SDK_PED_DES_TYPE eKeyType:des类型
                               const u8 *pheIn:待计算数据长度不足8位补8位
                               s32 iInLen:计算数据长度
                               u8 *pheOut:8字节mac字
** Returned value:
** Created By:		lqq2012.11.26
** Remarks:
*****************************************************************************/
s32 sdkPEDCalcMac(s32 iKeyIndex, SDK_PED_MAC_MODE eMode, SDK_PED_DES_TYPE eKeyType, const u8 * pheIn, s32 iInLen, u8 * pheOut)
{
    u8 takindex = 0;

    // 输入参数合法性判断
//    Assert(pheIn != NULL);
//    Assert(pheOut != NULL);
//    Assert(iKeyIndex >= 0 && iKeyIndex < 125);
//    Assert(iInLen >0);//shijianglong 2013.01.09 13:54

    if ((pheIn == NULL) || (pheOut == NULL) || (iKeyIndex < 0) || (iKeyIndex >= 125) || (iInLen <= 0))                        //shijianglong 2013.01.28 15:10
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
    takindex = sPedGetFactWkIndex((u8)iKeyIndex, eKeyType);

    if(SDK_SYS_MACHINE_K370 == sdkSysGetMachineCode(NULL))
    {
        return sdkPEDCalaMacForK370(takindex, eMode, eKeyType, pheIn, iInLen, pheOut);
    }

// -----------------------------------ECB---------------------------------------
    if (eMode == SDK_PED_ECB)
    {
        return sPedE90(takindex, pheOut,  pheIn, iInLen);
    }
// --------------ssssssssss--------------------CBC----------------------------------
    else if (eMode == SDK_PED_CBC)
    {
        return sPedCbc(takindex, eKeyType, pheOut, pheIn, iInLen, 0);
    }
// ----------------------------------X9.19----------------------------------
    else if (eMode == SDK_PED_X919)
    {
        return sPedCbc(takindex, eKeyType, pheOut, pheIn, iInLen, 1);
    }
    else if(eMode == SDK_PED_MAC_SM4)
    {
        return sdk_ped_calc_mac_sm4(pheOut, pheIn, iInLen, takindex);
    }
    else if(eMode == SDK_PED_CBC_SM4)
    {
        return sdk_ped_calc_mac_cbc_sm4(pheOut, pheIn, iInLen, takindex);
    }
    else
    {
        Assert(0);
        return SDK_ERR;
    }
}

/*****************************************************************************
** Descriptions:
** Parameters:          u8 ucTpkIndex
                               const u8 * pasPin
                               const u8 * phePan
                               u8 * pheOut
** Returned value:
** Created By:		fusuipu  2015.04.22
** Remarks:
*****************************************************************************/
static s32 sdkPedSM4InnerPinInput(u8 ucTpkIndex, const u8 * pasPin, const u8 * phePan, u8 * pheOut)
{
    u8 pin[17] = {0};
    u8 i = 0;
    u8 pinblock[17] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0};

    i = (u8)strlen(pasPin);                                                   // 获取密码的长度

    if (i >= 4 && i <= 12)                                          // 密码长度为4-12位
    {
        pinblock[0] = i;                                          // 第一个字节表示密码长度
        sdkAscToBcd(pinblock + 1, pasPin, i);                        // 将ASCII密码转换成BCD

        if ((i % 2) != 0)                                            // 奇数
        {
            pinblock[(i / 2) + 1] |= 0x0F;                        // 确保为0x0F
        }
        memset(pin, 0, sizeof(pin));
        sdkXOR8(pin, phePan, pinblock);                        // 异或得PINBLOCK
        sdkXOR8(&pin[8], &phePan[8], &pinblock[8]);            // 16byte
    }
    else
    {
        return SDK_PED_PIN_FORMAT_ERR;                                // 密码格式不符合
    }
    return sdkPEDWkDes(ucTpkIndex, SDK_PED_SM4, SDK_PED_ENCRYPT, pin, pheOut);
}

/*****************************************************************************
** Descriptions:
** Parameters:         u8 ucTpkIndex 实际索引
                               u8 *pasPin 密码ASC明文
                               u8 *phePan
                               u8 *pheOut 加密后结果
** Returned value:
** Created By:		lqq2012.11.26
** Remarks:
*****************************************************************************/
static s32 sPedE98(u8 ucTpkIndex, const u8 * pasPin, const u8 * phePan, u8 * pheOut)
{
    u8 pin[9];
    u8 i;
    u8 pinblock[9] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0};

    i = (u8)strlen(pasPin);                                                   // 获取密码的长度

    if (i >= 4 && i <= 12)                                          // 密码长度为4-12位
    {
        pinblock[0] = i;                                          // 第一个字节表示密码长度
        sdkAscToBcd(pinblock + 1, pasPin, i);                        // 将ASCII密码转换成BCD

        if ((i % 2) != 0)                                            // 奇数
        {
            pinblock[(i / 2) + 1] |= 0x0F;                        // 确保为0x0F
        }
        memset(pin, 0, sizeof(pin));
        sdkXOR8(pin, phePan, pinblock);                        // 异或得PINBLOCK
    }
    else
    {
        return SDK_PED_PIN_FORMAT_ERR;                                // 密码格式不符合
    }
    return sPedWkEncryptData(ucTpkIndex, pheOut, pin, 8);
}

/*****************************************************************************
** Descriptions:	往密码键盘发送随机数
** Parameters:          u8 *pheRandom 8字节随机数
** Returned value:
** Created By:		lqq2012.11.26
** Remarks:
*****************************************************************************/
static s32 sPedSendIccRandom(u8 * pheRandom)
{
    u16 cmd[5];

    sdk_ped_send(FIFO_RANDOM, pheRandom, 8);
    cmd[0] = FIFO_PINPADOK;
    return sdk_ped_receive(cmd, 1, NULL, NULL, SDK_PED_TIMEOUT, false);
}

/*****************************************************************************
** Descriptions:	广西农信的使用明文加密算法
** Parameters:          u8 *srcDate
                               u8 *pTempKey
** Returned value:
** Created By:		fusuipu  2016.04.06
** Remarks:
*****************************************************************************/
static void  sdkPEDGetEKey(u8 *srcDate, u8 *pTempKey)
{
    unsigned char i = 0;
    unsigned char buf1[64] = {0};
    unsigned char buf2[64] = {0};
    //1234567890ABCDEF1111111111FEDCBA
    unsigned char rootkey[16] = {0x12, 0x34, 0x56, 0x78, 0x90, 0xAB, 0xCD, 0xEF, 0x11, 0x11, 0x11, 0x11, 0x11, 0xFE, 0xDC, 0xBA};     //根密钥

    //离散出左边8个字节密钥
    sdkAscToBcd(buf1, srcDate, 16);
    sdkDes3S(true, buf1, rootkey);
    memcpy(pTempKey, buf1, 8);

    //离散出右边8个字节密钥
    memset(buf1, 0x00, sizeof(buf1));
    sdkAscToBcd(buf1, srcDate, 16);

    for(i = 0; i < 8; i++)
    {
        buf2[i] = ~buf1[i];
    }

    sdkDes3S(true, buf2, rootkey);
    memcpy(&pTempKey[8], buf2, 8);
}

/*****************************************************************************
** Descriptions:	使用用户传入的密钥与算法对键盘返回的密码明文加密获取密文
                    后返回密码密文
** Parameters:          const SDK_PED_PIN_CFG * pstPinCfg
                               const u8 * pbcAmt
                               u8 * pheOut
** Returned value:
** Created By:		fusuipu  2016.04.05
** Remarks:         Data长度可能是4，或者5。当长度为5，且第5个字节为0X01，加
                    密后密文上传PIN，否则明文上传PIN。明文上传的时候，第1个字
                    节决定是否可以不输入密码。第3个字节显示最小PIN长度，第4个
                    字节决定最长PIN长度。
   0x40模式：
        帧格式1：
   Must(1B)	Mode(1B)	Min(1B)	Max(1B)
        0X40
        帧格式2：
   Must(1B)	Mode(1B)	Min(1B)	Max(1B)	Encrypt(1B)
        0X40
        如Max小于Min，返回错误。
        如Max大于16，返回错误。
        帧格式1使用明文上传密码。
        帧格式2，Encrypt =0X01,加密上传，其他值使用明文上传。

*****************************************************************************/
static s32 sdkPEDInputPinWithUsrKey(const SDK_PED_PIN_CFG * pstPinCfg, const u8 * pbcAmt, u8 * pheOut)
{
    u8 date[128] = {0},  temp[128] = {0}, key[128] = {0};
    s32 i = 0, rst = 0;
    u16 len = 0, cmd[10] = {0};

    SDK_SYS_EXT_PARA *ext_para = NULL;
    u8 min_len = 0, max_len = 16;       //默认值

    if(NULL != pstPinCfg->pVar)
    {
        ext_para = (SDK_SYS_EXT_PARA *)pstPinCfg->pVar;

        if(0x01 == ext_para->siCmd)
        {
            min_len = ext_para->uiLPara;
            max_len = ext_para->uiWPara;

            if(min_len > max_len ||
               max_len > 16)
            {
                Assert(0);
                return SDK_PARA_ERR;
            }
        }
    }

    if (sdkPEDIsWithPinpad())
    {
        //must 可以不输入密码直接退出
        date[i++] = 1;
        //mode
        date[i++] = 0x40;
        //min
        date[i++] = min_len;
        //max
        date[i++] = max_len;
        sdk_ped_send(FIFO_INPUTPIN, date, i);

        cmd[0] = FIFO_INPUTPINOK;                        //shijianglong 2012.12.11 10:0修改指令错误
        rst = sdk_ped_receive(cmd, 1, temp, &len, pstPinCfg->iTimeOut, true);
        TraceHex("sdkPEDInputPinWithUsrKey", "date:", temp, len);

        if (rst != SDK_OK)
        {
            Assert(0);
            return rst;
        }
        else
        {
            if (!temp[0])                        //zhuoquan 20130204 返回无密码
            {
                return SDK_PED_NOPIN;
            }
            else
            {
                temp[0] = len - 1;
            }
        }
    }
    else
    {
        rst = sdkKbGetScanf(pstPinCfg->iTimeOut, temp, min_len, max_len, SDK_MMI_NUMBER | SDK_MMI_PWD, pstPinCfg->ucRow);

        if (rst == SDK_KEY_ESC)
        {
            Assert(0);
            return SDK_ESC;
        }

        if (rst == SDK_TIME_OUT)
        {
            Assert(0);
            return SDK_TIME_OUT;
        }

        /*================ END================== */
        if (temp[0] > 0 && temp[0] < 4)                        // 密码长度小于4
        {
            return SDK_PED_PIN_FORMAT_ERR;
        }

        if (!temp[0])                        //zhuoquan 20130204 返回无密码
        {
            return SDK_PED_NOPIN;
        }
    }
    memset(date, 0xFF, sizeof(date));
    date[0] = temp[0];
    sdkAscToBcd(&date[1], &temp[1], temp[0]);

    TraceHex("sbc", "block", pstPinCfg->uiBuf, 16);
    sdkPEDGetEKey(pstPinCfg->uiBuf, key);
    TraceHex("sbc", "key", key, 16);
    sdkDes3S(1, date, key);
    memcpy(pheOut, date, 8);
    TraceHex("sbc", "enc_rst", pheOut, 16);
    return SDK_OK;
}

/*****************************************************************************
** Descriptions:	输入密码
** Parameters:          const SDK_PED_PIN_CFG *pstPinCfg 密码属性配置
                                pbcAmt 6位Bcd码金额,不显示金额则为NULL
                               u8 *pheOut  密码相关数据，第一个字节长度，后边跟实际需要数据
** Returned value:
** Created By:		lqq2012.11.26
** Remarks:
*****************************************************************************/
s32 sdkPEDInputPIN(const SDK_PED_PIN_CFG * pstPinCfg, const u8 * pbcAmt, u8 * pheOut)
{
    u8 temp[128], buf[128], iccRandom[8], pwd[13], i;
    u16 le = 0, cmd[5];
    u8 tpkindex = 0;
    s32 rslt;
    SDK_SYS_EXT_PARA *ext_para = NULL;
    u8 min_len = 0, max_len = 0;

    // 输入参数合法性判断
//    Assert(pstPinCfg != NULL);
//    Assert(pheOut != NULL);
//    Assert(pstPinCfg->ucTpkIndex < 125);
    if ((pstPinCfg == NULL) || (pheOut == NULL) || (pstPinCfg->ucTpkIndex >= 125))                        //shijianglong 2013.01.28 15:10
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
    tpkindex = sPedGetFactWkIndex(pstPinCfg->ucTpkIndex, pstPinCfg->eKeyType);

    sdkSysPlaySoundFile("/mtd0/res/inputpwd.wav", 1);                        //shiweisong 2013.03.08 11:50
/*=======BEGIN: fusuipu 2016.04.05  9:49 modify 增加使用用户指定密钥与===========*/

    if(pstPinCfg->ePinMode == SDK_PED_PIN_DIVER)
    {
        return sdkPEDInputPinWithUsrKey(pstPinCfg, pbcAmt, pheOut);
    }
/*====================== END======================== */

    if(NULL != pstPinCfg->pVar)
    {
        ext_para = (SDK_SYS_EXT_PARA *)pstPinCfg->pVar;
    }

    if (sdkPEDIsWithPinpad())                        // 带密码键盘
    {
        if (pstPinCfg->ePinMode == SDK_PED_IC_OFFLINE_PIN)
        {
            Verify(sdkGetRandom(iccRandom, 8) >= 0);
            rslt = sPedSendIccRandom(iccRandom);

            if (rslt != SDK_OK)
            {
                Assert(0);                        //shiweisong 2013.01.31 13:43
                return SDK_ERR;
            }
        }

        //增加金额参数入口
        //---//zxx 20130226 13:44
        if (pbcAmt != NULL)
        {
            sdk_ped_send(FIFO_DISPAMOUNT, pbcAmt, 6);
            cmd[0] = FIFO_PINPADOK;
            rslt = sdk_ped_receive(cmd, 1, NULL, NULL, SDK_PED_TIMEOUT, false);

            if (rslt != SDK_OK)
            {
                Assert(0);
                return SDK_ERR;
            }
        }
        //---//
        memset(temp, 0, sizeof(temp));
        le = 0;

        temp[le++] = tpkindex;

        switch (pstPinCfg->ePinMode)
        {
             case SDK_PED_IC_ONLINE_PIN:                 // IC卡联机硬加密

               if(SDK_PED_SM4 != pstPinCfg->eKeyType)
               {
                   temp[le++] = 0;
               }
               else
               {
                   temp[le++] = 0x04;                   //IC卡联机硬加密使用SM4算法
               }
               break;

             case SDK_PED_IC_OFFLINE_PIN:
               temp[le++] = 0x55;                       // IC卡软加密
               break;

             case SDK_PED_MAG_PIN:

               if(SDK_PED_SM4 != pstPinCfg->eKeyType)
               {
                   temp[le++] = 1;                      // 加密方法,带主帐号加密,最大6位密码
               }
               else
               {
                   temp[le++] = 0x04;                   //IC卡联机硬加密使用SM4算法
               }
               break;

             default:
               Assert(0);                        //shiweisong 2013.01.31 13:43
               break;
        }

        if(SDK_PED_SM4 != pstPinCfg->eKeyType)
        {
            memcpy(&temp[le], pstPinCfg->hePan, 8);
            le += 8;
        }
        else
        {
            memcpy(&temp[le], pstPinCfg->uiBuf, 16);
            le += 16;
            temp[le++] = 1;
            temp[le++] = 4;
            temp[le++] = 12;
        }
        Assert(le <= sizeof(temp));                        //防止有些时候不小心改错了 shiweisong 2013.01.31 13:52

        while (1)
        {
            sdk_ped_send(FIFO_INPUTPIN, temp, le);
//            cmd[0] = FIFO_PINPADOK;
            cmd[0] = FIFO_INPUTPINOK;                        //shijianglong 2012.12.11 10:0修改指令错误
            cmd[1] = FIFO_VERIFYAMOUNT;
            cmd[2] = FIFO_REQUESTRANDOM;
            /*=======BEGIN: lilin20131226 modify===========*/
            cmd[3] = FIFO_DESENCRYPTERROR;                        //增加密码键盘加密错误返回
            rslt = sdk_ped_receive(cmd, 4, temp, &le, pstPinCfg->iTimeOut, true);
            /*================ END================== */

            if (rslt != SDK_OK)
            {
                return rslt;
            }
            else
            {
//                if(cmd[0] == FIFO_PINPADOK)
                if (cmd[0] == FIFO_INPUTPINOK)                        //shijianglong 2012.12.11 10:0修改指令错误
                {
                    //if(temp[0])
                    if (!temp[0])                        //zhuoquan 20130204 返回无密码
                    {
                        return SDK_PED_NOPIN;
                    }

                    if(SDK_PED_SM4 != pstPinCfg->eKeyType)
                    {
                        memcpy(pwd, &temp[1], 8);
                        i = 8;
                    }
                    else
                    {
                        memcpy(pwd, &temp[1], 16);
                        i = 16;
                    }

                    if (pstPinCfg->ePinMode == SDK_PED_IC_OFFLINE_PIN)
                    {
                        sdkDesS(0, pwd, iccRandom);                                        // 随机密钥解密
                        i = pwd[0];

                        if (i > 12)
                        {
                            i = 12;
                        }
                        memset(buf, 0, sizeof(buf));
                        Verify(sdkBcdToAsc(buf, &pwd[1], (i + 1) / 2) >= 0);
                        memset(pwd, 0, sizeof(pwd));
                        memcpy(pwd, buf, i);
                    }
                    /*=======BEGIN: lilin20120814 modify===========*/
                    pheOut[0] = i;
                    memcpy(&pheOut[1], pwd, i);
                    return SDK_OK;
                }
                else if (cmd[0] == FIFO_VERIFYAMOUNT && pstPinCfg->ePinMode != SDK_PED_MAG_PIN)
                {
                    if (temp[0] == 0x43)                        //shijianglong 2013.07.31 11:39  zxx
                    {
                        return SDK_ESC;
                    }
                    return SDK_OK;
                }
                else if (cmd[0] == FIFO_REQUESTRANDOM && pstPinCfg->ePinMode == SDK_PED_IC_OFFLINE_PIN)
                {
                    Verify(sdkGetRandom(iccRandom, 8) >= 0);
                    rslt = sPedSendIccRandom(iccRandom);

                    if (rslt != SDK_OK)
                    {
                        Assert(0);                        //shiweisong 2013.01.31 13:47
                        return SDK_ERR;
                    }
                    continue;
                }
                /*=======BEGIN: lilin20131226 modify===========*/
                //增加处理密码键盘失败处理
                else if(cmd[0] == FIFO_DESENCRYPTERROR)
                {
                    Assert(0);
                    return SDK_ERR;
                }
                /*================ END================== */
                else
                {
                    continue;
                }
            }
        }
    }
    else                        // 不带密码键盘
    {
        memset(temp, 0, sizeof(temp));

        i = (pstPinCfg->ePinMode == SDK_PED_MAG_PIN) ? 6 : 12;

        if(NULL != ext_para && 0x01 == ext_para->siCmd)
        {
            min_len = ext_para->uiLPara;
            max_len = (ext_para->uiWPara > i ? i : ext_para->uiWPara);

            if(min_len > max_len)
            {
                Assert(0);
                return SDK_PARA_ERR;
            }
        }
        else
        {
            min_len = 0;
            max_len = i;
        }
        /*=======BEGIN: 李琳20121106 modify===========*/
        rslt = sdkKbGetScanf(pstPinCfg->iTimeOut, temp, min_len, max_len, SDK_MMI_NUMBER | SDK_MMI_PWD, pstPinCfg->ucRow);

        if (rslt == SDK_KEY_ESC)
        {
            return SDK_ESC;
        }

        if (rslt == SDK_TIME_OUT)
        {
            return SDK_TIME_OUT;
        }

        /*================ END================== */
        if (temp[0] > 0 && temp[0] < 4)                        // 密码长度小于4
        {
            return SDK_PED_PIN_FORMAT_ERR;
        }
        else
        {
            if (!temp[0])                        //zhuoquan 20130204 返回无密码
            {
                return SDK_PED_NOPIN;
            }

            if (pstPinCfg->ePinMode == SDK_PED_IC_OFFLINE_PIN)
            {
                memcpy(pheOut, &temp[0], (temp[0] + 1));
            }
            else if(SDK_PED_SM4 == pstPinCfg->eKeyType)
            {
                pheOut[0] = 16;
                return sdkPedSM4InnerPinInput(pstPinCfg->ucTpkIndex, &temp[1], pstPinCfg->uiBuf, &pheOut[1]);
            }
            else
            {
                pheOut[0] = 8;
                i = temp[0];
                temp[i + 1] = 0;
                return sPedE98(tpkindex, &temp[1], pstPinCfg->hePan, &pheOut[1]);
            }
        }
    }
    return SDK_OK;
}
/*****************************************************************************
** Descriptions:	输入密码扩展接口
** Parameters:    	const SDK_PED_PIN_CFG * pstPinCfg
                               const u8 * pbcAmt
                               u8 * pheOut
                               s32 (*sdkCallFun)(u8 *pStrDisp
                               const u32 uiMode
                               const s32 siDispRow
                               void *pVar)
                               void *data
** Returned value:	
** Created By:		fusuipu  2016.07.20
** Remarks: 		满足由用户指定输入密码显示位置与样式的需求
*****************************************************************************/
s32 sdkPEDInputPINEx(const SDK_PED_PIN_CFG * pstPinCfg, const u8 * pbcAmt, u8 * pheOut, s32 (*callback)(u8 *pStrDisp, const u32 uiMode, const s32 siDispRow, void *pVar), void *data)
{
    u8 temp[128], buf[128], iccRandom[8], pwd[13], i;
    u16 le = 0, cmd[5];
    u8 tpkindex = 0;
    s32 rslt;
    SDK_SYS_EXT_PARA *ext_para = NULL;
    u8 min_len = 0, max_len = 0;

    // 输入参数合法性判断
//    Assert(pstPinCfg != NULL);
//    Assert(pheOut != NULL);
//    Assert(pstPinCfg->ucTpkIndex < 125);
    if ((pstPinCfg == NULL) || (pheOut == NULL) || (pstPinCfg->ucTpkIndex >= 125))                        //shijianglong 2013.01.28 15:10
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
    tpkindex = sPedGetFactWkIndex(pstPinCfg->ucTpkIndex, pstPinCfg->eKeyType);

    sdkSysPlaySoundFile("/mtd0/res/inputpwd.wav", 1);                        //shiweisong 2013.03.08 11:50
/*=======BEGIN: fusuipu 2016.04.05  9:49 modify 增加使用用户指定密钥与===========*/

    if(pstPinCfg->ePinMode == SDK_PED_PIN_DIVER)
    {
        return sdkPEDInputPinWithUsrKey(pstPinCfg, pbcAmt, pheOut);
    }
/*====================== END======================== */

    if(NULL != pstPinCfg->pVar)
    {
        ext_para = (SDK_SYS_EXT_PARA *)pstPinCfg->pVar;
    }

    if (sdkPEDIsWithPinpad())                        // 带密码键盘
    {
        if (pstPinCfg->ePinMode == SDK_PED_IC_OFFLINE_PIN)
        {
            Verify(sdkGetRandom(iccRandom, 8) >= 0);
            rslt = sPedSendIccRandom(iccRandom);

            if (rslt != SDK_OK)
            {
                Assert(0);                        //shiweisong 2013.01.31 13:43
                return SDK_ERR;
            }
        }

        //增加金额参数入口
        //---//zxx 20130226 13:44
        if (pbcAmt != NULL)
        {
            sdk_ped_send(FIFO_DISPAMOUNT, pbcAmt, 6);
            cmd[0] = FIFO_PINPADOK;
            rslt = sdk_ped_receive(cmd, 1, NULL, NULL, SDK_PED_TIMEOUT, false);

            if (rslt != SDK_OK)
            {
                Assert(0);
                return SDK_ERR;
            }
        }
        //---//
        memset(temp, 0, sizeof(temp));
        le = 0;

        temp[le++] = tpkindex;

        switch (pstPinCfg->ePinMode)
        {
             case SDK_PED_IC_ONLINE_PIN:                 // IC卡联机硬加密

               if(SDK_PED_SM4 != pstPinCfg->eKeyType)
               {
                   temp[le++] = 0;
               }
               else
               {
                   temp[le++] = 0x04;                   //IC卡联机硬加密使用SM4算法
               }
               break;

             case SDK_PED_IC_OFFLINE_PIN:
               temp[le++] = 0x55;                       // IC卡软加密
               break;

             case SDK_PED_MAG_PIN:

               if(SDK_PED_SM4 != pstPinCfg->eKeyType)
               {
                   temp[le++] = 1;                      // 加密方法,带主帐号加密,最大6位密码
               }
               else
               {
                   temp[le++] = 0x04;                   //IC卡联机硬加密使用SM4算法
               }
               break;

             default:
               Assert(0);                        //shiweisong 2013.01.31 13:43
               break;
        }

        if(SDK_PED_SM4 != pstPinCfg->eKeyType)
        {
            memcpy(&temp[le], pstPinCfg->hePan, 8);
            le += 8;
        }
        else
        {
            memcpy(&temp[le], pstPinCfg->uiBuf, 16);
            le += 16;
            temp[le++] = 1;
            temp[le++] = 4;
            temp[le++] = 12;
        }
        Assert(le <= sizeof(temp));                        //防止有些时候不小心改错了 shiweisong 2013.01.31 13:52

        while (1)
        {
            sdk_ped_send(FIFO_INPUTPIN, temp, le);
//            cmd[0] = FIFO_PINPADOK;
            cmd[0] = FIFO_INPUTPINOK;                        //shijianglong 2012.12.11 10:0修改指令错误
            cmd[1] = FIFO_VERIFYAMOUNT;
            cmd[2] = FIFO_REQUESTRANDOM;
            /*=======BEGIN: lilin20131226 modify===========*/
            cmd[3] = FIFO_DESENCRYPTERROR;                        //增加密码键盘加密错误返回
            rslt = sdk_ped_receive(cmd, 4, temp, &le, pstPinCfg->iTimeOut, true);
            /*================ END================== */

            if (rslt != SDK_OK)
            {
                return rslt;
            }
            else
            {
//                if(cmd[0] == FIFO_PINPADOK)
                if (cmd[0] == FIFO_INPUTPINOK)                        //shijianglong 2012.12.11 10:0修改指令错误
                {
                    //if(temp[0])
                    if (!temp[0])                        //zhuoquan 20130204 返回无密码
                    {
                        return SDK_PED_NOPIN;
                    }

                    if(SDK_PED_SM4 != pstPinCfg->eKeyType)
                    {
                        memcpy(pwd, &temp[1], 8);
                        i = 8;
                    }
                    else
                    {
                        memcpy(pwd, &temp[1], 16);
                        i = 16;
                    }

                    if (pstPinCfg->ePinMode == SDK_PED_IC_OFFLINE_PIN)
                    {
                        sdkDesS(0, pwd, iccRandom);                                        // 随机密钥解密
                        i = pwd[0];

                        if (i > 12)
                        {
                            i = 12;
                        }
                        memset(buf, 0, sizeof(buf));
                        Verify(sdkBcdToAsc(buf, &pwd[1], (i + 1) / 2) >= 0);
                        memset(pwd, 0, sizeof(pwd));
                        memcpy(pwd, buf, i);
                    }
                    /*=======BEGIN: lilin20120814 modify===========*/
                    pheOut[0] = i;
                    memcpy(&pheOut[1], pwd, i);
                    return SDK_OK;
                }
                else if (cmd[0] == FIFO_VERIFYAMOUNT && pstPinCfg->ePinMode != SDK_PED_MAG_PIN)
                {
                    if (temp[0] == 0x43)                        //shijianglong 2013.07.31 11:39  zxx
                    {
                        return SDK_ESC;
                    }
                    return SDK_OK;
                }
                else if (cmd[0] == FIFO_REQUESTRANDOM && pstPinCfg->ePinMode == SDK_PED_IC_OFFLINE_PIN)
                {
                    Verify(sdkGetRandom(iccRandom, 8) >= 0);
                    rslt = sPedSendIccRandom(iccRandom);

                    if (rslt != SDK_OK)
                    {
                        Assert(0);                        //shiweisong 2013.01.31 13:47
                        return SDK_ERR;
                    }
                    continue;
                }
                /*=======BEGIN: lilin20131226 modify===========*/
                //增加处理密码键盘失败处理
                else if(cmd[0] == FIFO_DESENCRYPTERROR)
                {
                    Assert(0);
                    return SDK_ERR;
                }
                /*================ END================== */
                else
                {
                    continue;
                }
            }
        }
    }
    else                        // 不带密码键盘
    {
        memset(temp, 0, sizeof(temp));

        i = (pstPinCfg->ePinMode == SDK_PED_MAG_PIN) ? 6 : 12;

        if(NULL != ext_para && 0x01 == ext_para->siCmd)
        {
            min_len = ext_para->uiLPara;
            max_len = (ext_para->uiWPara > i ? i : ext_para->uiWPara);

            if(min_len > max_len)
            {
                Assert(0);
                return SDK_PARA_ERR;
            }
        }
        else
        {
            min_len = 0;
            max_len = i;
        }
        
        /*=======BEGIN: 李琳20121106 modify===========*/
        rslt = sdkKbGetScanfEx(pstPinCfg->iTimeOut, temp, min_len, max_len, SDK_MMI_NUMBER | SDK_MMI_PWD, pstPinCfg->ucRow, callback, data, 0);

        if (rslt == SDK_KEY_ESC)
        {
            return SDK_ESC;
        }

        if (rslt == SDK_TIME_OUT)
        {
            return SDK_TIME_OUT;
        }

        /*================ END================== */
        if (temp[0] > 0 && temp[0] < 4)                        // 密码长度小于4
        {
            return SDK_PED_PIN_FORMAT_ERR;
        }
        else
        {
            if (!temp[0])                        //zhuoquan 20130204 返回无密码
            {
                return SDK_PED_NOPIN;
            }

            if (pstPinCfg->ePinMode == SDK_PED_IC_OFFLINE_PIN)
            {
                memcpy(pheOut, &temp[0], (temp[0] + 1));
            }
            else if(SDK_PED_SM4 == pstPinCfg->eKeyType)
            {
                pheOut[0] = 16;
                return sdkPedSM4InnerPinInput(pstPinCfg->ucTpkIndex, &temp[1], pstPinCfg->uiBuf, &pheOut[1]);
            }
            else
            {
                pheOut[0] = 8;
                i = temp[0];
                temp[i + 1] = 0;
                return sPedE98(tpkindex, &temp[1], pstPinCfg->hePan, &pheOut[1]);
            }
        }
    }
    return SDK_OK;
}
/*****************************************************************************
** Descriptions:	内置密码键盘显示交易金额
** Parameters:          u8 *pucAmt
                               s32 len
                               u8 *lpOut
** Returned value:
** Created By:		fusuipu  2013.12.16
** Remarks:
*****************************************************************************/
static s32 sdkPEDDispAmtWithoutPinpad(const u8 * pucAmt, s32 len, SDK_PED_SYMBOL eSymbol, u8 * lpOut)
{
    u8 temp[128] = {0};
    u8 buf[128] = {0};
    s32 i = 0;

    if(NULL == pucAmt || NULL == lpOut)
    {
        return SDK_PARA_ERR;
    }

    for(i = 0; i < len; i++)
    {
        if(0 != pucAmt[i])
        {
            break;
        }
    }

    sdkBcdToAsc(temp, &pucAmt[i], len - i);
    i = 0;

    while('0' == temp[i])
    {
        i++;
    }

    strcpy(buf, &temp[i]);
    memset(temp, 0, sizeof(temp));
    memcpy(temp, "0.00", 4);

    if(strlen(buf) == 1)                        //只输入了一位
    {
        temp[0] = '0';
        temp[1] = '.';
        temp[2] = '0';
        temp[3] = buf[0];
    }
    else if(strlen(buf) == 2)                        //只输入了两位
    {
        temp[0] = '0';
        temp[1] = '.';
        temp[2] = buf[0];
        temp[3] = buf[1];
    }
    else if(strlen(buf) > 2)                        //金额输入的长度大于2
    {
        for(i = 0; i < (strlen(buf) - 2); i++)
        {
            temp[i] = buf[i];
        }

        temp[i] = '.';
        i++;
        temp[i] = buf[i - 1];
        i++;
        temp[i] = buf[i - 1];
    }

    if(SDK_PED_NEGATIVE == eSymbol)
    {
        strcpy(lpOut, "-");
        strcat(lpOut, temp);
    }
    else
    {
        strcpy(lpOut, temp);
    }
    return SDK_OK;
}

/*****************************************************************************
** Descriptions:	显示确认金额
** Parameters:          const u8 *pbcAmt 6字节bcd
** Returned value:	用户按确认返回OK，其它情况返回err
** Created By:		lqq2012.11.26
** Remarks:
*****************************************************************************/
s32 sdkPEDDispAmt(SDK_PED_SYMBOL eSymbol, const u8 * pbcAmt)
{
    u16 cmd[5];
    u8 temp[30] = {0};
    u16 len = 0;
    s32 ret = 0;

//    Assert(pbcAmt != NULL);
    if (pbcAmt == NULL)                        //shijianglong 2013.01.28 15:10
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
//    sdk_ped_send(FIFO_DISPAMOUNT, pbcAmt, 6);
//    cmd[0] = FIFO_PINPADOK;
//    return sdk_ped_receive(cmd, 1, NULL, NULL, SDK_PED_TIMEOUT, false);

    memset(temp, 0, sizeof(temp));
    temp[len++] = (eSymbol == SDK_PED_POSITIVE) ? 0x00 : 0xff;
    //---//zxx 20130227 11:41统一金额位数为6位bcd码
    len += 2;                        //跳过两位
    memcpy(&temp[len], pbcAmt, 6);
    len += 6;

    //----//
    /*=======BEGIN: fusuipu 2013.12.16  13:39 modify 内置密码键盘直接在pos上显示===========*/
    if(false == sdkPEDIsWithPinpad())
    {
//        if(SDK_OK == sdkPEDDispAmtWithoutPinpad(pbcAmt, 6, eSymbol, amtbuf))
//        {
//            sdkDispClearScreen();
//            sdkDispFillRowRam(SDK_DISP_LINE1, 0, "交易金额", SDK_DISP_FDISP | SDK_DISP_CDISP | SDK_DISP_INCOL);
//            sdkDispFillRowRam(SDK_DISP_LINE3, 0, amtbuf, SDK_DISP_FDISP | SDK_DISP_CDISP | SDK_DISP_INCOL);
//            sdkDispFillRowRam(SDK_DISP_LINE5, 0, "确认          取消", SDK_DISP_FDISP | SDK_DISP_CDISP | SDK_DISP_INCOL);
//            sdkDispBrushScreen();
//            return sdkKbWaitKey(SDK_KEY_MASK_ENTER | SDK_KEY_MASK_ESC, 0);
//        }
        return SDK_OK;                        //如果不是
    }
    /*====================== END======================== */

    sdk_ped_send(FIFO_VERIFYAMOUNT, temp, len);
    cmd[0] = FIFO_VERIFYAMOUNT;
    memset(temp, 0, sizeof(temp));
    ret = sdk_ped_receive(cmd, 1, temp, &len, SDK_PED_TIMEOUT * 15, false);                        //zxx 20130131 16:11超时长一点

    if (ret == SDK_OK)
    {
        //45为确认，43为取消
        if (temp[0] == 0x45) {return SDK_OK; }
        else{return SDK_ERR; }
    }
    return ret;
}

/*****************************************************************************
** Descriptions:
** Parameters:          SDK_PED_BALANCE_SYMBOL eSymbol:
                                SDK_PED_POSITIVE: 余额为正值
                                SDK_PED_NEGATIVE: 余额为负值

                               const u8 *pbcBalance:余额（6 字节右对齐压缩BCD码）
** Returned value:
** Created By:		lqq2012.11.26
** Remarks:
*****************************************************************************/
s32 sdkPEDDispBalance(SDK_PED_SYMBOL eSymbol, const u8 * pbcBalance)
{
    u8 temp[256] = {0};
    u16 len = 0, cmd[5] = {0};

//    Assert(pbcBalance != NULL);
    if (pbcBalance == NULL)                        //shijianglong 2013.01.28 15:10
    {
        Assert(0);
        return SDK_PARA_ERR;
    }

    if(false == sdkPEDIsWithPinpad())
    {
        if(SDK_OK == sdkPEDDispAmtWithoutPinpad(pbcBalance, 6, eSymbol, temp))
        {
            sdkDispClearScreen();
            sdkDispFillRowRam(SDK_DISP_LINE1, 0, "余额", SDK_DISP_FDISP | SDK_DISP_CDISP | SDK_DISP_INCOL);
            sdkDispFillRowRam(SDK_DISP_LINE3, 0, temp, SDK_DISP_FDISP | SDK_DISP_CDISP | SDK_DISP_INCOL);
            sdkDispFillRowRam(sdk_dev_get_max_line(), 0, "确认          取消", SDK_DISP_FDISP | SDK_DISP_CDISP | SDK_DISP_INCOL);
            sdkDispBrushScreen();
            return sdkKbWaitKey(SDK_KEY_MASK_ENTER | SDK_KEY_MASK_ESC, 0);
        }
        return SDK_ERR;                        //如果不是
    }
    memset(temp, 0, sizeof(temp));
    temp[len++] = (eSymbol == SDK_PED_POSITIVE) ? 0x00 : 0xff;
    //---//zxx 20130227 11:41统一金额位数为6位bcd码
    len += 2;                        //跳过两位
    memcpy(&temp[len], pbcBalance, 6);
    len += 6;
    //----//
    sdk_ped_send(FIFO_DISPBALANCE, temp, len);
    cmd[0] = FIFO_PINPADOK;
    return sdk_ped_receive(cmd, 1, NULL, NULL, SDK_PED_TIMEOUT, false);
}

/*****************************************************************************
** Descriptions: 密码键盘取消
** Parameters:          void
** Returned value:
** Created By:		lqq2012.11.26
** Remarks:
*****************************************************************************/
void sdkPEDCancel(void)
{
    sdk_ped_send(FIFO_CANCELKEY, NULL, 0);
}

#if 0 /*Modify by shijianglong at 2013.05.10  15:20 */

/*****************************************************************************
** Descriptions:
** Parameters:          u8 iKeyIndex  密钥索引
                                bool ucTmkOrWk    主密钥0x01   工作密钥0x02
                               u8 ucCrypt 加密0x41             解密0x42
                               u8 *pheIn  8byte
                               u8 *pheOut 8byte
** Returned value:
** Created By:		lqq2012.11.26
** Remarks:
*****************************************************************************/
static s32 sPedDesCrypt(u8 ucKeyIndex, u8 ucTmkOrWk, u8 ucCrypt, const u8 * pheIn, u8 * pheOut)
{
//sdkpeddisp.c中抠过来//zxx 3.0 20130228 16:52
//--------------------------------------------------//
    typedef struct
    {
        u8 Step;                        // 接收步数
        u8 HeadLen;                        // 接收0x01长度
        u8 Text;                        // 贴标识
        u8 Comm;                        // 接收到的命令
        u8 Ret;                        // 数据状态 0:正常等待 1:接收到正确的数据 2:接收数据错误
        u8 Cs;                        // 校验和
        u8 Buf[512];                        // 数据
        u16 Len;                        // 数据长度
        u16 LenCun;                        // 数据接收到的长度
    } UartData;

    typedef struct
    {
        UartData *RecvUartData;                        // 接收串口数据结构体
        u8 Cmd;                                             // 发送的命令
        u8 const *pDataIn;                                  // 发送的数据
        u16 Len;                                            // 发送数据的长度
    } UartSendCfg;
    extern s32 SendDataToPed(const UartSendCfg * UartSend);
//------------------------------------------------------//
    u8 temp[256];
    u16 i = 0;                        // cmd[5];
    s32 rslt = 0;

    UartSendCfg UartSend;
    UartData RecvData;

    memset(temp, 0, sizeof(temp));
    i = 0;
    temp[i++] = ucCrypt;                        //shijianglong 2013.01.08 9:56修改入参不对问题。
    temp[i++] = 0x00;                        //ALG
    temp[i++] = ucTmkOrWk;
    temp[i++] = ucKeyIndex;
    memcpy(&temp[i], pheIn, 8);
    i += 8;
    //注:当机器有内置非接的时候，panel处理DD指令会优先发往内置非接
    //导致无法发送到外置密码键盘
    //可以考虑用FIFO_ENFORCERFDIRECTION强制指定非接方向，可用但这里不合适
    //实现主密钥加密直接用串口算了
#if 0 /*Modify by zxx 3.0 at 2013.02.28  16:33 */
    sdk_ped_send(FIFO_POSTKEYPADDD, temp, i);
    cmd[0] = FIFO_POSTKEYPADDD;
    rslt = sdk_ped_receive(cmd, 1, temp, &i, SDK_PED_TIMEOUT, false);

    if (rslt != SDK_OK)
    {
        return rslt;
    }
    else
    {
        if (temp[0] == ucCrypt && temp[1] == 0x00)
        {
            memcpy(pheOut, &temp[2], 8);
            return SDK_OK;
        }
    }
    return SDK_ERR;                        //

#endif /* if 0 */
    memset(&UartSend, 0, sizeof(UartSend));
    memset(&RecvData, 0, sizeof(RecvData));

    UartSend.RecvUartData = &RecvData;
    UartSend.Cmd = 0xdd;
    UartSend.pDataIn = temp;
    UartSend.Len = i;
    rslt = SendDataToPed(&UartSend);

    if (rslt == SDK_OK)
    {
        if (RecvData.Buf[0] == ucCrypt && RecvData.Buf[1] == 0x00)
        {
            memcpy(pheOut, &RecvData.Buf[2], 8);
            return SDK_OK;
        }
    }
    return SDK_ERR;
}

#endif /* if 0 */
/*****************************************************************************
** Descriptions:	K370主密钥解密函数
** Parameters:          s32 iIndex
                               const u8 *pheIn
                               u16 usInLen
                               u8 *pheOut
** Returned value:
** Created By:		fusuipu  2014.07.24
** Remarks:             K370K平台不支持DD扩展的0X20~0x26命令，如果要用主密钥解密数据，只能使用FIFO_DESDECRYPT 这条指令
                    来实现
*****************************************************************************/
static s32 sdkPEDTmkDesForK370(s32 iIndex, const u8 * pheIn, u16 usInLen, u8 * pheOut)
{
    u8 temp[1024] = {0};
    u16 len = 0;
    s32 rslt = 0;
    u16 cmd[2] = {FIFO_DESDECRYPTOK};
    s32 i = 0;

    memset(temp, 0, sizeof(temp));
    temp[len++] = iIndex;
    memcpy(&temp[len], pheIn, usInLen);
    len += usInLen;
    sdk_ped_send(FIFO_DESDECRYPT, temp, len);

    memset(temp, 0, sizeof(temp));
    rslt = sdk_ped_receive(cmd, 1, temp, &len, SDK_PED_TIMEOUT, true);

    if(SDK_OK != rslt)
    {
        Assert(0);
        Trace("libsdkped", "rslt = %d\r\n", rslt);
        return rslt;
    }
    TraceHex("libsdkped", "sdkPEDTmkDesForK370 rslt:", temp, len);
    Trace("libsdkped", "len = %d\r\n", len);
    memcpy(pheOut, &temp[1], len - 1);
    TraceHex("libsdkped", "sdkPEDTmkDesForK370 pheOut:", pheOut, i);
    return SDK_OK;
}

/*****************************************************************************
** Descriptions:	基于SM4算法的主秘钥加解密
** Parameters:          s32 iIndex
                               SDK_PED_KEY_TYPE eKeyType
                               SDK_PED_DES_TYPE eKeyDesType
                               SDK_PED_CRYPT eCrypt
                               SDK_PED_CRYPT_WAY eCryptWay
                               const u8 * pheIn
                               u16 usInLen
                               u8 * pheOut
** Returned value:
** Created By:		fusuipu  2015.04.14
** Remarks:
*****************************************************************************/
static s32 sdkPEDTmkSm4(s32 iIndex, SDK_PED_DES_TYPE eKeyDesType, SDK_PED_CRYPT eCrypt, const u8 * pheIn, s32 usInLen, u8 * pheOut)
{
    u8 factindex = 0;

    if(iIndex < 0 || NULL == pheIn || NULL == pheOut || usInLen < 0)
    {
        return SDK_PARA_ERR;
    }

    if(SDK_PED_SM4 != eKeyDesType)
    {
        return SDK_PARA_ERR;
    }
    factindex = sPedGetFactTmkIndex((u8)iIndex, eKeyDesType);

    if (eCrypt == SDK_PED_ENCRYPT)
    {
        return sdk_ped_wk_des_sm4(0x00, 0x00, factindex, pheOut, pheIn, usInLen, 10000);            //最大超时时间10s
    }
    else
    {
        return sdk_ped_wk_des_sm4(0x01, 0x00, factindex, pheOut, pheIn, usInLen, 10000);            //最大超时时间10s
    }
}

/*****************************************************************************
** Descriptions:	扩展密码键盘des加密
** Parameters:          s32 iIndex:索引
                               SDK_PED_KEY_TYPE eKeyType:主密钥或者工作密钥
                               SDK_PED_DES_TYPE eKeyDesType:主密钥或者工作密钥des类型
                               SDK_PED_CRYPT eCrypt:加密或者解密
                               SDK_PED_CRYPT_WAY eCryptWay:用于加解密的密钥长度类型
                               const u8 *pheIn:待加密数据
                               u16 usInLen:数据长度，必须8的倍数，否则参数错误
                               u8 *pheOut:获取返回数据
** Returned value:
** Created By:		zxx 2013.05.10
** Remarks:
*****************************************************************************/
s32 sdkPEDDesEx(s32 iIndex, SDK_PED_KEY_TYPE eKeyType, SDK_PED_DES_TYPE eKeyDesType, SDK_PED_CRYPT eCrypt, SDK_PED_CRYPT_WAY eCryptWay, const u8 * pheIn, u16 usInLen, u8 * pheOut)
{
    u8 factindex = 0;
    u8 destype = 0, key_type = 0, i = 0;
    u8 temp[600] = {0};
    u16 cmd[5] = {0}, send_cmd[10] = {0};
    s32 rslt = 0;
    u16 len = 0;

    if ((pheIn == NULL) || pheOut == NULL || usInLen > 512 || usInLen == 0 || usInLen % 8 != 0
        || (eKeyDesType == SDK_PED_DES_SINGLE && (eCryptWay == SDK_PED_DES_MIDDLE8 || eCryptWay == SDK_PED_DES_LAST8))
        || (eKeyType == SDK_PED_TMK && iIndex >= 100)
        || (eKeyType == SDK_PED_WK && iIndex >= 125))
    {
        Assert(0);
        return SDK_PARA_ERR;
    }

    if (eKeyType == SDK_PED_TMK)
    {
        factindex = sPedGetFactTmkIndex((u8)iIndex, eKeyDesType);

        /*=======BEGIN: fusuipu 2014.07.24  16:44 modify 根据和彭学斌确认
           K370K平台不支持DD扩展的0X20~0x26命令，如果要用主密钥解密数据，只能使用FIFO_DESDECRYPT 这条指令
           来实现
           ===========*/
        if(SDK_SYS_MACHINE_K370 == sdkSysGetMachineCode(NULL))
        {
            return sdkPEDTmkDesForK370(factindex, pheIn, usInLen, pheOut);
        }
        /*====================== END======================== */
    }
    else if (eKeyType == SDK_PED_WK)
    {
        factindex = sPedGetFactWkIndex((u8)iIndex, eKeyDesType);
    }

    if (eCrypt == SDK_PED_ENCRYPT) {destype |= 0x80; }

    if (eCryptWay == SDK_PED_DES_TOP8)
    {
        destype |= 0x01;
    }
    else if (eCryptWay == SDK_PED_DES_MIDDLE8)
    {
        destype |= 0x02;
    }
    else if (eCryptWay == SDK_PED_DES_LAST8)
    {
        destype |= 0x03;
    }
    temp[len++] = destype;
    temp[len++] = factindex;
    temp[len++] = 1;

    if (eKeyDesType == SDK_PED_DES_TRIPLE && eCryptWay == SDK_PED_DES_ALL && (usInLen == 16 || usInLen == 24))                        //shijianglong 2013.07.15 13:58
    {
        temp[len++] = 0x10;                        // 3des缓存索引
    }
    else
    {
        temp[len++] = 0;                        //des缓存索引
    }
    memcpy(send_cmd, temp, len);
    memcpy(&temp[len], pheIn, usInLen);
    len += usInLen;

    if (eKeyType == SDK_PED_TMK)
    {
        key_type = FIFO_PCIMKDES;
    }
    else if (eKeyType == SDK_PED_WK)
    {
        key_type = FIFO_PCIWKDES;
    }
    sdk_ped_send(key_type, temp, len);
    cmd[0] = key_type;

    rslt = sdk_ped_receive(cmd, 1, temp, &len, SDK_PED_TIMEOUT, false);

    if (rslt != SDK_OK)
    {
        return rslt;
    }
    else
    {
        if (temp[1] == 0x00)
        {
            memcpy(pheOut, &temp[2], len - 2);
            return SDK_OK;
        }
        /*DD 21 指令在内置密码键盘内，可以加密超过8字节数据，外置密码键盘超过8字节加密数据失败
           后边产品赵健修复DD 21 指令，但是考虑到外边已有的外置密码键盘，所以在sdk内部处理，如果
           外置密码键盘不具备多字节加密数据能力，且应用有调用，则在sdk内部自动按照每8个字节一组
           进行处理*/                                                                                                                                                                                                                                                                                                    //fusuipu 2015.11.20 11:2
        else
        {
            if(true == sdk_dev_get_pinpadstate() && usInLen > 8)
            {
                for(i = 0; i < usInLen / 8; i++)
                {
                    memset(temp, 0, sizeof(temp));
                    memcpy(temp, send_cmd, 4);
                    len = 4;
                    memcpy(&temp[len], &pheIn[i * 8], 8);
                    len += 8;
                    sdk_ped_send(key_type, temp, len);
                    cmd[0] = key_type;
                    rslt = sdk_ped_receive(cmd, 1, temp, &len, SDK_PED_TIMEOUT, false);

                    if (rslt != SDK_OK || temp[1] != 0x00)
                    {
                        Assert(0);
                        memset(pheOut, 0, i * 8);
                        return SDK_ERR;
                    }
                    memcpy(&pheOut[i * 8], &temp[2], 8);
                }

                return SDK_OK;
            }
        }
    }
    Assert(0);
    return SDK_ERR;
}

/*****************************************************************************
** Descriptions:	主密钥加解密数据
** Parameters:          s32 iTmkIndex
                               SDK_PED_DES_TYPE eTmkType
                               SDK_PED_CRYPT eCrypt
                               const u8 *pheIn
                               u8 *pheOut
** Returned value:
** Created By:		lqq2012.11.26
** Remarks:
*****************************************************************************/
s32 sdkPEDTmkDes(s32 iTmkIndex, SDK_PED_DES_TYPE eTmkType, SDK_PED_CRYPT eCrypt, const u8 * pheIn, u8 * pheOut)
{
    if ((pheIn == NULL) || (pheOut == NULL) || (iTmkIndex < 0) || (iTmkIndex >= 100))                        //shijianglong 2013.01.28 15:10
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
/*=======BEGIN: fusuipu 2015.04.14  18:40 modify 增加SM4主秘钥加解密===========*/

    if(SDK_PED_SM4 == eTmkType)
    {
        return sdkPEDTmkSm4(iTmkIndex, SDK_PED_SM4, eCrypt, pheIn, 16, pheOut);
    }
    else
/*====================== END======================== */
    {
        return sdkPEDDesEx(iTmkIndex, SDK_PED_TMK, eTmkType, eCrypt, SDK_PED_DES_ALL, pheIn, 8, pheOut);
    }
}

/*****************************************************************************
** Descriptions:	K370工作密钥加解密
** Parameters:          s32 iWkIndex
                               SDK_PED_DES_TYPE eWkType
                               SDK_PED_CRYPT eCrypt
                               const u8 *pheIn
                               u8 *pheOut
** Returned value:
** Created By:		lqq2012.11.26
** Remarks:
*****************************************************************************/
s32 sdkPEDWkDesForK370(s32 iWkIndex, SDK_PED_DES_TYPE eWkType, SDK_PED_CRYPT eCrypt, const u8 * pheIn, u8 * pheOut)
{
    s32 rslt = 0;
    u8 tak_buf[30] = {0};
    bool bEncrypt = false;

    if((rslt = sdkGetK370TakKey(iWkIndex, eWkType, tak_buf)) !=  SDK_OK)
    {
        return rslt;
    }

    if(SDK_PED_ENCRYPT == eCrypt)
    {
        bEncrypt = true;
    }
    else if(SDK_PED_DECRYPT == eCrypt)
    {
        bEncrypt = false;
    }
    else
    {
        return SDK_PARA_ERR;
    }
    memcpy(pheOut, pheIn, 8);

    if(SDK_PED_DES_SINGLE == eWkType)
    {
        sdkDesS(bEncrypt, pheOut, tak_buf);
    }
    else
    {
        sdkDes3S(bEncrypt, pheOut, tak_buf);
    }
    return SDK_OK;
}

/*****************************************************************************
** Descriptions:	工作密钥加解密
** Parameters:          s32 iWkIndex
                               SDK_PED_DES_TYPE eWkType
                               SDK_PED_CRYPT eCrypt
                               const u8 *pheIn
                               u8 *pheOut
** Returned value:
** Created By:		lqq2012.11.26
** Remarks:
*****************************************************************************/
s32 sdkPEDWkDes(s32 iWkIndex, SDK_PED_DES_TYPE eWkType, SDK_PED_CRYPT eCrypt, const u8 * pheIn, u8 * pheOut)
{
    u8 wkindex;

    // 输入参数合法性判断
//    Assert(pheIn != NULL);
//    Assert(pheOut != NULL);
//    Assert(iWkIndex >= 0 && iWkIndex < 125);
    if ((pheIn == NULL) || (pheOut == NULL) || (iWkIndex < 0) || (iWkIndex >= 125))                        //shijianglong 2013.01.28 15:10
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
    wkindex = sPedGetFactWkIndex((u8)iWkIndex, eWkType);

    if(SDK_SYS_MACHINE_K370 == sdkSysGetMachineCode(NULL))
    {
        return sdkPEDWkDesForK370(wkindex, eWkType, eCrypt, pheIn, pheOut);
    }

    //crypt = (eCrypt == SDK_PED_ENCRYPT) ? 0x41 : 0x42;   //ext_cmd  0x41:加密 0x42:解密
    //return sPedDesCrypt(wkindex, 0x02, crypt, pheIn, pheOut);
    if (eCrypt == SDK_PED_ENCRYPT)                        //shijianglong 2013.01.11 10:53
    {
        if(SDK_PED_SM4 != eWkType)
        {
            return sPedWkEncryptData(wkindex, pheOut, (u8 *)pheIn, 8);
        }
        else
        {
            return sdk_ped_wk_des_sm4(0x00, 0x01, wkindex, pheOut, pheIn, 16, 10000);        //最大超时时间10s
        }
    }
    else
    {
        if(SDK_PED_SM4 != eWkType)
        {
            return sPedWkDecryptData(wkindex, pheOut, (u8 *)pheIn, 8);
        }
        else
        {
            return sdk_ped_wk_des_sm4(0x01, 0x01, wkindex, pheOut, pheIn, 16, 10000);                 //最大超时时间10s
        }
    }
}

/*****************************************************************************
** Descriptions:	获取密码键盘版本
** Parameters:          u8 *pasVer 获取到的版本号
** Returned value:
** Created By:		lqq2012.11.26
** Remarks:
*****************************************************************************/
s32 sdkPEDGetPedVersion(u8 * pasPedVer)
{
    u8 temp[256];
    u16 len = 0, cmd[5];
    s32 rslt;

//    Assert(pasPedVer != NULL);
    if (pasPedVer == NULL)                        //shijianglong 2013.01.28 15:10
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
    sdk_ped_send(FIFO_PINPADVER, NULL, 0);
    cmd[0] = FIFO_ACKVER;
    rslt = sdk_ped_receive(cmd, 1, temp, &len, SDK_PED_TIMEOUT, false);

    if (rslt != SDK_OK)
    {
        return rslt;
    }
    memcpy(pasPedVer, temp, len);
    return len;
}

/*******************************************************************
   版          权: 新国都
   作          者: 郭泽贤
   函数名称: Private_sdkPEDSetSN
   函数功能: 设置密码键盘序列号
   输入参数:1. pDataIn: 密码键盘机身号
   输出参数: 无
   返   回  值:
                        SDK_OK: 成功
                        SDK_PARA_ERR: 参数错误
                        SDK_ERR: 失败
                        SDK_TIME_OUT: 密码键盘响应超时(单位: ms)
   修改备注:
   日期时间:2012.05.10 14:33:37
*******************************************************************/
extern s32 sdk_ped_save_pinpad_user_date(u8 *pasIn, s32 uiInLen);
extern s32 sdk_ped_read_pinpad_user_date(u8 *pasOut, s32 *uiOutLen);
s32 sdkPEDSaveUserData(const u8 * pDataIn, const s32 ucDataLen)
{
    u8 temp[512] = {0};
    u16 len = 0, cmd[5];
    s32 rslt;

    if (pDataIn == NULL || ucDataLen == 0)
    {
        Assert(0);
        return SDK_PARA_ERR;
    }

    if(sdkPEDIsWithPinpad())
    {
        len = 0;
        temp[len++] = 0x03;
        memcpy(&temp[len], pDataIn, (u8)ucDataLen);
        len += (u8)ucDataLen;
        sdk_ped_send(FIFO_POSTKEYPADDB, temp, len);
        cmd[0] = FIFO_POSTKEYPADDB;
        memset(temp, 0, sizeof(temp));
        rslt = sdk_ped_receive(cmd, 1, temp, &len, SDK_PED_TIMEOUT, false);

        if (rslt != SDK_OK)
        {
            return rslt;
        }
        else
        {
            if (temp[0] == 0x03 && temp[1] == 0x00)
            {
                return SDK_OK;
            }
        }
        Assert(0);
        return SDK_ERR;
    }
    else
    {
        return sdk_ped_save_pinpad_user_date(pDataIn, ucDataLen);
    }
}

/*****************************************************************************
** Descriptions:	读取应用自定义数据
** Parameters:          u8 * pDataOut
                               const s32 ucOutLen
** Returned value:
** Created By:		fusuipu  2016.03.07
** Remarks:
*****************************************************************************/
s32 sdkPEDReadUserData(u8 * pDataOut, const s32 ucOutLen)
{
    u8 temp[256] = {0};
    u16 len = 0, cmd[5];
    s32 rslt;

//    Assert(pasSn != NULL);
    if (pDataOut == NULL || ucOutLen == 0)                        //shijianglong 2013.01.28 15:10
    {
        Assert(0);
        return SDK_PARA_ERR;
    }

    if(sdkPEDIsWithPinpad())
    {
        len = 0;
        temp[len++] = 0x01;                    //命令字
        temp[len++] = (u8)ucOutLen;
        sdk_ped_send(FIFO_POSTKEYPADDB, temp, len);
        cmd[0] = FIFO_POSTKEYPADDB;
        memset(temp, 0, sizeof(temp));
        rslt = sdk_ped_receive(cmd, 1, temp, &len, SDK_PED_TIMEOUT, false);

        if (rslt != SDK_OK)
        {
            return rslt;
        }
        else
        {
            if (temp[0] == 0x01 && temp[1] == 0x00)                    //zxx 20130131 11:48加入判断，返回序列号除去长度命令字
            {
                memcpy(pDataOut, &temp[2], len - 2);
                return len - 2;
            }
        }
        Assert(0);
        return SDK_ERR;
    }
    else
    {
        len = ucOutLen;
        rslt = sdk_ped_read_pinpad_user_date(pDataOut, &len);

        if(SDK_OK == rslt)
        {
            return len;
        }
    }
}

/*****************************************************************************
** Descriptions:	获取密码键盘机身号
** Parameters:          u8 *pasSn
** Returned value:
** Created By:		lqq2012.11.26
** Remarks:
*****************************************************************************/
s32 sdkPEDGetPedSn(u8 * pasSn)
{
    u8 temp[256] = {0};
    u16 len = 0, cmd[5];
    s32 rslt;

//    Assert(pasSn != NULL);
    if (pasSn == NULL)                        //shijianglong 2013.01.28 15:10
    {
        Assert(0);
        return SDK_PARA_ERR;
    }
    len = 0;
    temp[len++] = 0x04;                        //命令字
    sdk_ped_send(FIFO_POSTKEYPADDB, temp, len);
    cmd[0] = FIFO_POSTKEYPADDB;
    memset(temp, 0, sizeof(temp));
    rslt = sdk_ped_receive(cmd, 1, temp, &len, SDK_PED_TIMEOUT, false);

    if (rslt != SDK_OK)
    {
        return rslt;
    }
    else
    {
        if (temp[0] == 0x04 && temp[1] == 0x00)                        //zxx 20130131 11:48加入判断，返回序列号除去长度命令字
        {
            memcpy(pasSn, &temp[2], len - 2);
            return len - 2;
        }
    }
    Assert(0);
    return SDK_ERR;
}

/*****************************************************************************
** Descriptions:	设置密码键盘语言
** Parameters:          SDK_PED_LANGUAGE eLanguage
** Returned value:
** Created By:		lqq2012.11.26
** Remarks:
*****************************************************************************/
s32 sdkPEDSetLanguage(SDK_PED_LANGUAGE eLanguage)
{
    u8 buf[256];
    u16 len = 0, cmd[5];
    s32 rslt = 0;

    memset(buf, 0, sizeof(buf));
    len = 0;
    buf[len++] = 0x75;                        // 命令字

    buf[len] = 0x00;                        //默认中文

    if (eLanguage == SDK_PED_EN)
    {
        buf[len] = 0x01;                        //英文
    }
    len++;

    sdk_ped_send(FIFO_POSTKEYPADDB, buf, len);
    cmd[0] = FIFO_POSTKEYPADDB;
    rslt = sdk_ped_receive(cmd, 1, buf, &len, SDK_PED_TIMEOUT, false);

    if (rslt != SDK_OK)
    {
        return rslt;
    }
    else
    {
        //zxx 20130131 14:25
        if (buf[0] == 0x75 && buf[1] == 0x00)                        //0x00成功
        {
            return SDK_OK;
        }
    }
    Assert(0);
    return SDK_ERR;
}

extern s32 PedDisplayStr(u8 * str, u8 row, u8 col, u8 atr);
extern s32 sdkPedRDispStr(u8 * str, u8 row, u8 col, u8 atr);                        //shijianglong 2013.01.14 20:27
/*****************************************************************************
** Descriptions:	在密码键盘显示数据
** Parameters:          u8 *pasStr 待显示字符串
                       u8 ucRow 待显示像素点纵向偏移
                       u8 ucCol待显示像素点横向偏移
                       u8 ucAtr对齐方式,以下方式可以异或处理
                        0x00   //正显(默认正显)
                        0x01   //反显
                        0x02   //插入一列
                        0x04   //左对齐
                        0x08   //居中
                        0x10   //右对齐
** Returned value:
** Created By:		lqq2012.11.27
** Remarks://shijianglong 2013.01.14 20:27
*****************************************************************************/
s32 sdkPEDDisplayStr(const u8 * pasStr, s32 ucRow, s32 ucCol, u32 ucAtr)
{
    FIFO fifo;
    s32 ret = 0;
    s32 timer = 0;

    if (pasStr == NULL || ucRow < 0 || ucCol < 0)                        //shijianglong 2013.01.28 15:25
    {
        Assert(0);
        return SDK_PARA_ERR;
    }

    if (sdkPEDIsWithPinpad() == false)                        //shijianglong 2013.04.19 15:11
    {
        sdkDispClearScreen();

        /*=======BEGIN: fusuipu 2013.12.19  14:13 modify 内置显示方式和外置保持一致，0为正显，1为反显===========*/
        if(1 == ucAtr)                        //1为反显
        {
            ucAtr &= ~SDK_DISP_FDISP;
            ucAtr |= SDK_DISP_NOFDISP;
        }
        else                                //0为正显，默认正显
        {
            ucAtr |= SDK_DISP_FDISP;
        }
        /*====================== END======================== */
        ret = sdkDispFillRowRam(ucRow, ucCol, pasStr, ucAtr);
        sdkDispBrushScreen();                        //显示
        sdkmSleep(3000);
        return ret;
    }
    sdk_dev_pack_fifo(FIFO_PINPADVER, "\x00", 1);
    timer = sdkTimerGetId();

    while (1)
    {
        if (sdk_dev_read_fifo(&fifo, 0))
        {
            if (fifo.Cmd == FIFO_ACKVER )
            {
                /*=======BEGIN: fusuipu 2015.02.11  15:41 modify
                   根据王宏杨反馈，G101密码键盘显示字符串非常慢，经过确认，在此次
                   需要判断fifo.Data[7]来决定后续是采用DB指令还是传统的指令显示字符串
                   之前的判断放到了上边的if里边，导致在非DB指令模式下，一定要等待超时才会
                   执行传统的显示模式，所以存在问题，此次修改，在接收到FIFO_ACKVER指令后，立即判断
                   使用DB指令和传统显示，二者选其一===========*/

                TraceHex("ped", "pinpadver is", fifo.Data, fifo.Len);
				//后边彩屏不带非接得密码键盘，也支持DB指令
                if((fifo.Data[7] &= 0x01) || memcmp(&fifo.Data[8], "1602", 4) >= 0)
                {
                    ret = SDK_OK;
                }
                else
                {
                    Assert(0);
                    ret = SDK_ERR;
                }
                /*====================== END======================== */
                break;
            }
        }

        if (sdkTimerIsEnd(timer, SDK_PED_TIMEOUT) == 1)
        {
            ret = SDK_TIME_OUT;
            break;
        }
    }

    if (ret == SDK_OK)
    {
        return sdkPedRDispStr(pasStr, ucRow, ucCol, ucAtr);
    }
    else
    {
        return PedDisplayStr(pasStr, ucRow, ucCol, ucAtr);
    }
}

extern s32 PedInputAmt(u8 * pbcDataOut);
/*****************************************************************************
** Descriptions:
** Parameters:          u8 *pbcAmtOut 输入的金额(6字节右对齐BCD)
** Returned value:
                          SDK_OK: 成功
                          SDK_PARA_ERR: 参数错误
                          SDK_TIME_OUT: 密码键盘响应超时(单位: ms)
** Created By:		lqq2012.11.27
** Remarks:
*****************************************************************************/
s32 sdkPEDInputAmt(u8 * pbcAmtOut)
{
    u8 temp[128] = {0};
//    u8 buf[128] = {0};
    s32 ret = 0;

    if (sdkPEDIsWithPinpad() == true)                        //shijianglong 2013.04.19 15:11
    {
        return PedInputAmt(pbcAmtOut);
    }
    else
    {
        ret = sdkKbGetScanf(SDK_PED_TIMEOUT * 15, temp, 0, 12, SDK_MMI_NUMBER | SDK_MMI_POINT | SDK_MMI_NOINPUT_QUIT, LINE3);
        Verify(sdkAscToBcdR(pbcAmtOut, &temp[1], 8) >= 0);
//        Verify(sdkRegulateAmount(pbcAmtOut, buf) >= 0);
        return ret;
    }
}

extern s32 PedInitKeyIC(void);
/*****************************************************************************
** Descriptions:	初始化密钥芯片
** Parameters:          void
** Returned value:
                        SDK_OK: 成功
                        SDK_ERR: 失败
                        SDK_TIME_OUT: 密码键盘响应超时(单位: ms)
** Created By:		lqq2012.11.27
** Remarks:
*****************************************************************************/
s32 sdkPEDInitKeyIC(void)
{
    return PedInitKeyIC();
}

/*****************************************************************************
** Descriptions:	获取密码键盘输入字符串
** Parameters:          SDK_PED_INPUT_STR eInputMode    输入字符串模式
                    u8 *pOut                        获取的密码键盘输入字符串
                    s32 siTimeOut                   超时时间
                    void *pVar                      预留指针变量
** Returned value:
** Created By:		fusuipu  2014.11.17
** Remarks:             SDK_PED_INPUT_PHONE     输入手机号,模式仅仅对最大长度进行了限制，只要用户输入的数字位数小于等于11位，都是允许的
                    SDK_PED_INPUT_CHECK_NUM 将Data域的提示语内容显示在LCD第一行，然后进入按键输入状态，等待用户输入，用户按【确认】键后pinpad返回数据
*****************************************************************************/
#define PED_INPUT_MAX_LEN  16
s32 sdkPEDGetInputStr(SDK_PED_INPUT_STR eInputMode, u8 * pOut, s32 siTimeOut, void *pVar)
{
    FIFO fifo;
    s32 len = 0, i = 0;
    u32 time_id = 0;
    SDK_SYS_EXT_PARA *ext_para = NULL;
    bool is_have_ext_para = false;
    u8 min_len = 0, max_len = 0;
    u8 *pstr = NULL;

    if(NULL == pOut || siTimeOut < 0)
    {
        return SDK_PARA_ERR;
    }
    memset(&fifo, 0, sizeof(FIFO));
    fifo.Data[i++] = (true == sdk_dev_get_pinpadstate() ? TOPINPAD : TOAUX51);

    if(SDK_PED_INPUT_PHONE == eInputMode)
    {
        fifo.Data[i++] = 0x00;                        //DD指令命令字 用于输入11位手机号。但pinpad仅仅对最大长度进行了限制，因此只要用户输入的数字位数小于等于11位，都是允许的。
    }
    else
    {
        //fusuipu 2016.03.10 14:22 增加密码键盘获取字符串长度可指定
        if(NULL != (ext_para = pVar) &&
           0x01 == ext_para->siCmd)
        {
            min_len = (ext_para->uiLPara >> 8) & 0xFF;
            max_len =  ext_para->uiLPara & 0xFF;

            if(min_len > max_len)
            {
                Assert(0);
                return SDK_PARA_ERR;
            }
            min_len = (min_len > PED_INPUT_MAX_LEN ? PED_INPUT_MAX_LEN : min_len);
            max_len = (max_len > PED_INPUT_MAX_LEN ? PED_INPUT_MAX_LEN : max_len);

            pstr = (u8 *)ext_para->uiWPara;
            fifo.Data[i++] = 0x31;
            len = strlen(pstr);
            len = (len > 128 ? 128 : len);
            memcpy(&fifo.Data[i], pstr, len);
            i += len;
            fifo.Data[i++] = min_len;
            fifo.Data[i++] = max_len;
        }
        else
        {
            fifo.Data[i++] = 0x30;                                  //DD指令命令字 用于输入11位手机号。但pinpad仅仅对最大长度进行了限制，因此只要用户输入的数字位数小于等于11位，都是允许的。
            len = strlen((u8 *)pVar);
            len = (len > 128 ? 128 : len);                          //标题不可能这么长的
            memcpy(&fifo.Data[i], (u8 *)pVar, len);
            i += len;
        }
    }
    sdk_dev_pack_fifo(FIFO_TEXTFROMPINPAD, &fifo.Data, i);
    time_id = sdkTimerGetId();

    while(1)
    {
        if(sdk_dev_read_fifo(&fifo, 0))
        {
            if(fifo.Cmd == FIFO_POSTKEYPADDD)
            {
                memcpy(pOut, &fifo.Data[2], fifo.Len - 2);                        //取出手机号
                return fifo.Len - 2;
            }
        }

        if(0 != siTimeOut && true == sdkTimerIsEnd(time_id, siTimeOut))
        {
            sdkPEDCancel();
            return SDK_TIME_OUT;
        }

        if(SDK_KEY_ESC == sdkKbGetKey())
        {
            sdkPEDCancel();
            return SDK_ESC;
        }
    }

    Assert(0);                        //走到这里肯定出错了
    return SDK_ERR;
}

