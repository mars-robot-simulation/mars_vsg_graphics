const float PI = 3.14159265359;
const float RECIPROCAL_PI = 0.31830988618;
const float RECIPROCAL_PI2 = 0.15915494;
const float EPSILON = 1e-6;
const float c_MinRoughness = 0.04;

// copied form vsgExample data/shaders/custom_pbr.frag
// Encapsulate the various inputs used by the various functions in the shading equation
// We store values in this struct to simplify the integration of alternative implementations
// of the shading terms, outlined in the Readme.MD Appendix.
struct PBRInfo
{
    float NdotL;                  // cos angle between normal and light direction
    float NdotV;                  // cos angle between normal and view direction
    float NdotH;                  // cos angle between normal and half vector
    float LdotH;                  // cos angle between light direction and half vector
    float VdotH;                  // cos angle between view direction and half vector
    float VdotL;                  // cos angle between view direction and light direction
    float perceptualRoughness;    // roughness value, as authored by the model creator (input to shader)
    float metalness;              // metallic value at the surface
    vec3 reflectance0;            // full reflectance color (normal incidence angle)
    vec3 reflectance90;           // reflectance color at grazing angle
    float alphaRoughness;         // roughness mapped to a more linear change in the roughness (proposed by [2])
    vec3 diffuseColor;            // color contribution from diffuse lighting
    vec3 specularColor;           // color contribution from specular lighting
};

float rcp(const in float value)
{
    return 1.0 / value;
}

float pow5(const in float value)
{
    return value * value * value * value * value;
}

// Find the normal for this fragment, pulling either from a predefined normal map
// or from the interpolated mesh normal and tangent attributes.
vec3 getNormal()
{
    vec3 result;
    result = normalize(normalVarying);
    if (!gl_FrontFacing)
        result = -result;
    return result;
}

// Basic Lambertian diffuse
// Implementation from Lambert's Photometria https://archive.org/details/lambertsphotome00lambgoog
// See also [1], Equation 1
vec3 BRDF_Diffuse_Lambert(PBRInfo pbrInputs)
{
    return pbrInputs.diffuseColor * RECIPROCAL_PI;
}

vec3 BRDF_Diffuse_Custom_Lambert(PBRInfo pbrInputs)
{
    return pbrInputs.diffuseColor * RECIPROCAL_PI * pow(pbrInputs.NdotV, 0.5 + 0.3 * pbrInputs.perceptualRoughness);
}

// [Gotanda 2012, "Beyond a Simple Physically Based Blinn-Phong Model in Real-Time"]
vec3 BRDF_Diffuse_OrenNayar(PBRInfo pbrInputs)
{
    float a = pbrInputs.alphaRoughness;
    float s = a;// / ( 1.29 + 0.5 * a );
    float s2 = s * s;
    float VoL = 2 * pbrInputs.VdotH * pbrInputs.VdotH - 1;		// double angle identity
    float Cosri = pbrInputs.VdotL - pbrInputs.NdotV * pbrInputs.NdotL;
    float C1 = 1 - 0.5 * s2 / (s2 + 0.33);
    float C2 = 0.45 * s2 / (s2 + 0.09) * Cosri * ( Cosri >= 0 ? 1.0 / max(pbrInputs.NdotL, pbrInputs.NdotV) : 1 );
    return pbrInputs.diffuseColor / PI * ( C1 + C2 ) * ( 1 + pbrInputs.perceptualRoughness * 0.5 );
}

// [Gotanda 2014, "Designing Reflectance Models for New Consoles"]
vec3 BRDF_Diffuse_Gotanda(PBRInfo pbrInputs)
{
    float a = pbrInputs.alphaRoughness;
    float a2 = a * a;
    float F0 = 0.04;
    float VoL = 2 * pbrInputs.VdotH * pbrInputs.VdotH - 1;		// double angle identity
    float Cosri = VoL - pbrInputs.NdotV * pbrInputs.NdotL;
    float a2_13 = a2 + 1.36053;
    float Fr = ( 1 - ( 0.542026*a2 + 0.303573*a ) / a2_13 ) * ( 1 - pow( 1 - pbrInputs.NdotV, 5 - 4*a2 ) / a2_13 ) * ( ( -0.733996*a2*a + 1.50912*a2 - 1.16402*a ) * pow( 1 - pbrInputs.NdotV, 1 + rcp(39*a2*a2+1) ) + 1 );
    //float Fr = ( 1 - 0.36 * a ) * ( 1 - pow( 1 - NoV, 5 - 4*a2 ) / a2_13 ) * ( -2.5 * Roughness * ( 1 - NoV ) + 1 );
    float Lm = ( max( 1 - 2*a, 0 ) * ( 1 - pow5( 1 - pbrInputs.NdotL ) ) + min( 2*a, 1 ) ) * ( 1 - 0.5*a * (pbrInputs.NdotL - 1) ) * pbrInputs.NdotL;
    float Vd = ( a2 / ( (a2 + 0.09) * (1.31072 + 0.995584 * pbrInputs.NdotV) ) ) * ( 1 - pow( 1 - pbrInputs.NdotL, ( 1 - 0.3726732 * pbrInputs.NdotV * pbrInputs.NdotV ) / ( 0.188566 + 0.38841 * pbrInputs.NdotV ) ) );
    float Bp = Cosri < 0 ? 1.4 * pbrInputs.NdotV * pbrInputs.NdotL * Cosri : Cosri;
    float Lr = (21.0 / 20.0) * (1 - F0) * ( Fr * Lm + Vd + Bp );
    return pbrInputs.diffuseColor * RECIPROCAL_PI * Lr;
}

vec3 BRDF_Diffuse_Burley(PBRInfo pbrInputs)
{
    float energyBias = mix(pbrInputs.perceptualRoughness, 0.0, 0.5);
    float energyFactor = mix(pbrInputs.perceptualRoughness, 1.0, 1.0 / 1.51);
    float fd90 = energyBias + 2.0 * pbrInputs.VdotH * pbrInputs.VdotH * pbrInputs.perceptualRoughness;
    float f0 = 1.0;
    float lightScatter = f0 + (fd90 - f0) * pow(1.0 - pbrInputs.NdotL, 5.0);
    float viewScatter = f0 + (fd90 - f0) * pow(1.0 - pbrInputs.NdotV, 5.0);

    return pbrInputs.diffuseColor * lightScatter * viewScatter * energyFactor;
}

vec3 BRDF_Diffuse_Disney(PBRInfo pbrInputs)
{
	float Fd90 = 0.5 + 2.0 * pbrInputs.perceptualRoughness * pbrInputs.VdotH * pbrInputs.VdotH;
    vec3 f0 = vec3(0.1);
	vec3 invF0 = vec3(1.0, 1.0, 1.0) - f0;
	float dim = min(invF0.r, min(invF0.g, invF0.b));
	float result = ((1.0 + (Fd90 - 1.0) * pow(1.0 - pbrInputs.NdotL, 5.0 )) * (1.0 + (Fd90 - 1.0) * pow(1.0 - pbrInputs.NdotV, 5.0 ))) * dim;
	return pbrInputs.diffuseColor * result;
}

// The following equation models the Fresnel reflectance term of the spec equation (aka F())
// Implementation of fresnel from [4], Equation 15
vec3 specularReflection(PBRInfo pbrInputs)
{
    //return pbrInputs.reflectance0 + (pbrInputs.reflectance90 - pbrInputs.reflectance0) * pow(clamp(1.0 - pbrInputs.VdotH, 0.0, 1.0), 5.0);
    return pbrInputs.reflectance0 + (pbrInputs.reflectance90 - pbrInputs.reflectance90*pbrInputs.reflectance0) * exp2((-5.55473 * pbrInputs.VdotH - 6.98316) * pbrInputs.VdotH);
}

// This calculates the specular geometric attenuation (aka G()),
// where rougher material will reflect less light back to the viewer.
// This implementation is based on [1] Equation 4, and we adopt their modifications to
// alphaRoughness as input as originally proposed in [2].
float geometricOcclusion(PBRInfo pbrInputs)
{
    float NdotL = pbrInputs.NdotL;
    float NdotV = pbrInputs.NdotV;
    float r = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;

    float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r + (1.0 - r) * (NdotL * NdotL)));
    float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r + (1.0 - r) * (NdotV * NdotV)));
    return attenuationL * attenuationV;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(PBRInfo pbrInputs)
{
    float roughnessSq = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;
    float f = (pbrInputs.NdotH * roughnessSq - pbrInputs.NdotH) * pbrInputs.NdotH + 1.0;
    return roughnessSq / (PI * f * f);
}

vec3 BRDF(vec3 u_LightColor, vec3 v, vec3 n, vec3 l, vec3 h, float perceptualRoughness, float metallic, vec3 specularEnvironmentR0, vec3 specularEnvironmentR90, float alphaRoughness, vec3 diffuseColor, vec3 specularColor, float ao)
{
    float unclmapped_NdotL = dot(n, l);

    vec3 reflection = -normalize(reflect(v, n));
    reflection.y *= -1.0f;

    float NdotL = clamp(unclmapped_NdotL, 0.001, 1.0);
    float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
    float NdotH = clamp(dot(n, h), 0.0, 1.0);
    float LdotH = clamp(dot(l, h), 0.0, 1.0);
    float VdotH = clamp(dot(v, h), 0.0, 1.0);
    float VdotL = clamp(dot(v, l), 0.0, 1.0);

    PBRInfo pbrInputs = PBRInfo(NdotL,
                                NdotV,
                                NdotH,
                                LdotH,
                                VdotH,
                                VdotL,
                                perceptualRoughness,
                                metallic,
                                specularEnvironmentR0,
                                specularEnvironmentR90,
                                alphaRoughness,
                                diffuseColor,
                                specularColor);

    // Calculate the shading terms for the microfacet specular shading model
    vec3 F = specularReflection(pbrInputs);
    float G = geometricOcclusion(pbrInputs);
    float D = microfacetDistribution(pbrInputs);

    // Calculation of analytical lighting contribution
    vec3 diffuseContrib = (1.0 - F) * BRDF_Diffuse_Disney(pbrInputs);
    vec3 specContrib = F * G * D / (4.0 * NdotL * NdotV);
    // Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
    vec3 color = NdotL * u_LightColor * (diffuseContrib + specContrib);

    color *= ao;

    return color;
}

float convertMetallic(vec3 diffuse, vec3 specular, float maxSpecular)
{
    float perceivedDiffuse = sqrt(0.299 * diffuse.r * diffuse.r + 0.587 * diffuse.g * diffuse.g + 0.114 * diffuse.b * diffuse.b);
    float perceivedSpecular = sqrt(0.299 * specular.r * specular.r + 0.587 * specular.g * specular.g + 0.114 * specular.b * specular.b);

    if (perceivedSpecular < c_MinRoughness)
    {
        return 0.0;
    }

    float a = c_MinRoughness;
    float b = perceivedDiffuse * (1.0 - maxSpecular) / (1.0 - c_MinRoughness) + perceivedSpecular - 2.0 * c_MinRoughness;
    float c = c_MinRoughness - perceivedSpecular;
    float D = max(b * b - 4.0 * a * c, 0.0);
    return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);
}


float rnd(float x, float y) {
  return fract(sin(dot(vec2(x,y) ,vec2(12.9898,78.233))) * 43758.5453);
}

void pixellight_frag(vec4 base, vec3 n, float shadow, out vec4 outcol) {
  vec3 reflected;
  vec3 color = vec3(0.0, 0.0, 0.0);
  vec4 screenPos = (pc.projection * modelVertex);
  vec4 numLights = lightData.values[0];
  int numAmbientLights = int(numLights[0]);
  int numDirectionalLights = int(numLights[1]);
  int lightDataIndex = 1;
  vec3 normal = getNormal();
  // todo: ensure that this is viewDir from example
  vec3 eye = normalize(  eyeVec );
  screenPos /= screenPos.w;

  float brightnessCutoff = 0.001;
  float perceptualRoughness = 0.0;
  float metallic;
  vec3 diffuseColor;
  vec4 baseColor;
  float ambientOcclusion = 1.0;
  vec3 f0 = vec3(0.04);
  // todo: handle vertexColor?
  baseColor = pbr.baseColorFactor;

  perceptualRoughness = pbr.roughnessFactor;
  metallic = pbr.metallicFactor;
  diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
  diffuseColor *= 1.0 - metallic;

  float alphaRoughness = perceptualRoughness * perceptualRoughness;

  vec3 specularColor = mix(f0, baseColor.rgb, metallic);

  // Compute reflectance.
  float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

  // For typical incident reflectance range (between 4% to 100%) set the grazing reflectance to 100% for typical fresnel effect.
  // For very low reflectance range on highly diffuse objects (below 4%), incrementally reduce grazing reflecance to 0%.
  float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);
  vec3 specularEnvironmentR0 = specularColor.rgb;
  vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;
  float shininess = 100.0f;

  // sum ambient lights
  if(numAmbientLights>0)
  {
      for(int i=0; i<numAmbientLights; ++i)
      {
          vec4 lightColor = lightData.values[lightDataIndex++];
          color += (baseColor.rgb * lightColor.rgb) * (lightColor.a * ambientOcclusion);
      }
  }  
  if(numDirectionalLights > 0)
  {
      for(int i=0; i<numDirectionalLights; ++i)
      {
          vec4 lightColor = lightData.values[lightDataIndex++];
          vec3 direction = -lightData.values[lightDataIndex++].xyz;
          float brightness = lightColor.a;
          // for shadowMapCount ...
          int shadowMapCount = int(lightData.values[lightDataIndex].r);
          if (shadowMapCount > 0)
          {
              lightDataIndex += 1 + 8 * shadowMapCount;
          }
          else
          {
              lightDataIndex++;
          }
          // if light is too shadowed to effect the rendering skip it
          if (brightness <= brightnessCutoff ) continue;
          vec3 l = direction;         // Vector from surface point to light
          vec3 h = normalize(l+eye);    // Half vector between both l and v
          float scale = brightness;

          color.rgb += BRDF(lightColor.rgb * scale, eye, normal, l, h, perceptualRoughness, metallic, specularEnvironmentR0, specularEnvironmentR90, alphaRoughness, diffuseColor, specularColor, ambientOcclusion);

      }
  }

  // for(int i=0; i<numLights; ++i) {
  //   if(lightIsSet[i]==1) {
  //     nDotL = max(dot( n, normalize(  -lightVec[i] ) ), 0.0);
  //     reflected = normalize(reflect(lightVec[i], n ) );
  //     rDotE = max(dot( reflected, eye ), 0.0);
  //     //#if USE_LSPSM_SHADOW
  //     //shadow = shadow2DProj( shadowTexture, gl_TexCoord[6] ).r;
  //     //#elif USE_PSSM_SHADOW
  //     //shadow = pssmAmount();
  //     //#endif

  //     // add diffuse and specular light
  //     if(lightIsDirectional[i] == 1) {
  //       atten = lightConstantAtt[i];
  //     }
  //     else {
  //       dist = length(lightVec[i]);
  //       atten = 1.0/(lightConstantAtt[i] +
  //                    lightLinearAtt[i] * dist +
  //                    lightQuadraticAtt[i] * dist * dist);
  //     }
  //     if(lightIsSpot[i]==1) {
  //       float spotEffect = dot( normalize( spotDir[i] ), normalize( lightVec[i]  ) );
  //       float spot = (spotEffect > lightCosCutoff[i]) ? 1.0 : 1.0-min(1.0, pow(lightSpotExponent[i]*(lightCosCutoff[i]-spotEffect), 2));
  //       diffuse_  += spot * shadow * (atten*diffuse[i]*gl_FrontMaterial.diffuse * nDotL);
  //       test_specular_ = (spot * shadow * atten*specular[i]
  //                         * pow(rDotE, gl_FrontMaterial.shininess));
  //       specular_ += (gl_FrontMaterial.shininess > 0) ? test_specular_ : vec4(0.0);
  //       ambient  += lightAmbient[i]*gl_FrontMaterial.ambient;
  //     }
  //     else {
  //       ambient  += lightAmbient[i]*gl_FrontMaterial.ambient;
  //       diffuse_  += shadow*(atten*diffuse[i]*gl_FrontMaterial.diffuse * nDotL);
  //       test_specular_ = shadow*(specular[i]
  //                         * pow(rDotE, gl_FrontMaterial.shininess)); //needed as in some driver implementations, pow(0,0) yields NaN
  //       specular_ += (gl_FrontMaterial.shininess > 0) ? test_specular_ : vec4(0.0);
  //     }
  //   }
  // }

  // calculate output color
  outcol = vec4(color, 1.0);
  //outcol = brightness* ((ambient + diffuse_)*base  + specular_ + gl_FrontMaterial.emission*base);

  /* if(drawLineLaser == 1) { */
  /*   vec3 lwP = positionVarying.xyz - lineLaserPos.xyz; */
  /*   if(abs(dot(lineLaserNormal.xyz, lwP)) < 0.002) { */
  /*     vec3 lwPNorm = normalize(lwP); */
  /*     vec3 directionNorm = normalize(lineLaserDirection); */
  /*     float v2Laser = acos( dot(directionNorm, lwPNorm) ); */
  /*     if( v2Laser < (lineLaserOpeningAngle / 2.0f) ){ */
  /*       outcol = lineLaserColor; */
  /*     } */
  /*   } */
  /* } */

  //outcol.a = alpha*base.a;

  // if(useNoise == 1) {
  //   float noiseScale = 6;
  //   outcol.rg += noiseAmmount*(texture2D( NoiseMap, noiseScale*screenPos.xy).zw-0.5);
  //   outcol.b += noiseAmmount*(texture2D( NoiseMap, noiseScale*(screenPos.xy+vec2(0.5, 0.5))).z-0.5);
  // }

  /* if(useFog == 1) { */
  /*   // FIXME: eyevec maybe in tbn space ! */
  /*   float fog = gl_Fog.density*clamp(gl_Fog.scale*(length(eyeVec)-gl_Fog.start) , 0.0, 1.0); */
  /*   outcol = mix(gl_Fog.color, outcol, 1-fog); */
  /* } */

  //outcol = vec4(positionVarying.x, shadow, positionVarying.y, 1.0);
}
