precision mediump float;

varying mediump vec2	v_TexCoord;

uniform mediump vec4	u_Color;
uniform sampler2D		s_texture;

void main(void)
{
	vec2 uv = v_TexCoord - (1.0 * floor(v_TexCoord/1.0));
	uv = uv * 0.5;
	gl_FragColor = u_Color * texture2D(s_texture, uv);
}
