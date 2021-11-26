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

#include "flare/util/loop.h"

namespace flare {

	/**
	* @class WorkLoop
	*/
	class WorkLoop {
	 public:
		inline WorkLoop(): _loop(nullptr) {}

		inline bool has_current_thread() {
			return Thread::current_id() == _thread_id;
		}

		bool is_continue(Thread& t) {
			ScopeLock scope(_mutex);

			if (!t.is_abort()) {
				/* 趁着循环运行结束到上面这句lock片刻时间拿到队列对像的线程,这里是最后的200毫秒,
				* 200毫秒后没有向队列发送新消息结束线程
				* * *
				* 这里休眠200毫秒给外部线程足够时间往队列发送消息
				*/
				// Thread::sleep(2e5);
				if ( m_loop->is_alive() && !t.is_abort() ) {
					return true; // 继续运行
				}
			}
			_loop = nullptr;
			_id = ThreadID();
			return false;
		}
		
		RunLoop* loop() {
			Lock lock(_mutex);

			if (is_exited()) {
				return nullptr;
			}
			if (_loop) {
				return _loop;
			}
			
			_id = Thread::fork([this](Thread& t) {
				_mutex.lock();
				_loop = RunLoop::current();
				_cond.notify_all();
				_mutex.unlock();
				do {
					_loop->run(2e7); // 20秒后没有新消息结束线程
				} while(is_continue(t));
				return 0;
			}, "private_loop");

			_cond.wait(lock); // wait

			return _loop;
		}

	 private:
		ThreadID _id;
		RunLoop* _loop;
		Mutex _mutex;
		Condition _cond;
	};

	static PrivateLoop* _work_loop = new PrivateLoop();

	RunLoop* work_loop() {
		return _work_loop->loop();
	}

	bool has_work_loop_thread() {
		return _work_loop->has_current_thread();
	}

}