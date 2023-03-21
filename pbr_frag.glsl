#version 400

in vec2 uv;
in vec3 frag_norm_v;
in vec3 frag_pos_v;
in vec3 frag_tangent_v; // Not needed unless using Normal Maps
in vec3 frag_bitangent_v; // Not needed unless using Normal Maps

out vec4 frag_color;

uniform sampler2D nor_tex;
uniform sampler2D diff_tex;
uniform sampler2D arm_tex;

uniform vec3 lightpos; //NOT NORMALIZED
uniform vec3 camerapos; //NOT NORMALIZED



const float PI = 3.141592;


//start with a value of 30-50 for intensity
//pass a non-normalize light_dir in and take its length to get distance
//this returns vec3 so you can change the color of your light
vec3 LightIntensity(vec3 light_dir){

}

vec3 HalfwayVec(vec3 camera_dir_normalized, vec3 light_dir_normalized){

}

//remember to clamp the denominator so you don't divide by zero (bound it below at like 0.0001)
float NDFTrowbridgeReitzGGX(float roughness, vec3 surf_norm, vec3 halfway_vec){

}

//bound the denominator below
//Disney says squaring the roughness prior to putting it in the rest of the equation looks better
float GeometrySchlickGGX(vec3 surf_norm, vec3 incoming_dir, float roughness){

}


//the instructions say you are supposed to multiply the two parts (start with this),
//other sources say to add them, wikipedia says pick the minimum
//from the light and camera term and bound it BELOW at 1 (so the result is >1)
//there's a lot of ways to try doing this and you can test them out until you find one
//you are satisfied with
float GSmithsMethod(vec3 surf_norm, vec3 camera_dir_normalized, vec3 light_dir_normalized, float roughness){

}

//you can do this in one line using the glsl "mix" instruction
//remember the base F0 value is vec3(0.04, 0.04, 0.04)
vec3 LerpF0Albedo(vec3 albedo, float metallic){

}

//some sources have the halfway vector here, others claim you are supposed to use the normal instead of the halfway vec
//again, you can play around with all of these
vec3 FresnelSchlick(vec3 F0, vec3 halfway_vec, vec3 camera_dir_normalized){

}

//when debugging, feel free to return the D, F, G, and other terms seperately to see what they look like
//this is how the images in the instructions are rendered
//remember camera_dir and light_dir are not normalized, you need to normalize them here
vec3 CookTorranceBRDF(vec3 albedo, float roughness, float metallic, vec3 camera_dir, vec3 light_dir, vec3 surf_norm){



    //COMPUTE MAIN BRDF HERE

    //return vec3(spec)*LightIntensity(light_dir)*NdotL;


}

vec3 ReflectanceEquation(vec3 albedo, float roughness, float metallic, vec3 camera_dir, vec3 light_dir, vec3 surf_norm){
    return CookTorranceBRDF(albedo, roughness, metallic, camera_dir, light_dir, surf_norm) ;
}


void main() {
    vec3 camera_dir = camerapos - frag_pos_v;
    vec3 light_dir = lightpos - frag_pos_v; 
    
    float roughness = texture(arm_tex, uv).y;
    float metallic = texture(arm_tex, uv).z;
    vec3 albedo = texture(diff_tex, uv).xyz;
 

    vec3 color = ReflectanceEquation(albedo, roughness, metallic, camera_dir, light_dir, frag_norm_v);
    color = color / (color + vec3(1.0)); //HDR tonemapping

    // uncomment each to test if you're getting the values properly    
    // frag_color = vec4(albedo, 1.0);
    // frag_color = vec4(metallic, metallic, metallic, 1.0); 
    // frag_color = vec4(roughness, roughness, roughness, 1.0);
    // frag_color = vec4(normal, 1.0);
    // frag_color = vec4(AO,AO,AO,1.0);
    // frag_color = vec4(vec3(dot(normalize(camera_dir),frag_norm_v)),1.0);
    // frag_color = vec4(vec3(dot(normalize(light_dir),frag_norm_v)),1.0);

    frag_color = vec4(color,1.0);

    
}
