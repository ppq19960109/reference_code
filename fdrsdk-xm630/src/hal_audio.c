
#include "sys_comm.h"
#include "xm_comm.h"
#include "hal_audio.h"
#include "hal_log.h"
#include "hal_gpio.h"

#define AUDIO_RECORD_FILENAME_LEN 64

typedef struct {
    pthread_t tid;
    pthread_mutex_t mutex;
    char filename[AUDIO_RECORD_FILENAME_LEN];
    uint8_t run;
    uint8_t start;
    uint8_t stop;
}hal_audio_hdl;

static hal_audio_hdl g_hal_audio_hd;

static int32_t _hal_audio_play(ADEC_CHN chn, char *filename)
{
    int32_t ret = 0;
    AUDIO_STREAM_S stStream;
    XM_U8 pAudioStream[320];
    FILE *fp = NULL;
    
    fp = fopen(filename, "r");
	if(fp == NULL){
		perror("fopen");
		return -1;
	}
    
    while(1) {
		ret = fread(pAudioStream, 1, 320, fp);
		if(ret == 0) {
			hal_info("Audio play end.\n");
			sleep(1);
			break;
		}
		stStream.u32Seq = 1;
		stStream.u64TimeStamp = 1;
		stStream.u32Len = ret;
		stStream.pStream = pAudioStream;
		ret = XM_MPI_ADEC_SendStream(chn, &stStream, XM_TRUE);
		if (XM_SUCCESS != ret) {
			hal_error("XM_MPI_ADEC_SendStream(%d) failed with %#x!\n", chn, ret);
			break;
		}
	}

    fclose(fp);
    return ret;
}

static void hal_audio_play_enable(void)
{
    hal_set_gpio_output(AUDIO_ENABLE_PIN, GPIO_HIGH);    
}

static void hal_audio_play_disable(void)
{
    hal_set_gpio_output(AUDIO_ENABLE_PIN, GPIO_LOW);    
}

int32_t hal_audio_play(char *filename, uint8_t volume)
{
    int32_t ret = 0;
    AIO_ATTR_S stAioAttr;
    ADEC_CHN_ATTR_S Adec_Chn;
	AENC_ATTR_G711_S stG711;
	
    AUDIO_DEV dev = 0;
    ADEC_CHN chn = 0;

    if(volume > 100) {
        volume = 100;
    }
    
    ret = XM_MPI_AO_SetVolume(dev, volume);
    if(XM_SUCCESS != ret) {
		hal_error("XM_MPI_AO_SetVolume err:0x%x\n", ret);
		return  -1;
	}

    stAioAttr.enSamplerate = AUDIO_SAMPLE_RATE_8000;
	stAioAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
	stAioAttr.enWorkmode = AIO_MODE_I2S_MASTER;
	stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MONO;
	stAioAttr.u32EXFlag = 1;
	stAioAttr.u32FrmNum = 30;
	stAioAttr.u32PtNumPerFrm = 160;
	stAioAttr.u32ChnCnt = 2;
	stAioAttr.u32ClkSel = 1;
	ret = XM_MPI_AO_SetPubAttr(dev, &stAioAttr);
	if(XM_SUCCESS != ret) {
		hal_error("set ao %d attr err: 0x%x\n", dev, ret);
		return  -1;
	}

    Adec_Chn.enType = PT_G711A;
	Adec_Chn.u32BufSize = 8;
	Adec_Chn.enMode = ADEC_MODE_PACK;
	Adec_Chn.pValue = &stG711;
	stG711.resv = 0;
	ret = XM_MPI_ADEC_CreateChn(chn ,&Adec_Chn);
	if(XM_SUCCESS != ret) {
		hal_error("creat adec %d channel err:0x%x\n", chn, ret);
		return  1;
	}
    
    hal_audio_play_enable();
	ret = XM_MPI_AO_Enable(0);
	if(XM_SUCCESS != ret) {
		hal_error("creat adec %d channel err:0x%x\n", dev, ret);
		goto end;
	}

    _hal_audio_play(chn, filename);

    ret = XM_MPI_AO_Disable(dev);
	if(ret != XM_SUCCESS) {
		hal_error("XM_MPI_AO_DisEnable dev 0 error\n");
	}
	
end:
    ret = XM_MPI_ADEC_DestroyChn(chn);
	if(XM_SUCCESS != ret) {
		hal_error("creat adec %d channel err:0x%x\n", chn, ret);
	}

    hal_audio_play_disable();
    usleep(50 * 1000);
    return ret;
}

int32_t hal_audio_record_start(char *filename)
{
    if(NULL == filename) {
        return -1;
    }
    
    pthread_mutex_lock(&g_hal_audio_hd.mutex);
    if(g_hal_audio_hd.start) {
        pthread_mutex_unlock(&g_hal_audio_hd.mutex);
        return -1;
    }
    strncpy(g_hal_audio_hd.filename, filename, AUDIO_RECORD_FILENAME_LEN - 1);
    g_hal_audio_hd.start = 1;
    g_hal_audio_hd.stop = 0;
    pthread_mutex_unlock(&g_hal_audio_hd.mutex);
    return 0;
}

int32_t hal_audio_record_stop(void)
{
    pthread_mutex_lock(&g_hal_audio_hd.mutex);
    if(g_hal_audio_hd.stop) {
        pthread_mutex_unlock(&g_hal_audio_hd.mutex);
        return -1;
    }
    g_hal_audio_hd.stop = 1;
    pthread_mutex_unlock(&g_hal_audio_hd.mutex);
    return 0;
}

static int32_t hal_audio_record_end(void)
{
    pthread_mutex_lock(&g_hal_audio_hd.mutex);
    g_hal_audio_hd.start = 0;
    pthread_mutex_unlock(&g_hal_audio_hd.mutex);
    return 0;
}

static int32_t hal_audio_record_init(void)
{
    int32_t rc = -1;
    int32_t ao_chn = 0;
    int32_t ao_dev = 0;
    int32_t ai_chn = 0;
    int32_t ai_dev = 0;
    int32_t ae_chn = 0;
	AENC_CHN_ATTR_S stAencAttr;
	AENC_ATTR_G711_S stG711;
    AIO_ATTR_S stAioAttr;
	AI_VQE_CONFIG_S aiVqeConfig;
    
	stAencAttr.enType = PT_G711A;
	stAencAttr.u32BufSize = 30;
	stAencAttr.pValue = &stG711;
	stG711.resv = 0;

	rc = XM_MPI_AENC_CreateChn(ae_chn, &stAencAttr);
	if (rc) {
		hal_error("create aenc err:0x%x\n", rc);
		return -1;
	}

    stAioAttr.enSamplerate = AUDIO_SAMPLE_RATE_8000;
	stAioAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
	stAioAttr.enWorkmode = AIO_MODE_I2S_MASTER;
	stAioAttr.enSoundmode = AUDIO_SOUND_MODE_MONO;
	stAioAttr.u32EXFlag = 1;
	stAioAttr.u32FrmNum = 30;
	stAioAttr.u32PtNumPerFrm = 160;
	stAioAttr.u32ChnCnt = 2;
	stAioAttr.u32ClkSel = 1;
	rc = XM_MPI_AI_SetPubAttr(ai_dev, &stAioAttr);
	if (rc) {
		hal_error("XM_MPI_AI_SetPubAttr(%d) failed with %d\n", ai_dev, rc);
		goto destory;
	}

	memset(&aiVqeConfig, 0, sizeof(AI_VQE_CONFIG_S));
	aiVqeConfig.bAecOpen = 0;
	aiVqeConfig.bAgcOpen = 0;
	aiVqeConfig.bAnrOpen = 1;
	rc = XM_MPI_AI_SetVqeAttr(ai_dev, ai_chn, ao_dev, ao_chn, &aiVqeConfig);
	if (rc) {
		hal_error("XM_MPI_AI_SetVqeAttr(%d) failed with %#x\n", ai_dev, rc);
		goto destory;
	}

	rc = XM_MPI_AI_Enable(ai_dev);
	if (rc) {
		hal_error("XM_MPI_AI_Enable(%d) failed with %d\n", ai_dev, rc);
		goto destory;
	} 
    
    return 0;
destory:
    XM_MPI_AENC_DestroyChn(ae_chn);
    return -1;
}

static int32_t hal_audio_record_destory(void)
{
    int32_t rc = -1;
    rc = XM_MPI_AI_Disable(0);
	if (rc) {
		hal_error("XM_MPI_AI_Disable [%d] failed\n", 0);
	} 

	rc = XM_MPI_AENC_DestroyChn(0);
	if(rc) {
		hal_error(" XM_MPI_AENC_DestroyChn :%d err:0x%x\n", 0, rc);
	}
	return 0;
}

static void *hal_audio_record_task(void *args)
{
    FILE *fp = NULL;
    AUDIO_STREAM_S stStream;

    //hal_audio_play("/var/nfs/welcome.pcm", 100);
    while(g_hal_audio_hd.run) {
        if(!g_hal_audio_hd.start) {
            usleep(50 * 1000);
            continue;
        }
        fp = fopen(g_hal_audio_hd.filename, "wb+");
        if(NULL == fp) {
            hal_audio_record_end();
            usleep(50 * 1000);
            continue;
        }
        while(g_hal_audio_hd.run && !g_hal_audio_hd.stop) {
            if(XM_MPI_AENC_GetStream(0, &stStream, XM_TRUE)) {
                hal_error("XM_MPI_AENC_GetStream failed.\r\n");
                usleep(50 * 1000);
                break;
            }
		    fwrite(stStream.pStream, 1, stStream.u32Len, fp);
	        fflush(fp);
		
		    if(XM_MPI_AENC_ReleaseStream(0, &stStream)) {
                hal_error("XM_MPI_AENC_ReleaseStream failed.\r\n");
            }
		    usleep(10);
        }
        fclose(fp);
        fp = NULL;
        hal_audio_record_end();
    }

    hal_audio_record_destory();
    pthread_exit(NULL);
}

int32_t hal_audio_init(void)
{
    hal_audio_play_disable();
    pthread_mutex_init(&g_hal_audio_hd.mutex, NULL);
    
    if(hal_audio_record_init()) {
        return -1;
    }
    g_hal_audio_hd.run = 1;
    if(pthread_create(&g_hal_audio_hd.tid, NULL, hal_audio_record_task, NULL) != 0) {
		hal_error("pthread_create fail\n");
		return -1;
	}
    pthread_detach(g_hal_audio_hd.tid);

    return 0;
}

int32_t hal_audio_destroy(void)
{
    pthread_mutex_lock(&g_hal_audio_hd.mutex);
    g_hal_audio_hd.run = 0;
    pthread_mutex_unlock(&g_hal_audio_hd.mutex);
    return 0;
}

