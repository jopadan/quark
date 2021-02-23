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

#ifndef __ftr__view__
#define __ftr__view__

#include "../util/string.h"
#include "../event.h"

/**
 * @ns ftr
 */

namespace ftr {

	class DrawData;
	class Draw;
	class GLDraw;
	class View;
	class PreRender;
	class ViewXML;
	class Texture;
	class StyleSheetsClass;
	class StyleSheetsScope;
	class BasicScroll;
	class Background;
	class ITextInput;

	#define FX_EACH_VIEWS(F)  \
		F(LAYOUT, Layout, layout) \
		F(BOX, Box, box)  \
		F(DIV, Div, div)  \
		F(BOX_SHADOW, BoxShadow, box_shadow)  \
		F(INDEP, Indep, indep)  \
		F(LIMIT, Limit, limit) \
		F(LIMIT_INDEP, LimitIndep, limit_indep) \
		F(IMAGE, Image, image)  \
		F(SCROLL, Scroll, scroll) \
		F(VIDEO, Video, video)  \
		F(ROOT, Root, root) \
		F(SPRITE, Sprite, sprite) \
		F(HYBRID, Hybrid, hybrid) \
		F(TEXT_NODE, TextNode, text_node) \
		F(SPAN, Span, span) \
		F(TEXT_FONT, TextFont, text_font) \
		F(BASIC_SCROLL, BasicScroll, basic_scroll) \
		F(ITEXT_INPUT, ITextInput, itext_input) \
		F(TEXT_LAYOUT, TextLayout, text_layout) \
		F(LABEL, Label, label) \
		F(BUTTON, Button, button) \
		F(PANEL, Panel, panel) \
		F(TEXT, Text, text) \
		F(INPUT, Input, input) \
		F(TEXTAREA, Textarea, textarea) \


	#define FX_DEFINE_CLASS(enum, type, name) class type;
	FX_EACH_VIEWS(FX_DEFINE_CLASS);
	#undef FX_DEFINE_CLASS

	#define FX_DEFINE_GUI_VIEW(enum, type, name) \
		public: \
		friend class GLDraw; \
		virtual type* as_##name() { return this; } \
		virtual ViewType view_type() const { return enum; }

	/**
	* @class View
	*/
	class FX_EXPORT View: public Notification<GUIEvent, GUIEventName, Reference> {
		FX_HIDDEN_ALL_COPY(View);
		public:
		
		View();
		
		/**
		* @destructor
		*/
		virtual ~View();
		
		/**
		* @enum ViewType
		*/
		enum ViewType {
			#define nx_def_view_type(enum, type, name) enum,
			INVALID = 0,
			VIEW,
			FX_EACH_VIEWS(nx_def_view_type)
			#undef nx_def_view_type
		};

		/**
		* @func view_type 获取视图类型
		*/
		virtual ViewType view_type() const { return VIEW; }
		
	#define nx_def_view_type(enum, type, name)  \
		virtual type* as_##name() { return nullptr; }
		FX_EACH_VIEWS(nx_def_view_type) // as type
	#undef nx_def_view_type

		/**
		* @enum MarkValue
		*/
		enum : uint32_t {
			M_NONE                  = 0,          /* 没有任何标记 */
			M_BASIC_MATRIX          = (1 << 0),   /* 基础矩阵变化, */
			M_TRANSFORM             = (1 << 1),   /* 矩阵变换（需要更新变换矩阵）这个标记具有继承性质 */
			M_MATRIX                = (M_BASIC_MATRIX | M_TRANSFORM),
			M_SHAPE                 = (1 << 2),   /* 形状变化,外形尺寸、原点位置都属于形状变化 */
			M_OPACITY               = (1 << 3),   /* 透明度 */
			M_VISIBLE               = (1 << 4),   /* 显示与隐藏 */
			M_INHERIT               = (M_TRANSFORM | M_OPACITY), /* 具有继承性质的变换标记 */
			M_LAYOUT                = (1 << 5),   // 第一次从外到内,初始盒子尺寸,如果有明确尺寸将不会触发三次布局
			M_SIZE_HORIZONTAL       = (1 << 6),   // 设置水平尺寸
			M_SIZE_VERTICAL         = (1 << 7),   // 设置垂直尺寸
			M_CONTENT_OFFSET        = (1 << 8),   // 第二次从内到外,设置内容偏移并挤压无明确尺寸的父盒子
			M_LAYOUT_THREE_TIMES    = (1 << 9),   // 第三次从外到内,设置无明确尺寸盒子,非所有无明确尺寸盒子都有三次
			M_BORDER                = (1 << 10),  // 边框
			M_BORDER_RADIUS         = (1 << 11),  // 边框圆角变化
			M_BACKGROUND_COLOR      = (1 << 12),  // 背景颜色变化
			M_TEXTURE               = (1 << 13),  // 纹理相关
			M_BOX_SHADOW            = (1 << 14),  // 阴影变化
			M_SCROLL                = (1 << 15),  //
			M_SCROLL_BAR            = (1 << 16),  //
			M_TEXT_SIZE             = (1 << 17),  // 文本尺寸变化
			M_TEXT_FONT             = (1 << 18),  //
			M_INPUT_STATUS          = (1 << 19),  // 文本输入状态改变
			M_CLIP                  = (1 << 20),  // 溢出内容修剪
			M_BACKGROUND            = (1 << 21),  // 背景变化
			M_STYLE_CLASS           = (1 << 30),  /* 变化class引起的样式变化 */
			M_STYLE_FULL            = (uint32_t(1) << 31),  /* 所有后后代视图都受到影响 */
			M_STYLE                 = (M_STYLE_CLASS | M_STYLE_FULL),
		};
		
		/**
		* @get inner_text {Ucs2String}
		*/
		String16 inner_text() const;
		
		/**
		* @func prepend # 前置元素
		* @arg child {View*} # 要前置的元素
		*/
		virtual void prepend(View* child) throw(Error);
		
		/**
		* @func append # 追加视图至结尾
		* @arg child {View*} # 要追加的元素
		*/
		virtual void append(View* child) throw(Error);

		/**
		* @func append_text # 追加文本到结尾
		*/
		virtual View* append_text(cString16& str) throw(Error);

		/**
		* @func before # 插入前
		* @arg view {View*} # 要插入的元素
		*/
		void before(View* view) throw(Error);

		/**
		* @func after # 插入后
		* @arg view {View*} # 要插入的元素
		*/
		void after(View* view) throw(Error);

		/**
		* @func remove # 删除当前视图,从内存清除
		*/
		virtual void remove();
		
		/**
		* @func remove_all_child # 删除所有子视图
		*/
		virtual void remove_all_child();
		
		/**
		* @func parent # 父级视图
		*/
		inline View* parent() { return _parent; }
		
		/**
		* @func prev # 上一个兄弟视图
		*/
		inline View* prev() { return _prev; }
		
		/**
		* @func next # 下一个兄视图
		*/
		inline View* next() { return _next; }
		
		/**
		* @func first # 第一个子视图
		*/
		inline View* first() { return _first; }
		
		/**
		* @func last # 最后一个子视图
		*/
		inline View* last() { return _last; }
		
		/**
		* @func action
		*/
		inline Action* action() { return _action; }
		
		/**
		* @func action
		*/
		void action(Action* action) throw(Error);
		
		/**
		* @func x # x轴位移
		*/
		inline float x() const { return _translate.x(); }
		
		/**
		* @func y # y轴位移
		*/
		inline float y() const { return _translate.y(); }
		
		/**
		* @func scale_x # x轴缩放
		*/
		inline float scale_x() const { return _scale.x(); }

		/**
		* @func scale_y # y轴缩放
		*/
		inline float scale_y() const { return _scale.y(); }

		/**
		* @func rotate_z # 旋转角度
		*/
		inline float rotate_z() const { return _rotate_z; }

		/**
		* @func skew_x # x轴斜歪角度
		*/
		inline float skew_x() const { return _skew.x(); }

		/**
		* @func skew_y # y轴斜歪角度
		*/
		inline float skew_y() const { return _skew.y(); }

		/**
		* @func opacity  # 可影响子视图的透明度值
		*                  Can affect the transparency value of sub view
		*/
		inline float opacity() const { return _opacity; }

		/**
		* @func visible  # 是否显示视图,包括子视图
		*                  Whether to display the view, including the sub view
		*/
		inline bool visible() const { return _visible; }
		
		/**
		* @func final_visible {bool} # 最终是否显示会受父视图的影响
		*/
		inline bool final_visible() const { return _final_visible; }
		
		/**
		* @func draw_visible
		*/
		inline bool draw_visible() const { return _draw_visible; }
		
		/**
		* @func translate
		*/
		inline Vec2 translate() const { return _translate; }

		/**
		* @func scale
		*/
		inline Vec2 scale() const { return _scale; }
		
		/**
		* @func skew
		*/
		inline Vec2 skew() const { return _skew; }

		/**
		* @func origin_x # x轴原点,以该点 位移,缩放,旋转,歪斜
		*                  To the view of displacement, rotate, scale, skew
		*/
		inline float origin_x() const { return _origin.x(); }

		/**
		* @func origin_y # y轴原点,以该点 位移,缩放,旋转,歪斜
		*                  To the view of displacement, rotate, scale, skew
		*/
		inline float origin_y() const { return _origin.y(); }

		/**
		* @func origin # 原点,以该点 位移,缩放,旋转,歪斜
		*                To the view of displacement, rotate, scale, skew
		*/
		inline Vec2 origin() const { return _origin; }
		
		/**
		* @func set_x
		*/
		void set_x(float value);
		
		/**
		* @func set_y
		*/
		void set_y(float value);
		
		/**
		* @func set_scale_x
		*/
		void set_scale_x(float value);
		
		/**
		* @func set_scale_y
		*/
		void set_scale_y(float value);
		
		/**
		* @func set_rotate_z
		*/
		void set_rotate_z(float value);
		
		/**
		* @func set_skew_x
		*/
		void set_skew_x(float value);
		
		/**
		* @func set_skew_y
		*/
		void set_skew_y(float value);
		
		/**
		* @func set_opacity
		*/
		void set_opacity(float value);
		
		/**
		* @func set_visible_1
		*/
		inline void set_visible_1(bool value) { set_visible(value); }
		
		/**
		* @func set_visible
		*/
		virtual void set_visible(bool value);
		
		/**
		* @func set_translate
		*/
		void set_translate(Vec2 value);
		
		/**
		* @func set_scale
		*/
		void set_scale(Vec2 value);
		
		/**
		* @func set_skew
		*/
		void set_skew(Vec2 skew);
		
		/**
		* @func set_origin_x
		*/
		void set_origin_x(float value);
		
		/**
		* @func set_origin_y
		*/
		void set_origin_y(float value);
		
		/**
		* @func set_origin
		*/
		void set_origin(Vec2 value);
			
		/**
		* @func need_draw
		*/
		inline bool need_draw() const { return _need_draw; }
		
		/**
		* @func set_need_draw
		*/
		void set_need_draw(bool value);

		/**
		* @func level # 视图在整个视图树中所处的层级
		*               0表示还没有加入到GUIApplication唯一的视图树中,根视图为1
		*/
		inline uint32_t level() const { return _level; }
		
		/**
		* @func transform # 设置视图变换
		* @arg translate {Vec2}
		* @arg scale {Vec2}
		* @arg [rotate_z = 0] {float}
		* @arg [skew = Vec2()] {Vec2}
		*/
		void transform(Vec2 translate, Vec2 scale, float rotate_z = 0, Vec2 skew = Vec2());

		/**
		* @func overlap_test 重叠测试,测试屏幕上的点是否与视图重叠
		*/
		virtual bool overlap_test(Vec2 point);
		
		/**
		* @func overlap_test_from_convex_quadrilateral
		*/
		static bool overlap_test_from_convex_quadrilateral(Vec2 quadrilateral_vertex[4], Vec2 point);
		
		/**
		* @func screen_rect_from_convex_quadrilateral
		*/
		static CGRect screen_rect_from_convex_quadrilateral(Vec2 quadrilateral_vertex[4]);
		
		/**
		* @func screen_region_from_convex_quadrilateral
		*/
		static Region screen_region_from_convex_quadrilateral(Vec2 quadrilateral_vertex[4]);
		
		/**
		* @func layout_offset #  获取布局偏移值
		*/
		virtual Vec2 layout_offset();
		
		/**
		* @func layout_in_offset 获取布局内部偏移值
		*/
		virtual Vec2 layout_in_offset();
		
		/**
		* @func layout_offset_from
		*/
		Vec2 layout_offset_from(View* parents);
		
		/**
		* @func screen_rect
		*/
		virtual CGRect screen_rect();
		
		/**
		* @func matrix 基础矩阵,通过计算从父视图矩阵开始的位移,缩放,旋转,歪斜得到的矩阵。
		*/
		const Mat& matrix();
		
		/**
		* @func final_matrix
		*/
		const Mat& final_matrix();
		
		/**
		* @func final_opacity
		*/
		float final_opacity();
		
		/**
		* @func position
		*/
		Vec2 position();
		
		/**
		* @func classs
		*/
		inline StyleSheetsClass* classs() { return _classs; }
		
		/**
		* "cls1 clas2 clas3"
		* @func set_class
		*/
		void set_class(cString& names);
		
		/**
		* @func set_class
		*/
		void set_class(const Array<String>& names);
		
		/**
		* @func add_class
		*/
		void add_class(cString& names);
		
		/**
		* @func remove_class
		*/
		void remove_class(cString& names);
		
		/**
		* @func toggle_class
		*/
		void toggle_class(cString& names);
		
		/**
		* @func receive()
		*/
		inline bool receive() const { return _receive; }
		
		/**
		* @func receive()
		*/
		inline void set_receive(bool value) { _receive = value; }
		
		/**
		* @func focus();
		*/
		bool focus();
		
		/**
		* @func blur();
		*/
		bool blur();
		
		/**
		* @func is_focus();
		*/
		bool is_focus() const;
		
		/**
		* @func set_is_focus()
		*/
		void set_is_focus(bool value);
		
		/**
		* @func can_become_focus() 是否可以成为焦点
		*/
		virtual bool can_become_focus();
		
		/**
		* @func trigger_listener_change()
		*/
		virtual void trigger_listener_change(const NameType& name, int count, int change);
		
		/**
		* @func first_button
		*/
		Button* first_button();
		
		/**
		* @func has_child(child)
		*/
		bool has_child(View* child);
		
	protected:
		
		/**
		* @func trigger()
		*/
		ReturnValue& trigger(const NameType& name, GUIEvent& evt, bool need_send = false);
		ReturnValue trigger(const NameType& name, bool need_send = false);
		
		/**
		* @func refresh_styles
		*/
		void refresh_styles(StyleSheetsScope* sss);
		
		/**
		* @func mark 标记该视图已经发生改变
		*/
		void mark(uint32_t value);
		
		/**
		* @func mark_pre 标记该视图已经发生改变并加入绘制前预处理
		*/
		void mark_pre(uint32_t value);
		
		/**
		* @func visit child draw
		*/
		inline void visit(Draw* draw) {
			visit(draw, mark_value & M_INHERIT, _need_draw);
		}
		
		/**
		* @func visit child draw
		*/
		void visit(Draw* draw, uint32_t inherit_mark, bool need_draw = false);
		
		/**
		* @func solve transform
		*/
		void solve();
		
		/**
		* @func set_draw_visible
		*/
		virtual void set_draw_visible();
		
		/**
		* @func accept_text
		*/
		virtual void accept_text(Array<String16>& out) const;
		
		/**
		* @func set_parent 设置父视图
		*/
		virtual void set_parent(View* parent) throw(Error);
		
		/**
		* @func draw 绘制视图,通过这个函数向GUP发送绘图命令
		*/
		virtual void draw(Draw* draw);
		
	private:

		View*   _parent;     /* 父视图 */
		View*   _prev;       /* 上一个兄弟视图,通过这些属性组成了一个双向链表 */
		View*   _next;       /* 下一个兄视图 */
		View*   _first;      /* 第一个子视图 */
		View*   _last;       /* 最后一个子视图 */
		Vec2    _translate;  /* 平移向量 */
		Vec2    _scale;      /* 缩放向量 */
		Vec2    _skew;       /* 倾斜向量 */
		float   _rotate_z;   /* z轴旋转角度值 */
		float   _opacity;    /* 可影响子视图的透明度值 */
		/*
		*  不可能同时所有视图都会发生改变,如果视图树很庞大的时候,
		*  如果涉及到布局时为了跟踪其中一个视图的变化就需要遍历整颗视图树,为了避免这种情况
		*  把标记的视图独立到视图外部按视图等级进行分类以双向环形链表形式存储(PreRender)
		*  这样可以避免访问那些没有发生改变的视图并可以根据视图等级顺序访问.
		*/
		View* _prev_pre_mark;      /* 上一个标记的预处理标记视图 */
		View* _next_pre_mark;      /* 下一个标记的预处理标记视图 */
		
		StyleSheetsClass* _classs;
		
	protected:
		
		uint32_t _level;         /* 视图在整个视图树中所处的层级
													* 0表示还没有加入到GUIApplication唯一的视图树中,根视图为1 */
		Mat  _matrix;        /* 基础矩阵,从父视图矩阵开始的矩阵,通过translate/scale/skew/rotate得到 */
		Vec2 _origin;        /* 以该点 位移,缩放,旋转,歪斜 */
		Mat  _final_matrix;  /* 父视图矩阵乘以基础矩阵等于最终变换矩阵 */
		float _final_opacity;/* 最终的不透明值 */
		uint32_t mark_value;      /* 这些标记后的视图会在开始帧绘制前进行更新.
													* 需要这些标记的原因主要是为了最大程度的节省性能开销,
													* 因为程序在运行过程中可能会频繁的更新视图局部属性也可能视图很少发生改变.
													*
													*  1.如果对每次更新如果都更新GPU中的数据那么对性能消耗那将是场灾难,
													*  那么记录视图所有的局部变化,待到到需要真正帧渲染时统一进行更新.
													* */
		bool _visible;       /* 是否显示视图,包括子视图 */
		bool _final_visible; /* 最终是否显示,受父视图_visible影响 */
		bool _draw_visible;  /* 该状态标识视图是否在屏幕或区域范围内(是否需要绘图),这个状态会忽略`_visible`值 */
		bool _need_draw;             /* 忽略视图visible值,强制绘制子视图 */
		bool _child_change_flag;     /* 子视图有变化标记,调用draw后重置 */
		bool _receive;               /* 是否接收事件 */
		DrawData*       _ctx_data;   /* 绘图上下文需要的数据 */
		Action*         _action;
		
		FX_DEFINE_INLINE_CLASS(Inl);
		FX_DEFINE_INLINE_CLASS(EventInl);
		FX_DEFINE_INLINE_CLASS(ActionInl);
		
		friend class TextLayout;
		friend class TextFont;
		friend class Layout;
		friend class PreRender;
		friend class GLDraw;
		friend class GUIEventDispatch;
		friend class DisplayPort;
		friend class TextNode;
		friend class Label;
		friend class Span;
		friend class StyleSheetsClass;
		friend class BasicScroll;
		friend class Background;
	};

}
#endif
