@module cutout
@ctype mat4 glm::mat4

@vs vs
in vec3 aPos;
in vec3 aNormal;
in vec2 aTexCoord;

out vec3 Norm;
out vec2 TexCoord;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
};

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    Norm = aNormal;
    TexCoord = aTexCoord;
}
@end

@fs fs
out vec4 FragColor;

in vec3 Norm;
in vec2 TexCoord;

uniform sampler2D texture1;

void main() {
    vec4 diffuseTexture = texture(texture1, TexCoord);
    if(diffuseTexture.a != 1.0)
        discard;

    FragColor = diffuseTexture;
}
@end

@program cutout vs fs