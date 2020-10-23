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
    
    //gamma correct:
    out_color.xyz = pow(out_color.xyz, vec3(0.4545));
    out_color.w = 1.f;
}


