/* ***** BEGIN LICENSE BLOCK *****
 * Distributed under the BSD license:
 *
 * Copyright (c) 2015-2016, blue.chu
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

var fs      = require('fs');
var path    = require('path');
var inputs  = process.argv.slice(2);
var prefix  = inputs.pop();
var output_cc = inputs.pop();
var output_h = inputs.pop();
var Buffer  = require('buffer').Buffer;
var check_file_is_change = require('./check').check_file_is_change;

var round = `
float nx_round(float num) {
	float r = floor(num);
	if ( num - r >= 0.5 ) {
		return r + 1.0;
	} else {
		return r;
	}
}
vec2 nx_round(vec2 num) {
	return vec2(nx_round(num.x), nx_round(num.y));
}
`;

function format_string() {
	var rev = arguments[0];
	for (var i = 1, len = arguments.length; i < len; i++)
		rev = rev.replace(new RegExp('\\{' + (i - 1) + '\\}', 'g'), arguments[i]);
	return rev;
}

function write(fp) {
	for (var i = 1; i < arguments.length; i++) {
		fs.writeSync(fp, arguments[i], 'utf-8');
		fs.writeSync(fp, '\n', 'utf-8');
	}
}

function write_no_line_feed(fp) {
	for (var i = 1; i < arguments.length; i++) {
		fs.writeSync(fp, arguments[i], 'utf-8');
	}
}

function strip_comment(code) {
	return code.replace(/\/\/.*$/mg, '');
}

function resolve_include(input, paths, code) {
	if (paths[input]) {
		return '';
	}
	paths[input] = true;

	if (!code) {
		code = strip_comment(fs.readFileSync(input).toString('utf8'));
	}
		
	return code.replace(/^#include\s+"([^"]+)"/gm, function (all, a) {
		return  a.match(/\.(vp|fp)\.glsl$/i) ?
						'' : resolve_include(path.resolve(path.dirname(input), a), paths);
	});
}

function transformation_to_es2(glsl_code, type_vp) {
	var reg = /^\s*(?:layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s+)?(in|out)\s+((lowp|mediump|highp)\s+)?([a-zA-Z][a-zA-Z2-4]{2,})\s+([a-zA-Z0-9\_]+)\s*(\[\s*\d+\s*\])?;\s*$/mg;
	var es2_frag_out_name = [];
	var is_use_round = false;

	glsl_code = glsl_code.replace(reg, function(all, layout, inout, c, d, type, id) {
		if (type_vp) {
			if (inout == 'in') {
				return all.replace('in', 'attribute');
			} else { // out
				return all.replace('out', 'varying');
			}
		} else { // fp
			if (inout == 'in') {
				return all.replace('in', 'varying');
			} else { // out
				if (type == 'vec4') {
					es2_frag_out_name.push({ type: type, id: id });
				}
				return '';
			}
		}
		return all;
	});

	if (type_vp) {
		glsl_code = glsl_code.replace(/gl_VertexID/g, 'nx_VertexID');
	} 
	else { // fp
		if (es2_frag_out_name.length) {
			var reg = new RegExp(es2_frag_out_name[0].id, 'g');
			glsl_code = glsl_code.replace(reg, 'gl_FragColor');
		}
	}

	glsl_code = glsl_code.replace(/#version\s+\d+\s+(es)?/g, '');

	// glsl_code = glsl_code.replace(/(?<![a-zA-Z0-9\$_])texture\s*\(/g, 'texture2D(');
	glsl_code = glsl_code.replace(/([^a-zA-Z0-9\$_])texture\s*\(/g, '$1texture2D(');
	glsl_code = glsl_code.replace(/([^a-zA-Z0-9\$_])round\s*\(/g, function(all, a) {
		is_use_round = true;
		return a + 'nx_round(';
	});

	if (is_use_round)
		glsl_code = round + glsl_code;

	// console.log('++++++++++++++++++++', glsl_code);

	return glsl_code;
}

function find_uniforms_attributes(glsl_code, uniforms, uniform_blocks, attributes, type_vp) {

	// find uniform and attribute
	var glsl_code_reg = /^\s*(?:layout\s*\(\s*location\s*=\s*(\d+)\s*\)\s+)?(uniform|attribute|in)\s+((lowp|mediump|highp)\s+)?[a-zA-Z][a-zA-Z2-4]{2,}\s+([a-zA-Z0-9\_]+)\s*(\[\s*\d+\s*\])?;\s*$/mg;
	var glsl_code_mat = glsl_code_reg.exec(glsl_code);
	
	while ( glsl_code_mat ) {
		
		if (glsl_code_mat[2] == 'uniform') {
			// 剔除重复的名称
			if (uniforms.indexOf(glsl_code_mat[5]) == -1) {
				uniforms.push(glsl_code_mat[5]);
			}
		} else if (glsl_code_mat[2] == 'in') {
			if ( type_vp ) {
				attributes.push(glsl_code_mat[5]);
			}
		} else {
			attributes.push(glsl_code_mat[5]);
		}
		
		glsl_code_mat = glsl_code_reg.exec(glsl_code);
	}
	
	// find uniform block
	glsl_code_reg = /^\s*uniform\s+([a-zA-Z0-9\$_]+)\s*\{/mg;
	glsl_code_mat = glsl_code_reg.exec(glsl_code);
	
	while ( glsl_code_mat ) { // 剔除重复的名称
		if (uniform_blocks.indexOf(glsl_code_mat[1]) == -1) {
			uniform_blocks.push(glsl_code_mat[1]);
		}
		glsl_code_mat = glsl_code_reg.exec(glsl_code);
	}
}

function resolve_glsl(name, vert_code, frag_code, hpp, cpp) {

	console.log(`gen-glsl ${name}`);

	write(hpp,
		`struct struct_${name} {`,
			'  const char* name;',
			'  const unsigned char* source_vp;',
			'  const  unsigned long source_vp_len;',
			'  const unsigned char* source_fp;',
			'  const  unsigned long source_fp_len;',
			'  const unsigned char* es2_source_vp;',
			'  const  unsigned long es2_source_vp_len;',
			'  const unsigned char* es2_source_fp;',
			'  const  unsigned long es2_source_fp_len;',
			'  const char* shader_uniforms;',
			'  const char* shader_uniform_blocks;',
			'  const char* shader_attributes;',
			'  unsigned int shader;',
			'  const int is_test;'
	);

	var source_vp;
	var source_vp_len;
	var source_fp;
	var source_fp_len;
	var es2_source_vp;
	var es2_source_vp_len;
	var es2_source_fp;
	var es2_source_fp_len;
	var is_test = /^test/.test(name);
	var uniforms = [];
	var uniform_blocks = [];
	var attributes = [];

	[vert_code, frag_code].forEach(function(glsl_code, index) {
		glsl_code = glsl_code.replace(/^\s+/mg, '');
		var type_vp = !index;
		var es2_glsl_code = transformation_to_es2(glsl_code, type_vp);

		find_uniforms_attributes(glsl_code, uniforms, uniform_blocks, attributes, type_vp);

		var buff = Buffer.from(glsl_code).toJSON().data;
		var es2_buff = Buffer.from(es2_glsl_code).toJSON().data;

		if (type_vp) {
			source_vp = buff;
			source_vp_len = buff.length;
			es2_source_vp = es2_buff;
			es2_source_vp_len = es2_buff.length;
		} else {
			source_fp = buff;
			source_fp_len = buff.length;
			es2_source_fp = es2_buff;
			es2_source_fp_len = es2_buff.length;
		}
		buff.push(0); es2_buff.push(0);
	});

	// ---------------- output hpp ---------------- 

	uniforms.concat(uniform_blocks).forEach(function(uniform) {
		write(hpp, `  int ${uniform};`);
	});
	attributes.forEach(function(attribute) {
		write(hpp, `  unsigned int ${attribute};`);
	});
	write(hpp, format_string('} extern {0};', name));
	
	// ---------------- output cpp ---------------- 

	write(cpp, format_string('const unsigned char {0}_source_vp[] = { {1} };', name, source_vp));
	write(cpp, format_string('const unsigned char {0}_source_fp[] = { {1} };', name, source_fp));
	write(cpp, format_string('const unsigned char {0}_es2_source_vp[] = { {1} };', name, es2_source_vp));
	write(cpp, format_string('const unsigned char {0}_es2_source_fp[] = { {1} };', name, es2_source_fp));
	write(cpp, format_string('struct struct_{0} {0} = {', name)),
	write(cpp, format_string('  "{0}",', name));
	write(cpp, format_string('  {0}_source_vp,{1},', name, source_vp_len));
	write(cpp, format_string('  {0}_source_fp,{1},', name, source_fp_len));
	write(cpp, format_string('  {0}_es2_source_vp,{1},', name, es2_source_vp_len));
	write(cpp, format_string('  {0}_es2_source_fp,{1},', name, es2_source_fp_len));
	write(cpp, format_string('  "{0}",', uniforms.join(',')));
	write(cpp, format_string('  "{0}",', uniform_blocks.join(',')));
	write(cpp, format_string('  "{0}",', attributes.join(',')));
	write(cpp, '  0,');
	write(cpp, is_test ? '  1,' : '  0,');

	uniforms.concat(uniform_blocks).forEach(function(uniform) {
		write(cpp, '  0,');
	});
	attributes.forEach(function(attribute, i) {
		write(cpp, '  ' + i + ',');
	});
	write(cpp, '};');
}

function main() {

	if ( !check_file_is_change(inputs.concat([__filename]), [output_h, output_cc]) ) {
		return;
	}

	console.log(process.cwd(), output_h, output_cc);

	var hpp = fs.openSync(output_h, 'w');
	var cpp = fs.openSync(output_cc, 'w');
	var main_inputs = [];

	inputs.forEach(function(input) {
		var mat = input.match(/[\/\\](inl\-|_)?([^\/\\]+?)(_)?\.glsl$/i);
		if ( mat && !mat[1] && !mat[3] ) {
			main_inputs.push({ input: input, filename: mat[2] });
		}
	});

	var now = Date.now();
	write(hpp,
		'#ifndef __shader_natives_' + now,
		'#define __shader_natives_' + now,
		'namespace qk {',
		'namespace shader {',
		'#pragma pack(push,4)',
	);

	write(cpp,
		`#include "./${path.basename(output_h)}"`,
		'#include "quark/gl/gl.h"',
		'namespace qk {',
		'namespace shader {',
	);

	var names = [];

	main_inputs.forEach(function(e) {
		var input = e.input;
		var name = e.filename.replace(/[\.-]/gm, '_');
		var code = strip_comment(fs.readFileSync(input).toString('utf8'));
		var codes = { vert: '', frag: '' };

		var reg = /^\s*#(vert|frag)\s*$/mg;
		var prev = { index: 0, type: '' };

		var mat = reg.exec(code);
		while (mat) {
			if (prev.index) {
				codes[prev.type] += code.substring(prev.index, mat.index);
			}
			prev.index = mat.index + mat[0].length;
			prev.type = mat[1];
			mat = reg.exec(code);
		}

		if (prev.type && prev.index < code.length ) {
			codes[prev.type] += code.substring(prev.index, code.length);
		}
		if ( codes.vert && codes.frag ) {
			names.push(name);
			codes.vert = resolve_include(input, {}, codes.vert);
			codes.frag = resolve_include(input, {}, codes.frag);
			resolve_glsl(name, codes.vert, codes.frag, hpp, cpp);
		}
	});

	write(hpp, '#pragma pack(pop)');
	write(hpp, '}', '}', '#endif'); // end

	// init block
	var name = path.basename(output_cc).replace(/[\.-]/gm, '_');
	write(cpp, `FX_INIT_BLOCK(${name}) {`);
	names.forEach(e=>{
		write(cpp, `  GLDraw::register_gl_shader((qk::GLShader*)(&${e}));`)
	});
	write(cpp, '}');
	write(cpp, '}', '}');

	fs.closeSync(hpp);
	fs.closeSync(cpp);
}

main();
