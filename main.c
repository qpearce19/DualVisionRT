#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#include <stdio.h>
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

typedef int bool;
#define true 1
#define false 0 

int main(int argc, char *argv[]) {
    AVFormatContext *pFormatCtx = NULL;
	AVFormatContext *pFormatCtx2 = NULL; // 2nd video
 
    int             i, videoStream, videoStream2; // 2nd video
    AVCodecContext  *pCodecCtx = NULL;
	AVCodecContext	*pCodecCtx2 = NULL; // 2nd video

    AVCodec         *pCodec = NULL;
	AVCodec			*pCodec2 = NULL; // 2nd video


    AVFrame         *pFrame = NULL;
	AVFrame			*pFrame2 = NULL; // 2nd video
    AVPacket        packet;
	AVPacket		packet2; //2nd video
    int             frameFinished, frameFinished2; // 2nd video

    AVDictionary    *optionDict = NULL;
	AVDictionary	*optionDict2 = NULL; // 2nd video

    struct SwsContext *sws_ctx = NULL;
	struct SwsContext *sws_ctx2 = NULL;

    SDL_Overlay     *bmp = NULL;
    SDL_Surface     *screen = NULL;
    SDL_Rect        rect;
    SDL_Event       event;

	SDL_Surface* display_level = NULL;
	//TTF_Font* font = TTF_OpenFont("even.ttf", 32);
	//display_level = TTF_RenderText_Solid (font, "level: ", textColor);

    if(argc < 3){
        fprintf(stderr, "Usage: test <file> \n");
        exit(1);
    }

    av_register_all();

    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)){
        fprintf(stderr,"Could not initialize SDL - %s " + *SDL_GetError());
        exit(1);
    }

    /**
    *打开一个文件
    */
    if(avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0)
        return -1;

	if(avformat_open_input(&pFormatCtx2, argv[2], NULL, NULL) != 0)
        return -1;

    /**
	 *为pFormatCtx->streams填充上正确的信息
	 */
    if(avformat_find_stream_info(pFormatCtx, NULL) < 0)
        return -1;

	if(avformat_find_stream_info(pFormatCtx2, NULL) < 0)
        return -1;

    /**
	 *手工调试函数，将文件信息在终端输出
	 */
    av_dump_format(pFormatCtx, 0, argv[1], 0);

	av_dump_format(pFormatCtx2, 0, argv[2], 0);


    videoStream=-1;
	for ( i = 0; i < pFormatCtx->nb_streams; i++)
	  if(pFormatCtx -> streams[i] -> codec -> codec_type == AVMEDIA_TYPE_VIDEO) {
	    videoStream = i;
	    break;
	  }

	if(videoStream == -1)
	  return -1;

	videoStream2=-1;
	for ( i = 0; i < pFormatCtx2->nb_streams; i++)
	  if(pFormatCtx2 -> streams[i] -> codec -> codec_type == AVMEDIA_TYPE_VIDEO) {
	    videoStream2 = i;
	    break;
	  }

	if(videoStream2 == -1)
	  return -1;



    /**
     *从 vedio stream 中获取对应的解码器上下文的指针
     */
    pCodecCtx = pFormatCtx -> streams[videoStream] -> codec;

	pCodecCtx2 = pFormatCtx2 -> streams[videoStream2] -> codec;

    /**
     *根据 codec_id 找到对应的解码器
     */
    pCodec = avcodec_find_decoder(pCodecCtx -> codec_id);

    if(pCodec == NULL){
        fprintf(stderr, "Unsupported codec ! \n");
        return -1;
    }

	pCodec2 = avcodec_find_decoder(pCodecCtx2 -> codec_id);

    if(pCodec2 == NULL){
        fprintf(stderr, "Unsupported codec ! \n");
        return -1;
    }

    /**
     * 打开解码器
     */
    if(avcodec_open2(pCodecCtx, pCodec, &optionDict) <0 )
        return -1;

	if(avcodec_open2(pCodecCtx2, pCodec2, &optionDict2) <0 )
        return -1;

    /**
     * 为frame 申请内存
     */
    pFrame = av_frame_alloc();
	pFrame2 = av_frame_alloc();

	/*
    #ifdef __DARWIN__
        screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 0, 0);
    #else
        screen = SDL_SetVideoMode(1024, 768, 24, 0);
    #endif // __DARWIN__
	*/
	screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 0, 0);

    if(!screen){
        fprintf(stderr, "SDL : could not set video mode - exiting \n");
        exit(1);
    }


	SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 255, 0, 0));
		
    /**
     * 申请一个 overlay , 将 yuv数据给 screen
     */
    bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height, SDL_YV12_OVERLAY, screen);

    sws_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
                             AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);

	sws_ctx2 = sws_getContext(pCodecCtx2->width, pCodecCtx2->height, pCodecCtx2->pix_fmt, pCodecCtx2->width, pCodecCtx2->height,
                             AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);

    i = 0;
	bool numVideo = false;
for (;;)
{
    if (numVideo == false)
	{
		if (av_read_frame(pFormatCtx, &packet) < 0)
			break;

        if(packet.stream_index == videoStream)
		{
            
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);

            if(frameFinished)
			{
                SDL_LockYUVOverlay(bmp);

                AVPicture pict;
                pict.data[0] = bmp->pixels[0];
                pict.data[1] = bmp->pixels[2];
                pict.data[2] = bmp->pixels[1];

                pict.linesize[0] = bmp->pitches[0];
                pict.linesize[1] = bmp->pitches[2];
                pict.linesize[2] = bmp->pitches[1];

                sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data, pFrame->linesize,
                          0, pCodecCtx->height, pict.data, pict.linesize);

                SDL_UnlockYUVOverlay(bmp);

                rect.x = 0;
                rect.y = 0;
                rect.w = pCodecCtx->width;
                rect.h = pCodecCtx->height;

                SDL_DisplayYUVOverlay(bmp, &rect);

				//display_text (600, 600, font, display_level, screen);
                SDL_Delay(10);

            }
        }

		

        av_free_packet(&packet);
        SDL_PollEvent(&event);

        switch (event.type) {

            case SDL_QUIT:
                SDL_Quit();
                exit(0);
                break;

            default:
                break;
        }
	
	numVideo = true;
    }
	else
	{
	if (numVideo == true)
	{
		if(av_read_frame(pFormatCtx2, &packet2) < 0)
			break;

        if(packet.stream_index == videoStream2)
		{
            
            avcodec_decode_video2(pCodecCtx2, pFrame2, &frameFinished2, &packet2);

            if(frameFinished2)
			{
                SDL_LockYUVOverlay(bmp);

                AVPicture pict2;
                pict2.data[0] = bmp->pixels[0];
                pict2.data[1] = bmp->pixels[2];
                pict2.data[2] = bmp->pixels[1];

                pict2.linesize[0] = bmp->pitches[0];
                pict2.linesize[1] = bmp->pitches[2];
                pict2.linesize[2] = bmp->pitches[1];

                sws_scale(sws_ctx2, (uint8_t const * const *)pFrame2->data, pFrame2->linesize,
                          0, pCodecCtx2->height, pict2.data, pict2.linesize);

                SDL_UnlockYUVOverlay(bmp);

                rect.x = 0;
                rect.y = 0;
                rect.w = pCodecCtx2->width;
                rect.h = pCodecCtx2->height;

                SDL_DisplayYUVOverlay(bmp, &rect);
                SDL_Delay(10);

            }
        }

		

        av_free_packet(&packet2);
        SDL_PollEvent(&event);

        switch (event.type) {

            case SDL_QUIT:
                SDL_Quit();
                exit(0);
                break;

            default:
                break;
        }

	numVideo = false;
    }
	}

}


    av_free(pFrame);
	av_free(pFrame2);
    
	avcodec_close(pCodecCtx);
	avcodec_close(pCodecCtx2);

    avformat_close_input(&pFormatCtx);
	avformat_close_input(&pFormatCtx2);

    return 0;
}
/*
void display_text (int x, int y, TTF_Font* font, SDL_Surface* text, SDL_Surface* destination){
	SDL_Rect offset;

	offset.x = x;
	offset.y = y;

	SDL_BlitSurface (text, NULL, destination, &offset);

}
*/
