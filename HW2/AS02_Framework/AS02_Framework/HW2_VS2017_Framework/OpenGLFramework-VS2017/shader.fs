#version 330 core

out vec4 FragColor;
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

uniform int lightIdxP;
uniform LightInfo light[3];
uniform MaterialInfo material;

uniform mat4 NormalMat;
uniform mat4 ModelMat;
uniform mat4 ViewMat;

uniform vec3 lightColor = vec3(1.0f, 1.0f, 1.0f);
uniform vec3 viewPos = vec3(0.0f, 0.0f, 2.0f);

uniform int vertex_or_perpixel;

vec3 directionalLight(vec3 N, vec3 V)
{
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
    float Attenuation = 0.05 + 0.3* Distance + 0.6 *Distance * Distance;
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

void main() {

    vec3 N = normalize(vertex_normal);
    vec3 V = -f_vertexInView;
    vec3 color = vec3(0, 0, 0);

    if (lightIdxP == 0)
        color += directionalLight(N, V);
    else if (lightIdxP == 1)
        color += pointLight(N, V);
    else if (lightIdxP == 2)
        color += spotLight(N, V);

    if (vertex_or_perpixel == 0)
        FragColor = vec4(vertex_color, 1.0f);
    else
        FragColor = vec4(color, 1.0f);
}
