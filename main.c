#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

typedef int bool;
#define true 1
#define false 0

typedef struct VideoState {
	// First video.
    AVFormatContext 	*pFormatCtx;
    AVCodecContext  	*pCodecCtx;
    AVCodec         	*pCodec;
    AVFrame         	*pFrame;
    AVPacket        	packet;
    AVDictionary    	*optionDict;
	int					videoStream;
	int 				i;

} VideoState; 

typedef struct SdlState {
	// SDL Surface for show video.
    SDL_Overlay     	*bmp;
    SDL_Surface     	*screen;
    SDL_Rect        	rect;
    SDL_Event       	event;
	SDL_Rect			marker;
} SdlState;

int main(int argc, char *argv[]) {

	// Test input.
    if(argc < 3){
        fprintf(stderr, "Usage: ./appName <file1> <file2> \n");
        exit(1);
    }

	int delay = 10;

    struct SwsContext 	*sws_ctx = NULL;
	struct SwsContext	*sws_ctx2 = NULL;	

	VideoState *vs1;
	vs1 = av_mallocz(sizeof(VideoState));
	
	VideoState *vs2;
	vs2 = av_mallocz(sizeof(VideoState));

	SdlState *ss;
	ss = av_mallocz(sizeof(SdlState));

	// SDL Rectangle - marker.
	ss->marker.x = 0;
	ss->marker.y = 0;
	ss->marker.w = 25;
	ss->marker.h = 25;

	// Init ffmpeg and SDL.
    av_register_all();
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)){
        fprintf(stderr,"Could not initialize SDL - %s " + *SDL_GetError());
        exit(1);
    }

    // Try to open files.
    if(avformat_open_input(&vs1->pFormatCtx, argv[1], NULL, NULL) != 0)
        return -1;
	if(avformat_open_input(&vs2->pFormatCtx, argv[2], NULL, NULL) != 0)
        return -1;

    // Load and fill the correct information for pFormatCtx->streams.
    if(avformat_find_stream_info(vs1->pFormatCtx, NULL) < 0)
        return -1;
	if(avformat_find_stream_info(vs2->pFormatCtx, NULL) < 0)
        return -1;

    // Manual debugging function, the file information in the terminal output.
    av_dump_format(vs1->pFormatCtx, 0, argv[1], 0);
	av_dump_format(vs2->pFormatCtx, 0, argv[2], 0);

	// Try to find video stream1 and save his number.
    vs1->videoStream=-1;
	for (vs1->i = 0; vs1->i < vs1->pFormatCtx->nb_streams; vs1->i++)
	  if(vs1->pFormatCtx->streams[vs1->i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
	    vs1->videoStream = vs1->i;
	    break;
	  }
	if(vs1->videoStream == -1)
	  return -1;

	// Try to find video stream2 and save his number.
	vs2->videoStream=-1;
	for (vs2->i = 0; vs2->i < vs2->pFormatCtx->nb_streams; vs2->i++)
	  if(vs2->pFormatCtx->streams[vs2->i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
	    vs2->videoStream = vs2->i;
	    break;
	  }
	if(vs2->videoStream == -1)
	  return -1;

	// Get the pointer to the corresponding decoder context from the video stream.
    vs1->pCodecCtx = vs1->pFormatCtx->streams[vs1->videoStream]->codec;
	vs2->pCodecCtx = vs2->pFormatCtx->streams[vs2->videoStream]->codec;

    // Find the corresponding decoder according to codec_id.
    vs1->pCodec = avcodec_find_decoder(vs1->pCodecCtx->codec_id);
	vs2->pCodec = avcodec_find_decoder(vs2->pCodecCtx->codec_id);

    if(vs1->pCodec == NULL || vs2->pCodec == NULL){
        fprintf(stderr, "Unsupported codec ! \n");
        return -1;
    }

    // Open apropriate decoders.
    if(avcodec_open2(vs1->pCodecCtx, vs1->pCodec, &vs1->optionDict) < 0)
        return -1;
	if(avcodec_open2(vs2->pCodecCtx, vs2->pCodec, &vs2->optionDict) < 0)
        return -1;

    // Allocate memory for frames.
    vs1->pFrame = av_frame_alloc();
	vs2->pFrame = av_frame_alloc();


	// Configure screen.
    #ifdef __DARWIN__
        ss->screen = SDL_SetVideoMode(vs1->pCodecCtx->width, vs1->pCodecCtx->height, 0, 0);
    #else
        ss->screen = SDL_SetVideoMode(vs1->pCodecCtx->width, vs1->pCodecCtx->height, 24, 0);
    #endif // __DARWIN__
	//ss->screen = SDL_SetVideoMode(vs1->pCodecCtx->width, vs1->pCodecCtx->height, 0, 0);
    if(!ss->screen){
        fprintf(stderr, "SDL : could not set video mode - exiting \n");
        exit(1);
    }
		
    // Applay for an overlay, the YUV data to the screen.
    ss->bmp = SDL_CreateYUVOverlay(vs1->pCodecCtx->width, vs1->pCodecCtx->height, SDL_YV12_OVERLAY, ss->screen);

    sws_ctx = sws_getContext(vs1->pCodecCtx->width, vs1->pCodecCtx->height, vs1->pCodecCtx->pix_fmt,
							 vs1->pCodecCtx->width, vs1->pCodecCtx->height, AV_PIX_FMT_YUV420P,
							 SWS_BILINEAR, NULL, NULL, NULL);

	sws_ctx2 = sws_getContext(vs2->pCodecCtx->width, vs2->pCodecCtx->height, vs2->pCodecCtx->pix_fmt,
							  vs2->pCodecCtx->width, vs2->pCodecCtx->height, AV_PIX_FMT_YUV420P,
							  SWS_BILINEAR, NULL, NULL, NULL);

	// Window
	ss->rect.x = 0;
    ss->rect.y = 25;
    ss->rect.w = vs1->pCodecCtx->width;
    ss->rect.h = vs1->pCodecCtx->height;

	for (;;) {

		if (showFrame(vs1, ss, sws_ctx, delay, 1) < 0)
			return -1;
/*
		if (showFrame(vs1, ss, sws_ctx, delay, 1) < 0)
			return -1;

		if (showFrame(vs2, ss, sws_ctx2, delay, 0) < 0)
			return -1;
*/
		if (showFrame(vs2, ss, sws_ctx2, delay, 0) < 0)
			return -1;		

        switch (ss->event.type) {

            case SDL_QUIT:
                SDL_Quit();
                exit(0);
                break;

            default:
                break;
        }

	}

    av_free(vs1->pFrame);
	avcodec_close(vs1->pCodecCtx);
	avformat_close_input(&vs1->pFormatCtx);

	av_free(vs2->pFrame);
	avcodec_close(vs2->pCodecCtx);
	avformat_close_input(&vs2->pFormatCtx);
    
    return 0;
}

void showMarker(SdlState *ss, bool color)
{
	int r = 0, g = 0, b = 0;
	if (color) {
		r = 0; g = 0; b = 0;
	}
	else {
		r = 255; g = 255; b = 255;
	}
	SDL_FillRect(ss->screen, &ss->marker, SDL_MapRGB(ss->screen->format, r, g, b));
	//SDL_Flip(screen);
	SDL_UpdateRect(ss->screen, 0, 0, 0, 0);
}


int showFrame(VideoState *vs, SdlState *ss, struct SwsContext *sws_ctx, int delay, bool color)
{
	if (av_read_frame(vs->pFormatCtx, &vs->packet) < 0)
		return -1;//break;

	int frameFinished = 0;
   	if(vs->packet.stream_index == vs->videoStream) {
		avcodec_decode_video2(vs->pCodecCtx, vs->pFrame, &frameFinished, &vs->packet);
        if(frameFinished) {
            SDL_LockYUVOverlay(ss->bmp);

            AVPicture pict;
            pict.data[0] = ss->bmp->pixels[0];
            pict.data[1] = ss->bmp->pixels[2];
            pict.data[2] = ss->bmp->pixels[1];

            pict.linesize[0] = ss->bmp->pitches[0];
            pict.linesize[1] = ss->bmp->pitches[2];
            pict.linesize[2] = ss->bmp->pitches[1];

			// Convert the image into YUV format that SDL uses.
            sws_scale(sws_ctx, (uint8_t const * const *)vs->pFrame->data, vs->pFrame->linesize,
                      0, vs->pCodecCtx->height, pict.data, pict.linesize);

            SDL_UnlockYUVOverlay(ss->bmp);

			showMarker(ss, color);		

            SDL_DisplayYUVOverlay(ss->bmp, &ss->rect);
            SDL_Delay(delay);
        }
    }
	SDL_PollEvent(&ss->event);
    av_free_packet(&vs->packet);

	return 0;
}

