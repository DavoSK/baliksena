@module universal
@ctype mat4 glm::mat4
@ctype vec3 glm::vec3
@ctype vec4 glm::vec4

@vs vs
in vec3 aPos;
in vec3 aNormal;
in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Norm;  
out vec2 TexCoord;
out vec3 Env;
out vec3 ViewDir;
out vec3 Light;

#define NR_POINT_LIGHTS 30
struct dir_light_t {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};  

struct point_light_t {    
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float range;
};

uniform vs_dir_light {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
} dir_light;

uniform vs_point_lights {
    vec4 position[NR_POINT_LIGHTS];  
    vec4 ambient[NR_POINT_LIGHTS];
    vec4 diffuse[NR_POINT_LIGHTS];
    vec4 specular[NR_POINT_LIGHTS];
    vec4 range[NR_POINT_LIGHTS];
} point_lights;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 viewPos;
    float billboard;
    float pointLightsCount;
};

dir_light_t getDirLight();
point_light_t getPointLight(int index);

vec3 calcDirLight(dir_light_t light, vec3 normal, vec3 viewDir);
vec3 calcPointLight(point_light_t light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    ViewDir = normalize(FragPos - viewPos);
    Env = reflect(ViewDir, normalize(aNormal)) * vec3(1.0, 1.0, -1.0);
    Norm = normalize(mat3(transpose(inverse(model))) * aNormal);

    TexCoord = aTexCoord;

    //NOTE: dir light
    vec3 light = calcDirLight(getDirLight(), Norm, ViewDir);

    //NOTE: point lights
    for(int i = 0; i < int(pointLightsCount); i++) {
        light += calcPointLight(getPointLight(i), Norm, FragPos, ViewDir);
    }

    Light = light;

    mat4 modelView = view * model;
    if(billboard > 0.0) {
        modelView[0][0] = length(vec3(model[0]));
        modelView[0][1] = 0.0; 
        modelView[0][2] = 0.0; 

        modelView[2][0] = 0.0; 
        modelView[2][1] = 0.0; 
        modelView[2][2] = 1.0; 
    }

    vec4 P = modelView * vec4(aPos, 1.0);
    gl_Position = projection * P;
}

dir_light_t getDirLight() {
    return dir_light_t(
        dir_light.direction,
        dir_light.ambient,
        dir_light.diffuse,
        dir_light.specular
    );
}

point_light_t getPointLight(int index) {
    for (int i = 0; i < NR_POINT_LIGHTS; ++i) {
        if (i == index) {
            return point_light_t(
                point_lights.position[i].xyz,
                point_lights.ambient[i].xyz,
                point_lights.diffuse[i].xyz,
                point_lights.specular[i].xyz,
                point_lights.range[i].x
            );
        }
    }
}

vec3 calcDirLight(dir_light_t light, vec3 normal, vec3 viewDir) {
    //light.direction.xyz = light.direction.xzy;
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 reflectDir = reflect(-lightDir, normal);
    vec3 ambient = light.ambient;
    vec3 diffuse = light.diffuse * diff;
    return ambient + diffuse;
}

vec3 calcPointLight(point_light_t light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    //NOTE: will be used for attenuation.
    float distance = length(light.position - fragPos);
    
    //NOTE: get a lighting direction vector from the light to the vertex.
    vec3 lightDir = normalize(light.position - fragPos);

    //NOTE: calculate the dot product of the light vector and vertex normal. If the normal and light vector are
    // pointing in the same direction then it will get max illumination.
    float diff = max(dot(normal, lightDir), 0.1);
    
    float attenuation = clamp(1.0 - distance / light.range, 0, 1);
    vec3 ambient = light.ambient;
    vec3 diffuse = light.diffuse * diff;
    ambient *= attenuation;
    diffuse *= attenuation;
    return (ambient + diffuse);
}
@end

@fs fs
out vec4 FragColor;

in vec3 FragPos;
in vec3 Norm;  
in vec2 TexCoord;
in vec3 Env;
in vec3 ViewDir;
in vec3 Light;

uniform fs_params {
    float envMode;
    float envRatio;
    vec3 ambientLight;
};

uniform sampler2D diffuseSampler, envSampler;

void main() {
    vec4 diffuseTexture = texture(diffuseSampler, TexCoord);
    
    //NOTE: check for cutout
    if(diffuseTexture.a != 1.0)
        discard;

    //TODO: check for alpha blending
    
    vec4 lightDiffuse = vec4(max(Light, ambientLight), 1.0) * diffuseTexture.rgba;

    //NOTE: check for env blending
    vec3 envUvStuff = normalize(vec3(Env.x, max((Env.y - 1.0) * 0.65 + 1.0, 0.0), Env.z));
    vec2 envUV = (envUvStuff.xz / (2.0 * (1.0 + envUvStuff.y))) + 0.5;
    vec4 envTexture = texture(envSampler, envUV);

    //NOTE: env mode blending type
    // 0 - ratio, 1 - mul, 2 - additiv, else disabled
    if(int(envMode) == 0) {
        FragColor = mix(lightDiffuse, envTexture, envRatio);
    } else if(int(envMode) == 1)  {
        FragColor = lightDiffuse * envTexture;
    } else if(int(envMode) == 2) {
        FragColor = lightDiffuse + envTexture;
    } else {
        FragColor = lightDiffuse;
    }
}

@end

@program universal vs fs