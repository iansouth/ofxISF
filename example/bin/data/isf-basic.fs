/*{
	"DESCRIPTION": "basic color",
	"CREDIT": "by markpitchless",
	"ISFVSN": "2.0",
	"CATEGORIES": [
		"TEST-GLSL"
	],
	"INPUTS": [
		{
			"NAME": "blurAmount",
			"TYPE": "float"
		}
	]
}*/

void main()
{
	gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0) * blurAmount;
}
