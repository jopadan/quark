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

#include "ftr/util/loop.h"
#include "ftr/app-1.h"
#include "ftr/event.h"
#include "ftr/display-port.h"
#include "ftr/util/loop.h"
#include "ftr/util/http.h"
#include "ftr/sys.h"
#include "linux-gl-1.h"
#include "linux-ime-helper-1.h"
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XInput2.h>
#include <X11/Xatom.h>
#include <signal.h>
#include <unistd.h>
#include <alsa/asoundlib.h>

FX_NS(ftr)

class LinuxApplication;
static LinuxApplication* application = nullptr;
static GLDrawProxy* gl_draw_context = nullptr;
typedef DisplayPort::Orientation Orientation;

#if DEBUG || FX_MORE_LOG
cchar* MOUSE_KEYS[] = {
	"left",
	"second (or middle)",
	"right",
	"pull_up",
	"pull_down",
	"pull_left",
	"pull_right",
};
#endif

/**
 * @class LinuxApplication
 */
class LinuxApplication {
 public:
	typedef NonObjectTraits Traits;

	LinuxApplication()
	: m_host(nullptr)
	, m_dpy(nullptr)
	, m_root(0)
	, m_win(0)
	, m_render_looper(nullptr)
	, m_dispatch(nullptr)
	, m_current_orientation(Orientation::ORIENTATION_INVALID)
	, m_screen(0), m_s_width(0), m_s_height(0)
	, m_x(0), m_y(0), m_width(1), m_height(1)
	, m_w_width(1), m_w_height(1)
	, m_is_init(0), m_exit(0)
	, m_xft_dpi(96.0)
	, m_xwin_scale(1.0)
	, m_main_loop(0)
	, m_ime(nullptr)
	, m_mixer(nullptr)
	, m_multitouch_device(nullptr)
	, m_element(nullptr)
	, m_is_fullscreen(0)
	{
		ASSERT(!application); application = this;
	}

	~LinuxApplication() {
		if (m_mixer) {
			snd_mixer_free(m_mixer);
			snd_mixer_detach(m_mixer, "default");
			snd_mixer_close(m_mixer);
			m_mixer = nullptr;
			m_element = nullptr;
		}
		if (m_win) {
			XDestroyWindow(m_dpy, m_win); m_win = 0;
		}
		if (m_dpy) {
			XCloseDisplay(m_dpy); m_dpy = nullptr;
		}
		if (m_ime) {
			delete m_ime; m_ime = nullptr;
		}
		application = nullptr;
		gl_draw_context = nullptr;
	}

	void post_message(cCb& cb) {
		ASSERT(m_win);
		{
			ScopeLock lock(m_queue_mutex);
			m_queue.push(cb);
		}
		// XExposeEvent event;
		// event.type = Expose;
		// event.display = m_dpy;
		// event.window = m_win;
		// event.x = 0;
		// event.y = 0;
		// event.width = 1;
		// event.height = 1;
		// event.count = 0;
		// Status status = XSendEvent(m_dpy, m_win, true, ExposureMask, (XEvent*)&event);

		XCirculateEvent event;
		event.type = CirculateNotify;
		event.display = m_dpy;
		event.window = m_win;
		event.place = PlaceOnTop;
		XSendEvent(m_dpy, m_win, false, NoEventMask, (XEvent*)&event);
	}

	/**
	 * create x11 window
	 */
	Window create_xwindow () {

		m_xset.event_mask = NoEventMask
			| KeyPressMask
			| KeyReleaseMask
			| EnterWindowMask   // EnterNotify
			| LeaveWindowMask   // LeaveNotify
			| KeymapStateMask
			| ExposureMask
			| FocusChangeMask   // FocusIn, FocusOut
		;

		if (!m_multitouch_device) {
			m_xset.event_mask |= NoEventMask
				| ButtonPressMask
				| ButtonReleaseMask
				| PointerMotionMask // MotionNotify
				| Button1MotionMask // Motion
				| Button2MotionMask // Motion
				| Button3MotionMask // Motion
				| Button4MotionMask // Motion
				| Button5MotionMask // Motion
				| ButtonMotionMask  // Motion
			;
		}

		DLOG("XCreateWindow, x:%d, y:%d, w:%d, h:%d", m_x, m_y, m_width, m_height);

		Window win = XCreateWindow(
			m_dpy, m_root,
			m_x, m_y,
			m_width, m_height, 0,
			DefaultDepth(m_dpy, 0),
			InputOutput,
			DefaultVisual(m_dpy, 0),
			CWBackPixel | CWEventMask | CWBorderPixel | CWColormap, &m_xset
		);

		// DLOG("m_xset.background_pixel 3, %d", m_xset.background_pixel);

		ASSERT(win, "Cannot create XWindow");

		if (m_multitouch_device) {
			DLOG("m_multitouch_device");

			XIEventMask eventmask;
			byte mask[3] = { 0,0,0 };

			eventmask.deviceid = XIAllMasterDevices;
			eventmask.mask_len = sizeof(mask);
			eventmask.mask = mask;

			XISetMask(mask, XI_TouchBegin);
			XISetMask(mask, XI_TouchUpdate);
			XISetMask(mask, XI_TouchEnd);

			XISelectEvents(m_dpy, win, &eventmask, 1);
		}

		XStoreName(m_dpy, win, *m_title); // set window title name
		XSetWMProtocols(m_dpy, win, &m_wm_delete_window, True);

		return win;
	}

	void run() {
		m_host = Inl_GUIApplication(app());
		m_dispatch = m_host->dispatch();
		m_render_looper = new RenderLooper(m_host);
		m_main_loop = m_host->main_loop();
		m_win = create_xwindow();
		m_ime = new LINUXIMEHelper(m_host, m_dpy, m_win);

		XMapWindow(m_dpy, m_win); // Activate window

		if (m_is_fullscreen) {
			request_fullscreen(true);
		}

		XEvent event;

		// run message main_loop
		do {
			XNextEvent(m_dpy, &event);

			if (is_exited()) break;

			resolved_queue(); // resolved message queue

			if (XFilterEvent(&event, None)) continue;

			switch (event.type) {
				case Expose:
					handle_expose(event);
					break;
				case MapNotify:
					if (m_is_init) {
						DLOG("event, MapNotify, Window onForeground");
						m_host->triggerForeground();
						m_render_looper->start();
					}
					break;
				case UnmapNotify:
					DLOG("event, UnmapNotify, Window onBackground");
					m_host->triggerBackground();
					m_render_looper->stop();
					break;
				case FocusIn:
					DLOG("event, FocusIn, Window onResume");
					m_ime->focus_in();
					m_host->triggerResume();
					break;
				case FocusOut:
					DLOG("event, FocusOut, Window onPause");
					m_ime->focus_out();
					m_host->triggerPause();
					break;
				case KeyPress:
					DLOG("event, KeyDown, keycode: %ld", event.xkey.keycode);
					m_ime->key_press(&event.xkey);
					m_dispatch->keyboard_adapter()->dispatch(event.xkey.keycode, 0, 1);
					break;
				case KeyRelease:
					DLOG("event, KeyUp, keycode: %d", event.xkey.keycode);
					m_dispatch->keyboard_adapter()->dispatch(event.xkey.keycode, 0, 0);
					break;
				case ButtonPress:
					DLOG("event, MouseDown, button: %s", MOUSE_KEYS[event.xbutton.button - 1]);
					m_dispatch->dispatch_mousepress(KeyboardKeyName(event.xbutton.button), true);
					break;
				case ButtonRelease:
					DLOG("event, MouseUp, button: %s", MOUSE_KEYS[event.xbutton.button - 1]);
					m_dispatch->dispatch_mousepress(KeyboardKeyName(event.xbutton.button), false);
					break;
				case MotionNotify: {
					Vec2 scale = m_host->display_port()->scale();
					DLOG("event, MouseMove: [%d, %d]", event.xmotion.x, event.xmotion.y);
					m_dispatch->dispatch_mousemove(event.xmotion.x / scale[0], event.xmotion.y / scale[1]);
					break;
				}
				case ClientMessage:
					if (event.xclient.message_type == m_wm_protocols && 
						(Atom)event.xclient.data.l[0] == m_wm_delete_window) {
						m_exit = 1; // exit
					}
					break;
				case GenericEvent:
					/* event is a union, so cookie == &event, but this is type safe. */
					if (XGetEventData(m_dpy, &event.xcookie)) {
						XHandleXinput2Event(&event.xcookie);
						XFreeEventData(m_dpy, &event.xcookie);
					}
					break;
				default:
					DLOG("event, %d", event.type);
					break;
			}
		} while(!m_exit);

		destroy();
	}

	void XHandleXinput2Event(XGenericEventCookie* cookie) {

		XIDeviceEvent* xev = (XIDeviceEvent*)cookie->data;
		Vec2 scale = m_host->display_port()->scale();

		List<GUITouch> touchs = {
			{
				uint(xev->detail + 20170820),
				0, 0,
				float(xev->event_x / scale.x()),
				float(xev->event_y / scale.y()),
				0,
				false,
				nullptr,
			}
		};

		switch(cookie->evtype) {
			case XI_TouchBegin:
				DLOG("event, XI_TouchBegin, deviceid: %d, sourceid: %d, detail: %d, x: %f, y: %f", 
					xev->deviceid, xev->sourceid, xev->detail, float(xev->event_x), float(xev->event_y));
				m_dispatch->dispatch_touchstart( move(touchs) );
				break;
			case XI_TouchEnd:
				DLOG("event, XI_TouchEnd, deviceid: %d, sourceid: %d, detail: %d, x: %f, y: %f", 
					xev->deviceid, xev->sourceid, xev->detail, float(xev->event_x), float(xev->event_y));
				m_dispatch->dispatch_touchend( move(touchs) );
				break;
			case XI_TouchUpdate:
				DLOG("event, XI_TouchUpdate, deviceid: %d, sourceid: %d, detail: %d, x: %f, y: %f", 
					xev->deviceid, xev->sourceid, xev->detail, float(xev->event_x), float(xev->event_y));
				m_dispatch->dispatch_touchmove( move(touchs) );
				break;
		}
	}

	void handle_expose(XEvent& event) {
		DLOG("event, Expose");
		XWindowAttributes attrs;
		XGetWindowAttributes(m_dpy, m_win, &attrs);

		m_w_width = attrs.width;
		m_w_height = attrs.height;

		typedef Callback<RunLoop::PostSyncData> Cb;
		m_host->render_loop()->post_sync(Cb([this](Cb::Data &ev) {
			if (m_is_init) {
				CGRect rect = {Vec2(), get_window_size()};
				gl_draw_context->refresh_surface_size(&rect);
				m_host->refresh_display(); // 刷新显示
			} else {
				m_is_init = 1;
				ASSERT(gl_draw_context->create_surface(m_win));
				gl_draw_context->initialize();
				m_host->triggerLoad();
				m_host->triggerForeground();
				m_render_looper->start();
			}
			ev.data->complete();
		}));
	}

	inline LINUXIMEHelper* ime() {
		return m_ime;
	}

	void initialize_display() {
		if (!m_dpy) {
			m_dpy = XOpenDisplay(nullptr);
			ASSERT(m_dpy, "Error: Can't open display");
			m_xft_dpi = get_monitor_dpi();
			m_xwin_scale = m_xft_dpi / 96.0;
		}
	}

	inline float xwin_scale() {
		initialize_display(); // init display
		return m_xwin_scale;
	}

	void initialize(cJSON& options) {
		ASSERT(XInitThreads(), "Error: Can't init X threads");

		cJSON& o_x = options["x"];
		cJSON& o_y = options["y"];
		cJSON& o_w = options["width"];
		cJSON& o_h = options["height"];
		cJSON& o_b = options["background"];
		cJSON& o_t = options["title"];
		cJSON& o_fc = options["fullScreen"];
		cJSON& o_et = options["enableTouch"];
		
		int is_enable_touch = 0;

		if (o_t.is_string()) m_title = o_t.to_string();
		if (o_fc.is_bool()) m_is_fullscreen = o_fc.to_bool();
		if (o_fc.is_int()) m_is_fullscreen = o_fc.to_int();
		if (o_et.is_bool()) is_enable_touch = o_et.to_bool();
		if (o_et.is_int()) is_enable_touch = o_et.to_int();

		initialize_display(); // init display

		m_root = XDefaultRootWindow(m_dpy);
		m_screen = DefaultScreen(m_dpy);

		m_w_width = m_width = m_s_width   = XDisplayWidth(m_dpy, m_screen);
		m_w_height = m_height = m_s_height = XDisplayHeight(m_dpy, m_screen);
		m_wm_protocols     = XInternAtom(m_dpy, "WM_PROTOCOLS"    , False);
		m_wm_delete_window = XInternAtom(m_dpy, "WM_DELETE_WINDOW", False);

		DLOG("screen width: %d, height: %d, dpi scale: %f", m_s_width, m_s_height, m_xwin_scale);
		
		m_xset.background_pixel = 0;
		m_xset.border_pixel = 0;
		m_xset.background_pixmap = None;
		m_xset.border_pixmap = None;
		m_xset.event_mask = NoEventMask;
		m_xset.do_not_propagate_mask = NoEventMask;

		// DLOG("m_xset.background_pixel 1, %d", m_xset.background_pixel);

		if (o_b.is_uint()) m_xset.background_pixel = o_b.to_uint();
		if (o_w.is_uint()) m_width = FX_MAX(1, o_w.to_uint()) * m_xwin_scale;
		if (o_h.is_uint()) m_height = FX_MAX(1, o_h.to_uint()) * m_xwin_scale;
		if (o_x.is_uint()) m_x = o_x.to_uint() * m_xwin_scale; else m_x = (m_s_width - m_width) / 2;
		if (o_y.is_uint()) m_y = o_y.to_uint() * m_xwin_scale; else m_y = (m_s_height - m_height) / 2;

		// DLOG("m_xset.background_pixel 2, %d", m_xset.background_pixel);

		if (is_enable_touch)
			initialize_multitouch();

		initialize_master_volume_control();
	}

	void initialize_multitouch() {
		ASSERT(!m_multitouch_device);

		Atom touchAtom = XInternAtom(m_dpy, "TOUCHSCREEN", true);
		if (touchAtom == None) {
			touchAtom = XInternAtom(m_dpy, XI_TOUCHSCREEN, false);
			if (touchAtom == None) return;
		}

		int inputDeviceCount = 0;
		XDeviceInfo* devices = XListInputDevices(m_dpy, &inputDeviceCount);
		XDeviceInfo* touchInfo = nullptr;

		for (int i = 0; i < inputDeviceCount; i++) {
			if (devices[i].type == touchAtom) {
				touchInfo = devices + i;
				break;
			}
		}
		if (!touchInfo) {
			return;
		}

		m_multitouch_device =  XOpenDevice(m_dpy, touchInfo->id);
		if (!m_multitouch_device)
			return;

		DLOG("X11 Touch enable active for device «%s»", touchInfo->name);

		Atom enabledAtom = XInternAtom(m_dpy, "Device Enabled", false);

		byte enabled = 1;
		XChangeDeviceProperty(m_dpy, m_multitouch_device,
			enabledAtom, XA_INTEGER, 8, PropModeReplace, &enabled, 1);
	}

	int get_master_volume() {
		long volume = 0;
		if (m_element) {
			snd_mixer_selem_get_playback_volume(m_element, SND_MIXER_SCHN_FRONT_LEFT, &volume);
		}
		return volume;
	}

	void set_master_volume(int volume) {
		volume = FX_MAX(0, volume);
		volume = FX_MIN(100, volume);

		const snd_mixer_selem_channel_id_t chs[] = {
			SND_MIXER_SCHN_FRONT_LEFT,
			SND_MIXER_SCHN_FRONT_RIGHT,
			SND_MIXER_SCHN_REAR_LEFT,
			SND_MIXER_SCHN_REAR_RIGHT,
			SND_MIXER_SCHN_FRONT_CENTER,
			SND_MIXER_SCHN_WOOFER,
			SND_MIXER_SCHN_SIDE_LEFT,
			SND_MIXER_SCHN_SIDE_RIGHT,
			SND_MIXER_SCHN_REAR_CENTER,
		};
		int len = sizeof(snd_mixer_selem_channel_id_t) / sizeof(chs);

		for (int i = 0; i < len; i++) {
			snd_mixer_selem_set_playback_volume(m_element, chs[i], volume);
		}
	}

	void request_fullscreen(bool fullscreen) {
		int mask = CWBackPixel | CWEventMask | CWBorderPixel | CWColormap;
		int x, y, width, height;
		if (fullscreen) {
			XWindowAttributes attrs;
			XGetWindowAttributes(m_dpy, m_win, &attrs);
			m_x = attrs.x;
			m_y = attrs.y;
			x = 0; y = 0;
			m_width = m_w_width;
			m_height = m_w_height;
			width = XDisplayWidth(m_dpy, m_screen);
			height = XDisplayHeight(m_dpy, m_screen);
			mask |= CWOverrideRedirect;
			m_xset.override_redirect = True;
		} else {
			x = m_x; y = m_y;
			width = m_width;
			height = m_height;
			m_xset.override_redirect = False;
		}
		XChangeWindowAttributes(m_dpy, m_win, mask, &m_xset);
		XMoveResizeWindow(m_dpy, m_win, x, y, width, height);
	}

	inline Vec2 get_window_size() {
		return Vec2(m_w_width, m_w_height);
	}

	inline Display* get_x11_display() {
		return m_dpy;
	}

	void initialize_master_volume_control() {
		ASSERT(!m_mixer);
		snd_mixer_open(&m_mixer, 0);
		snd_mixer_attach(m_mixer, "default");
		snd_mixer_selem_register(m_mixer, NULL, NULL);
		snd_mixer_load(m_mixer);
		/* 取得第一個 element，也就是 Master */
		m_element = snd_mixer_first_elem(m_mixer); DLOG("element,%p", m_element);
		/* 設定音量的範圍 0 ~ 100 */
		if (m_element) {
			snd_mixer_selem_set_playback_volume_range(m_element, 0, 100);
		}
	}

	void resolved_queue() {
		List<Cb> queue;
		{
			ScopeLock lock(m_queue_mutex);
			if (m_queue.length()) {
				queue = move(m_queue);
			}
		}
		if (queue.length()) {
			for (auto& i: queue) {
				CbD data = { 0, m_host, 0 };
				i.value()->call(data);
			}
		}
	}

	void destroy() {
		if (!is_exited()) {
			m_render_looper->stop();
			ftr::safeExit(0);
		}
		XDestroyWindow(m_dpy, m_win); m_win = 0;
		XCloseDisplay(m_dpy); m_dpy = nullptr; // disconnect x display
		DLOG("LinuxApplication Exit");
	}

	float get_monitor_dpi() {
		char* ms = XResourceManagerString(m_dpy);
		float dpi = 96.0;
		if (ms) {
			DLOG("Entire DB:\n%s", ms);
			XrmDatabase db = XrmGetStringDatabase(ms);
			XrmValue value;
			char* type = nullptr;
			if (XrmGetResource(db, "Xft.dpi", "String", &type, &value) == True) {
				if (value.addr) {
					dpi = atof(value.addr);
				}
			}
		}
		DLOG("DPI: %f", dpi);
		return dpi;
	}

 private:
	// @methods data:
	AppInl* m_host;
	Display* m_dpy;
	Window m_root, m_win;
	RenderLooper* m_render_looper;
	GUIEventDispatch* m_dispatch;
	Orientation m_current_orientation;
	int m_screen, m_s_width, m_s_height;
	int m_x, m_y;
	uint m_width, m_height;
	std::atomic_int m_w_width, m_w_height;
	bool m_is_init, m_exit;
	float m_xft_dpi, m_xwin_scale;
	XSetWindowAttributes m_xset;
	Atom m_wm_protocols, m_wm_delete_window;
	RunLoop* m_main_loop;
	String m_title;
	LINUXIMEHelper* m_ime;
	List<Cb> m_queue;
	Mutex m_queue_mutex;
	snd_mixer_t* m_mixer;
	XDevice* m_multitouch_device;
	snd_mixer_elem_t* m_element;
	bool m_is_fullscreen;
};

Vec2 __get_window_size() {
	ASSERT(application);
	return application->get_window_size();
}

Display* __get_x11_display() {
	ASSERT(application);
	return application->get_x11_display();
}

// sync to x11 main message loop
void __dispatch_x11_async(cCb& cb) {
	ASSERT(application);
	return application->post_message(cb);
}

/**
 * @func pending() 挂起应用进程
 */
void GUIApplication::pending() {
	// exit(0);
}

/**
 * @func open_url()
 */
void GUIApplication::open_url(cString& url) {
	if (vfork() == 0) {
		execlp("xdg-open", "xdg-open", *url, NULL);
	}
}

/**
 * @func send_email
 */
void GUIApplication::send_email(cString& recipient,
																cString& subject,
																cString& cc, cString& bcc, cString& body) 
{
	String arg = "mailto:";

	arg += recipient + '?';

	if (!cc.is_empty()) {
		arg += "cc=";
		arg += cc + '&';
	}
	if (!bcc.is_empty()) {
		arg += "bcc=";
		arg += bcc + '&';
	}

	arg += "subject=";
	arg += URI::encode(subject) + '&';

	if (!body.is_empty()) {
		arg += "body=";
		arg += URI::encode(body);
	}

	// xdg-open
	// mailto:haorooms@126.com?cc=name2@rapidtables.com&bcc=name3@rapidtables.com&
	// subject=The%20subject%20of%20the%20email&body=The%20body%20of%20the%20email

	if (vfork() == 0) {
		execlp("xdg-open", "xdg-open", *arg, NULL);
	}
}

/**
 * @func initialize(options)
 */
void AppInl::initialize(cJSON& options) {
	DLOG("AppInl::initialize");
	ASSERT(!gl_draw_context);
	application->initialize(options);
	gl_draw_context = GLDrawProxy::create(this, options);
	m_draw_ctx = gl_draw_context->host();
}

/**
 * @func ime_keyboard_open
 */
void AppInl::ime_keyboard_open(KeyboardOptions options) {
	application->ime()->open(options);
}

/**
 * @func ime_keyboard_can_backspace
 */
void AppInl::ime_keyboard_can_backspace(bool can_backspace, bool can_delete) {
	application->ime()->set_keyboard_can_backspace(can_backspace, can_delete);
}

/**
 * @func ime_keyboard_close
 */
void AppInl::ime_keyboard_close() {
	application->ime()->close();
}

/**
 * @func ime_keyboard_spot_location
 */
void AppInl::ime_keyboard_spot_location(Vec2 location) {
	application->ime()->set_spot_location(location);
}

/**
 * @func set_volume_up()
 */
void AppInl::set_volume_up() {
	application->set_master_volume(application->get_master_volume() + 1);
}

/**
 * @func set_volume_down()
 */
void AppInl::set_volume_down() {
	application->set_master_volume(application->get_master_volume() - 1);
}

/**
 * @func default_atom_pixel
 */
float DisplayPort::default_atom_pixel() {
	// LOG(application->xwin_scale());
	return 1.0 / application->xwin_scale();
}

/**
 * @func keep_screen(keep)
 */
void DisplayPort::keep_screen(bool keep) {
	// TODO ..
}

/**
 * @func status_bar_height()
 */
float DisplayPort::status_bar_height() {
	return 0;
}

/**
 * @func default_status_bar_height
 */
float DisplayPort::default_status_bar_height() {
	return 0;
}

/**
 * @func set_visible_status_bar(visible)
 */
void DisplayPort::set_visible_status_bar(bool visible) {
}

/**
 * @func set_status_bar_text_color(color)
 */
void DisplayPort::set_status_bar_style(StatusBarStyle style) {
}

/**
 * @func request_fullscreen(fullscreen)
 */
void DisplayPort::request_fullscreen(bool fullscreen) {
	__dispatch_x11_async(Cb([fullscreen](CbD& e) {
		application->request_fullscreen(fullscreen);
	}));
}

/**
 * @func orientation()
 */
Orientation DisplayPort::orientation() {
	return ORIENTATION_INVALID;
}

/**
 * @func set_orientation(orientation)
 */
void DisplayPort::set_orientation(Orientation orientation) {
}

FX_END

using namespace ftr;

extern "C" FX_EXPORT int main(int argc, char* argv[]) {
	Handle<LinuxApplication> h = new LinuxApplication();

	/**************************************************/
	/**************************************************/
	/************** Start GUI Application *************/
	/**************************************************/
	/**************************************************/
	AppInl::runMain(argc, argv);

	if ( app() ) {
		h->run();
	}

	return 0;
}
