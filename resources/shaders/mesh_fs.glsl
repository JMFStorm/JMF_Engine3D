#version 330 core

struct Pointlight {
    vec3 position;
    vec3 diffuse;
    float range;
    float specular;
    float intensity;
};

struct Spotlight {
    vec3 position;
    vec3 direction;
    vec3 diffuse;
    float specular;
    float range;
    float cutoff;
    float outer_cutoff;
};

struct Material {
    sampler2D color_texture;
    sampler2D specular_texture;
    float specular_mult;
    float shininess;
};

in VS_OUT {
    vec3 fragPos;
    vec3 fragNormal;
    vec2 TexCoord;
    vec4 FragPosLightSpace;
} fs_in;

uniform sampler2D shadowMap;

uniform Material material;
uniform bool use_texture;
uniform bool use_specular_texture;
uniform vec3 view_coords;

uniform vec3 shadow_map_light_pos;

uniform vec3 global_ambient_light;

uniform int pointlights_count;
uniform Pointlight pointlights[20];

uniform int spotlights_count;
uniform Spotlight spotlights[20];

out vec4 FragColor;

vec3 point_lights_color(Pointlight light, vec3 frag_normal, vec3 frag_pos, vec3 view_dir)
{
    vec3 specular = vec3(0, 0, 0);
    vec3 light_dir = normalize(light.position - frag_pos);
    float light_radius = light.range;

    float distance = length(light.position - frag_pos);
    float attenuation = 1.0 / (1.0 + (distance / light_radius)); // * (distance / light_radius));

    float diff = max(dot(frag_normal, light_dir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.color_texture, fs_in.TexCoord));

    if (use_specular_texture)
    {
        vec3 halfway_dir = normalize(light_dir + view_dir);  
        float spec = pow(max(dot(frag_normal, halfway_dir), 0.0), material.shininess);

        specular = light.specular * spec * vec3(texture(material.specular_texture, fs_in.TexCoord)) * material.specular_mult;
        specular *= diffuse;
    }

    diffuse *= attenuation;
    specular *= attenuation;

    return vec3(diffuse + specular) * light.intensity;
}

vec3 spotlight_color(Spotlight light, vec3 frag_normal, vec3 frag_pos, vec3 view_dir)
{
    vec3 specular = vec3(0, 0, 0);
    vec3 light_dir = normalize(light.position - frag_pos);
    float light_radius = light.range;
    float distance = length(light.position - frag_pos);
    float attenuation = 1.0 / (1.0 + (distance / light_radius));

    float theta = dot(light_dir, normalize(-light.direction));
    float epsilon = light.cutoff - light.outer_cutoff;
    float intensity = clamp((theta - light.outer_cutoff) / epsilon, 0.0, 1.0);

    float diff = max(dot(frag_normal, light_dir), 0.0);
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.color_texture, fs_in.TexCoord));

    if (use_specular_texture)
    {
        vec3 halfway_dir = normalize(light_dir + view_dir);  
        float spec = pow(max(dot(frag_normal, halfway_dir), 0.0), material.shininess);

        specular = light.specular * spec * vec3(texture(material.specular_texture, fs_in.TexCoord)) * material.specular_mult;
        specular *= diffuse;
    }

    diffuse *= attenuation;
    specular *= attenuation;

    return vec3(diffuse + specular) * intensity;
}

float ShadowCalculation(vec4 fragPosLightSpace, vec3 frag_norm)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    if(projCoords.z > 1.0) return 0.0;

    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    vec3 light_dir = normalize(shadow_map_light_pos - fs_in.fragPos);
    float fragment_dot = (1.0 - dot(frag_norm, light_dir));

    float bias_max = 0.0005;
    float bias_min = 0.00005;
    float bias = max(bias_max * fragment_dot, bias_min);
    if (currentDepth - bias < closestDepth) return 0.0;

    float shadow = 0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }

    shadow /= 9.0;
    return shadow;
}  

void main()
{
    vec3 color_result = vec3(0);
    vec3 norm = normalize(fs_in.fragNormal);
    vec3 view_dir = normalize(view_coords - fs_in.fragPos);

    vec3 ambient = global_ambient_light * texture(material.color_texture, fs_in.TexCoord).rgb;

    for (int i = 0; i < pointlights_count; i++)
    {
        color_result += point_lights_color(pointlights[i], norm, fs_in.fragPos, view_dir);
    }

    for (int i = 0; i < spotlights_count; i++)
    {
        color_result += spotlight_color(spotlights[i], norm, fs_in.fragPos, view_dir);
    }

    float shadow = ShadowCalculation(fs_in.FragPosLightSpace, fs_in.fragNormal);
    color_result = color_result * (1.0 - shadow) + ambient;

    FragColor = vec4(color_result, 1.0);
}
