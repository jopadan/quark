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

#ifndef __ftr__value__
#define __ftr__value__

#include "ftr/util/util.h"
#include "ftr/util/array.h"
#include "ftr/mathe.h"

/**
 * @ns ftr
 */

FX_NS(ftr)

class FontFamilysID;

namespace value {
	
#define FX_ENUM_VALUE(F) \
	F(AUTO,               auto) \
	F(FULL,               full) \
	F(PIXEL,              pixel) \
	F(PERCENT,            percent) \
	F(MINUS,              minus) \
	F(INHERIT,            inherit) \
	F(VALUE,              value) \
	F(THIN,               thin)              /*100*/ \
	F(ULTRALIGHT,         ultralight)        /*200*/ \
	F(LIGHT,              light)             /*300*/ \
	F(REGULAR,            regular)           /*400*/ \
	F(MEDIUM,             medium)            /*500*/ \
	F(SEMIBOLD,           semibold)          /*600*/ \
	F(BOLD,               bold)              /*700*/ \
	F(HEAVY,              heavy)             /*800*/ \
	F(BLACK,              black)             /*900*/ \
	F(THIN_ITALIC,        thin_italic)       /*100*/ \
	F(ULTRALIGHT_ITALIC,  ultralight_italic) /*200*/ \
	F(LIGHT_ITALIC,       light_italic)      /*300*/ \
	F(ITALIC,             italic)            /*400*/ \
	F(MEDIUM_ITALIC,      medium_italic)     /*500*/ \
	F(SEMIBOLD_ITALIC,    semibold_italic)   /*600*/ \
	F(BOLD_ITALIC,        bold_italic)       /*700*/ \
	F(HEAVY_ITALIC,       heavy_italic)      /*800*/ \
	F(BLACK_ITALIC,       black_italic)      /*900*/ \
	F(OTHER,              other) \
	F(NONE,               none) \
	F(OVERLINE,           overline) \
	F(LINE_THROUGH,       line_through) \
	F(UNDERLINE,          underline) \
	F(LEFT,               left) \
	F(CENTER,             center) \
	F(RIGHT,              right) \
	F(LEFT_REVERSE,       left_reverse) \
	F(CENTER_REVERSE,     center_reverse) \
	F(RIGHT_REVERSE,      right_reverse) \
	F(TOP,                top) \
	F(BOTTOM,             bottom) \
	F(MIDDLE,             middle) \
	F(REPEAT,             repeat) \
	F(REPEAT_X,           repeat_x) \
	F(REPEAT_Y,           repeat_y) \
	F(MIRRORED_REPEAT,    mirrored_repeat) \
	F(MIRRORED_REPEAT_X,  mirrored_repeat_x) \
	F(MIRRORED_REPEAT_Y,  mirrored_repeat_y) \
	F(NORMAL,             normal) \
	F(CLIP,               clip) \
	F(ELLIPSIS,           ellipsis) \
	F(CENTER_ELLIPSIS,    center_ellipsis) \
	F(NO_WRAP,            no_wrap) \
	F(NO_SPACE,           no_space) \
	F(PRE,                pre) \
	F(PRE_LINE,           pre_line) \
	F(WRAP,               wrap) \
	/* keyboard type */ \
	F(ASCII,              ascii) \
	F(NUMBER,             number) \
	F(URL,                url) \
	F(NUMBER_PAD,         number_pad) \
	F(PHONE,              phone) \
	F(NAME_PHONE,         name_phone) \
	F(EMAIL,              email) \
	F(DECIMAL,            decimal) \
	F(TWITTER,            twitter) \
	F(SEARCH,             search) \
	F(ASCII_NUMBER,       ascii_numner) \
	/* keyboard return type */ \
	F(GO,                 go) \
	F(JOIN,               join) \
	F(NEXT,               next) \
	F(ROUTE,              route) \
	F(SEND,               send) \
	F(DONE,               done) \
	F(EMERGENCY,          emergency) \
	F(CONTINUE,           continue) \

	/**
	 * @enum KeyboardType
	 */
#define FX_KEYBOARD_TYPE(F) \
	F(KeyboardType, NORMAL) \
	F(KeyboardType, ASCII) \
	F(KeyboardType, NUMBER) \
	F(KeyboardType, URL) \
	F(KeyboardType, NUMBER_PAD) \
	F(KeyboardType, PHONE) \
	F(KeyboardType, NAME_PHONE) \
	F(KeyboardType, EMAIL) \
	F(KeyboardType, DECIMAL) \
	F(KeyboardType, TWITTER) \
	F(KeyboardType, SEARCH) \
	F(KeyboardType, ASCII_NUMBER) \
	
	/**
	 * @enum KeyboardReturnType
	 */
#define FX_KEYBOARD_RETURN_TYPE(F) \
	F(KeyboardReturnType, NORMAL) \
	F(KeyboardReturnType, GO) \
	F(KeyboardReturnType, JOIN) \
	F(KeyboardReturnType, NEXT) \
	F(KeyboardReturnType, ROUTE) \
	F(KeyboardReturnType, SEARCH) \
	F(KeyboardReturnType, SEND) \
	F(KeyboardReturnType, DONE) \
	F(KeyboardReturnType, EMERGENCY) \
	F(KeyboardReturnType, CONTINUE) \
	
	/**
	 * @enum Direction
	 */
#define FX_DIRECTION(F) \
	F(Direction, NONE) \
	F(Direction, LEFT) \
	F(Direction, RIGHT) \
	F(Direction, TOP) \
	F(Direction, BOTTOM) \

	/**
	 * @enum ValueType
	 */
#define FX_VALUE_TYPE(F) \
 F(ValueType, AUTO)    /* 自动值  auto */ \
 F(ValueType, FULL)    /* 吸附到父视图(client边距与父视图重叠) full */ \
 F(ValueType, PIXEL)   /* 像素值  px */ \
 F(ValueType, PERCENT) /* 百分比  % */ \
 F(ValueType, MINUS)   /* 减法(parent-value) ! */ \

	/**
	 * @enum BackgroundPositionType
	 */
#define FX_BACKGROUND_POSITION_TYPE(F) \
	F(ValueType, PIXEL)     /* 像素值  px */ \
	F(ValueType, PERCENT)   /* 百分比  % */ \
	F(ValueType, LEFT)      /* 居左 */ \
	F(ValueType, RIGHT)     /* 居右  % */ \
	F(ValueType, CENTER)    /* 居中 */ \
	F(ValueType, TOP)       /* 居上 */ \
	F(ValueType, BOTTOM)    /* 居下 */ \

	/**
	 * @enum BackgroundSizeType
	 */
#define FX_BACKGROUND_SIZE_TYPE(F) \
	F(ValueType, AUTO)      /* 自动值  auto */ \
	F(ValueType, PIXEL)     /* 像素值  px */ \
	F(ValueType, PERCENT)   /* 百分比  % */ \

	/**
	 * @enum TextValueType
	 */
#define FX_TEXT_VALUE_TYPE(F) \
	F(TextValueType, INHERIT) \
	F(TextValueType, VALUE) \

	/**
	 * @enum TextStyleEnum
	 */
#define FX_TEXT_STYLE_ENUM(F) \
	F(TextStyleEnum, THIN) \
	F(TextStyleEnum, ULTRALIGHT) \
	F(TextStyleEnum, LIGHT) \
	F(TextStyleEnum, REGULAR) \
	F(TextStyleEnum, MEDIUM) \
	F(TextStyleEnum, SEMIBOLD) \
	F(TextStyleEnum, BOLD) \
	F(TextStyleEnum, HEAVY) \
	F(TextStyleEnum, BLACK) \
	F(TextStyleEnum, THIN_ITALIC) \
	F(TextStyleEnum, ULTRALIGHT_ITALIC) \
	F(TextStyleEnum, LIGHT_ITALIC) \
	F(TextStyleEnum, ITALIC) \
	F(TextStyleEnum, MEDIUM_ITALIC) \
	F(TextStyleEnum, SEMIBOLD_ITALIC) \
	F(TextStyleEnum, BOLD_ITALIC) \
	F(TextStyleEnum, HEAVY_ITALIC) \
	F(TextStyleEnum, BLACK_ITALIC) \
	F(TextStyleEnum, OTHER) \
	
	/**
	 * @enum TextDecorationEnum
	 */
#define FX_TEXT_DECORATION_ENUM(F) \
	F(TextDecoration, NONE)           /* 没有 */ \
	F(TextDecoration, OVERLINE)       /* 上划线 */ \
	F(TextDecoration, LINE_THROUGH)   /* 中划线 */ \
	F(TextDecoration, UNDERLINE)      /* 下划线 */ \
	
#define FX_TEXT_OVERFLOW_ENUM(F) \
	F(TextOverflowEnum, NORMAL)          /* 不做任何处理 */ \
	F(TextOverflowEnum, CLIP)            /* 剪切 */ \
	F(TextOverflowEnum, ELLIPSIS)        /* 剪切并显示省略号 */ \
	F(TextOverflowEnum, CENTER_ELLIPSIS) /* 剪切并居中显示省略号 */ \
	
#define FX_TEXT_WHITE_SPACE_ENUM(F) \
	F(TextWhiteSpaceEnum, NORMAL)           /* 保留所有空白,使用自动wrap */ \
	F(TextWhiteSpaceEnum, NO_WRAP)          /* 合并空白序列,不使用自动wrap */ \
	F(TextWhiteSpaceEnum, NO_SPACE)         /* 合并空白序列,使用自动wrap */ \
	F(TextWhiteSpaceEnum, PRE)              /* 保留所有空白,不使用自动wrap */ \
	F(TextWhiteSpaceEnum, PRE_LINE)         /* 合并空白符序列,但保留换行符,使用自动wrap */ \
	F(TextWhiteSpaceEnum, WRAP)             /* 保留所有空白,强制使用自动wrap */ \
	
	/**
	 * @enum TextAlign 文本对齐方式
	 */
#define FX_TEXT_ALIGN(F) \
	F(TextAlign, LEFT)           /* 左对齐 */ \
	F(TextAlign, CENTER)         /* 居中 */ \
	F(TextAlign, RIGHT)          /* 右对齐 */ \
	F(TextAlign, LEFT_REVERSE)   /* 左对齐并反向 */ \
	F(TextAlign, CENTER_REVERSE) /* 居中对齐并反向 */ \
	F(TextAlign, RIGHT_REVERSE)  /* 右对齐并反向 */ \
	
	/**
	 * @enum Align 对齐方式
	 */
#define FX_ALIGN(F) \
	F(Align, NONE) \
	F(Align, LEFT) \
	F(Align, RIGHT) \
	F(Align, CENTER) \
	F(Align, TOP) \
	F(Align, BOTTOM) \
	
	/**
	 * @enum ContentAlign div 内容对齐方式
	 */
#define FX_CONTENT_ALIGN(F) \
	F(ContentAlign, LEFT)    /* 水平左对齐 */ \
	F(ContentAlign, RIGHT)   /* 水平右对齐 */ \
	F(ContentAlign, TOP)     /* 垂直上对齐 */ \
	F(ContentAlign, BOTTOM)  /* 垂直下对齐 */ \
	
	/**
	 * @enum Repeat 纹理重复方式
	 */
#define FX_REPEAT(F) \
	F(Repeat, NONE) \
	F(Repeat, REPEAT) \
	F(Repeat, REPEAT_X) \
	F(Repeat, REPEAT_Y) \
	F(Repeat, MIRRORED_REPEAT) \
	F(Repeat, MIRRORED_REPEAT_X) \
	F(Repeat, MIRRORED_REPEAT_Y) \
	
	#define DEF_ENUM_VALUE(NAME, NAME2) NAME,
	#define DEF_ENUM(Type, NAME) NAME = Enum::NAME,
	
	enum Enum {
		FX_ENUM_VALUE(DEF_ENUM_VALUE)
	};
	
	enum class KeyboardType: byte {
		FX_KEYBOARD_TYPE(DEF_ENUM)
	};

	enum class KeyboardReturnType: byte {
		FX_KEYBOARD_RETURN_TYPE(DEF_ENUM)
	};

	enum class Direction: byte {
		FX_DIRECTION(DEF_ENUM)
	};

	enum class ValueType: byte {
		FX_VALUE_TYPE(DEF_ENUM)
	};

	enum class BackgroundPositionType: byte {
		FX_BACKGROUND_POSITION_TYPE(DEF_ENUM)
	};
	
	enum class BackgroundSizeType: byte {
		FX_BACKGROUND_SIZE_TYPE(DEF_ENUM)
	};
	
	enum class TextValueType: byte {
		FX_TEXT_VALUE_TYPE(DEF_ENUM)
	};

	enum class TextStyleEnum: byte {
		FX_TEXT_STYLE_ENUM(DEF_ENUM)
	};

	enum class TextDecorationEnum: byte {
		FX_TEXT_DECORATION_ENUM(DEF_ENUM)
	};
	
	enum class TextOverflowEnum: byte {
		FX_TEXT_OVERFLOW_ENUM(DEF_ENUM)
	};
	
	enum class TextWhiteSpaceEnum: byte {
		FX_TEXT_WHITE_SPACE_ENUM(DEF_ENUM)
	};

	enum class TextAlign: byte {
		FX_TEXT_ALIGN(DEF_ENUM)
	};

	enum class Align: byte {
		FX_ALIGN(DEF_ENUM)
	};

	enum class ContentAlign: byte {
		FX_CONTENT_ALIGN(DEF_ENUM)
	};
	
	enum class Repeat: byte {
		FX_REPEAT(DEF_ENUM)
	};

	#undef DEF_ENUM_VALUE
	#undef DEF_ENUM
	
	struct Rect {
		Vec2 origin;
		Vec2  size;
	};
	
	struct Region {
		float x, y;
		float x2, y2;
		float w, h;
	};
	
	/**
	 * @struct Border
	 */
	struct FX_EXPORT Border {
		float width;
		Color color;
		inline Border(float w = 0,
									Color c = Color())
		: width(w), color(c) {}
	};
	
	/**
	 * @struct Shadow
	 */
	struct FX_EXPORT Shadow {
		float   offset_x;
		float   offset_y;
		float   size;
		Color   color;
		bool operator==(const Shadow&) const;
		inline bool operator!=(const Shadow& value) const { return ! operator==(value); }
	};
	
	// Compound value
	
	/**
	 * @struct ValueTemplate
	 */
	template<typename Type, Type TypeInit, typename Value = float>
	struct FX_EXPORT ValueTemplate {
		Type type;
		Value value;
		inline bool operator==(const ValueTemplate& val) const {
			return val.type == type && val.value == value;
		}
		inline bool operator!=(const ValueTemplate& value) const {
			return !operator==(value);
		}
		inline ValueTemplate(Type t = TypeInit, Value v = 0): type(t), value(v) {}
		inline ValueTemplate(Value v) : type(Type::PIXEL), value(v) {}
	};
	
	typedef ValueTemplate<ValueType, ValueType::AUTO> Value;
	typedef ValueTemplate<BackgroundPositionType, BackgroundPositionType::PIXEL> BackgroundPosition;
	typedef ValueTemplate<BackgroundSizeType, BackgroundSizeType::AUTO> BackgroundSize;
	
	struct BackgroundPositionCollection {
		BackgroundPosition x, y;
	};
	struct BackgroundSizeCollection {
		BackgroundSize x, y;
	};
	
	/**
	 * @struct TextColor
	 */
	struct FX_EXPORT TextColor {
		TextValueType type;
		Color value;
	};
	
	/**
	 * @struct TextSize
	 */
	struct FX_EXPORT TextSize {
		TextValueType type;
		float value;
	};
	
	/**
	 * @struct TextFamily
	 */
	struct FX_EXPORT TextFamily {
		TextFamily(TextValueType type = TextValueType::INHERIT);
		TextFamily(TextValueType type, const FontFamilysID* cffid);
		TextValueType type;
		const FontFamilysID* value;
		cString& name() const;
		const Array<String>& names() const;
	};
	
	/**
	 * @struct TextStyle
	 */
	struct FX_EXPORT TextStyle {
		TextValueType type;
		TextStyleEnum value;
	};
	
	/**
	 * @struct TextShadow
	 */
	struct FX_EXPORT TextShadow {
		TextValueType type;
		Shadow  value;
	};
	
	/**
	 * @struct TextLineHeightValue
	 */
	struct FX_EXPORT TextLineHeightValue {
		float height;
		inline void set_auto() { height = 0; }
		inline bool is_auto() const { return height <= 0; }
		inline bool operator!=(const TextLineHeightValue& value) const {
			return height != value.height;
		}
		inline bool operator==(const TextLineHeightValue& value) const {
			return height == value.height;
		}
	};
	
	/**
	 * @struct TextLineHeight
	 */
	struct FX_EXPORT TextLineHeight {
		TextValueType  type;
		TextLineHeightValue value;
	};
	
	/**
	 * @struct TextDecoration
	 */
	struct FX_EXPORT TextDecoration {
		TextValueType  type;
		TextDecorationEnum  value;
	};
	
	/**
	 * @struct TextOverflow
	 */
	struct FX_EXPORT TextOverflow {
		TextValueType  type;
		TextOverflowEnum  value;
	};
	
	/**
	 * @struct TextWhiteSpace
	 */
	struct FX_EXPORT TextWhiteSpace {
		TextValueType  type;
		TextWhiteSpaceEnum  value;
	};
	
}

using value::Direction;
using value::ValueType;
using value::BackgroundPositionType;
using value::BackgroundSizeType;
using value::TextValueType;
using value::TextDecorationEnum;
using value::TextOverflowEnum;
using value::TextWhiteSpaceEnum;
using value::TextAlign;
using value::Align;
using value::ContentAlign;
using value::Repeat;
using value::Value;
using value::BackgroundPosition;
using value::BackgroundSize;
using value::Border;
using value::TextLineHeightValue;
using value::TextStyleEnum;
using value::Shadow;
typedef value::Rect CGRect;
using value::Region;
using value::KeyboardType;
using value::KeyboardReturnType;
using value::TextColor;
using value::TextSize;
using value::TextStyle;
using value::TextFamily;
using value::TextLineHeight;
using value::TextShadow;
using value::TextDecoration;
using value::TextOverflow;
using value::TextWhiteSpace;

FX_END
#endif
