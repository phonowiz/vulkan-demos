#version 450
#extension GL_ARB_separate_shader_objects: enable


layout(location = 0) in vec2 in_frag_coord;
layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D final_render;

//based off of https://catlikecoding.com/unity/tutorials/advanced-rendering/fxaa/

layout(binding = 1) uniform FXAA_INPUT
{
    vec4    maintex_texel_size;     ////vec4(1/texture_width, 1/texture_height, 0.0f, 0.0f)
    float   contrast_threshold;
                                    //  contrast_threshold range:
                                    //   0.0833 - upper limit (default, the start of visible unfiltered edges)
                                    //   0.0625 - high quality (faster)
                                    //   0.0312 - visible limit (slower)
    float   relative_threshold;
                                    //  relative threshold range.
                                    //   0.333 - too little (faster)
                                    //   0.250 - low quality
                                    //   0.166 - default
                                    //   0.125 - high quality
                                    //   0.063 - overkill (slower)
    float   subpixel_blending;
                                    // Choose the amount of sub-pixel aliasing removal.
                                    // This can effect sharpness.
                                    //   1.00 - upper limit (softer)
                                    //   0.75 - default amount of filtering
                                    //   0.50 - lower limit (sharper, less sub-pixel aliasing removal)
                                    //   0.25 - almost off
                                    //   0.00 - completely off
    
} fxaa_input;

struct edge_data
{
    bool  is_horizontal;
    float pixel_step;
    float opposite_luminance;
    float gradient;
};

struct luminance_data {
    float m, n, e, s, w;
    float ne, nw, se, sw;
    float highest, lowest, contrast;
};

float sample_luminance(vec2 uv, float u_offset, float v_offset)
{
    uv += fxaa_input.maintex_texel_size.xy * vec2(u_offset, v_offset);
    return texture(final_render, uv).a;
}
vec3 sample_color(vec2 uv)
{
    return texture(final_render, uv).rgb;
}

luminance_data sample_luminance_neighborhood(vec2 uv)
{
    luminance_data l;
    
    l.m = sample_luminance(uv,  0,  0);
    l.n = sample_luminance(uv,  0,  1);
    l.e = sample_luminance(uv,  1,  0);
    l.s = sample_luminance(uv,  0, -1);
    l.w = sample_luminance(uv, -1,  0);

    l.ne = sample_luminance(uv,  1,  1);
    l.nw = sample_luminance(uv, -1,  1);
    l.se = sample_luminance(uv,  1, -1);
    l.sw = sample_luminance(uv, -1, -1);

    l.highest = max(max(max(max(l.n, l.e), l.s), l.w), l.m);
    l.lowest = min(min(min(min(l.n, l.e), l.s), l.w), l.m);
    l.contrast = l.highest - l.lowest;
    
    return l;
}

bool should_skip_pixel (luminance_data l)
{
    float threshold =
                max(fxaa_input.contrast_threshold, fxaa_input.relative_threshold * l.highest);
    return l.contrast < threshold;
}


float determine_pixel_blend_factor (luminance_data l)
{
    //north, east, south, west pixels have more weight because they
    //are closer to the center according to the blog post
    float fxaa_filter = 2.0f * (l.n + l.e + l.s + l.w);
    fxaa_filter += l.ne + l.nw + l.se + l.sw;
    
    //12 comes from the fact that 4 pixes have a weight of 2, 4 have
    //a weight of 1, so 2(4) + 4 = 12.
    fxaa_filter *= 1.0 / 12.0f;
    fxaa_filter = abs(fxaa_filter - l.m);
    fxaa_filter = clamp(fxaa_filter / l.contrast, 0.0f, 1.0f);

    float blend_factor = smoothstep(0, 1, fxaa_filter);
    return blend_factor * blend_factor * fxaa_input.subpixel_blending;
}


edge_data determine_edge(luminance_data l)
{
    edge_data e;
    
    //these two variables have measured horizontal and vertial contrast
    //we use a 3x3 box for analysis.
    
    //for the horizontal contrast, the first line looks at the contrast at the center
    //colum of the box.  This contrast has the most weight, that's why we mulitpy it by 2.
    
    //to determine horizontal contrast, we have analyze the rows of our 3x3 box
    //to determine vertical contrast, we have to analyze the columns
    
    
    float horizontal =
    abs(l.n + l.s - 2.0f * l.m) * 2.0f + //center column
    abs(l.ne + l.se - 2.0f * l.e) +   //right column from cetner
    abs(l.nw + l.sw - 2.0f * l.w);    //left column from center
    
    //the same applies to the vertical contrast, but instead of columns, is rows...
    float vertical =
    abs(l.e + l.w - 2.0f * l.m) * 2.0f +
    abs(l.ne + l.nw - 2.0f * l.n) +
    abs(l.se + l.sw - 2.0f * l.s);
    
    //is_horizontal refers to the direction of our edge inside of our 3x3 box
    e.is_horizontal = horizontal >= vertical;

    //north is the positive direction for horizontal, south is negative
    //east  is positive for vertical, w is negative
    float positive_luminance = e.is_horizontal ? l.n : l.e;
    float negative_luminance = e.is_horizontal ? l.s : l.w;
    float positive_gradient = abs(positive_luminance - l.m);
    float negative_gradient = abs(negative_luminance - l.m);

    e.pixel_step =
        e.is_horizontal ? fxaa_input.maintex_texel_size.y : fxaa_input.maintex_texel_size.x;
    
    
    //here we save the information of the direction of gratest change
    
    //if negative_gradient <= positive_gradient
    e.opposite_luminance = positive_luminance;
    e.gradient = positive_gradient;
    //else...
    if (positive_gradient < negative_gradient)
    {
        e.pixel_step = -e.pixel_step;
        e.opposite_luminance = negative_luminance;
        e.gradient = negative_gradient;
    }

    return e;
}

#if defined(LOW_QUALITY)
    #define EDGE_STEP_COUNT 4
    #define EDGE_STEPS 1, 1.5, 2, 4
    #define EDGE_GUESS 12
#else
    #define EDGE_STEP_COUNT 10
    #define EDGE_STEPS 1, 1.5, 2, 2, 2, 2, 2, 2, 2, 4
    #define EDGE_GUESS 8
#endif

float edgeSteps[EDGE_STEP_COUNT] = { EDGE_STEPS };

float determine_edge_blend_factor (luminance_data l, edge_data e, vec2 uv)
{
    vec2 uvEdge = uv;
    vec2 edgeStep;
    

    if (e.is_horizontal)
    {
        uvEdge.y += e.pixel_step * 0.5f;
        edgeStep = vec2(fxaa_input.maintex_texel_size.x, 0);
    }
    else
    {
        uvEdge.x += e.pixel_step * 0.5f; //the point of this is to sample in between pixels
        edgeStep = vec2(0, fxaa_input.maintex_texel_size.y);
    }
    
    float edgeLuminance = (l.m + e.opposite_luminance) * 0.5f; //here we are sampling between pixels
    float gradientThreshold = e.gradient * 0.25f; //.25f comes from the blog, I think was picked because it looks good

    vec2 puv = uvEdge + edgeStep * edgeSteps[0];
    float pLuminanceDelta = sample_luminance(puv, 0, 0) - edgeLuminance;
    bool pAtEnd = abs(pLuminanceDelta) >= gradientThreshold;

    //Here we look for the POSITIVE direction end of edge
    for (int i = 1; i < EDGE_STEP_COUNT && !pAtEnd; i++) {
        puv += edgeStep * edgeSteps[i];
        pLuminanceDelta = sample_luminance(puv, 0, 0) - edgeLuminance;
        pAtEnd = abs(pLuminanceDelta) >= gradientThreshold;
    }
    if (!pAtEnd) {
        puv += edgeStep * EDGE_GUESS;
    }
    
    vec2 nuv = uvEdge - edgeStep * edgeSteps[0];
    float nLuminanceDelta = sample_luminance(nuv,0,0) - edgeLuminance;
    bool nAtEnd = abs(nLuminanceDelta) >= gradientThreshold;

    //Here we look for the NEGATIVE direction end of edge
    for (int i = 1; i < EDGE_STEP_COUNT && !nAtEnd; i++) {
        nuv -= edgeStep * edgeSteps[i];
        nLuminanceDelta = sample_luminance(nuv,0,0) - edgeLuminance;
        nAtEnd = abs(nLuminanceDelta) >= gradientThreshold;
    }
    if (!nAtEnd) {
        nuv -= edgeStep * EDGE_GUESS;
    }

    float pDistance, nDistance;
    //if vertical...
    pDistance = puv.y - uv.y;
    nDistance = uv.y - nuv.y;
    //else..
    if (e.is_horizontal) {
        pDistance = puv.x - uv.x;
        nDistance = uv.x - nuv.x;
    }
    
    float shortestDistance = 0;
    bool deltaSign;
    
    //if nDistance < pDistance...
    shortestDistance = nDistance;
    deltaSign = (nLuminanceDelta) >= 0;
    //else...
    if (pDistance <= nDistance)
    {
        shortestDistance = pDistance;
        deltaSign = (pLuminanceDelta) >= 0;
    }
    
    
    //the following if statement consistently picks the same side of the edge
    //to anti alias, depending on which side you are standing.  The pixel being analyzed
    //is either just on the edge or just off the edge.
    
    //from the perspective of the pixel where you are standing, you are always picking the same direction.
    //think of it this way: if you are facing north, you can pick your left direction (relative to you).
    //Now if you are facing south, you again pick the left direction (relative to you).
    
    //Relative to you, you are being consistent at picking left, but relative to the image, you are either picking
    //east or west.
    
    //Back to this code, the direction your facing is determined by the (l.m-edge_luminance).  You are going to anti-alias
    //the direction of the edge in which the gradient's sign is different than the gradient from where you are standing
    //towards the front (here "front" is a direction relative to you).
    
    //This is necessary because we are going to analyze pixels just on the edge and just off the edge,
    //and we don't want to anti alias pixels next to each other.  Between 2 adjacent pixels, one gets anti aliasing treatment,
    //one doesn't.
    
    if (deltaSign == (l.m - edgeLuminance >= 0))
    {
        return 0;
    }
    //here (shortest_distance / (pDistance + nDistance)) cannot be greater than .5f because shortest distance
    //is equal to pDistance or nDistance
    return 0.5 - shortestDistance / (pDistance + nDistance);
}


const mat3 ACESInputMat =
{
    {0.59719, 0.35458, 0.04823},
    {0.07600, 0.90834, 0.01566},
    {0.02840, 0.13383, 0.83777}
};

// ODT_SAT => XYZ => D60_2_D65 => sRGB
const mat3 ACESOutputMat =
{
    { 1.60475, -0.53108, -0.07367},
    {-0.10208,  1.10813, -0.00605},
    {-0.00327, -0.07276,  1.07602}
};

vec3 RRTAndODTFit(vec3 v)
{
    vec3 a = v * (v + 0.0245786f) - 0.000090537f;
    vec3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

vec3 ACESFitted(vec3 color)
{
    color = transpose(ACESInputMat) * color;

    // Apply RRT and ODT
    color = RRTAndODTFit(color);

    color = transpose(ACESOutputMat) * color;

    // Clamp to [0, 1]
    color = clamp(color, 0.0f, 1.0f);

    return color;
}

vec3 ACESFilm(vec3 x)
{
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0f, 1.0f);
}

// Accurate for 4000K < Temp < 25000K
// in: correlated color temperature
// out: CIE 1931 chromaticity
vec2 D_IlluminantChromaticity( float Temp )
{
    // Correct for revision of Plank's law
    // This makes 6500 == D65
    Temp *= 1.4388 / 1.438;

    float x =    Temp <= 7000 ?
                0.244063 + ( 0.09911e3 + ( 2.9678e6 - 4.6070e9 / Temp ) / Temp ) / Temp :
                0.237040 + ( 0.24748e3 + ( 1.9018e6 - 2.0064e9 / Temp ) / Temp ) / Temp;
    
    float y = -3 * x*x + 2.87 * x - 0.275;

    return vec2(x,y);
}

// Accurate for 1000K < Temp < 15000K
// [Krystek 1985, "An algorithm to calculate correlated colour temperature"]
vec2 PlanckianLocusChromaticity( float Temp )
{
    float u = ( 0.860117757f + 1.54118254e-4f * Temp + 1.28641212e-7f * Temp*Temp ) / ( 1.0f + 8.42420235e-4f * Temp + 7.08145163e-7f * Temp*Temp );
    float v = ( 0.317398726f + 4.22806245e-5f * Temp + 4.20481691e-8f * Temp*Temp ) / ( 1.0f - 2.89741816e-5f * Temp + 1.61456053e-7f * Temp*Temp );

    float x = 3*u / ( 2*u - 8*v + 4 );
    float y = 2*v / ( 2*u - 8*v + 4 );

    return vec2(x,y);
}

vec3 xyY_2_XYZ( vec3 xyY )
{
    vec3 XYZ;
    XYZ[0] = xyY[0] * xyY[2] / max( xyY[1], 1e-10);
    XYZ[1] = xyY[2];
    XYZ[2] = (1.0 - xyY[0] - xyY[1]) * xyY[2] / max( xyY[1], 1e-10);

    return XYZ;
}


mat3 ChromaticAdaptation( vec2 src_xy, vec2 dst_xy )
{
    // Von Kries chromatic adaptation

    // Bradford
    mat3 ConeResponse =
    {
        {0.8951,  0.2664, -0.1614},
        {-0.7502,  1.7135,  0.0367},
        {0.0389, -0.0685,  1.0296},
    };
    
    ConeResponse = transpose(ConeResponse);
    mat3 InvConeResponse =
    {
        {0.9869929, -0.1470543,  0.1599627},
        {0.4323053,  0.5183603,  0.0492912},
        {-0.0085287,  0.0400428,  0.9684867},
    };
    
    InvConeResponse = transpose(InvConeResponse);
    

    vec3 src_XYZ = xyY_2_XYZ( vec3( src_xy, 1 ) );
    vec3 dst_XYZ = xyY_2_XYZ( vec3( dst_xy, 1 ) );

    vec3 src_coneResp = ConeResponse * src_XYZ ;
    vec3 dst_coneResp = ConeResponse * dst_XYZ ;

    mat3 VonKriesMat =
    {
        { dst_coneResp[0] / src_coneResp[0], 0.0, 0.0 },
        { 0.0, dst_coneResp[1] / src_coneResp[1], 0.0 },
        { 0.0, 0.0, dst_coneResp[2] / src_coneResp[2] }
    };
    
    VonKriesMat = transpose(VonKriesMat);

    return InvConeResponse * ( VonKriesMat * ConeResponse );
}

float Square( float x)
{
    return x * x;
}

vec2 PlanckianIsothermal( float Temp, float Tint )
{
    float u = ( 0.860117757f + 1.54118254e-4f * Temp + 1.28641212e-7f * Temp*Temp ) / ( 1.0f + 8.42420235e-4f * Temp + 7.08145163e-7f * Temp*Temp );
    float v = ( 0.317398726f + 4.22806245e-5f * Temp + 4.20481691e-8f * Temp*Temp ) / ( 1.0f - 2.89741816e-5f * Temp + 1.61456053e-7f * Temp*Temp );

    float ud = ( -1.13758118e9f - 1.91615621e6f * Temp - 1.53177f * Temp*Temp ) / Square( 1.41213984e6f + 1189.62f * Temp + Temp*Temp );
    float vd = (  1.97471536e9f - 705674.0f * Temp - 308.607f * Temp*Temp ) / Square( 6.19363586e6f - 179.456f * Temp + Temp*Temp );

    vec2 uvd = normalize( vec2( u, v ) );

    // Correlated color temperature is meaningful within +/- 0.05
    u += -uvd.y * Tint * 0.05f;
    v +=  uvd.x * Tint * 0.05f;
    
    float x = 3*u / ( 2*u - 8*v + 4 );
    float y = 2*v / ( 2*u - 8*v + 4 );

    return vec2(x,y);
}




vec3 WhiteBalance( vec3 LinearColor )
{
    
    // REC 709 primaries
    mat3 XYZ_2_sRGB_MAT =
    {
        { 3.2409699419, -1.5373831776, -0.4986107603},
        {-0.9692436363,  1.8759675015,  0.0415550574},
        {0.0556300797, -0.2039769589,  1.0569715142},
    };
    
    mat3 sRGB_2_XYZ_MAT =
    {
        {0.4124564, 0.3575761, 0.1804375},
        {0.2126729, 0.7151522, 0.0721750},
        {0.0193339, 0.1191920, 0.9503041},
    };
    float WhiteTint = 0.0f;
    XYZ_2_sRGB_MAT = transpose(XYZ_2_sRGB_MAT);
    sRGB_2_XYZ_MAT = transpose(sRGB_2_XYZ_MAT);
    
    //TODO: this could be set from outside shader
    float WhiteTemp = 6500.0f;
    vec2 SrcWhiteDaylight = D_IlluminantChromaticity( WhiteTemp );
    vec2 SrcWhitePlankian = PlanckianLocusChromaticity( WhiteTemp );

    vec2 SrcWhite = WhiteTemp < 4000 ? SrcWhitePlankian : SrcWhiteDaylight;
    vec2 D65White = vec2( 0.31270,  0.32900 );

    {
        // Offset along isotherm
        vec2 Isothermal = PlanckianIsothermal( WhiteTemp, WhiteTint ) - SrcWhitePlankian;
        SrcWhite += Isothermal;
    }

    mat3 WhiteBalanceMat = ChromaticAdaptation( SrcWhite, D65White );
    WhiteBalanceMat =  XYZ_2_sRGB_MAT *  (WhiteBalanceMat * sRGB_2_XYZ_MAT );

    return WhiteBalanceMat * LinearColor ;
}



void main()
{
    luminance_data l = sample_luminance_neighborhood(in_frag_coord);
    if (should_skip_pixel(l)) {
        out_color =  vec4(sample_color(in_frag_coord),1.0f);
        out_color = max(vec4(0), out_color);
    }
    else
    {
        vec2 uv = in_frag_coord;
        float pixel_blend = determine_pixel_blend_factor(l);
        edge_data e = determine_edge(l);
        float edge_blend = determine_edge_blend_factor(l, e, uv);
        float final_blend = max(pixel_blend, edge_blend);

        //if edge is horizontal, we sample vertically
        if (e.is_horizontal) {
            uv.y += e.pixel_step * final_blend;
        }
        else {
            uv.x += e.pixel_step * final_blend;
        }
        out_color = vec4(sample_color(uv), 1.0f);
        
        out_color = max(vec4(0), out_color);
    }
    
    out_color.xyz *= 1.4f;
    
    mat3 XYZ_2_AP1_MAT =
    {
        {1.6410233797, -0.3248032942, -0.2364246952},
        {-0.6636628587,  1.6153315917,  0.0167563477},
        {0.0117218943, -0.0082844420,  0.9883948585},
    };
    
    XYZ_2_AP1_MAT = transpose(XYZ_2_AP1_MAT);
    
    // Bradford chromatic adaptation transforms between ACES white point (D60) and sRGB white point (D65)
    mat3 D65_2_D60_CAT =
    {
        {1.01303,    0.00610531, -0.014971},
        {0.00769823, 0.998165,   -0.00503203},
        {-0.00284131, 0.00468516,  0.924507},
    };
    
    D65_2_D60_CAT = transpose(D65_2_D60_CAT);
    
    mat3 sRGB_2_XYZ_MAT =
    {
        {0.4124564, 0.3575761, 0.1804375},
        {0.2126729, 0.7151522, 0.0721750},
        {0.0193339, 0.1191920, 0.9503041},
    };
    sRGB_2_XYZ_MAT = transpose(sRGB_2_XYZ_MAT);
    
    // REC 709 primaries
    mat3 XYZ_2_sRGB_MAT =
    {
        {3.2409699419, -1.5373831776, -0.4986107603},
        {-0.9692436363,  1.8759675015,  0.0415550574},
        {0.0556300797, -0.2039769589,  1.0569715142},
    };
    
    XYZ_2_sRGB_MAT = transpose(XYZ_2_sRGB_MAT);
    
    
    mat3 D60_2_D65_CAT =
    {
        {0.987224,   -0.00611327, 0.0159533},
        {-0.00759836,  1.00186,    0.00533002},
        {0.00307257, -0.00509595, 1.08168},
    };
    
    D60_2_D65_CAT = transpose(D60_2_D65_CAT);
    
    
    mat3 AP1_2_XYZ_MAT =
    {
        {0.6624541811, 0.1340042065, 0.1561876870},
        {0.2722287168, 0.6740817658, 0.0536895174},
        {-0.0055746495, 0.0040607335, 1.0103391003},
    };
    AP1_2_XYZ_MAT = transpose(AP1_2_XYZ_MAT);
    
    mat3 sRGB_2_AP1 = XYZ_2_AP1_MAT * (D65_2_D60_CAT * sRGB_2_XYZ_MAT);
    mat3 AP1_2_sRGB = XYZ_2_sRGB_MAT * (D60_2_D65_CAT * AP1_2_XYZ_MAT);

    
    
    out_color.xyz = sRGB_2_AP1 * out_color.xyz;
    out_color.xyz = WhiteBalance(out_color.xyz);
    out_color.xyz = ACESFitted(out_color.xyz);
    out_color.xyz = AP1_2_sRGB * out_color.xyz;
    
    //gamma correct:
    out_color.xyzw = pow(out_color.xyzw, vec4(0.4545));
}


