#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec3 aNormal;

out vec3 f_vertexInView;
out vec3 vertex_color;
out vec3 vertex_normal;
out vec3 FragPos;
uniform mat4 mvp;

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

uniform int lightIdxV;			
uniform LightInfo light[3];
uniform MaterialInfo material;

uniform mat4 NormalMat;
uniform mat4 ModelMat;
uniform mat4 ViewMat;

uniform vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
uniform vec3 viewPos = vec3(0.0f, 0.0f, 2.0f);

vec3 directionalLight(vec3 N, vec3 V)
{
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * lightColor;

    vec3 lightDir = normalize(light[0].position.xyz);

    float diff = max(dot(N, lightDir), 0.0);
    vec3 diffuse = max(diff * light[0].Ld.xyz, vec3(0.0));

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, N);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), light[0].shininess);
    vec3 specular = spec * lightColor;

    return (light[0].La * material.Ka + diffuse * material.Kd + specular * material.Ks);
}

vec3 pointLight(vec3 N, vec3 V)
{
    float ambientStrength = 0.15;
    vec3 ambient = ambientStrength * lightColor;

    vec4 lightInView = ViewMat * vec4(light[1].position.x, light[1].position.y, light[1].position.z, 0);    
    vec3 lightDir = normalize(light[1].position.xyz - FragPos);

    float diff = max(dot(N, lightDir), 0.0);
    vec3 diffuse = max(diff * light[1].Ld, vec3(0.0));

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, N);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), light[1].shininess);
    vec3 specular = spec * lightColor;
    float Distance = length(lightDir);
    float Attenuation = 0.01 + 0.8 * Distance + 0.1 * Distance * Distance;
    float fatt = min(1.0 / Attenuation, 1);

    return light[0].La * material.Ka + fatt * diffuse * material.Kd + fatt * specular * material.Ks;
}

vec3 spotLight(vec3 N, vec3 V)
{
    vec4 lightInView = ViewMat * vec4(-light[2].position.x, -light[2].position.y, light[2].position.z, 0);  
    vec3 lightDir = normalize(light[2].position.xyz - FragPos);

    float diff = max(dot(N, lightDir), 0.0);
    vec3 diffuse = max(diff * light[2].Ld.xyz, vec3(0.0));

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, N);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), light[2].shininess);
    vec3 specular = spec * lightColor;

    float Distance = length(f_vertexInView - lightInView.xyz);
    float Attenuation = 0.05 + 0.3 * Distance + 0.6 * Distance * Distance;
    float fatt = min(1.0 / Attenuation, 1);

    float Cutoff = cos(radians(light[2].spotCutoff));

    float spotDot = dot(normalize(FragPos - vec3(light[2].position.x, light[2].position.y, light[2].position.z)), normalize(vec4(0.0f, 0.0f, -1.0f, 0.0f).xyz));
    float spotCoefficient;
    float spotExponent = 50;
    if (spotDot > Cutoff)
        spotCoefficient = pow(spotDot, spotExponent);
    else
        spotCoefficient = 0.0;

    return light[0].La * material.Ka + spotCoefficient * fatt * (diffuse * material.Kd + specular * material.Ks);
}

void main()
{
   
    vec4 vertexInView = ViewMat * ModelMat * vec4(aPos.x, aPos.y, aPos.z, 1.0);
    vec4 normalInView = transpose(inverse(NormalMat)) * vec4 (aNormal, 0.0);

    FragPos = vec3(ModelMat * vec4(aPos, 1.0));
    f_vertexInView = -vertexInView.xyz;
    vertex_normal = normalInView.xyz; 

    vec3 N = normalize(normalInView.xyz); 
    vec3 V = -vertexInView.xyz;	

    if (lightIdxV == 0)
        vertex_color = directionalLight(N, V);
    else if(lightIdxV ==1)
        vertex_color = pointLight(N, V);
    else if(lightIdxV == 2)
        vertex_color = spotLight(N, V);

	// [TODO]
	gl_Position = mvp * vec4(aPos.x, aPos.y, aPos.z, 1.0);
}

