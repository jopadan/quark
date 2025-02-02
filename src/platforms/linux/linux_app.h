/* ***** BEGIN LICENSE BLOCK *****
 * Distributed under the BSD license:
 *
 * Copyright (c) 2015, blue.chu
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

// @private head

#ifndef __quark_platforms_linux__linux_app__
#define __quark_platforms_linux__linux_app__

#include <X11/Xlib.h>
#include "../../util/macros.h"

typedef Window XWindow;
typedef Display XDisplay;

namespace qk {
	class Window;

	class LinuxIMEHelper {
	public:
		static LinuxIMEHelper* Make(WindowImpl* impl,
			int inputStyle = XIMPreeditPosition);
		virtual ~LinuxIMEHelper() = 0;
		virtual void key_press(XKeyPressedEvent *event) = 0;
		virtual void focus_in() = 0;
		virtual void focus_out() = 0;
	};

	class WindowImpl: public SafeFlag {
	public:
		Qk_DEFINE_PROP_GET(Window*, win, Protected);
		Qk_DEFINE_PROP_GET(XWindow, xwin, Protected);
		Qk_DEFINE_PROP_GET(XDisplay*, xdpy, Protected);
		Qk_DEFINE_PROP_GET(LinuxIMEHelper*, ime, Protected);
	};

	XDisplay* openXDisplay(); // open default xdisplay
	float dpiForXDisplay(); // get dpi for default xdisplay
	void post_messate_main(Cb cb, bool sync = false); // sync to x11 main message loop
}

#endif