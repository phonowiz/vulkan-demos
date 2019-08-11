#version 450 core
#extension GL_ARB_separate_shader_objects : enable

//Author:  Rafael Sabino
// Date:    02/28/2018


//(1) This implementation is based on the one found here: http://prideout.net/blog/?p=64 "The Little Grasshoper"
//(2) pretty good explanation of AABB/ray intersection can be found here:
//https://www.scratchapixel.com/lessons/3d-basic-rendering/minimal-ray-tracer-rendering-simple-shapes/ray-box-intersection


float   step_size = 0.005f;
float   max_samples = 200;


layout(location = 0) in  vec3 frag_obj_pos;

layout(binding = 1) uniform UBO
{
    mat4    mvp_inverse;
    vec4    box_eye_position;
    float screen_height;
    float screen_width;


}ubo;

layout(binding = 2) uniform sampler3D texture_3d;


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
    //excellent explanation of unprojection and how they work: https://www.derschmale.com/2014/09/28/unprojections-explained/
    vec3 box_ray_direction =  frag_obj_pos.xyz - ubo.box_eye_position.xyz;

    Ray eye = Ray( frag_obj_pos.xyz, normalize(box_ray_direction) );
    AABB aabb = AABB(vec3(-1.f), vec3(+1.f));

    float tnear, tfar;
    out_color =  vec4(0.0f);

    
    if(IntersectBox(eye, aabb, tnear, tfar))
    {
        if (tnear < 0.0) tnear = 0.0;

        vec3 ray_start = eye.Origin + eye.Dir * tnear;
        vec3 ray_stop = eye.Origin + eye.Dir * tfar;

        // Transform from object space to texture coordinate space:
        // note that the box we are intersecting against has dimension ranges from -1 to 1 in all axis
        ray_start = 0.5 * (ray_start + 1.0);
        ray_stop = 0.5 * (ray_stop + 1.0);

        // Perform the ray marching:
        vec3 pos = ray_start;
        vec3 step = normalize(ray_stop-ray_start) * step_size;
        float travel = distance(ray_stop, ray_start);

        for (int i=0; i < max_samples && travel > 0.0; ++i, pos += step, travel -= step_size)
        {
            //vec3 s = vec3(pos.x, 1- pos.y, pos.z);
            out_color += texture(texture_3d, pos);
        }

        //out_color = vec4(frag_obj_pos.x,0.0f, 0.0f, 1.0f);

    }
    

}
