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

/* material section */
struct material_t {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform vs_material {
    vec4 ambient;
    vec4 diffuse;
    vec4 emissive;
} material;

material_t getMaterial();

/* --------------- */

/* lights section */
#define NUM_LIGHTS 8

const uint LightType_Dir        = 0;
const uint LightType_Point      = 1;
const uint LightType_Ambient    = 2;
const uint LightType_Spot       = 3;

struct light_t {
    int type;    
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    float far;
    float near;
};

//NOTE: use w as type
//NOTE: use x as range, y z w resered for spot light
uniform vs_lights {
    vec4 position[NUM_LIGHTS];
    vec4 ambient[NUM_LIGHTS];
    vec4 diffuse[NUM_LIGHTS];
    vec4 range[NUM_LIGHTS];
} lights;

light_t getLight(int index);
vec3 computeLight(light_t light, vec3 normal, vec3 fragPos, vec3 viewDir, material_t mat);

/* --------------- */

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 viewPos;
    //vec3 ambientLight;
    float billboard;
    float lightsCount;
};

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    ViewDir = normalize(FragPos - viewPos);
    Env = reflect(ViewDir, normalize(aNormal)) * vec3(1.0, -1.0, 1.0);
    Norm = normalize(mat3(transpose(inverse(model))) * aNormal);  
    TexCoord = aTexCoord;

    //NOTE: calculate light
    vec3 light = vec3(0.0);
    material_t mat = getMaterial();
    for(int i = 0; i < int(lightsCount); i++) {
        light += computeLight(getLight(i), Norm, FragPos, ViewDir, mat);
    }

    Light = light;

    //NOTE: billboarding
    //TODO: lock specific axis
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

material_t getMaterial() {
    return material_t( 
        material.ambient.xyz, 
        material.diffuse.xyz,
        material.emissive.xyz
    );
}

light_t getLight(int index) {
    return light_t(
        int(lights.position[index].w),
        lights.position[index].xyz,
        lights.ambient[index].xyz,
        lights.diffuse[index].xyz,
        lights.range[index].x,
        lights.range[index].y
    );
}

vec3 computeLight(light_t light, vec3 normal, vec3 fragPos, vec3 viewDir, material_t mat) {
    switch(light.type) {
        case LightType_Ambient: {
            return light.ambient;
        }
        case LightType_Dir: {
            vec3 lightDir = normalize(-light.position);
            float intensity = clamp(dot(normalize(normal), normalize(lightDir)), 0.0, 1.0);
            vec3 diffuse = (light.diffuse * (intensity * mat.diffuse));
            return diffuse;
        }
        case LightType_Point: {
            vec3 lightVec = (light.position - fragPos);
            float dist = length(lightVec);
            lightVec = normalize(lightVec);

            if(dist <= light.near) dist = 1.0;
            else if(dist < light.far) dist = (dist - light.near) / (light.far - light.near) * -1.0 + 1.0;
            else dist = 0.0;

            return light.diffuse * max(dot(lightVec, normalize(normal)), 0.0) * dist * mat.diffuse;
        }
        case LightType_Spot: {
            
        }
        default:
            return vec3(0.0);
    }
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
};

uniform sampler2D diffuseSampler;
uniform sampler2D envSampler;

void main() {
    vec4 diffuseTexture = texture(diffuseSampler, TexCoord);

    //NOTE: check for cutout
    if(diffuseTexture.a != 1.0)
        discard;

    //TODO: check for alpha blending
    vec4 lightDiffuse = vec4(Light, 1.0) * diffuseTexture.rgba;

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