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

#include "ftr/image-codec.h"

FX_NS(ftr)

class TGAImageCodec::_Inl : public TGAImageCodec {
#define _inl_tga(self) static_cast<TGAImageCodec::_Inl*>(self)
	public:
#pragma pack(push,1)
	struct Header {
		byte idlength;              /* 00h Size of Image ID field */
		byte color_map_type;        /* 01h Color map type */
		// 2-rgb
		// 3-grayscale
		// 10-rle rgb
		// 11-rle grayscale
		byte data_type_code;        /* 02h Image type code */
		uint16 color_map_origin;    /* 03h Color map origin */
		uint16 color_map_length;    /* 05h Color map length */
		byte color_map_depth;       /* 07h Depth of color map entries */
		uint16 x_origin;            /* 08h X origin of image */
		uint16 y_origin;            /* 0Ah Y origin of image */
		uint16 width;               /* 0Ch Width of image */
		uint16 height;              /* 0Eh Height of image */
		// 16、24、32
		byte bits_per_pixel;        /* 10h Image pixel size */
		byte image_descriptor;      /* 11h Image descriptor byte */
	};
#pragma pack(pop)

	typedef void (TGAImageCodec::_Inl::*ReadDataBlackFunc)(byte** in, byte** out, int alpha);
	
	/**
	 * @func m_parse_rgb_rle # 解析RLE RGB图
	 * @arg data {const byte*} # 图像数据指针
	 * @arg new_data {byte*} # 新的通胀图像数据指针
	 * @arg pixex_size {int} # 图像像素数量
	 * @arg func {ReadDataBlackFunc} # 处理函数
	 * @private
	 */
	void m_parse_rgb_rle(byte* in, byte* out,
											 int bytes,
											 int pixex_size, ReadDataBlackFunc func, int alpha) {
		for (int i = 0; i < pixex_size; i++) {
			byte mask = in[0];
			in++;
			(this->*func)(&in, &out, alpha);
			
			int j = mask & 0x7f;    // data[0] & 01111111
			if (mask & 0x80) {       // data[0] & 10000000
				// 相同的像素
				byte* cp = out - bytes;
				for (int k = 0; k < j; k++, i++) {
					memcpy(out, cp, bytes);
					out += bytes;
				}
			} else {
				for (int k = 0; k < j; k++, i++) {
					(this->*func)(&in, &out, alpha);
				}
			}
		}
	}
	
	// RLE 灰度图
	void m_parse_gray_rle(byte* in, byte* out, int bytes, int pixex_size, int alpha) {
		for (int i = 0; i < pixex_size; i++) {
			byte mask = in[0];
			in += 1;
			m_read_gray_data_black(&in, &out, bytes, alpha);
			
			int j = mask & 0x7f;    // data[0] & 01111111
			if (mask & 0x80) {       // data[0] & 10000000
				// 相同的像素
				byte* tmp = out - 2;
				for (int k = 0; k < j; k++, i++) {
					memcpy(out, tmp, 2);
					out += 4;
				}
			}
			else {
				for (int k = 0; k < j; k++, i++) {
					m_read_gray_data_black(&in, &out, bytes, alpha);
				}
			}
		}
	}
	
	void m_read_16_data_black(byte** in, byte** out, int alpha) {
		uint16* in_ = (uint16*)*in;
		uint16* out_ = (uint16*)*out;
		*out_ = (in_[0] << 1) | (alpha ? (in_[0] & 0x8000) : 1);
		*in = (byte*)(in_ + 1);
		*out = (byte*)(out_ + 1);
	}
	
	void m_read_24_data_black(byte** in, byte** out, int alpha) {
		byte* in_ = *in;
		byte* out_ = *out;
		out_[2] = in_[0];
		out_[1] = in_[1];
		out_[0] = in_[2];
		*in = in_ + 3;
		*out = out_ + 3;
	}
	
	void m_read_32_data_black(byte** in, byte** out, int alpha) {
		byte* in_ = *in;
		byte* out_ = *out;
		out_[2] = in_[0];
		out_[1] = in_[1];
		out_[0] = in_[2];
		out_[3] = alpha ? in_[3] : 255;
		*in = in_ + 4;
		*out = out_ + 4;
	}
	
	void m_read_gray_data_black(byte** in, byte** out, int bytes, int alpha) {
		byte* in_ = *in;
		byte* out_ = *out;
		out_[0] = in_[0];
		out_[1] = alpha ? in_[1] : 255;
		*in = in_ + bytes;
		*out = out_ + bytes;
	}
};

static void premultiplied_alpha(byte* data, int pixex_size) {
	for(int i = 0; i < pixex_size; i++){
		float alpha = data[3] / 255;
		data[0] *= alpha;
		data[1] *= alpha;
		data[2] *= alpha;
		data += 4;
	}
}

static Buffer flip_vertical(cchar* data, int width, int height, int bytes) {
	
	Buffer rev(width * height * bytes);
	char* p = *rev;
	int row_size = width * bytes;
	char* tmp = p + (height - 1) * row_size;
	
	for ( int i = 0; i < height; i++ ) {
		memcpy(tmp, data, row_size);
		tmp -= row_size;
		data += row_size;
	}
	return rev;
}

Array<PixelData> TGAImageCodec::decode(cBuffer& data) {
	Array<PixelData> rv;
	
	_Inl::Header* header = (_Inl::Header*)*data; // 适用小端格式CPU
	// parse image
	int alpha = header->image_descriptor & 0x08;
	int bytes = header->bits_per_pixel / 8; // 2、3、4
	byte code = header->data_type_code;
	
	PixelData::Format format;
	_Inl::ReadDataBlackFunc func;
	
	if (bytes == 2) {
		if (code == 2 || code == 10) { // RGB | RLE RGB
			format = PixelData::RGBA5551; // RGBA5551
		} else { // GRAY | RLE GRAY
			format = PixelData::LUMINANCE_ALPHA88;
		}
		func = &TGAImageCodec::_Inl::m_read_16_data_black;
	} else if (bytes == 3) {
		format = PixelData::RGB888;
		func = &TGAImageCodec::_Inl::m_read_24_data_black;
	} else {
		format = alpha ? PixelData::RGBA8888: PixelData::RGBX8888;
		func = &TGAImageCodec::_Inl::m_read_32_data_black;
	}
	
	int width = header->width;
	int height = header->height;
	int pixex_size = width * height;
	int out_size = pixex_size * bytes;
	Buffer out(out_size);
	byte* out_ = (byte*)out.value();
	byte* in_ = ((byte*)data.value()) + sizeof(_Inl::Header) + header->idlength;
	
	switch ( code ) {
		case 2:  // RGB
		case 10: // RLE RGB
			if ( code == 2 ) { // RGB
				for ( int i = 0; i < pixex_size; i++ ) {
					(_inl_tga(this)->*func)(&in_, &out_, alpha);
				}
			}
			else {  // RLE RGB
				_inl_tga(this)->m_parse_rgb_rle(in_, out_, bytes, pixex_size, func, alpha);
			}
			break;
		case 3:  // GRAY
			for (int i = 0; i < pixex_size; i++) {
				out_[0] = in_[0];
				out_[1] = alpha ? in_[1] : 255;
				in_ += bytes; out_ += bytes;
			}
			break;
		case 11: // RLE GRAY
			_inl_tga(this)->m_parse_gray_rle(in_, out_, bytes, pixex_size, alpha);
			break;
		default:
			LOG("Parse tga image error, data type code undefined");
			return Array<PixelData>();
	}
	
	// 表示像素是从底部开始的,需要调个头
	if ( ! (header->image_descriptor & 0x20) ) {
	 // BOTTOM_LEFT
		out = flip_vertical(*out, width, height, bytes);
	}
	rv.push( PixelData(out, width, height, format, false) );
	return rv;
}

PixelData TGAImageCodec::decode_header(cBuffer& data) {
	_Inl::Header* header = (_Inl::Header*)*data;
	bool alpha = header->image_descriptor & 0x08;
	int bytes = header->bits_per_pixel / 8; // 2、3、4
	byte code = header->data_type_code;
	PixelData::Format format;
	
	if( bytes == 2) {
		if (code == 2 || code == 10) { // RGB | RLE RGB
			format = PixelData::RGBA5551;
		} else { // GRAY | RLE GRAY
			format = PixelData::LUMINANCE_ALPHA88;
		}
	} else if (bytes == 3) {
		format = PixelData::RGB888;
	} else {
		format = alpha ? PixelData::RGBA8888: PixelData::RGBX8888;
	}
	return PixelData(Buffer(), header->width, header->height, format, false);
}

Buffer TGAImageCodec::encode(cPixelData& pixel_data) {
	
	if (pixel_data.format() == PixelData::RGBA8888) {
	
		int size = sizeof(_Inl::Header) + pixel_data.width() * pixel_data.height() * 4;
		
		char* ret_data_p = new char[size];
		Buffer ret_data(ret_data_p, size);

		_Inl::Header header;
		header.idlength = 0;
		header.color_map_type = 0;
		header.data_type_code = 2;
		header.color_map_origin = 0;
		header.color_map_length = 0;
		header.color_map_depth = 0;
		header.x_origin = 0;
		header.y_origin = 0;
		header.width = pixel_data.width();
		header.height = pixel_data.height();
		header.bits_per_pixel = 32;
		header.image_descriptor =  0x08 | 0x20; //alpha flag | top-left flag
		
		memcpy(ret_data_p, &header, sizeof(_Inl::Header));
		
		cBuffer& pixel_data_d = pixel_data.body();
		int pixex_size = pixel_data_d.length() / 4;
		byte* tmp = (byte*)ret_data_p;
		const byte* data = (const byte*)*pixel_data_d;

		// 写入BGRA数据
		if ( pixel_data.is_premultiplied_alpha() ) {
			for (int i = 0; i < pixex_size; i++) {
				float alpha = data[3] / 255;
				tmp[2] = data[0] / alpha;
				tmp[1] = data[1] / alpha;
				tmp[0] = data[2] / alpha;
				tmp[3] = data[3];
				tmp += 4;
				data += 4;
			}
		}
		else {
			for ( int i = 0; i < pixex_size; i++ ) {
				tmp[2] = data[0];
				tmp[1] = data[1];
				tmp[0] = data[2];
				tmp[3] = data[3];
				tmp += 4;
				data += 4;
			}
		}
		return ret_data;
	}
	LOG("Pixel data: Invalid data, required for RGBA 8888 format");
	return Buffer();
}

FX_END
