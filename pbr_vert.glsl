#version 400
layout (location = 0) in vec3 vert_pos;
layout (location = 1) in vec3 vert_norm;
layout (location = 2) in vec2 vert_uv;
layout (location = 3) in vec4 vert_tangent;


uniform mat4 MVP;

out vec2 uv;
out vec3 frag_pos_v;
out vec3 frag_norm_v;
out vec3 frag_tangent_v;
out vec3 frag_bitangent_v;


void main(){
    vec4 pos = vec4(vert_pos, 1.0);
    gl_Position = MVP*pos;
    
    frag_pos_v = pos.xyz;
    uv = vert_uv;
    frag_tangent_v = normalize(vert_tangent.xyz);
    
    frag_norm_v = normalize(vert_norm);
    
    // **TODO** 
    // if using Normal Maps, replace the below value with the formula for bitangent
    frag_bitangent_v = vec3(0) ; // PLACEHOLDER VALUE




}
