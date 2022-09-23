#version 330

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec2 aTexCoord;

out vec2 texCoord;
out vec3 f_vertexInView;
out vec3 vertex_color;
out vec3 vertex_normal;
out vec3 FragPos;

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

uniform mat4 um4p;	// projection matrix
uniform mat4 um4v;	// camera viewing transformation matrix
uniform mat4 um4m;	// rotation matrix
uniform LightInfo light;
uniform MaterialInfo material;

uniform vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
uniform vec3 viewPos = vec3(0.0f, 0.0f, 2.0f);

vec3 directionalLight(vec3 N, vec3 V)
{
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * lightColor;

    vec3 lightDir = normalize(light.position.xyz);

    float diff = max(dot(N, lightDir), 0.0);
    vec3 diffuse = max(diff * light.Ld.xyz, vec3(0.0));

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, N);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), light.shininess);
    vec3 specular = spec * lightColor;

    return (light.La * material.Ka + diffuse * material.Kd + specular * material.Ks);
}


void main() 
{
	vec4 vertexInView = um4v * um4m * vec4(aPos.x, aPos.y, aPos.z, 1.0);
    vec4 normalInView = transpose(inverse(um4v * um4m)) * vec4 (aNormal, 0.0);

	//FragPos = vec3(um4m * vec4(aPos, 1.0));
    f_vertexInView = vertexInView.xyz;
    vertex_normal = normalInView.xyz; 

	vec3 N = normalize(normalInView.xyz); 
    vec3 V = -vertexInView.xyz;	

	vertex_color = directionalLight(N, V);
	
	texCoord = aTexCoord;
	gl_Position = um4p * um4v * um4m * vec4(aPos.x, aPos.y, aPos.z, 1.0);
}
