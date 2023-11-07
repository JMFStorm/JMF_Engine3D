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

in vec3 fragPos;
in vec3 fragNormal;
in vec2 TexCoord;

uniform Material material;
uniform bool use_texture;
uniform bool use_specular_texture;
uniform vec3 view_coords;

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
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.color_texture, TexCoord));

    if (use_specular_texture)
    {
        vec3 halfway_dir = normalize(light_dir + view_dir);  
        float spec = pow(max(dot(frag_normal, halfway_dir), 0.0), material.shininess);

        specular = light.specular * spec * vec3(texture(material.specular_texture, TexCoord)) * material.specular_mult;
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
    vec3 diffuse = light.diffuse * diff * vec3(texture(material.color_texture, TexCoord));

    if (use_specular_texture)
    {
        vec3 halfway_dir = normalize(light_dir + view_dir);  
        float spec = pow(max(dot(frag_normal, halfway_dir), 0.0), material.shininess);

        specular = light.specular * spec * vec3(texture(material.specular_texture, TexCoord)) * material.specular_mult;
        specular *= diffuse;
    }

    diffuse *= attenuation;
    specular *= attenuation;

    return vec3(diffuse + specular) * intensity;
}

void main()
{
    vec3 color_result = vec3(0);
    vec3 norm = normalize(fragNormal);
    vec3 view_dir = normalize(view_coords - fragPos);

    vec3 ambient = global_ambient_light * texture(material.color_texture, TexCoord).rgb;
    color_result += ambient;

    for (int i = 0; i < pointlights_count; i++)
    {
        color_result += point_lights_color(pointlights[i], norm, fragPos, view_dir);
    }

    for (int i = 0; i < spotlights_count; i++)
    {
        color_result += spotlight_color(spotlights[i], norm, fragPos, view_dir);
    }

    FragColor = vec4(color_result, 1.0);
}
