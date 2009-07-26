varying mediump vec2	v_textureCoord;

uniform mediump vec4	Col;
uniform sampler2D		s_texture;

void main(void)
{
	gl_FragColor = Col * texture2D(s_texture, v_textureCoord);
}
