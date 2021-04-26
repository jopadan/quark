#vert
#include "_box-radius.glsl"

// 只能使用4顶点的实例绘制

uniform vec4 background_color;

out vec4 f_color;

void main() {
	vec2 v;
	if (is_radius) { // 使用84个顶点绘制一个圆角矩形
		v = vertex();
	} else { // 使用4个顶点
		if (gl_VertexID == 0) v = vertex_ac.xy;
		else if (gl_VertexID == 1) v = vertex_ac.zy;
		else if (gl_VertexID == 2) v = vertex_ac.zw;
		else v = vertex_ac.xw;
	}
	f_color = background_color * vec4(1.0, 1.0, 1.0, opacity);
	
	gl_Position = r_matrix * v_matrix * vec4(v.xy, 0.0, 1.0);
}

#frag
#include "_version.glsl"

in  lowp vec4 f_color;
out lowp vec4 FragColor;

void main() {
	FragColor = f_color;
}
