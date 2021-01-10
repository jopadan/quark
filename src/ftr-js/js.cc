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

#include "ftr/util/string.h"
#include "js-1.h"
#include "native-inl-js.h"
#include "native-lib-js.h"
#include "ftr.h"
#include "ftr/util/http.h"
#include "ftr/util/codec.h"
#include "value.h"

/**
 * @ns ftr::js
 */

JS_BEGIN

using namespace native_js;

Buffer JSValue::ToBuffer(Worker* worker, Encoding en) const {
	switch (en) {
		case Encoding::hex: // 解码 hex and base64
		case Encoding::base64:
			return Coder::decoding_to_byte(en, ToStringValue(worker,1));
		case Encoding::utf8:// ucs2编码成utf8
			return ToStringValue(worker).collapse_buffer();
		case Encoding::ucs2:
		case Encoding::utf16: {
			Ucs2String str = ToUcs2StringValue(worker);
			uint len = str.length() * 2;
			return Buffer((char*)str.collapse(), len);
		}
		default: // 编码
			return Coder::encoding(en, ToStringValue(worker));
	}
}

bool JSValue::IsBuffer() const {
  return IsTypedArray() || IsArrayBuffer();
}

bool JSValue::IsBuffer(Worker *worker) const {
  return IsBuffer();
}

WeakBuffer JSValue::AsBuffer(Worker *worker) {
  if (IsTypedArray(worker)) {
    return static_cast<JSTypedArray*>(this)->weakBuffer(worker);
  } else if (IsArrayBuffer()) {
    return static_cast<JSArrayBuffer*>(this)->weakBuffer(worker);
  }
  return WeakBuffer();
}

Maybe<Map<String, int>> JSObject::ToIntegerMap(Worker* worker) {
	Map<String, int> r;
	
	if ( IsObject(worker) ) {
	
		Local<JSArray> names = GetPropertyNames(worker);
		if ( names.IsEmpty() ) return Maybe<Map<String, int>>();
		
		for ( uint i = 0, len = names->Length(worker); i < len; i++ ) {
			Local<JSValue> key = names->Get(worker, i);
			if ( names.IsEmpty() ) return Maybe<Map<String, int>>();
			Local<JSValue> val = Get(worker, key);
			if ( val.IsEmpty() ) return Maybe<Map<String, int>>();
			if ( val->IsNumber(worker) ) {
				r.set( key->ToStringValue(worker), val->ToNumberValue(worker) );
			} else {
				r.set( key->ToStringValue(worker), val->ToBooleanValue(worker) );
			}
		}
	}
	return Maybe<Map<String, int>>(move(r));
}

Maybe<Map<String, String>> JSObject::ToStringMap(Worker* worker) {
	Map<String, String> r;
	
	if ( IsObject(worker) ) {
		Local<JSArray> names = GetPropertyNames(worker);
		if ( names.IsEmpty() ) return Maybe<Map<String, String>>();
		
		for ( uint i = 0, len = names->Length(worker); i < len; i++ ) {
			Local<JSValue> key = names->Get(worker, i);
			if ( names.IsEmpty() ) return Maybe<Map<String, String>>();
			Local<JSValue> val = Get(worker, key);
			if ( val.IsEmpty() ) return Maybe<Map<String, String>>();
			r.set( key->ToStringValue(worker), val->ToStringValue(worker) );
		}
	}
	return Maybe<Map<String, String>>(move(r));
}

Maybe<JSON> JSObject::ToJSON(Worker* worker) {
	JSON r = JSON::object();
	
	if ( IsObject(worker) ) {
		Local<JSArray> names = GetPropertyNames(worker);
		if ( names.IsEmpty() ) return Maybe<JSON>();
		
		for ( uint i = 0, len = names->Length(worker); i < len; i++ ) {
			Local<JSValue> key = names->Get(worker, i);
			if ( names.IsEmpty() ) return Maybe<JSON>();
			Local<JSValue> val = Get(worker, key);
			if ( val.IsEmpty() ) return Maybe<JSON>();
			String key_s = key->ToStringValue(worker);
			if (val->IsUint32(worker)) {
				r[key_s] = val->ToUint32Value(worker);
			} else if (val->IsInt32(worker)) {
				r[key_s] = val->ToInt32Value(worker);
			} else if (val->IsNumber(worker)) {
				r[key_s] = val->ToInt32Value(worker);
			} else if (val->IsBoolean(worker)) {
				r[key_s] = val->ToBooleanValue(worker);
			} else if (val->IsNull(worker)) {
				r[key_s] = JSON::null();
			} else {
				r[key_s] = val->ToStringValue(worker);
			}
		}
	}
	return Maybe<JSON>(move(r));
}

Local<JSValue> JSObject::GetProperty(Worker* worker, cString& name) {
	return Get(worker, worker->New(name, 1));
}

Maybe<Array<String>> JSArray::ToStringArrayMaybe(Worker* worker) {
	Array<String> rv;
	if ( IsArray(worker) ) {
		for ( uint i = 0, len = Length(worker); i < len; i++ ) {
			Local<JSValue> val = Get(worker, i);
			if ( val.IsEmpty() )
				return Maybe<Array<String>>();
			rv.push( val->ToStringValue(worker) );
		}
	}
	return Maybe<Array<String>>(move(rv));
}

Maybe<Array<double>> JSArray::ToNumberArrayMaybe(Worker* worker) {
	Array<double> rv;
	if ( IsArray(worker) ) {
		double out;
		for ( uint i = 0, len = Length(worker); i < len; i++ ) {
			Local<JSValue> val = Get(worker, i);
			if ( val.IsEmpty() || !val->ToNumberMaybe(worker).To(out) )
				return Maybe<Array<double>>();
			rv.push( out );
		}
	}
	return Maybe<Array<double>>(move(rv));
}

Maybe<Buffer> JSArray::ToBufferMaybe(Worker* worker) {
	Buffer rv;
	if ( IsArray(worker) ) {
		double out;
		for ( uint i = 0, len = Length(worker); i < len; i++ ) {
			Local<JSValue> val = Get(worker, i);
			if ( val.IsEmpty() || !val->ToNumberMaybe(worker).To(out) )
				return Maybe<Buffer>();
			rv.push( out );
		}
	}
	return Maybe<Buffer>(move(rv));
}

WeakBuffer JSArrayBuffer::weakBuffer(Worker* worker) {
	int size = ByteLength(worker);
	char* data = Data(worker);
	return WeakBuffer(data, size);
}

WeakBuffer JSTypedArray::weakBuffer(Worker* worker) {
  auto buffer = Buffer(worker);
  char* data = buffer->Data(worker);
  int offset = ByteOffset(worker);
  int len = ByteLength(worker);
  return WeakBuffer(data + offset, len);
}

void JSClass::Export(Worker* worker, cString& name, Local<JSObject> exports) {
	uint64 id = ID();
	auto js_class = IMPL::js_class(worker);
	js_class->reset_constructor(id);
	Local<JSFunction> func = js_class->get_constructor(ID());
	exports->SetProperty(worker, name, func);
}

Local<JSFunction> JSClass::GetFunction(Worker* worker) {
	uint64 id = ID();
	auto js_class = IMPL::js_class(worker);
	js_class->reset_constructor(id);
	return js_class->get_constructor(id);
}

uint64 JSClass::ID() const {
	return reinterpret_cast<const JSClassIMPL*>(this)->id();
}

Local<JSObject> JSClass::NewInstance(uint argc, Local<JSValue>* argv) {
	auto cls = reinterpret_cast<JSClassIMPL*>(this);
	Local<JSFunction> func = IMPL::js_class(cls->worker())->get_constructor(cls->id());
	ASSERT( !func.IsEmpty() );
	return func->NewInstance(cls->worker(), argc, argv);
}

template <> void PersistentBase<JSClass>::Reset() {
	if ( val_ ) {
		reinterpret_cast<JSClassIMPL*>(val_)->release();
		val_ = nullptr;
	}
}

template <> template <>
void PersistentBase<JSClass>::Reset(Worker* worker, const Local<JSClass>& other) {
	if ( val_ ) {
		reinterpret_cast<JSClassIMPL*>(val_)->release();
		val_ = nullptr;
		worker_ = nullptr;
	}
	if ( !other.IsEmpty() ) {
		ASSERT(worker);
		val_ = *other;
		worker_ = worker;
		reinterpret_cast<JSClassIMPL*>(val_)->retain();
	}
}

template<> template<>
void CopyablePersistentClass::Copy(const PersistentBase<JSClass>& that) {
	Reset(that.worker_, that.local());
}

// -------------------------------------------------------------------------------------

struct NativeModule {
	String name;
	String file;
	Worker::BindingCallback binding;
	const LIB_NativeJSCode* native_code;
};

static Map<String, NativeModule>* native_modules = nullptr;

// @private __require__
static void require_native(FunctionCall args) {
	JS_WORKER(args);
	JS_HANDLE_SCOPE();
	if (args.Length() < 1) {
		JS_THROW_ERR("Bad argument.");
	}
	String name = args[0]->ToStringValue(worker);
	Local<JSValue> r = worker->bindingModule(name);
	if (!r.IsEmpty()) {
		JS_RETURN(r);
	}
}

Worker* IMPL::initialize() {
	HandleScope scope(m_host);
	m_native_modules.Reset(m_host, m_host->NewObject());
	m_classs = new JSClassStore(m_host);
	m_strs = new CommonStrings(m_host);
	ASSERT(m_global.local()->IsObject(m_host));
	m_global.local()->SetProperty(m_host, "global", m_global.local());
	m_global.local()->SetMethod(m_host, "__require__", require_native);

	auto globalThis = m_host->New("globalThis");
	if ( !m_global.local()->Has(m_host, globalThis) ) {
		m_global.local()->Set(m_host, globalThis, m_global.local());
	}
	return m_host;
}

void IMPL::release() {
	Release(m_values); m_values = nullptr;
	Release(m_strs); m_strs = nullptr;
	delete m_classs; m_classs = nullptr;
	m_native_modules.Reset();
	m_global.Reset();
}

IMPL::IMPL()
: m_host(nullptr)
, m_thread_id(Thread::current_id())
, m_values(nullptr), m_strs(nullptr)
, m_classs(nullptr), m_is_node(0) {
	m_host = new Worker(this);
}

IMPL::~IMPL() {
	release();
}

static Local<JSValue> TriggerEventFromUtil(Worker* worker,
	cString& name, int argc = 0, Local<JSValue> argv[] = 0)
{
	Local<JSObject> _util = worker->bindingModule("_util").To();
	ASSERT(!_util.IsEmpty());

	Local<JSValue> func = _util->GetProperty(worker, String("__on").push(name).push("_native"));
	if (!func->IsFunction(worker)) {
		return Local<JSValue>();
	}
	return func.To<JSFunction>()->Call(worker, argc, argv);
}

static int TriggerExit_1(Worker* worker, cString& name, int code) {
	JS_HANDLE_SCOPE();
	Local<JSValue> argv = worker->New(code);
	Local<JSValue> rc = TriggerEventFromUtil(worker, name, 1, &argv);
	if (!rc.IsEmpty() && rc->IsInt32(worker)) {
		return rc->ToInt32Value(worker);
	} else {
		return code;
	}
}

static bool TriggerException(Worker* worker, cString& name, int argc, Local<JSValue> argv[]) {
	JS_HANDLE_SCOPE();
	Local<JSValue> rc = TriggerEventFromUtil(worker, name, argc, argv);
	if (!rc.IsEmpty() && rc->ToBooleanValue(worker)) {
		return true;
	} else {
		return false;
	}
}

int IMPL::TriggerExit(int code) {
	return TriggerExit_1(m_host, "Exit", code);
}

int IMPL::TriggerBeforeExit(int code) {
	return TriggerExit_1(m_host, "BeforeExit", code);
}

bool IMPL::TriggerUncaughtException(Local<JSValue> err) {
	return TriggerException(m_host, "UncaughtException", 1, &err);
}

bool IMPL::TriggerUnhandledRejection(Local<JSValue> reason, Local<JSValue> promise) {
	Local<JSValue> argv[] = { reason, promise };
	return TriggerException(m_host, "UncaughtException", 2, argv);
}

Worker* Worker::create() {
	return IMPL::create();
}

void Worker::registerModule(cString& name, BindingCallback binding, cchar* file) {
	if (!native_modules) {
		native_modules = new Map<String, NativeModule>();
	}
	native_modules->set(name, { name, file ? file : __FILE__, binding });
}

Local<JSValue> Worker::bindingModule(cString& name) {
	Local<JSValue> str = New(name);
	Local<JSValue> r = m_inl->m_native_modules.local()->Get(this, str);

	if (!r->IsUndefined()) {
		return r.To<JSObject>();
	}

	auto it = native_modules->find(name);
	if (it.is_null()) {
		return m_inl->binding_node_module(name);
	}

	NativeModule& mod = it.value();
	Local<JSObject> exports = NewObject();

	if (mod.binding) {
		mod.binding(exports, this);
	} else if (mod.native_code) {
		exports = runNativeScript(WeakBuffer((char*)
			mod.native_code->code, 
			mod.native_code->count), 
			String(mod.native_code->name) + mod.native_code->ext,
			exports
		).To();
		if ( exports.IsEmpty() ) { // error
			return exports;
		}
	}
	m_inl->m_native_modules.local()->Set(this, str, exports);
	return exports;
}

Worker::Worker(IMPL* inl): m_inl(inl) {
	// register core native module
	static int initializ_core_native_module = 0;
	if ( initializ_core_native_module++ == 0 ) {
		if (!native_modules) {
			native_modules = new Map<String, NativeModule>();
		}
		for (int i = 0; i < LIB_native_js_count_; i++) {
			const LIB_NativeJSCode* code = LIB_native_js_ + i;
			native_modules->set(code->name, { code->name, code->name, 0, code });
		}
	}
}

Worker::~Worker() {
	delete m_inl; m_inl = nullptr;
}

ValueProgram* Worker::values() {
	return m_inl->m_values;
}

CommonStrings* Worker::strs() {
	return m_inl->m_strs; 
}

ThreadID Worker::threadId() {
	return m_inl->m_thread_id;
}

Local<JSObject> Worker::global() {
	return m_inl->m_global.local();
}

Local<JSObject> Worker::NewError(cchar* errmsg, ...) {
	FX_STRING_FORMAT(errmsg, str);
	Error err(ERR_UNKNOWN_ERROR, str);
	return New(err);
}

Local<JSObject> Worker::New(const HttpError& err) {
	Local<JSObject> rv = New(*static_cast<cError*>(&err));
	if ( !rv.IsEmpty() ) {
		if (!rv->Set(this, strs()->status(), New(err.status()))) return Local<JSObject>();
		if (!rv->Set(this, strs()->url(), New(err.url()))) return Local<JSObject>();
		if (!rv->Set(this, strs()->code(), New(err.code()))) return Local<JSObject>();
	}
	return rv;
}

Local<JSArray> Worker::New(Array<Dirent>& ls) { return New(move(ls)); }
Local<JSArray> Worker::New(Array<FileStat>& ls) { return New(move(ls)); }
Local<JSUint8Array> Worker::New(Buffer& buff) { return New(move(buff)); }
Local<JSObject> Worker::New(FileStat& stat) { return New(move(stat)); }
Local<JSObject> Worker::NewError(cError& err) { return New(err); }
Local<JSObject> Worker::NewError(const HttpError& err) { return New(err); }

Local<JSObject> Worker::New(FileStat&& stat) {
	Local<JSFunction> func = m_inl->m_classs->get_constructor(JS_TYPEID(FileStat));
	ASSERT( !func.IsEmpty() );
	Local<JSObject> r = func->NewInstance(this);
	*Wrap<FileStat>::unpack(r)->self() = move(stat);
	return r;
}

Local<JSObject> Worker::NewInstance(uint64 id, uint argc, Local<JSValue>* argv) {
	Local<JSFunction> func = m_inl->m_classs->get_constructor(id);
	ASSERT( !func.IsEmpty() );
	return func->NewInstance(this, argc, argv);
}

Local<JSUint8Array> Worker::NewUint8Array(Local<JSString> str, Encoding en) {
	Buffer buff = str->ToBuffer(this, en);
	return New(buff);
}

Local<JSUint8Array> Worker::NewUint8Array(int size, char fill) {
  auto ab = NewArrayBuffer(size);
  if (fill)
    memset(ab->Data(this), fill, size);
  return NewUint8Array(ab);
}

Local<JSUint8Array> Worker::NewUint8Array(Local<JSArrayBuffer> ab) {
  return NewUint8Array(ab, 0, ab->ByteLength(this));
}

void Worker::throwError(cchar* errmsg, ...) {
	FX_STRING_FORMAT(errmsg, str);
	throwError(NewError(*str));
}

bool Worker::hasView(Local<JSValue> val) {
	return m_inl->m_classs->instanceof(val, ftr::View::VIEW);
}

bool Worker::hasInstance(Local<JSValue> val, uint64 id) {
	return m_inl->m_classs->instanceof(val, id);
}

JS_END
