#include "SwrResample.h"
#include "qdebug.h"

int SwrResample::Init(int64_t src_ch_layout, int64_t dst_ch_layout, int src_sample_rate, int dst_sample_rate, AVSampleFormat src_sample_format, AVSampleFormat dst_sample_format, int src_nb_sample)
{
    src_sample_fmt_ = src_sample_format;
    dst_sample_fmt_ = dst_sample_format;

    src_rate_ = src_sample_rate;
    dst_rate_ = dst_sample_rate;

    int ret;
    swr_ctx = swr_alloc();
    if (!swr_ctx)
    {
        qDebug() << "alloc swrcontext failed";
        ret = AVERROR(ENOMEM);
        return ret;
    }

    //设置swrctx参数
    av_opt_set_int(swr_ctx, "in_channel_layout", src_ch_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate", src_sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", src_sample_format, 0);

    av_opt_set_int(swr_ctx, "out_channel_layout", dst_ch_layout, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate", dst_sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", dst_sample_format, 0);

    if ((ret = swr_init(swr_ctx)) < 0)
    {
        qDebug() << "swrctx init failed";
        return ret;
    }

    src_nb_channel = av_get_channel_layout_nb_channels(src_ch_layout);
    
    //每次采集多少样点
    src_nb_sample_ = src_nb_sample;

    //样本数进行重计算(计算出换了HZ之后不改变ms值的情况下一次采样多少个样本)
    max_dst_nb_sample_ = dst_nb_sample_ = av_rescale_rnd(src_nb_sample, dst_rate_, src_rate_, AV_ROUND_UP);


    ret = av_samples_alloc_array_and_samples(&src_data_, &src_linesize, src_nb_channel, src_nb_sample_, src_sample_format, 0);
    if (ret < 0)
    {
        qDebug() << "could not alloc src_data";
        return AVERROR(ENOMEM);
    }

    dst_nb_channel = av_get_channel_layout_nb_channels(dst_ch_layout);

    ret = av_samples_alloc_array_and_samples(&dst_data_, &dst_linesize, dst_nb_channel,
        dst_nb_sample_, dst_sample_format, 0);
    if (ret < 0) {
        qDebug() << "could not alloc dst_data";
        return -1;
    }
    return 0;
}

int SwrResample::WriteInput(const char* data, uint64_t len)
{
    int planar = av_sample_fmt_is_planar(src_sample_fmt_);
    int data_size = av_get_bytes_per_sample(src_sample_fmt_);

    if (planar)
    {

    }
    else
    {
        memcpy(src_data_[0], data, len);
    }
    return 0;
}

int SwrResample::SwrConvert(char** data)
{
    int ret = 0;
    //
    dst_nb_sample_ = av_rescale_rnd(swr_get_delay(swr_ctx,src_rate_)+src_nb_sample_, 
        dst_rate_, src_rate_, AVRounding::AV_ROUND_UP);

    if (dst_nb_sample_ > max_dst_nb_sample_)
    {
        av_freep(&dst_data_[0]);
        ret = av_samples_alloc(dst_data_, &dst_linesize, dst_nb_channel, dst_nb_sample_, dst_sample_fmt_, 1);
        if (ret < 0)
            return -1;
        max_dst_nb_sample_ = dst_nb_sample_;
    }

    ret = swr_convert(swr_ctx, dst_data_, dst_nb_sample_,(const uint8_t **) src_data_, src_nb_sample_);
    if (ret < 0)
    {
        qDebug() << "swr_convert is failed";
        return -1;
    }
   
    //ret 为每个通道输出的样本数
    int dst_buffsize = av_samples_get_buffer_size(nullptr, dst_nb_channel, ret, dst_sample_fmt_, 1);

    int planar = av_sample_fmt_is_planar(dst_sample_fmt_);
    if (planar)
    {

    }
    else
    {
        *data = (char*)(dst_data_[0]);
    }
    return dst_buffsize;
}

void SwrResample::Close()
{
    if (src_data_)
        av_freep(&src_data_[0]);
    av_freep(&src_data_);

    if (dst_data_)
        av_freep(&dst_data_[0]);
    av_freep(&dst_data_);

    swr_free(&swr_ctx);
}
