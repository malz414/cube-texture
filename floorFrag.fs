#version 410 core

vec3 getDirectionalLight(vec3 norm, vec3 viewDir, vec3 lightDirection, vec2 texCoords, float shadow);
vec3 getPointLight(vec3 norm, vec3 viewDir, vec3 lightDirection, vec2 texCoords);
vec3 getSpotLight(vec3 norm, vec3 viewDir,vec3 lightDirection, vec2 texCoords);
float calcShadow(vec4 fragPosLightSpace);
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir);


in vec3 normal ;
in vec3 posWS ;
in vec2 uv ;
in mat3 TBN;

struct pointLight{
	vec3 position;
	vec3 color;
	float Kc ;
	float Kl;
	float Ke;
};

struct spotLight{
	vec3 position;
	vec3 direction;
	vec3 color;
	float Kc ;
	float Kl;
	float Ke;

	float innerRad;
	float outerRad;
};

layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 brightColor;

uniform float bloomBrightness;

uniform vec3 lightCol;
uniform vec3 lightDir;
uniform vec3 objectCol;
uniform vec3 viewPos;
uniform float PXscale;
uniform vec3 lightDirection;
uniform vec3 viewDir;

uniform pointLight pLight;
uniform spotLight sLight;

uniform sampler2D diffuseTexture;
uniform sampler2D specTexture;
uniform sampler2D normalMap;
uniform sampler2D dispMap;

uniform bool DL;
uniform bool PL;
uniform bool SL;

float ambientFactor = 0.2;
float shine = 16 ;
float specularStrength = 0.2;

uniform sampler2D depthMap;
uniform mat4 lightSpaceMatrix;

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir){
	float height = texture(dispMap,texCoords).r;
	return texCoords - viewDir.xy * (height * PXscale);
}



void main()
{    	

	vec3 norm = normalize(normal);
	vec2 texCoords = uv;
	vec3 result = vec3(0.0);
	vec3 lightDirection = normalize(-lightDir);
	vec3 viewDir = normalize(viewPos - posWS);

	texCoords = ParallaxMapping(uv, viewDir);

		vec4 posLS = lightSpaceMatrix * vec4(posWS, 1.0);
	float shadow = calcShadow(posLS);
	
	//shadow = shadow*0.5;

	//vec3 getDirectionalLight(vec3 norm, vec3 viewDir, float shadow)
	
//	result = result + getDirectionalLight(norm, viewDir, shadow);


	 norm = texture(normalMap, texCoords).xyz;
	norm = norm*2.0-1.0;
	norm = normalize(TBN*norm);


	if(DL)
	result = result + getDirectionalLight(norm, viewDir, lightDirection, texCoords, shadow);
	if(PL)
	result = result + getPointLight(norm, viewDir, lightDirection, texCoords);
	if(SL)
	result = result + getSpotLight(norm, viewDir, lightDirection, texCoords);

	FragColor = vec4(result,1.0) ;

	float brightness = (result.x + result.y, result.z);
	//float brightness = max(max(result.r, result.g), result.b);
	if(brightness > bloomBrightness)
	brightColor = FragColor;
	else brightColor = vec4(vec3(0.0), 1.0);
}

vec3 getDirectionalLight(vec3 norm, vec3 viewDir, vec3 lightDirection, vec2 texCoords, float shadow){
	vec3 diffMapColor = texture(diffuseTexture, texCoords).xyz;
	float specMapColor = texture(specTexture, uv).x;

	vec3 ambientColor = lightCol*diffMapColor*ambientFactor;
	//diff
	float diffuseFactor = dot(norm, -lightDir);
	diffuseFactor = max(diffuseFactor,0.0);
	vec3 diffuseColor = lightCol*diffMapColor*diffuseFactor ;
	//spec
	vec3 reflectDir = reflect(lightDir, norm) ;
	float specularFactor = dot(viewDir, reflectDir) ;
	//blinn
	//vec3 halfwayDir = normalize(-lightDir + viewDir) ;
	//float specularFactor = dot(normal, halfwayDir) ;


	specularFactor = max(specularFactor,0.0) ;
	specularFactor = pow(specularFactor, shine);
	vec3 specularColor = lightCol * specularFactor * specMapColor;
	
	vec3 result = ambientColor +(1.0-shadow) * (diffuseColor + specularColor);
	return result;
}
vec3 getPointLight (vec3 norm, vec3 viewDir, vec3 lightDirection, vec2 texCoords){

	vec3 diffMapColor = texture(diffuseTexture, texCoords).xyz;
	float specMapColor = texture(specTexture, uv).x;

	 float dist = length(pLight.position - posWS) ;
	 float attn = 1.0/(pLight.Kc + (pLight.Kl*dist) + (pLight.Ke*(dist*dist)));
	 vec3 pLightDir = normalize(pLight.position - posWS);

	 vec3 ambientColor = lightCol*diffMapColor*ambientFactor;
	 ambientColor = ambientColor * attn ; 

	float diffuseFactor = dot(norm, pLightDir);
	diffuseFactor = max(diffuseFactor,2.0);
	vec3 diffuseColor = pLight.color*diffMapColor*diffuseFactor ;
	diffuseColor = diffuseColor *attn;

	vec3 reflectDir = reflect(pLightDir, norm) ;
	float specularFactor = dot(viewDir, reflectDir) ;
	//blinn
	//vec3 halfwayDir = normalize(-lightDir + viewDir) ;
	//float specularFactor = dot(normal, halfwayDir) ;
	specularFactor = max(specularFactor, 0.0) ;
	specularFactor = pow(specularFactor, shine);
	vec3 specularColor = pLight.color * specularFactor * specularStrength;
	specularColor = specularColor*attn ;
	vec3 pointLightResult = ambientColor + diffuseColor + specularColor;
	return pointLightResult;
}


vec3 getSpotLight (vec3 norm, vec3 viewDir, vec3 lightDirection, vec2 texCoords){
	//spotLight
	
	vec3 diffMapColor = texture(diffuseTexture, texCoords).xyz;
	float specMapColor = texture(specTexture, uv).x;

	 float dist = length(sLight.position - posWS) ;
	 float attn = 1.0/(sLight.Kc + (sLight.Kl*dist) + (sLight.Ke*(dist*dist)));
	 vec3 sLightDir = normalize(sLight.position - posWS);

	float diffuseFactor = dot(norm, sLightDir);
	diffuseFactor = max(diffuseFactor,0.0);
	vec3 diffuseColor = sLight.color*diffMapColor*diffuseFactor ;
	diffuseColor = diffuseColor *attn;

	vec3 reflectDir = reflect(sLightDir, norm) ;
	float specularFactor = dot(viewDir, reflectDir) ;
	//blinn
	//vec3 halfwayDir = normalize(-lightDir + viewDir) ;
	//float specularFactor = dot(normal, halfwayDir) ;
	specularFactor = max(specularFactor, 0.0) ;
	specularFactor = pow(specularFactor, shine);
	vec3 specularColor = sLight.color * specularFactor * specularStrength;
	specularColor = specularColor*attn ;

	float theta = dot(-sLightDir, normalize(sLight.direction));
	float denom = (sLight.innerRad - sLight.outerRad);
	float illum = (theta - sLight.outerRad) / denom ;
	illum = clamp(illum, 0.0, 1.0);
	diffuseColor = diffuseColor * illum;
	specularColor = specularColor * illum;

	vec3 spotLightResult = diffuseColor + specularColor;
	return spotLightResult;
}
float calcShadow(vec4 fragPosLightSpace){
vec2 texelSize = 1.0/ textureSize(depthMap,0);
vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
projCoords = projCoords * 0.5 + 0.5;
float closestDepth = texture (depthMap, projCoords.xy).r;
float currentDepth = projCoords.z;
float shadow = 0.0;
if(currentDepth > closestDepth)
shadow = .90;
return shadow;
}