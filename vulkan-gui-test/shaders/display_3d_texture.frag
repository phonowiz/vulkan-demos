#version 450 core


//Author:  Rafael Sabino
// Date:    02/28/2018


//(1) This implementation is based on the one found here: http://prideout.net/blog/?p=64 "The Little Grasshoper"
//(2) pretty good explanation of AABB/ray intersection can be found here:
//https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection


float   step_size = 0.01f;
float   max_samples = 80;


layout(location = 0) in  vec3 frag_world_position;

layout(binding = 1) uniform UBO
{
    vec3    eye_world_position;
    float   focal_length;
}ubo;

layout(binding = 2) uniform sampler2D texture_3d;

layout(location = 0) out vec4 out_color;



struct Ray {
    vec3 Origin;
    vec3 Dir;
};

struct AABB {
    vec3 Min;
    vec3 Max;
};


bool IntersectBox(Ray r, AABB aabb, out float t0, out float t1)
{
    vec3 invR = 1.0 / r.Dir;
    //these are equation 2 of the (2) article mentioned on top of page
    vec3 tbot = invR * (aabb.Min-r.Origin);
    vec3 ttop = invR * (aabb.Max-r.Origin);
    vec3 tmin = min(ttop, tbot);
    vec3 tmax = max(ttop, tbot);
    //find the minimum ray t value between x, y, and z
    vec2 t = max(tmin.xx, tmin.yz);
    t0 = max(t.x, t.y);
    //find maximum ray t value between x, y, and z
    t = min(tmax.xx, tmax.yz);
    t1 = min(t.x, t.y);

    return t0 <= t1;
}

void main()
{
    vec3 ray_direction =  frag_world_position - ubo.eye_world_position;

    Ray eye = Ray( ubo.eye_world_position, normalize(ray_direction) );
    AABB aabb = AABB(vec3(-1.0), vec3(+1.0));
    
    float tnear, tfar;
    out_color = vec4(1.0f);
    if(IntersectBox(eye, aabb, tnear, tfar))
    {
//        if (tnear < 0.0) tnear = 0.0;
//
//        vec3 rayStart = eye.Origin + eye.Dir * tnear;
//        vec3 rayStop = eye.Origin + eye.Dir * tfar;
//
//        // Transform from object space to texture coordinate space:
//        // note that the box we are intersecting against has dimension ranges from -1 to 1 in all axis
//        rayStart = 0.5 * (rayStart + 1.0);
//        rayStop = 0.5 * (rayStop + 1.0);
//
//        // Perform the ray marching:
//        vec3 pos = rayStart;
//        vec3 step = normalize(rayStop-rayStart) * stepSize;
//        float travel = distance(rayStop, rayStart);
//
//        for (int i=0; i < maxSamples && travel > 0.0; ++i, pos += step, travel -= stepSize)
//        {
//            vec3 samplePoint = pos;
//            fragColor += texture(texture3D, samplePoint);
//        }
        out_color = vec4(1.0f, 1.0f, 0.0f, 1.0f);
    }
    

}
