#version 410 core
out vec4 FragColor;



uniform sampler2D image;
uniform sampler2D bloomBlur;
in vec2 uv;



void main()
{             
 vec3 hdrColor = texture(image,uv).xyz;
 vec3 bloomColor = texture(bloomBlur, uv).xyz;
 hdrColor += bloomColor;

 vec3 reinhard = hdrColor/(hdrColor + vec3(1.0));
 FragColor = vec4(reinhard, 0.40);
    
}

