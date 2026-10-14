#version 450 core

layout(location = 0) in vec2 inFragTextureCoordinate;
layout(location = 1) in vec3 inFragPositionWorld;
layout(location = 2) in vec3 inFragNormalWorld;

layout(location = 0) out vec4 outColor;

struct Light_Type
{
    uint point;
    uint directional;
    uint spot;
};

struct Light
{
    vec4 position;
    vec4 direction;

    // properties
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    // controls.x = inner cutoff angle
    // controls.y = outer cutoff angle
    vec4 controls;
    // settings.x = enabled
    // settings.y = type
    uvec4 settings;
    uint luminance;
};

layout(binding = 0, set = 0) uniform GlobalUniformBufferObject
{
    mat4 proj;
    mat4 view;
    mat4 inverseView;
}
globalUbo;

layout(binding = 1, set = 0) uniform SceneLightInfo
{
    uint numLights;
}
sceneLightInfo;

layout(binding = 2, set = 0) readonly buffer globalLightBuffer
{
    Light lights[];
};

layout(binding = 0, set = 2) uniform sampler2D textureSampler;

// Constant terrain material.
const vec3 MAT_AMBIENT = vec3(1.0);
const vec3 MAT_DIFFUSE = vec3(1.0);
const vec3 MAT_SPECULAR = vec3(1.0);
const float MAT_SHININESS = 32.0;

Light_Type createLightTypeStruct()
{
    Light_Type lightChecker = {0x0, 0x1, 0x2};
    return lightChecker;
}

void main()
{
    Light_Type lightChecker = createLightTypeStruct();

    vec3 ambientLight = vec3(0.0);
    vec3 diffuseLight = vec3(0.0);
    vec3 specularLight = vec3(0.0);
    vec3 surfaceNormal = normalize(inFragNormalWorld);

    vec3 cameraPosWorld = globalUbo.inverseView[3].xyz;
    vec3 viewDirection = normalize(cameraPosWorld - inFragPositionWorld);

    if (sceneLightInfo.numLights != 0)
    {
        for (int i = 0; i < int(sceneLightInfo.numLights); i++)
        {
            // check if the current light object is a spotlight
            bool isSpot = ((lights[i].settings.y & lightChecker.spot) != 0);
            bool isDirectional = ((lights[i].settings.y & lightChecker.directional) != 0);

            if (lights[i].settings.x == 1)
            {
                // light is enabled
                vec3 directionToLight;
                if (isDirectional)
                {
                    directionToLight = normalize(-lights[i].direction.xyz);
                }
                else
                {
                    directionToLight = normalize(lights[i].position.xyz - inFragPositionWorld.xyz);
                }

                // distance of direction vector squared
                float attenuation = 1.0 / dot(directionToLight, directionToLight);

                // apply ambient light (no attenuation for ambient sources)
                ambientLight += lights[i].ambient.xyz * lights[i].ambient.w;

                // calculate cosine value of difference between fragment vec to light and light direction
                float theta = max(dot(surfaceNormal, directionToLight), 0.0);
                float epsilon = lights[i].controls.x - lights[i].controls.y; // inner cutoff - outer cutoff
                float spotIntensity =
                    clamp((theta - lights[i].controls.y) / epsilon, 0.0, 1.0); // want to keep intensity between 0 and 1

                // apply lighting calculations
                vec3 lightColor = (lights[i].diffuse.xyz * lights[i].diffuse.w) * attenuation;

                vec3 rawDiffuse = lightColor * theta;
                // apply attenuation to light sources that are not directional
                if (!isDirectional)
                    rawDiffuse *= attenuation;

                // specular light
                vec3 halfAngle = normalize(directionToLight + viewDirection);
                float blinnTerm = dot(surfaceNormal, halfAngle);
                blinnTerm = clamp(blinnTerm, 0.0, 1.0);
                blinnTerm = theta != 0.0 ? blinnTerm : 0.0;
                // apply arbitrary power "s" -- high values results in sharper highlight
                blinnTerm = pow(blinnTerm, MAT_SHININESS);

                vec3 rawSpecular = ((lights[i].specular.xyz * lights[i].specular.w) * blinnTerm);
                // apply attenuation to light sources that are not directional
                if (!isDirectional)
                    rawSpecular *= attenuation;

                // apply any futher modifiers if needed (i.e. spot light outer ring)
                if (isSpot && (theta < lights[i].controls.x))
                {
                    // apply outer ring intensities to fragment light effects
                    diffuseLight += rawDiffuse * spotIntensity;
                    specularLight += rawSpecular * spotIntensity;
                }
                else
                {
                    diffuseLight += rawDiffuse;
                    specularLight += rawSpecular;
                }
            }
        }

        ambientLight *= MAT_AMBIENT;
        diffuseLight *= MAT_DIFFUSE;
        specularLight *= MAT_SPECULAR;

        vec3 totalSurfaceColor =
            (ambientLight + diffuseLight + specularLight) * vec3(texture(textureSampler, inFragTextureCoordinate));
        outColor = vec4(totalSurfaceColor, 1.0);
    }
    else
    {
        // no lights: fall back to the raw orthoimage
        outColor = vec4(texture(textureSampler, inFragTextureCoordinate));
    }
}