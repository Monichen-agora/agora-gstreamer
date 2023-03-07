#include <stdbool.h>
#include <fstream>
#include "agorac.h"

#include <string>
#include <cstring>
#include <iostream>
#include <fstream>


//agora header files
#include "NGIAgoraRtcConnection.h"
#include "IAgoraService.h"
#include "AgoraBase.h"

#include "helpers/agoradecoder.h"
#include "helpers/agoraencoder.h"
#include "helpers/agoralog.h"
#include "helpers/localconfig.h"

//#include "userobserver.h"
#include "observer/connectionobserver.h"
#include "helpers/context.h"
//#include <gst/gst.h>

#include "helpers/utilities.h"
#include "agoratype.h"
#include "helpers/agoraconstants.h"

#include "helpers/uidtofile.h"

#include "agoraio.h"

using namespace std;

//do not use it before calling agora_init
void agora_log(agora_context_t* ctx, const char* message){
   ctx->log_func(ctx->log_ctx, message);
}

int agoraio_send_audio(AgoraIoContext_t* ctx,
                     const unsigned char * buffer, 
                     unsigned long len,
                     long timestamp){

    if(ctx->agoraIo!=nullptr){
        ctx->agoraIo->sendAudio(buffer, len, timestamp);
     }

    return 0;
}

int  agoraio_send_audio_with_duration(AgoraIoContext_t* ctx,  
                                       const unsigned char* buffer,  
							                  unsigned long len,
							                  long timestamp,
							                  long  duration){
   if(ctx->agoraIo!=nullptr){
        ctx->agoraIo->sendAudio(buffer, len, timestamp, duration);
     }

   return 0;
}

void agora_set_log_function(agora_context_t* ctx, agora_log_func_t f, void* log_ctx){

    ctx->log_func=f;
    ctx->log_ctx=log_ctx;
}

void agora_log_message(agora_context_t* ctx, const char* message){

   if(ctx->callConfig->useDetailedAudioLog()){
      logMessage(std::string(message));
   }
}

void agora_dump_audio_to_file(agora_context_t* ctx, unsigned char* data, short sampleCount)
{
    if(ctx->callConfig->dumpAudioToFile()==false){
       return;
    }

   std::ofstream meidaFile(ctx->audioDumpFileName, std::ios::binary|std::ios::app);	
   meidaFile.write(reinterpret_cast<const char*>(data), sampleCount*sizeof(float)); 
   meidaFile.close();
}

AgoraIoContext_t*  agoraio_init(agora_config_t* config){

    AgoraIoContext_t* ctx=new AgoraIoContext_t;
    if(ctx==nullptr){
        return NULL;
    }

    ctx->agoraIo=std::make_shared<AgoraIo>(config->verbose,
                                           config->fn,
                                           config->userData, 
                                           config->in_audio_delay,
                                           config->in_video_delay,
                                           config->out_audio_delay,
                                           config->out_video_delay,
                                           config->sendOnly,
                                           config->enableProxy,
                                           config->proxy_timeout,
                                           config->proxy_ips,
					                            config->transcode);

    auto ret=ctx->agoraIo->init(config->app_id,
                                config->ch_id,
                                config->user_id,
                                config->is_audiouser, 
                                config->enc_enable, 
                                config->enable_dual,
                                config->dual_vbr, 
                                config->dual_width, 
                                config->dual_height,
                                config->min_video_jb, 
                                config->dfps);

    if(ret==false){
       return nullptr;
    }

    return ctx;
}

int  agoraio_send_video(AgoraIoContext_t* ctx,  
                                const unsigned char* buffer,  
							           unsigned long len, 
								        int is_key_frame,
							           long timestamp){

        return ctx->agoraIo->sendVideo( buffer, len, is_key_frame, timestamp);

}

#define MAX_DATA_SIZE   65536
int  agoraio_send_video_text(AgoraIoContext_t* ctx,  
                                const unsigned char* buffer,  
							           unsigned long len, 
								        int is_key_frame,
							           long timestamp){

        unsigned char *buffer_text;
	std::string custom_data = "Hello Agora!!!";
	std::string ending_text = "AgoraWrc";
	unsigned long new_len = len;
        uint32_t data_len, data_len_le = 0;
	FILE *fp;
        char *custom_data_array;
        custom_data_array = (char*)malloc(MAX_DATA_SIZE);
	int ret;

        fp = fopen("./datafile", "rb");
	if (fp != NULL) {
	  fseek(fp, 0L, SEEK_END);
          data_len = ftell(fp);
	  data_len = data_len > MAX_DATA_SIZE? MAX_DATA_SIZE : data_len;
	  fseek(fp, 0L, SEEK_SET);
	  fread(custom_data_array, sizeof(char), data_len, fp); //read in a custom data from a file
	  fclose(fp);
	} else {
          cout << "no datafile";
	  exit(-1);
	}

	//Custom data format: videoFrameData+customData+customDataLength+‘AgoraWrc’
	buffer_text = (unsigned char*) malloc(len + MAX_DATA_SIZE);
	memcpy(buffer_text, buffer, len);
	//data_len = custom_data.size();
	//memcpy(buffer_text+len, custom_data.c_str(), data_len);
	memcpy(buffer_text+len, custom_data_array, data_len);

	//convert data_len to little endian
	data_len_le = (data_len&0xff)<<24 | (data_len&0xff00)<< 8 | (data_len&0xff0000)>> 8 | (data_len&0xff0000) >> 24;
	memcpy(buffer_text+len+data_len, &data_len_le, sizeof(data_len));
	memcpy(buffer_text+len+data_len + sizeof(data_len), ending_text.c_str(), ending_text.size()); //data_len is now only one byte
	new_len = len + data_len + sizeof(data_len) + ending_text.size();

	free(custom_data_array);
	ret = ctx->agoraIo->sendVideo( buffer_text, new_len, is_key_frame, timestamp);
	free(buffer_text);
	return ret;

}

void agoraio_disconnect(AgoraIoContext_t** ctx){

   if(ctx==nullptr){
      std::cout<<"cannot disconnect agora!\n";
   }
   (*ctx)->agoraIo->disconnect();

   delete (*ctx);
}

void logText(const char* message){
    logMessage(message);
}

void  agoraio_set_paused(AgoraIoContext_t* ctx, int flag){

    if(ctx==nullptr){
         return;
    }

    (ctx)->agoraIo->setPaused(flag);
}

void agoraio_set_event_handler(AgoraIoContext_t* ctx, event_fn fn, void* userData){

    if(ctx==nullptr)  return;

    ctx->agoraIo->setEventFunction(fn, userData);
}

void agoraio_set_video_out_handler(AgoraIoContext_t* ctx, agora_media_out_fn fn, void* userData){
   
   if(ctx==nullptr)  return;

   ctx->agoraIo->setVideoOutFn(fn, userData);
}

void agoraio_set_audio_out_handler(AgoraIoContext_t* ctx, agora_media_out_fn fn, void* userData){

   if(ctx==nullptr)  return;

   ctx->agoraIo->setAudioOutFn(fn, userData);
}

void agoraio_set_sendonly_flag(AgoraIoContext_t* ctx, int flag){

  if(ctx==nullptr)  return;

   ctx->agoraIo->setSendOnly(flag);
}
