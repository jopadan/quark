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

#include "ftr/util/fs.h"
#include "ftr/util/http.h"
#include "ftr-js/js.h"
#include "ftr-js/str.h"
#include "cb-1.h"

JS_BEGIN

template<class Type, class Err = Error>
Cb get_callback_for_type(Worker* worker, Local<JSValue> cb) {
	if ( !cb.IsEmpty() && cb->IsFunction(worker) ) {
		CopyablePersistentFunc func(worker, cb.To<JSFunction>());
		
		return Cb([worker, func](CbD& d) {
			ASSERT(!func.IsEmpty());
			HandleScope scope(worker);
			JS_CALLBACK_SCOPE();
			Local<JSFunction> f = func.local();
			
			if ( d.error ) {
				Local<JSValue> arg = worker->New(*static_cast<const Err*>(d.error));
				f->Call(worker, 1, &arg);
			} else {
				Type* data = static_cast<Type*>(d.data);
				Local<JSValue> args[2] = { worker->NewNull(), worker->New(*data) };
				f->Call(worker, 2, args);
			}
		});
	} else {
		return 0;
	}
}

Local<JSValue> convert_buffer(Worker* worker, Buffer& buffer, Encoding encoding) {
	Local<JSValue> result;
	switch (encoding) { // buffer
		case Encoding::unknown:
			result = worker->New(buffer);
			break;
		case Encoding::hex: // 编码
		case Encoding::base64: { // string
			String str = Coder::encoding(encoding, buffer).collapse_string();
			result = worker->New(str);
			break;
		}
		default: { // string
			Ucs2String str(Coder::decoding_to_uint16(encoding, buffer));
			result = worker->New(str);
			break;
		}
	}
	return result;
}

template<class Err = Error>
Cb get_callback_for_buffer2(Worker* worker, Local<JSValue> cb, Encoding encoding) {
	if ( !cb.IsEmpty() && cb->IsFunction(worker) ) {
		CopyablePersistentFunc func(worker, cb.To<JSFunction>());

		return Cb([worker, func, encoding](CbD& d) {
			ASSERT(!func.IsEmpty());
			JS_HANDLE_SCOPE();
			JS_CALLBACK_SCOPE();
			Local<JSFunction> f = func.local();

			if ( d.error ) {
				Local<JSValue> arg = worker->New(*static_cast<const Err*>(d.error));
				f->Call(worker, 1, &arg);
			} else {
				Buffer* bf = static_cast<Buffer*>(d.data);
				Local<JSValue> args[2] = { worker->NewNull(), convert_buffer(worker, *bf, encoding) };
				f->Call(worker, 2, args);
			}
		});
	} else {
		return 0;
	}
}

template<class Err = Error>
Cb get_callback_for_io_stream2(Worker* worker, Local<JSValue> cb) {
	if ( !cb.IsEmpty() && cb->IsFunction(worker) ) {
		CopyablePersistentFunc func(worker, cb.To<JSFunction>());
		
		return Cb([worker, func](CbD& d) {
			ASSERT(!func.IsEmpty());
			HandleScope scope(worker);
			JS_CALLBACK_SCOPE();
			
			Local<JSFunction> f = func.local();
			
			if ( d.error ) {
				Local<JSValue> arg = worker->New(*static_cast<const Err*>(d.error));
				f->Call(worker, 1, &arg);
			} else {
				IOStreamData* data = static_cast<IOStreamData*>(d.data);
				Local<JSObject> arg = worker->NewObject();
				arg->Set(worker, worker->strs()->data(), worker->New(data->buffer()) );
				arg->Set(worker, worker->strs()->complete(), worker->New(data->complete()) );
				arg->Set(worker, worker->strs()->size(), worker->New(data->size()) );
				arg->Set(worker, worker->strs()->total(), worker->New(data->total()) );
				Local<JSValue> args[2] = { worker->NewNull(), arg };
				f->Call(worker, 2, args);
			}
		});
	} else {
		return 0;
	}
}

template<class Err = Error>
Cb get_callback_for_response_data2(Worker* worker, Local<JSValue> cb) {
	if ( !cb.IsEmpty() && cb->IsFunction(worker) ) {
		CopyablePersistentFunc func(worker, cb.To<JSFunction>());
		
		return Cb([worker, func](CbD& d) {
			ASSERT(!func.IsEmpty());
			HandleScope scope(worker);
			JS_CALLBACK_SCOPE();
			
			Local<JSFunction> f = func.local();
			
			if ( d.error ) {
				Local<JSValue> arg = worker->New(*static_cast<const Err*>(d.error));
				f->Call(worker, 1, &arg);
			} else {
				HttpHelper::ResponseData* data = static_cast<HttpHelper::ResponseData*>(d.data);
				Local<JSObject> arg = worker->NewObject();
				arg->Set(worker, worker->strs()->data(), worker->New(data->data) );
				arg->Set(worker, worker->strs()->httpVersion(), worker->New(data->http_version) );
				arg->Set(worker, worker->strs()->statusCode(), worker->New(data->status_code) );
				arg->Set(worker, worker->strs()->responseHeaders(), worker->New(data->response_headers) );
				Local<JSValue> args[2] = { worker->NewNull(), arg };
				f->Call(worker, 2, args);
			}
		});
	} else {
		return 0;
	}
}

Cb get_callback_for_none(Worker* worker, Local<JSValue> cb) {
	if ( !cb.IsEmpty() && cb->IsFunction(worker) ) {
		CopyablePersistentFunc func(worker, cb.To<JSFunction>());
		return Cb([worker, func](CbD& d) {
			ASSERT(!func.IsEmpty());
			HandleScope scope(worker);
			JS_CALLBACK_SCOPE();
			Local<JSFunction> f = func.local();
			if ( d.error ) {
				Local<JSValue> arg = worker->New(*static_cast<const Error*>(d.error));
				f->Call(worker, 1, &arg);
			} else {
				f->Call(worker);
			}
		});
	} else {
		return 0;
	}
}

Cb get_callback_for_buffer(Worker* worker, Local<JSValue> cb, Encoding encoding) {
	return get_callback_for_buffer2(worker, cb, encoding);
}

Cb get_callback_for_buffer_http_error(Worker* worker, Local<JSValue> cb, Encoding encoding) {
	return get_callback_for_buffer2<HttpError>(worker, cb, encoding);
}

Cb get_callback_for_response_data_http_error(Worker* worker, Local<JSValue> cb) {
	return get_callback_for_response_data2<HttpError>(worker, cb);
}

Cb get_callback_for_io_stream(Worker* worker, Local<JSValue> cb) {
	return get_callback_for_io_stream2(worker, cb);
}

Cb get_callback_for_io_stream_http_error(Worker* worker, Local<JSValue> cb) {
	return get_callback_for_io_stream2<HttpError>(worker, cb);
}

Cb get_callback_for_array_dirent(Worker* worker, Local<JSValue> cb) {
	return get_callback_for_type<Array<Dirent>>(worker, cb);
}

Cb get_callback_for_bool(Worker* worker, Local<JSValue> cb) {
	return get_callback_for_type<Bool>(worker, cb);
}

Cb get_callback_for_int(Worker* worker, Local<JSValue> cb) {
	return get_callback_for_type<Int>(worker, cb);
}

Cb get_callback_for_file_stat(Worker* worker, Local<JSValue> cb) {
	return get_callback_for_type<FileStat>(worker, cb);
}

JS_END
