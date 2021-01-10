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

#include <limits>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include <unistd.h>
#include "ftr/version.h"
#include "ftr/util/util.h"
#include "ftr/util/string.h"
#include "ftr/util/codec.h"
#include "ftr/util/loop.h"
#include "ftr/util/error.h"

#if FX_UNIX
#include <sys/utsname.h>
#include <unistd.h>
#endif

#if FX_ANDROID
#include <android/android.h>
#endif

#if FX_APPLE
#include <mach/mach_time.h>
#include <mach/mach.h>
#include <mach/clock.h>

#define clock_gettime clock_gettime2

static clock_serv_t get_clock_port(clock_id_t clock_id) {
	clock_serv_t clock_r;
	host_get_clock_service(mach_host_self(), clock_id, &clock_r);
	return clock_r;
}

static clock_serv_t clock_realtime = get_clock_port(CALENDAR_CLOCK);
static mach_port_t clock_monotonic = get_clock_port(SYSTEM_CLOCK);

int clock_gettime2(clockid_t id, struct timespec *tspec) {
	mach_timespec_t mts;
	int retval = 0;
	if (id == CLOCK_MONOTONIC) {
		retval = clock_get_time(clock_monotonic, &mts);
		if (retval != 0) {
			return retval;
		}
	} else if (id == CLOCK_REALTIME) {
		retval = clock_get_time(clock_realtime, &mts);
		if (retval != 0) {
			return retval;
		}
	} else {
		/* only CLOCK_MONOTOIC and CLOCK_REALTIME clocks supported */
		return -1;
	}
	tspec->tv_sec = mts.tv_sec;
	tspec->tv_nsec = mts.tv_nsec;
	return 0;
}

#endif

#if defined(__GLIBC__) || defined(__GNU_LIBRARY__)
# define FX_VLIBC_GLIBC 1
#endif

#if FX_VLIBC_GLIBC || FX_BSD
# include <cxxabi.h>
# include <dlfcn.h>
# include <execinfo.h>
#elif FX_QNX
# include <backtrace.h>
#endif  // FX_VLIBC_GLIBC || FX_BSD

#if FX_GNUC && !FX_ANDROID
# define IMMEDIATE_CRASH() __builtin_trap()
#else
# define IMMEDIATE_CRASH() ((void(*)())0)()
#endif

#ifndef nx_stderr
# define nx_stderr stdout
#endif

FX_NS(ftr)

#define define_number(T) \
template<> const T Number<T>::min(std::numeric_limits<T>::min());\
template<> const T Number<T>::max(std::numeric_limits<T>::max());
	define_number(float);
	define_number(double);
	define_number(char);
	define_number(byte);
	define_number(int16);
	define_number(uint16);
	define_number(int);
	define_number(uint);
	define_number(int64);
	define_number(uint64);

void Console::log(cString& str) {
	printf("%s\n", *str);
}
void Console::warn(cString& str) {
	printf("Warning: %s\n", *str);
}
void Console::error(cString& str) {
	fprintf(nx_stderr, "%s\n", *str);
}
void Console::print(cString& str) {
	printf("%s", *str);
}
void Console::print_err(cString& str) {
	fprintf(nx_stderr, "%s", *str);
}
void Console::clear() {
	// noop
}

Console* _default_console = nullptr;

void Console::set_as_default() {
	if (_default_console != this) {
		delete _default_console;
		_default_console = this;
	}
}

static Console* default_console() {
	if (!_default_console) {
		New<Console>()->set_as_default();
	}
	return _default_console;
}

namespace console {
	
	void report_error(cchar* format, ...) {
		FX_STRING_FORMAT(format, str);
		printf("%s", *str);
	}
	
	// Attempts to dump a backtrace (if supported).
	void dump_backtrace() {
#if FX_VLIBC_GLIBC || FX_BSD
		void* trace[100];
		int size = backtrace(trace, 100);
		report_error("\n==== C stack trace ===============================\n\n");
		if (size == 0) {
			report_error("(empty)\n");
		} else {
			for (int i = 1; i < size; ++i) {
				report_error("%2d: ", i);
				Dl_info info;
				char* demangled = NULL;
				if (!dladdr(trace[i], &info) || !info.dli_sname) {
					report_error("%p\n", trace[i]);
				} else if ((demangled = abi::__cxa_demangle(info.dli_sname, 0, 0, 0))) {
					report_error("%s\n", demangled);
					free(demangled);
				} else {
					report_error("%s\n", info.dli_sname);
				}
			}
		}
#elif FX_QNX
		char out[1024];
		bt_accessor_t acc;
		bt_memmap_t memmap;
		bt_init_accessor(&acc, BT_SELF);
		bt_load_memmap(&acc, &memmap);
		bt_sprn_memmap(&memmap, out, sizeof(out));
		error(out);
		bt_addr_t trace[100];
		int size = bt_get_backtrace(&acc, trace, 100);
		report_error("\n==== C stack trace ===============================\n\n");
		if (size == 0) {
			report_error("(empty)\n");
		} else {
			bt_sprnf_addrs(&memmap, trace, size, const_cast<char*>("%a\n"),
										 out, sizeof(out), NULL);
			report_error(out);
		}
		bt_unload_memmap(&memmap);
		bt_release_accessor(&acc);
#endif  // FX_VLIBC_GLIBC || FX_BSD
	}
	
	void log(char msg) {
		default_console()->log( String::format("%d", msg) );
	}
	
	void log(byte msg) {
		default_console()->log( String::format("%u", msg) );
	}

	void log(int16 msg) {
		default_console()->log( String::format("%d", msg) );
	}

	void log(uint16 msg) {
		default_console()->log( String::format("%u", msg) );
	}

	void log(int msg) {
		default_console()->log( String::format("%d", msg) );
	}
	
	void log(uint msg) {
		default_console()->log( String::format("%u", msg) );
	}

	void log(float msg) {
		default_console()->log( String::format("%f", msg) );
	}

	void log(double msg) {
		default_console()->log( String::format("%lf", msg) );
	}

	void log(int64 msg) {
#if FX_ARCH_64BIT
		default_console()->log( String::format("%ld", msg) );
#else
		default_console()->log( String::format("%lld", msg) );
#endif
	}
	
#if FX_ARCH_32BIT
	void log(long msg) {
		default_console()->log( String::format("%ld", msg) );
	}
	void log(unsigned long msg) {
		default_console()->log( String::format("%lu", msg) );
	}
#endif 

	void log(uint64 msg) {
#if FX_ARCH_64BIT
		default_console()->log( String::format("%lu", msg) );
#else
		default_console()->log( String::format("%llu", msg) );
#endif
	}

	void log(bool msg) {
		default_console()->log( msg ? "true": "false" );
	}
	
	void log(cchar* format, ...) {
		FX_STRING_FORMAT(format, str);
		default_console()->log(str);
	}
	
	void log(cString& msg) {
		default_console()->log(msg);
	}
	
	void log_ucs2(cUcs2String& msg) {
		String s = Coder::encoding(Encoding::utf8, msg);
		default_console()->log(s);
	}
		
	void print(cchar* format, ...) {
		FX_STRING_FORMAT(format, str);
		default_console()->print(str);
	}

	void print(cString& str) {
		default_console()->print(str);
	}
	
	void print_err(cchar* format, ...) {
		FX_STRING_FORMAT(format, str);
		default_console()->print_err(str);
	}
	
	void print_err(cString& str) {
		default_console()->print_err(str);
	}
	
	void warn(cchar* format, ...) {
		FX_STRING_FORMAT(format, str);
		default_console()->warn(str);
	}
	
	void warn(cString& str) {
		default_console()->warn(str);
	}
	
	void error(cchar* format, ...) {
		FX_STRING_FORMAT(format, str);
		default_console()->error(str);
	}
	
	void error(cString& str) {
		default_console()->error(str);
	}

	void error(const Error& err) {
		String str = String::format("Error: %d \n message:\n\t%s", err.code(), *err.message());
		default_console()->error(err.message());
	}
	
	void tag(cchar* tag, cchar* format, ...) {
		FX_STRING_FORMAT(format, str);
		default_console()->print(String::format("%s ", tag));
		default_console()->log(str);
	}

	void clear() {
		default_console()->clear();
	}
	
} // end namescape console {

static cchar* I64BIT_TABLE =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-";

String SimpleHash::digest() {
	String rev;
	do {
		rev += I64BIT_TABLE[_hash & 0x3F];
	} while (_hash >>= 6);
	_hash = 5381;
	return rev;
}

uint hash_code(cchar* data, uint len) {
	SimpleHash hash;
	hash.update(data, len);
	return hash.hash_code();
}

String hash(cchar* data, uint len) {
	SimpleHash hash;
	hash.update(data, len);
	return hash.digest();
}

String hash(cString& str) {
	return hash(*str, str.length());
}

int random(uint start, uint end) {
	static uint id;
	srand(uint(time(NULL) + id));
	id = rand();
	return (id % (end - start + 1)) + start;
}

int fix_random(uint a, ...) {
	int i = 0;
	int total = a;
	va_list ap;
	va_start(ap, a);
	while (1) {
		int e = va_arg(ap, int);
		if (e < 1) {
			break;
		}
		total += e;
	}
	//
	va_start(ap, a);
	int r = random(0, total - 1);
	total = a;
	if (r >= total) {
		while (1) {
			i++;
			int e = va_arg(ap, int);
			if (e < 1) {
				break;
			}
			total += e;
			if (r < total) {
				break;
			}
		}
	}
	va_end(ap);
	return i;
}

void fatal(cchar* file, uint line, cchar* func, cchar* msg, ...) {
	fflush(stdout);
	fflush(nx_stderr);
	if (msg) {
		FX_STRING_FORMAT(msg, str);
		default_console()->print_err("\n\n\n");
		default_console()->error(str);
	}
	console::report_error("#\n# Fatal error in %s, line %d, func %s\n# \n\n", file, line, func);
	console::dump_backtrace();
	fflush(stdout);
	fflush(nx_stderr);
	IMMEDIATE_CRASH();
}

static std::atomic<uint64> id(10);

uint64 iid() {
	return id++;
}

uint iid32() {
	return id++ % Uint::max;
}

String version() {
	return FTR_VERSION;
}

String platform() {
#if  FX_IOS || FX_OSX
		static String _name("darwin");
#elif FX_LINUX
		static String _name("linux");
#elif  FX_ANDROID
			static String _name("android");
#elif  FX_WIN
		static String _name("win32");
#elif FX_NACL
		static String _name("nacl"); // ?
#elif  FX_QNX
		static String _name("qnx"); // ?
#else
	# error no support
#endif
	return _name;
}

FX_END
