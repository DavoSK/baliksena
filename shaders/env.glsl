@module env
@ctype mat4 glm::mat4
@ctype vec3 glm::vec3

@vs vs
in vec3 aPos;
in vec3 aNormal;
in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Norm;  
out vec2 TexCoord;
out vec3 Env;
out vec3 ViewDir;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 viewPos;
};

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    ViewDir = normalize(FragPos - viewPos);
    Env = reflect(ViewDir, normalize(aNormal)) * vec3(1.0, 1.0, -1.0);
    Norm = aNormal;
    TexCoord = aTexCoord;

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
@end

@fs fs
out vec4 FragColor;

in vec3 FragPos;
in vec3 Norm;  
in vec2 TexCoord;
in vec3 Env;
in vec3 ViewDir;

uniform fs_params {
    float envMode;
    float envRatio;
};

uniform sampler2D diffuseSampler, envSampler;

void main() {
    vec4 diffuseTexture = texture(diffuseSampler, TexCoord);
    
    vec3 envUvStuff = normalize(vec3(Env.x, max((Env.y - 1.0) * 0.65 + 1.0, 0.0), Env.z));
    vec2 envUV = (envUvStuff.xz / (2.0 * (1.0 + envUvStuff.y))) + 0.5;
    vec4 envTexture = texture(envSampler, envUV);

    //NOTE: env mode blending type
    // 0 - ratio, 1 - mul, 2 - additive 
    if(int(envMode) == 0) {
        FragColor = mix(diffuseTexture, envTexture, envRatio);
    } else if(int(envMode) == 1)  {
        FragColor = diffuseTexture * envTexture;
    } else if(int(envMode) == 2) {
        FragColor = diffuseTexture + envTexture;
    }
}
@end

@program env vs fs