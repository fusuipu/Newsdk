#include "sdkGlobal.h"
#include <pthread.h>

static volatile pthread_t gpthread_lamp = 0; //线程
static volatile SDK_PED_LAMP_CTRL gstLampCtrl; //
static volatile bool bIsPthreadReset = true;
typedef enum
{
    LampRed = 0,
    LampBlue,
    LampGreen,
    LampYellow,
}eLampClor;

typedef enum
{
    SDK_PED_FIFO_CTROL = 0,     //通过FIFO指令控制
    SDK_PED_API_CTROL  = 1,     //通过调用libdev的api接口控制
}SDK_PED_LAMP_CTROL_MODE;
/*****************************************************************************
** Descriptions:	外置非接灯控制
** Parameters:          const SDK_PED_LAMP_CTRL *pstLampCtrl
** Returned value:
** Created By:		fusuipu  2014.08.14
** Remarks:
*****************************************************************************/
static s32 sdkSysCtrolLamp (u8 uiLamp1, u8 uiLamp2, u8 uiLamp3, u8 uiLamp4, u32 uiMode)
{
    u8 temp[64] = {0};
    s32 len = 0;

    if(SDK_PED_FIFO_CTROL == uiMode)
    {
        temp[len++] = (true == sdk_dev_get_pinpadstate() ? 1 : 0);         //内置非接
        temp[len++] = (true == sdkSysIsRfIn() ? 1 : 0);     // 0-外接非接键盘，1-pos机
        temp[len++] = 0x01;
        temp[len++] = 0x69;     //命令字,控制LED灯的状态，4个LED(蓝黄绿红)
        temp[len++] = uiLamp1;
        temp[len++] = uiLamp2;
        temp[len++] = uiLamp3;
        temp[len++] = uiLamp4;
        temp[len++] = 0;
        temp[len++] = 0;
        sdk_dev_pack_fifo(FIFO_ENFORCERFDIRECTION, temp, len);
    }
    else
    {
        sdk_dev_misc_ctrl_led(LampBlue, uiLamp1);
        sdk_dev_misc_ctrl_led(LampYellow, uiLamp2);
        sdk_dev_misc_ctrl_led(LampGreen, uiLamp3);
        sdk_dev_misc_ctrl_led(LampRed, uiLamp4);
    }
    return SDK_OK;
}

/*****************************************************************************
** Descriptions:	通过FIFO指令控制非接灯
** Parameters:          void *arg
** Returned value:
** Created By:		fusuipu  2014.08.18
** Remarks:
*****************************************************************************/
static void sdkSysThreadCtrolLamp (void *arg)
{
    s32 dly_time = 0;
    s32 silamp1 = 0, silamp2 = 0,  silamp3 = 0,  silamp4 = 0;
    u8 mode = ((u8*)arg)[0];
    u32 ui_time_id = 0;
    bool is_update_state = false;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL); //允许退出线程
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL); //设置立即取消


    while (1)
    {
        if(true == bIsPthreadReset)
        {
            silamp1 = (gstLampCtrl.eLamp1 == SDK_LAMP_ON ? SDK_LAMP_ON : SDK_LAMP_OFF);
            silamp2 = (gstLampCtrl.eLamp2 == SDK_LAMP_ON ? SDK_LAMP_ON : SDK_LAMP_OFF);
            silamp3 = (gstLampCtrl.eLamp3 == SDK_LAMP_ON ? SDK_LAMP_ON : SDK_LAMP_OFF);
            silamp4 = (gstLampCtrl.eLamp4 == SDK_LAMP_ON ? SDK_LAMP_ON : SDK_LAMP_OFF);
            sdkSysCtrolLamp(silamp1, silamp2, silamp3, silamp4, mode);
            bIsPthreadReset = false;
            dly_time = 0;

            if (gstLampCtrl.siTimers != 0) {ui_time_id = sdkTimerGetId(); }
        }
        dly_time = (dly_time + 1) % 50;

        if(dly_time < 2)
        {
            silamp1 = (gstLampCtrl.eLamp1 > 0 ? SDK_LAMP_ON : SDK_LAMP_OFF);
            silamp2 = (gstLampCtrl.eLamp2 > 0 ? SDK_LAMP_ON : SDK_LAMP_OFF);
            silamp4 = (gstLampCtrl.eLamp4 > 0 ? SDK_LAMP_ON : SDK_LAMP_OFF);
        }
        else
        {
            silamp1 = (gstLampCtrl.eLamp1 == SDK_LAMP_TWINK ? SDK_LAMP_OFF : gstLampCtrl.eLamp1);
            silamp2 = (gstLampCtrl.eLamp2 == SDK_LAMP_TWINK ? SDK_LAMP_OFF : gstLampCtrl.eLamp2);
            silamp4 = (gstLampCtrl.eLamp4 == SDK_LAMP_TWINK ? SDK_LAMP_OFF : gstLampCtrl.eLamp4);
        }

        if(dly_time % 5 == 0 && gstLampCtrl.eLamp3 == SDK_LAMP_TWINK)
        {
            silamp3 = (silamp3 == SDK_LAMP_ON ? SDK_LAMP_OFF : SDK_LAMP_ON);
        }

        if((gstLampCtrl.eLamp1 == SDK_LAMP_TWINK ||
            gstLampCtrl.eLamp2 == SDK_LAMP_TWINK ||
            gstLampCtrl.eLamp4 == SDK_LAMP_TWINK) &&
           (dly_time == 0 || dly_time == 2))
        {
            is_update_state = true;
        }

        if(gstLampCtrl.eLamp3 == SDK_LAMP_TWINK &&
           dly_time % 5 == 0)
        {
            is_update_state = true;
        }

        if(true == is_update_state)
        {
            sdkSysCtrolLamp(silamp1, silamp2, silamp3, silamp4, mode);
            is_update_state = false;
        }

        if (gstLampCtrl.siTimers != 0) //黄少波 2014.02.17 8:35同步李琳
        {
            if (sdkTimerIsEnd(ui_time_id, (u32)gstLampCtrl.siTimers))
            {
                sdkSysCtrolLamp(SDK_LAMP_OFF, SDK_LAMP_OFF, SDK_LAMP_OFF, SDK_LAMP_OFF, mode);
                memset(&gstLampCtrl, 0, sizeof(gstLampCtrl));       //fusuipu 2015.05.04 10:54  时间到，将参数清0，防止后续影响
                break;
            }
        }
        sdkmSleep(100);
    }

    gpthread_lamp = 0;         //线程结束时置线程id为0
    pthread_detach(pthread_self());
    pthread_exit(0);
}

/*****************************************************************************
** Descriptions:	关闭所有的非接灯
** Parameters:          void
** Returned value:
** Created By:		fusuipu  2014.12.04
** Remarks:         在线程超时退出、多应用切换、进入低功耗的时候会调用
*****************************************************************************/
static s32 sdkPEDCloseAllLamp (void)
{
    s32 mach = sdkSysGetMachineCode(NULL);
    u8 mode = 0;

    if (sdkSysIsRfIn() == false)
    {
        if(SDK_SYS_MACHINE_K360P == mach ||
           SDK_SYS_MACHINE_K370P == mach ||
           SDK_SYS_MACHINE_V70D == mach ||
           SDK_SYS_MACHINE_G870 == mach ||
           SDK_SYS_MACHINE_G870D == mach ||
           SDK_SYS_MACHINE_G3 == mach   ||
           SDK_SYS_MACHINE_N1C == mach   ||
           SDK_SYS_MACHINE_G2Q == mach   ||
           SDK_SYS_MACHINE_G3M == mach    ||
           SDK_SYS_MACHINE_G3N == mach   ||
           SDK_SYS_MACHINE_G2M == mach   ||
           SDK_SYS_MACHINE_G2N == mach 
           )
       //huangbx 20160122 添加G2M G2N G3M G3N
        {
            return SDK_OK;                 //上述机型无外置密码键盘，直接退出
        }
        mode = SDK_PED_FIFO_CTROL;
    }
    else
    {
        if(SDK_SYS_MACHINE_G870 == mach  ||
           SDK_SYS_MACHINE_G891 == mach  ||
           SDK_SYS_MACHINE_K820P == mach  ||
           SDK_SYS_MACHINE_K390P == mach  ||
           SDK_SYS_MACHINE_K350P == mach  ||
           SDK_SYS_MACHINE_G2Q == mach ||
           SDK_SYS_MACHINE_K360P == mach )
        {
            mode = SDK_PED_API_CTROL;
        }
        else if(SDK_SYS_MACHINE_G810 == mach ||
                SDK_SYS_MACHINE_V70D == mach ||
                SDK_SYS_MACHINE_G870D == mach ||
                SDK_SYS_MACHINE_G3 == mach ||
                SDK_SYS_MACHINE_N1C == mach   ||
                SDK_SYS_MACHINE_K320P == mach||
                SDK_SYS_MACHINE_G3M == mach    ||
                SDK_SYS_MACHINE_G3N == mach   ||
                SDK_SYS_MACHINE_G2M == mach   ||
                SDK_SYS_MACHINE_G2N == mach )
                //huangbx 20160122 添加G2M G2N G3M G3N
        {
            mode = SDK_PED_FIFO_CTROL;
        }
    }
    sdkSysCtrolLamp(SDK_LAMP_OFF, SDK_LAMP_OFF, SDK_LAMP_OFF, SDK_LAMP_OFF, mode);
    return SDK_OK;
}

/*****************************************************************************
** Descriptions:	关闭非接指示灯线程
** Parameters:          void
** Returned value:
** Created By:		fusuipu  2014.06.04
** Remarks:             在多应用切换的时候，要关掉线程，所以需要将此函数单独出来
                    然后在多应用切换里调用
*****************************************************************************/
s32 sdk_dev_close_ped_lamp_pthreth (void)
{
    void *status = NULL;

    if(gpthread_lamp != 0)
    {
        if(0 != pthread_cancel(gpthread_lamp))
        {
            return SDK_ERR;
        }

        if(0 != pthread_join(gpthread_lamp, &status))
        {
            return SDK_ERR;
        }
        gpthread_lamp = 0;
    }
    sdkPEDCloseAllLamp();
    return SDK_OK;
}

/*****************************************************************************
** Descriptions:
** Parameters:          const SDK_PED_LAMP_CTRL *pstLampCtrl
** Returned value:
** Created By:		shiweisong  2013.12.03
** Remarks:
*****************************************************************************/
static s32 sdkPEDPthreadCntrollamp (const SDK_PED_LAMP_CTRL * pstLampCtrl, bool IsApiFun)
{
    u8 buf[512] = {0};

    if(NULL == pstLampCtrl)
    {
        return SDK_PARA_ERR;
    }
    buf[0] = (IsApiFun == true ? SDK_PED_API_CTROL : SDK_PED_FIFO_CTROL);
    //先保存设置，在进入低功耗恢复的时候用得上
    memcpy((void*) &gstLampCtrl, (void*)pstLampCtrl, sizeof(SDK_PED_LAMP_CTRL));

    if ((pstLampCtrl->siTimers != 0
         && (pstLampCtrl->eLamp1 | pstLampCtrl->eLamp2 | pstLampCtrl->eLamp3 | pstLampCtrl->eLamp4) != 0)
        || pstLampCtrl->eLamp1 == SDK_LAMP_TWINK
        || pstLampCtrl->eLamp2 == SDK_LAMP_TWINK
        || pstLampCtrl->eLamp3 == SDK_LAMP_TWINK
        || pstLampCtrl->eLamp4 == SDK_LAMP_TWINK)    //时间不为0的或是闪烁的开线程来维持
    {
        bIsPthreadReset = true;

        if (gpthread_lamp == 0)
        {
            if(0 != pthread_create((void *) &gpthread_lamp, NULL, (void *)sdkSysThreadCtrolLamp, buf))
            {
                Assert(0);
                return SDK_ERR;
            }
            Trace("libsdkped", "gpthread_lamp id:%d\r\n", gpthread_lamp);
        }
    }
    else
    {
        sdk_dev_close_ped_lamp_pthreth();   //先清0之前的非接灯设置
        sdkSysCtrolLamp(pstLampCtrl->eLamp1, pstLampCtrl->eLamp2, pstLampCtrl->eLamp3, pstLampCtrl->eLamp4, buf[0]);
    }
    return SDK_OK;
}

/*****************************************************************************
** Descriptions:	判断K390P机型使用FIFO还是直接调用libdev接口控制非接灯
** Parameters:          void
** Returned value:
** Created By:		fusuipu  2015.07.28
** Remarks:             根据和肖红辉确认，K390P机型，硬件版本大于等于0x0A时，使用
                    PIN512控制，其他硬件版本使用GPIO方式控制
*****************************************************************************/
static bool sdkPEDIsK390PUseDdiCtrl(void)
{
    static u8 flag = 2;
    
    if(2 == flag)
    {
        if(sdk_dev_mis_get_HWversion() >= 0x0A)
        {                 
            flag = false;
        }
        else
        {
            flag = true;
        }
   }
    
    return flag;
}

/*****************************************************************************
** Descriptions:	控制非接led灯
** Parameters:          const SDK_LAMP_CTRL *pstLampCtrl:控制灯类型和开关状态
** Returned value:	SDK_OK 成功
                    SDK_TIME_OUT 超时
** Created By:		shijianglong  2013.07.12
** Remarks:             暂时只支持K3102。

   只有G810,860,870D是512控制的,其他的机型要用IO控制
   qiaodayong 110225130
   1、增加fifo指令:FIFO_ENFORCERFDIRECTION     0x0086      由上层决定RF指令(DB
   and DD

   指令)发送方向:pos 机，还是外界rf键盘

    格式:
        1B              1B            1B        1B
    是否带密码键盘  指令发送方向    卡类型      指令

    是否带密码键盘: 0-yes,1-no
    指令发送方向:   0-外接非接键盘，1-pos机
    卡类型:         0-mifare    1-type A and type B

*****************************************************************************/
s32 sdkPEDLampControl (const SDK_PED_LAMP_CTRL * pstLampCtrl)
{
    s32 mach = sdkSysGetMachineCode(NULL);
    bool ctrl_mode = false;
  
    if (sdkSysIsRfIn() == false)
    {
        return SDK_ERR;

        /*Modify by fusuipu at 2015.04.16  15:54 G101DR外置密码键盘在非接灯控制和电签时，非接灯指令会造成
           电签的失败，为了避免这种情况，和李琳讨论确认，由sdk关闭非接灯的外置密码键盘的
           控制*/
#if 0

        if(SDK_SYS_MACHINE_K360P == mach ||
           SDK_SYS_MACHINE_K370P == mach ||
           SDK_SYS_MACHINE_V70D == mach ||
           SDK_SYS_MACHINE_G870 == mach ||
           SDK_SYS_MACHINE_G870D == mach ||
           SDK_SYS_MACHINE_G3 == mach  ||
           SDK_SYS_MACHINE_N1C == mach   ||
           SDK_SYS_MACHINE_G2Q == mach  )
        {
            return SDK_ERR;                 //上述机型无外置密码键盘，直接退出
        }
        sdkPEDPthreadCntrollamp(pstLampCtrl, false);
#endif /* if 0 */
    }
    else
    {        
        if(SDK_SYS_MACHINE_K390P == mach)
        {
            ctrl_mode = sdkPEDIsK390PUseDdiCtrl();
            sdkPEDPthreadCntrollamp(pstLampCtrl, ctrl_mode);
        }
        else if(SDK_SYS_MACHINE_G870 == mach  ||
           SDK_SYS_MACHINE_G891 == mach  ||
           SDK_SYS_MACHINE_K820P == mach  ||
           SDK_SYS_MACHINE_K390P == mach  ||
           SDK_SYS_MACHINE_K350P == mach  ||
           SDK_SYS_MACHINE_G2Q  == mach ||
           SDK_SYS_MACHINE_K360P == mach )
        {
            sdkPEDPthreadCntrollamp(pstLampCtrl, true);
        }
        else if(SDK_SYS_MACHINE_G810 == mach ||
                SDK_SYS_MACHINE_V70D == mach ||
                SDK_SYS_MACHINE_G870D == mach ||
                SDK_SYS_MACHINE_G3 == mach ||
                SDK_SYS_MACHINE_N1C == mach   ||
                SDK_SYS_MACHINE_K320P == mach||
                SDK_SYS_MACHINE_G3M == mach    ||
                SDK_SYS_MACHINE_G3N == mach   ||
                SDK_SYS_MACHINE_G2M == mach   ||
                SDK_SYS_MACHINE_G2N == mach )
            //huangbx 20160122 添加G2M G2N G3M G3N
        {
            sdkPEDPthreadCntrollamp(pstLampCtrl, false);
        }
    }
    return SDK_OK;
}

/*****************************************************************************
** Descriptions:	恢复非接灯闪烁
** Parameters:          void
** Returned value:
** Created By:		fusuipu  2014.12.04
** Remarks:             低功耗唤醒之后使用
*****************************************************************************/
s32 sdk_dev_resume_ped_lamp (void)
{
    return sdkPEDLampControl(&gstLampCtrl);
}

