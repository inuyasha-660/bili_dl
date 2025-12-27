#include "api/api.h"
#include <inttypes.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int api_video_merge(char *filename_video, char *filename_audio, char *outdir,
                    char *outname)
{
    fprintf(stderr, "INFO: Merging video and audio\n");

    // 创建音频&视频上下文
    AVFormatContext *a_pFormatContext = avformat_alloc_context();
    if (!a_pFormatContext) {
        fprintf(stderr, "Error: Failed to alloc memory for a_pFormatContext\n");
        return -1;
    }
    AVFormatContext *v_pFormatContext = avformat_alloc_context();
    if (!v_pFormatContext) {
        fprintf(stderr, "Error: Failed to alloc memory for v_pFormatContext\n");
        return -1;
    }
    // 创建输出上下文
    AVFormatContext *out_ctx = NULL;
    size_t len = snprintf(NULL, 0, "%s/%s.mp4", outdir, outname);
    char *out_path = (char *)malloc((len + 1) * sizeof(char));
    snprintf(out_path, len + 1, "%s/%s.mp4", outdir, outname);
    avformat_alloc_output_context2(&out_ctx, NULL, "mp4", out_path);

    // 打开媒体文件
    if (avformat_open_input(&a_pFormatContext, filename_audio, NULL, NULL) !=
        0) {
        fprintf(stderr, "Error: Failed to open %s\n", filename_audio);
        return -1;
    }
    if (avformat_open_input(&v_pFormatContext, filename_video, NULL, NULL) !=
        0) {
        fprintf(stderr, "Error: Failed to open %s\n", filename_video);
        return -1;
    }

    // 读取流信息
    if (avformat_find_stream_info(a_pFormatContext, NULL) < 0) {
        fprintf(stderr, "Error: Failed to get a_pFormaContext(stream)info\n");
        return -1;
    }
    if (avformat_find_stream_info(v_pFormatContext, NULL) < 0) {
        fprintf(stderr, "Error: Failed to get v_pFormaContext(stream)info\n");
        return -1;
    }

    // 创建输出流
    AVStream *out_v_stream = avformat_new_stream(out_ctx, NULL);
    avcodec_parameters_copy(out_v_stream->codecpar,
                            v_pFormatContext->streams[0]->codecpar);
    out_v_stream->codecpar->codec_tag = 0;

    AVStream *out_a_stream = avformat_new_stream(out_ctx, NULL);
    avcodec_parameters_copy(out_a_stream->codecpar,
                            a_pFormatContext->streams[0]->codecpar);
    out_a_stream->codecpar->codec_tag = 0;

    avio_open(&out_ctx->pb, out_path, AVIO_FLAG_WRITE);
    if (avformat_write_header(out_ctx, NULL) != 0) {
        fprintf(stderr, "Error: Failed to write header\n");
    }

    AVPacket *pkt_v = av_packet_alloc();
    int ret_v = av_read_frame(v_pFormatContext, pkt_v);
    AVPacket *pkt_a = av_packet_alloc();
    int ret_a = av_read_frame(a_pFormatContext, pkt_a);

    while (ret_v >= 0 || ret_a >= 0) {
        int write_next = 0;
        if (ret_v >= 0 && ret_a >= 0) {
            long cmp = av_compare_ts(
                pkt_v->pts, v_pFormatContext->streams[0]->time_base, pkt_a->pts,
                a_pFormatContext->streams[0]->time_base);
            write_next = (cmp <= 0); // 视频早或相等，优先写视频
        } else if (ret_v >= 0) {
            write_next = 1; // 视频
        } else {
            write_next = 0; // 音频
        }

        AVPacket *pkt_w;
        AVFormatContext *in_ctx;
        AVStream *in_stream, *out_stream;
        int *ret_read;

        if (write_next) {
            pkt_w = pkt_v;
            in_ctx = v_pFormatContext;
            in_stream = v_pFormatContext->streams[0];
            out_stream = out_v_stream;
            ret_read = &ret_v;
        } else {
            pkt_w = pkt_a;
            in_ctx = a_pFormatContext;
            in_stream = a_pFormatContext->streams[0];
            out_stream = out_a_stream;
            ret_read = &ret_a;
        }

        av_packet_rescale_ts(pkt_w, in_stream->time_base,
                             out_stream->time_base);
        pkt_w->stream_index = out_stream->index;

        // 写入数据
        av_interleaved_write_frame(out_ctx, pkt_w);
        av_packet_unref(pkt_w);

        *ret_read = av_read_frame(in_ctx, pkt_w);
    }

    av_write_trailer(out_ctx);

    free(out_path);
    avio_closep(&out_ctx->pb);
    avformat_free_context(out_ctx);
    avformat_close_input(&v_pFormatContext);
    avformat_close_input(&a_pFormatContext);
    return 0;
}