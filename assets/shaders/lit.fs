#version 330

// Basic lit shader (fragment): one directional sun (Lambert + Blinn specular) + ambient.
// Scene uniforms are set by eng::RenderQueue; material uniforms by eng::Material parameters.

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in vec4 fragColor;

// raylib defaults
uniform sampler2D texture0;
uniform vec4      colDiffuse;

// Scene (eng::RenderQueue, from eng::World)
uniform vec3 sunDirection;      // direction the light travels, world space
uniform vec3 sunColor;
uniform vec3 ambientColor;
uniform vec3 viewPosition;

// Material parameters (eng::Material::SetParameter), 0 when not set
uniform float emissive;         // 0 = fully lit, 1 = unlit albedo
uniform float specularStrength;
uniform float alphaCutoff;      // 0 = off, else texels with alpha below it are discarded (cutout)

out vec4 finalColor;

void main()
{
    vec4 albedo = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
    if (alphaCutoff > 0.0 && albedo.a < alphaCutoff)
        discard;

    vec3  normal   = normalize(fragNormal);
    vec3  toLight  = normalize(-sunDirection);
    float diffuse  = max(dot(normal, toLight), 0.0);

    vec3  toView   = normalize(viewPosition - fragPosition);
    vec3  halfway  = normalize(toLight + toView);
    float specular = pow(max(dot(normal, halfway), 0.0), 32.0) * specularStrength * step(0.0, diffuse);

    vec3 lit   = albedo.rgb * (ambientColor + sunColor * diffuse) + sunColor * specular;
    vec3 color = mix(lit, albedo.rgb, clamp(emissive, 0.0, 1.0));

    finalColor = vec4(color, albedo.a);
}
