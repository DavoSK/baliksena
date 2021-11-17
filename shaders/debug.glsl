@module debug
@ctype mat4 glm::mat4
@ctype vec3 glm::vec3

@vs vs
in vec3 aPos;
out vec3 Color;

uniform vs_params {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 color;
};

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    Color = color;
}
@end

@fs fs
out vec4 FragColor;
in vec3 Color;

void main() {
    FragColor = vec4(Color, 1.0);
}
@end

@program debug vs fs