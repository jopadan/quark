/* ***** BEGIN LICENSE BLOCK *****
 * Distributed under the BSD license:
 *
 * Copyright (c) 2015, xuewen.chu
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of xuewen.chu nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL xuewen.chu BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * ***** END LICENSE BLOCK ***** */

#ifndef __ftr__media_codec_1__
#define __ftr__media_codec_1__

extern "C" {
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}
#include "ftr/media-codec.h"
#include "ftr/util/loop-1.h"

FX_NS(ftr)

typedef MultimediaSource::BitRateInfo BitRateInfo;
typedef MultimediaSource::Extractor   Extractor;
typedef MultimediaSource::Inl         Inl;
typedef MultimediaSource::TrackInfo   TrackInfo;

class SoftwareMediaCodec;

class MultimediaSource::Inl: public ParallelWorking {
 public:
	
	Inl(MultimediaSource*, cString& uri, RunLoop* loop);
	
	/**
	 * @destructor
	 */
	virtual ~Inl();
	
	/**
	 * @func set_delegate
	 */
	void set_delegate(Delegate* delegate);
	
	/**
	 * @func bit_rate_index
	 */
	uint bit_rate_index();
	
	/**
	 * @func bit_rates
	 */
	const Array<BitRateInfo>& bit_rate();
	
	/**
	 * @func bit_rate
	 */
	bool select_bit_rate(uint index);
	
	/**
	 * @func extractor
	 */
	Extractor* extractor(MediaType type);
		
	/**
	 * @func seek
	 * */
	bool seek(uint64 timeUs);

	/**
	 * @func start
	 * */
	void start();

	/**
	 * @func start
	 * */
	void stop();
	
	/**
	 * @func is_active
	 */
	inline bool is_active() {
		return  m_status == MULTIMEDIA_SOURCE_STATUS_READY ||
						m_status == MULTIMEDIA_SOURCE_STATUS_WAIT;
	}
	
	/**
	 * @func disable_wait_buffer
	 */
	void disable_wait_buffer(bool value);
	
	/**
	 * @func get_stream
	 */
	AVStream* get_stream(const TrackInfo& track);
	
 private:

	typedef Extractor::SampleData SampleData;
	
	void reset();
	bool has_empty_extractor();
	void extractor_flush(Extractor* ex);
	BitRateInfo read_bit_rate_info(AVFormatContext* fmt_ctx, int i, int size);
	void select_multi_bit_rate2(uint index);
	void read_stream(Thread& t, AVFormatContext* fmt_ctx, cString& uri, uint bit_rate_index);
	bool extractor_push(Extractor* ex, AVPacket& pkt, AVStream* stream, double tbn);
	bool extractor_advance(Extractor* ex);
	bool extractor_advance_no_wait(Extractor* ex);
	Extractor* valid_extractor(AVMediaType type);
	bool has_valid_extractor();
	void trigger_error(cError& err);
	void trigger_wait_buffer();
	void trigger_ready_buffer();
	void trigger_eof();
	inline Mutex& mutex() { return m_mutex; }
	
	friend class MultimediaSource;
	friend class Extractor;
	friend class MediaCodec;
	friend class SoftwareMediaCodec;
	
	MultimediaSource*           m_host;
	URI                         m_uri;
	MultimediaSourceStatus      m_status;
	Delegate*                   m_delegate;
	uint                        m_bit_rate_index;
	Array<BitRateInfo>          m_bit_rate;
	Map<int, Extractor*>        m_extractors;
	uint64                      m_duration;
	AVFormatContext*            m_fmt_ctx;
	Mutex                       m_mutex;
	bool                        m_read_eof;
	bool                        m_disable_wait_buffer;
};

FX_END
#endif
