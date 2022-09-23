#version 330

in vec2 texCoord;

out vec4 fragColor;
in vec3 vertex_color;
in vec3 vertex_normal;
in vec3 f_vertexInView;
in vec3 FragPos;

struct LightInfo
{
    vec3 position;
    vec3 spotDirect;
    vec3 Ld;
    vec3 La;      
    vec3 Ls;            
    float spotExponent;
    float spotCutoff;
    float Ac;
    float Al;
    float Aq;
    float shininess;
};

struct MaterialInfo
{
    vec3 Ka;
    vec3 Kd;
    vec3 Ks;
};

// [TODO] passing texture from main.cpp
// Hint: sampler2D
uniform sampler2D tex;

uniform LightInfo light;
uniform MaterialInfo material;

uniform vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
uniform vec3 viewPos = vec3(0.0f, 0.0f, 2.0f);

vec3 directionalLight(vec3 N, vec3 V)
{
    vec3 lightDir = normalize(light.position.xyz);
    float diff = max(dot(N, lightDir), 0.0);
    vec3 diffuse = max(diff * light.Ld.xyz, vec3(0.0));

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, N);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), light.shininess);
    vec3 specular = spec * lightColor;

    return (light.La * material.Ka + diffuse * material.Kd + specular * material.Ks);
}

void main() {
    vec3 N = normalize(vertex_normal);
    vec3 V = -f_vertexInView;
	vec3 color = vec3(0, 0, 0);

	color = directionalLight(N, V);

	fragColor = vec4(color, 1.0f);

	// [TODO] sampleing from texture
	// Hint: texture
	vec4 texColor = vec4(texture(tex, texCoord).rgb, 1.0);
	fragColor =  fragColor * texColor;
}
