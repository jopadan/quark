/* ***** BEGIN LICENSE BLOCK *****
 * Distributed under the BSD license:
 *
 * Copyright © 2015-2016, blue.chu
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of blue.chu nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL blue.chu BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ***** END LICENSE BLOCK ***** */

#include "./media_inl.h"

namespace qk {

	class SoftwareMediaCodec: public MediaCodec {
	public:
		SoftwareMediaCodec(const Stream &stream, AVCodecContext* ctx)
			: MediaCodec(stream)
			, _ctx(ctx)
			, _packet(nullptr)
			, _frame(av_frame_alloc())
			, _swr(nullptr)
			, _sws(nullptr)
			, _threads(1)
		{
			Qk_Assert(_frame);
		}

		~SoftwareMediaCodec() override {
			close();
			avcodec_free_context(&_ctx);
			av_frame_free(&_frame);
		}

		void init_swr() {
			if (_ctx->sample_fmt == AV_SAMPLE_FMT_S16)
				return;
			//cChar *layout_name = av_get_channel_layout_name(_ctx->channel_layout);
			//cChar *layout_name = av_get_channel_description(0);
			//Qk_DLog("layout_name, %s", layout_name);
			_swr = swr_alloc_set_opts(_swr,
				_ctx->channel_layout, // out ch layout
				AV_SAMPLE_FMT_S16, // out sample format, signed 16 bits
				_ctx->sample_rate, // out sample rate
				_ctx->channel_layout, // in ch layout
				_ctx->sample_fmt, // in sample foramt
				_ctx->sample_rate, // in_sample_rate
				0, nullptr
			);
			swr_init(_swr);
		}

		void init_sws() {
			if (_ctx->pix_fmt == AV_PIX_FMT_YUV420P || _ctx->pix_fmt == AV_PIX_FMT_NV12)
				return;
			_sws = sws_getContext(
				_ctx->width,
				_ctx->height,
				_ctx->pix_fmt,
				_ctx->width,
				_ctx->height,
				AV_PIX_FMT_YUV420P,
				SWS_BILINEAR, NULL, NULL, NULL
			);
		}

		bool is_open() const override {
			return avcodec_is_open(_ctx);
		}

		bool open(const Stream *stream) override {
			if ( !avcodec_is_open(_ctx) ) {
				if (!stream)
					stream = &_stream;
				Qk_Assert_Eq(stream->codec_id, _stream.codec_id);
				ScopeLock lock(_mutex);
				Qk_Assert(_ctx->codec);
				Qk_Assert_Eq(_ctx->codec_id, stream->extra.codecpar->codec_id);

				if ( _threads > 1 ) { // set threads
					if (    (_ctx->codec->capabilities & AV_CODEC_CAP_FRAME_THREADS)
							&& !(_ctx->flags & AV_CODEC_FLAG_TRUNCATED)
							&& !(_ctx->flags & AV_CODEC_FLAG_LOW_DELAY)
							&& !(_ctx->flags2 & AV_CODEC_FLAG2_CHUNKS)
					) {
						_ctx->thread_count = _threads;
						_ctx->active_thread_type = FF_THREAD_FRAME;
					}
				}

				/* Copy codec parameters from input stream to output codec context */
				if (avcodec_parameters_to_context(_ctx, stream->extra.codecpar) >= 0) {
					if (avcodec_open2(_ctx, nullptr, nullptr) >= 0) {
						if (stream != &_stream)
							_stream = *stream; // copy stream infomaction
						if ( type() == kAudio_MediaType ) {
							init_swr();
						} else {
							init_sws();
						}
					}
				}
				flush2();
			}
			return avcodec_is_open(_ctx);
		}

		void flush2() {
			if ( avcodec_is_open(_ctx) ) {
				avcodec_flush_buffers(_ctx);
				delete _packet; _packet = nullptr;
			}
		}

		void close() override {
			if (avcodec_is_open(_ctx)) {
				ScopeLock lock(_mutex);
				flush2();
				avcodec_close(_ctx);
				if (_swr) {
					swr_free(&_swr);
				}
				if (_sws) {
					sws_freeContext(_sws); _sws = nullptr;
				}
			}
		}

		void flush() override {
			ScopeLock scope(_mutex);
			flush2();
		}

		int send_packet(const Packet *pkt) override {
			ScopeLock scope(_mutex);
			// * @return 0 on success, otherwise negative error code:
			// *      AVERROR(EAGAIN):   input is not accepted right now - the packet must be
			// *                         resent after trying to read output
			// *      AVERROR_EOF:       the decoder has been flushed, and no new packets can
			// *                         be sent to it (also returned if more than 1 flush
			// *                         packet is sent)
			// *      AVERROR(EINVAL):   codec not opened, it is an encoder, or requires flush
			// *      AVERROR(ENOMEM):   failed to add packet to internal queue, or similar
			// *      other errors: legitimate decoding errors
			// */
			return avcodec_send_packet(_ctx, pkt->avpkt);
		}

		int send_packet(Extractor *extractor) override {
			if (!extractor || !avcodec_is_open(_ctx)) {
				return AVERROR(EINVAL);
			}
			Qk_Assert_Eq(type(), extractor->type());

			ScopeLock scope(_mutex);
			if (!_packet) {
				_packet = extractor->advance();
			}
			if (!_packet) {
				return AVERROR(EAGAIN);
			}
			//auto ts = time_monotonic();
			int rc = avcodec_send_packet(_ctx, _packet->avpkt);
			if (rc == 0) {
				delete _packet; _packet = nullptr;
				//Qk_DLog("avcodec_send_packet, %d", time_monotonic() - ts);
			}
			return rc;
		}

		Frame* receive_frame() override {
			ScopeLock scope(_mutex);
			Frame tmp;
			int rc;
			if ( type() == kVideo_MediaType ) {
				rc = receive_frame_video(tmp);
			} else {
				rc = receive_frame_audio(tmp);
			}
			if (rc == 0) {
				auto f = (Frame*)::malloc(sizeof(Frame) + sizeof(AVFrame));
				*f = tmp;
				f->avframe = reinterpret_cast<AVFrame*>(f + 1);
				av_frame_move_ref(f->avframe, _frame);
				f->data = f->avframe->data;
				f->linesize = reinterpret_cast<uint32_t*>(f->avframe->linesize);
				return f;
			}
			return nullptr;
		}

		int receive_frame_video(Frame &out) {
			int rc = avcodec_receive_frame(_ctx, _frame);
			if (rc == 0) {
				auto unit = 1000000.0 * _stream.time_base[0] / _stream.time_base[1];
				auto w = _ctx->width, h = _ctx->height;

				if (_sws) {
					AVFrame dest;
					memset(&dest, 0, sizeof(AVPicture));

					auto buf = av_buffer_alloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, w, h, 1));
					Qk_Assert_Eq(buf->size,
						av_image_fill_arrays(dest.data, dest.linesize, buf->data, AV_PIX_FMT_YUV420P, w, h, 1)
					);
					Qk_Assert_Eq(h, sws_scale(_sws,
						_frame->data,
						_frame->linesize,
						0, h,
						dest.data,
						dest.linesize
					));
					for (int i = 0; i < FF_ARRAY_ELEMS(_frame->buf); i++) {
						av_buffer_unref(&_frame->buf[i]);
					}
					_frame->buf[0] = buf;
					_frame->format = AV_PIX_FMT_YUV420P;
					*((AVPicture*)_frame) = *((AVPicture*)&dest);
				}
				if (_frame->format == AV_PIX_FMT_YUV420P) { // yuv420p
					out.format = kYUV420P_ColorType;
					out.dataitems = 3;
				} else { // yuv420sp
					out.format = kYUV420SP_ColorType;
					out.dataitems = 2;
					Qk_Assert_Eq(_frame->format, AV_PIX_FMT_NV12);
				}
				out.pts = Int64::max(_frame->pts * unit, 0);
				out.pkt_duration = _frame->pkt_duration * unit;
				out.nb_samples = 0;
				out.width = w;
				out.height = h;
				//Qk_DLog("sws_scale, %d", time_monotonic() - ts);
			}
			return rc;
		}

		int receive_frame_audio(Frame &out) {
			int rc = avcodec_receive_frame(_ctx, _frame);
			if (rc == 0) {
				auto unit = 1000000.0 * _stream.time_base[0] / _stream.time_base[1];
				if (_swr) {
					auto nb_samples = _frame->nb_samples;
					auto sample_bytes = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
					auto fsize = nb_samples * _frame->channels * sample_bytes;
					auto noalloc = _frame->buf[1] && _frame->buf[1]->size >= fsize;
					Qk_Assert_Ne(fsize, 0);
					auto buf = noalloc ? _frame->buf[1]: av_buffer_alloc(fsize);
					Qk_Assert_Eq(nb_samples,
						swr_convert(_swr, &buf->data, nb_samples, (const uint8_t**)_frame->data, nb_samples)
					);
					if (!noalloc) {
						av_buffer_unref(_frame->buf); // free old buf
						_frame->buf[0] = buf;
					}
					_frame->format = AV_SAMPLE_FMT_S16;
					_frame->data[0] = buf->data;
					_frame->linesize[0] = fsize;
				}
				Qk_Assert_Eq(_frame->format, AV_SAMPLE_FMT_S16);
				out.dataitems = 1;
				out.pts = Int64::max(_frame->pts * unit, 0);
				out.pkt_duration = _frame->pkt_duration * unit;;
				out.nb_samples = _frame->nb_samples;
				out.width = 0;
				out.height = 0;
				out.format = AV_SAMPLE_FMT_S16;
			}
			return rc;
		}

		void set_threads(uint32_t value) override {
			_threads = Uint32::clamp(value, 1, 8);
		}

	private:
		AVCodecContext* _ctx;
		Packet*         _packet; // send a packet to decoder
		AVFrame*        _frame; // temp av frame
		SwrContext*     _swr;
		SwsContext*     _sws;
		uint32_t        _threads;
		Mutex           _mutex;
	};

	/**
	* @method MediaCodec_software create software decoder
	*/
	MediaCodec* MediaCodec_software(MediaType type, Extractor* ex) {
		SoftwareMediaCodec* rv = nullptr;
		if ( ex ) {
			auto codec = avcodec_find_decoder((AVCodecID)ex->stream().codec_id);
			auto ctx = avcodec_alloc_context3(codec);
			if (ctx) {
				rv = new SoftwareMediaCodec(ex->stream(), ctx);
			}
		}
		return rv;
	}
}
