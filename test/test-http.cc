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

#include <flare/util/util.h>
#include <flare/util/http.h>
#include <flare/util/string.h>
#include <flare/util/fs.h>
#include <flare/util/cb.h>

using namespace flare;

void test_http(int argc, char **argv) {
	LOG(HttpHelper::get_sync("http://www.163.com/"));
	
	String m3u8 = "http://huace.cdn.ottcloud.tv/huace/videos/dst/2016/08/"
	"14461-ai-de-zhui-zong-01-ji_201608180551/14461-ai-de-zhui-zong-01-ji.m3u8";
	String m3u8_2 = "http://huace.cdn.ottcloud.tv/huace/videos/dst/2016/08/"
	"14461-ai-de-zhui-zong-01-ji_201608180551/hls1600k/14461-ai-de-zhui-zong-01-ji.m3u8";
	String url  = "http://www.baidu.com/";
	
	HttpHelper::download_sync("https://www.baidu.com/", Path::documents("baidu3.htm"));
	
	LOG(FileHelper::read_file_sync(Path::documents("baidu3.htm")));
	
	typedef Callback<StreamResponse> SCb;
	
	HttpHelper::get_stream(url, SCb([](SCb::Data& d) {
		if ( d.error ) {
			LOG(d.error->message());
		} else {
			StreamResponse* data = d.data;
			int len = data->buffer().length();
			LOG(String(data->buffer().collapse(), len));
			LOG("%llu/%llu, complete: %i", data->size(), data->total(), data->complete());
			
			FileHelper::abort(data->id());
		}
	}));
	
	RunLoop::current()->run();
	
	LOG("END");
}
