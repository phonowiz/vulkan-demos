

#version 450
#extension GL_ARB_separate_shader_objects: enable

/*
MIT License

Copyright (c) 2019 - 2020 Dimas "Dimev", "Skythedragon" Leenman

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Update 1 (25-9-2019): added 2 lines to prevent mie from shining through objects inside the atmosphere
Update 2 (2-10-2019): made use of HW_PERFORMANCE to improve performance on mobile (reduces number of samples), also added a sun
Update 3 (5-10-2019): added a license
Update 4 (28-11-2019): atmosphere now correctly blocks light from the scene passing through, and added an ambient scattering term
Update 5 (28-11-2019): mouse drag now changes the time of day
Update 6 (28-11-2019): atmosphere now doesn't use the ray sphere intersect function, meaning it's only one function
Update 7 (22-12-2019): Compacted the mie and rayleigh parts into a single vec2 + added a basic skylight
Update 8 (15-5-2020): Added ozone absorption (Can also be used as absorption in general)

Scattering works by calculating how much light is scattered to the camera on a certain path/
This implementation does that by taking a number of samples across that path to check the amount of light that reaches the path
and it calculates the color of this light from the effects of scattering.

There are two types of scattering, rayleigh and mie
rayleigh is caused by small particles (molecules) and scatters certain colors better than others (causing a blue sky on earth)
mie is caused by bigger particles (like water droplets), and scatters all colors equally, but only in a certain direction.
Mie scattering causes the red sky during the sunset, because it scatters the remaining red light

To know where the ray starts and ends, we need to calculate where the ray enters and exits the atmosphere
We do this using a ray-sphere intersect

The scattering code is based on https://www.scratchapixel.com/lessons/procedural-generation-virtual-worlds/simulating-sky
with some modifications to allow moving the planet, as well as objects inside the atmosphere, correct light absorbsion
from objects in the scene and an ambient scattering term tp light up the dark side a bit if needed

the camera also moves up and down, and the sun rotates around the planet as well

Note:     Because rayleigh is a long word to type, I use ray instead on most variable names
        the same goes for position (which becomes pos), direction (which becomes dir) and optical (becomes opt)
*/

//check out https://www.shadertoy.com/view/wlBXWK where I grabbed most of this code

// first, lets define some constants to use (planet radius, position, and scattering coefficients)
//#define PLANET_POS vec3(0.0) /* the position of the planet */
//#define PLANET_RADIUS 6371e3 /* radius of the planet */
#define ATMOS_RADIUS 6471e3 /* radius of the atmosphere */

// and the heights (how far to go up before the scattering has no effect)
#define HEIGHT_RAY 8e3 /* rayleigh height */
#define HEIGHT_MIE 1.2e3 /* and mie */
#define HEIGHT_ABSORPTION 30e3 /* at what height the absorption is at it's maximum */
#define ABSORPTION_FALLOFF 3e3 /* how much the absorption decreases the further away it gets from the maximum height */
// and the steps (more looks better, but is slower)
// the primary step has the most effect on looks
#if HW_PERFORMANCE==0
// edit these if you are on mobile
#define PRIMARY_STEPS 4
#define LIGHT_STEPS 4
# else
// and these on desktop
#define PRIMARY_STEPS 64 /* primary steps, affects quality the most */
#define LIGHT_STEPS 4 /* light steps, how much steps in the light direction are taken */
#endif

// camera mode, 0 is on the ground, 1 is in space, 2 is moving, 3 is moving from ground to space
//#define CAMERA_MODE 2


layout(location = 0) out vec4 out_color;

//layout(input_attachment_index = 0, binding = 0 ) uniform subpassInput normals;
//layout(input_attachment_index = 1, binding = 1 ) uniform subpassInput depth;
//layout(input_attachment_index = 2, binding = 2 ) uniform subpassInput positions;
//layout(input_attachment_index = 3, binding = 3 ) uniform subpassInput albedos;

layout(binding = 0) writeonly restrict coherent uniform imageCube cubemap_texture;

layout(binding =1, std140) uniform _atmospheric_state
{
    mat4    positive_x;
//    mat4    negative_x;
//    mat4    positive_y;
//    mat4    positive_z;
//    mat4    negative_z;

    vec4    ray_beta;
    vec4    mie_beta;
    vec4    ambient_beta;
    vec4    absorption_beta;
    vec4    planet_position;
    vec4    light_direction;
    
    vec4    cam_position;
    vec2    screen_size;
    float   planet_radius;
    float   g;
    float   view_steps;
    float   light_steps;

}atmosphere_state;

/*
Next we'll define the main scattering function.
This traces a ray from start to end and takes a certain amount of samples along this ray, in order to calculate the color.
For every sample, we'll also trace a ray in the direction of the light,
because the color that reaches the sample also changes due to scattering
*/
vec3 calculate_scattering(
    vec3 start,                 // the start of the ray (the camera position)
    vec3 dir,                     // the direction of the ray (the camera vector)
    float max_dist,             // the maximum distance the ray can travel (because something is in the way, like an object)
    vec3 scene_color,            // the color of the scene
    vec3 light_dir,             // the direction of the light
    vec3 light_intensity,        // how bright the light is, affects the brightness of the atmosphere
    vec3 planet_position,         // the position of the planet
    float planet_radius,         // the radius of the planet
    float atmo_radius,             // the radius of the atmosphere
    vec3 beta_ray,                 // the amount rayleigh scattering scatters the colors (for earth: causes the blue atmosphere)
    vec3 beta_mie,                 // the amount mie scattering scatters colors
    vec3 beta_absorption,       // how much air is absorbed
    vec3 beta_ambient,            // the amount of scattering that always occurs, cna help make the back side of the atmosphere a bit brighter
    float g,                     // the direction mie scatters the light in (like a cone). closer to -1 means more towards a single direction
    float height_ray,             // how high do you have to go before there is no rayleigh scattering?
    float height_mie,             // the same, but for mie
    float height_absorption,    // the height at which the most absorption happens
    float absorption_falloff,    // how fast the absorption falls off from the absorption height
    int steps_i,                 // the amount of steps along the 'primary' ray, more looks better but slower
    int steps_l                 // the amount of steps along the light ray, more looks better but slower
) {
    // add an offset to the camera position, so that the atmosphere is in the correct position
    start -= planet_position;
    // calculate the start and end position of the ray, as a distance along the ray
    // we do this with a ray sphere intersect
    float a = dot(dir, dir);
    float b = 2.0 * dot(dir, start);
    float c = dot(start, start) - (atmo_radius * atmo_radius);
    float d = (b * b) - 4.0 * a * c;
    
    // stop early if there is no intersect
    if (d < 0.0) return scene_color;
    
    // calculate the ray length
    vec2 ray_length = vec2(
        max((-b - sqrt(d)) / (2.0 * a), 0.0),
        min((-b + sqrt(d)) / (2.0 * a), max_dist)
    );
    
    // if the ray did not hit the atmosphere, return a black color
    if (ray_length.x > ray_length.y) return scene_color;
    // prevent the mie glow from appearing if there's an object in front of the camera
    bool allow_mie = max_dist > ray_length.y;
    // make sure the ray is no longer than allowed
    ray_length.y = min(ray_length.y, max_dist);
    ray_length.x = max(ray_length.x, 0.0);
    // get the step size of the ray
    float step_size_i = (ray_length.y - ray_length.x) / float(steps_i);
    
    // next, set how far we are along the ray, so we can calculate the position of the sample
    // if the camera is outside the atmosphere, the ray should start at the edge of the atmosphere
    // if it's inside, it should start at the position of the camera
    // the min statement makes sure of that
    float ray_pos_i = ray_length.x;
    
    // these are the values we use to gather all the scattered light
    vec3 total_ray = vec3(0.0); // for rayleigh
    vec3 total_mie = vec3(0.0); // for mie
    
    // initialize the avg density value. This is used to calculate how much air was in the ray.
    vec3 opt_i = vec3(0.0);
    
    // also init the scale height, avoids some vec2's later on
    vec2 scale_height = vec2(height_ray, height_mie);
    
    // Calculate the Rayleigh and Mie phases.
    // This is the color that will be scattered for this ray
    // mu, mumu and gg are used quite a lot in the calculation, so to speed it up, precalculate them
    float mu = dot(dir, light_dir);
    float mumu = mu * mu;
    float gg = atmosphere_state.g * atmosphere_state.g;
    float phase_ray = 3.0 / (50.2654824574 /* (16 * pi) */) * (1.0 + mumu);
    float phase_mie = allow_mie ? 3.0 / (25.1327412287 /* (8 * pi) */) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg)) : 0.0;
    
    // now we need to sample the 'primary' ray. this ray gathers the light that gets scattered onto it
    for (int i = 0; i < steps_i; ++i) {
        
        // calculate where we are along this ray
        vec3 pos_i = start + dir * (ray_pos_i + step_size_i * 0.5);
        
        // and how high we are above the surface
        float height_i = length(pos_i) - atmosphere_state.planet_radius;
        
        // now calculate the density of the particles (both for rayleigh and mie)
        vec3 density = vec3(exp(-height_i / scale_height), 0.0);
        
        // and the absorption density. this is for ozone, which scales together with the rayleigh,
        // but absorbs the most at a specific height, so use the sech function for a nice curve falloff for this height
        // clamp it to avoid it going out of bounds. This prevents weird black spheres on the night side
        density.z = clamp((1.0 / cosh((height_absorption - height_i) / absorption_falloff)) * density.x, 0.0, 1.0);
        density *= step_size_i;
        
        // Add these densities to the avg density value, so that we know how many particles are on this ray.
        opt_i += density;

        // Calculate the step size of the light ray.
        // again with a ray sphere intersect
        // a, b, c and d are already defined
        a = dot(light_dir, light_dir);
        b = 2.0 * dot(light_dir, pos_i);
        c = dot(pos_i, pos_i) - (atmo_radius * atmo_radius);
        d = (b * b) - 4.0 * a * c;

        // no early stopping, this one should always be inside the atmosphere
        // calculate the ray length
        float step_size_l = (-b + sqrt(d)) / (2.0 * a * float(steps_l));

        // and the position along this ray
        // this time we are sure the ray is in the atmosphere, so set it to 0
        float ray_pos_l = 0.0;

        // and the avg density value of this ray towards light
        vec3 opt_l = vec3(0.0);
        
        // now sample the light ray
        // this is similar to what we did before
        for (int l = 0; l < steps_l; ++l)
        {

            // calculate where we are along this ray
            vec3 pos_l = pos_i + light_dir * (ray_pos_l + step_size_l * 0.5);

            // the heigth of the position
            float height_l = length(pos_l) - atmosphere_state.planet_radius;

            // calculate the particle avg density, and add it.  One for rayleigh, and for mie.
            vec3 density_l = vec3(exp(-height_l / scale_height), 0.0);
            
            // this equation does not show up on the blog, we are just going to trust it.
            //the blog mentions that atmosphere absorption is ignored, but in this code it is not, and visually it looks good :)
            density_l.z = clamp((1.0 / cosh((height_absorption - height_l) / absorption_falloff)) * density_l.x, 0.0, 1.0);
            opt_l += density_l * step_size_l;

            // and increment where we are along the light ray.
            ray_pos_l += step_size_l;
            
        }
        
        // Now we need to calculate the attenuation
        // this is essentially how much light reaches the current sample point due to scattering.
        // this is Equation 5 in the blog.
        vec3 attn = exp(-(beta_mie * (opt_i.y + opt_l.y) + beta_ray * (opt_i.x + opt_l.x) + beta_absorption * (opt_i.z + opt_l.z)));

        // accumulate the scattered light (how much will be scattered towards the camera), we are missing beta_ray which will
        //be multiplied at the last minute in the return statement.  This is the integral in equation 4
        total_ray += density.x * attn;
        total_mie += density.y * attn;

        // and increment the position on this ray
        ray_pos_i += step_size_i;
        
    }
    
    // calculate how much light can pass through the atmosphere.  This is not exactly the function described in the blog, but it gives good results..
    vec3 opacity = exp(-(beta_mie * opt_i.y + beta_ray * opt_i.x + beta_absorption * opt_i.z));
    
    // calculate and return the final color, this is equation 4 in the blog.  Equation 4 is evaluated twice, one for rayleigh, one for mie,
    //and we add those results together.
    //beta_ray shows up here because it is constant and it can be taken out of the integral, the blog does not mention this and in the equation
    //as printed in the blog it doesn't show up.
    //total_ray is the result of the integral in equation 4, and phase_ray/phase_mie are the phase functions for rayleigh and mie.
    
    return (
            phase_ray * beta_ray * total_ray // rayleigh color
               + phase_mie * beta_mie * total_mie // mie
            + opt_i.x * beta_ambient // and ambient
    ) * light_intensity + scene_color * opacity; // now make sure the background is rendered correctly
}

vec3 get_camera_vector(mat4 inverse_proj) {

    vec2 uv = (2.0f * gl_FragCoord.xy / atmosphere_state.screen_size.xy) -1;
    vec4 proj = vec4(uv,1.0f,0.0f);
    proj.xyz = normalize(proj.xyz);
    
    vec4 ans = inverse_proj * proj;
    return normalize(vec3(ans.x, -ans.y, ans.z));
}


vec3 compute_atmos_color(vec3 camera_vector )
{
    vec4 scene = vec4(0.0, 0.0, 0.0, 1e12);
    
    // add a sun, if the angle between the ray direction and the light direction is small enough, color the pixels white
    //scene.xyz = vec3(dot(camera_vector, atmosphere_state.light_direction.xyz) > 0.9998 ? 3.0 : 0.0);
    
    scene.xyz = calculate_scattering(
        atmosphere_state.cam_position.xyz,                // the position of the camera
        camera_vector.xyz,                     // the camera vector (ray direction of this pixel)
        scene.w,                         // max dist, essentially the scene depth
        scene.xyz,                        // scene color, the color of the current pixel being rendered
        atmosphere_state.light_direction.xyz,                        // light direction
        vec3(40.0f),                        // light intensity, 40 looks nice
        atmosphere_state.planet_position.xyz,                        // position of the planet
        atmosphere_state.planet_radius,                  // radius of the planet in meters
        ATMOS_RADIUS,                   // radius of the atmosphere in meters
        atmosphere_state.ray_beta.xyz,                        // Rayleigh scattering coefficient
        atmosphere_state.mie_beta.xyz,                       // Mie scattering coefficient
        atmosphere_state.absorption_beta.xyz,                // Absorbtion coefficient
        atmosphere_state.ambient_beta.xyz,                    // ambient scattering, turned off for now. This causes the air to glow a bit when no light reaches it
        atmosphere_state.g,                              // Mie preferred scattering direction
        HEIGHT_RAY,                     // Rayleigh scale height
        HEIGHT_MIE,                     // Mie scale height
        HEIGHT_ABSORPTION,                // the height at which the most absorption happens
        ABSORPTION_FALLOFF,                // how fast the absorption falls off from the absorption height
        PRIMARY_STEPS,                     // steps in the ray direction
        LIGHT_STEPS                     // steps in the light direction
    );
    
    //return vec3(1.0f, 0.0f, 0.0f);
    return scene.xyz;
}


void main()
{
    
#define TEXTURE_CUBE_MAP_POSITIVE_X    0
#define TEXTURE_CUBE_MAP_NEGATIVE_X    1
#define TEXTURE_CUBE_MAP_POSITIVE_Y    2
#define TEXTURE_CUBE_MAP_NEGATIVE_Y    3
#define TEXTURE_CUBE_MAP_POSITIVE_Z    4
#define TEXTURE_CUBE_MAP_NEGATIVE_Z    5
    
    ivec3 voxel = ivec3(gl_FragCoord.x, gl_FragCoord.y, 0);
    vec4 color = vec4(0);
    color.w = 1.0f;
    
    vec3 cam_vector = get_camera_vector(atmosphere_state.positive_x);
    voxel.z = TEXTURE_CUBE_MAP_POSITIVE_X;
    color.xyz = compute_atmos_color(cam_vector);
    
    out_color = vec4(color.xyz, 1.0f);
    //imageStore(cubemap_texture, voxel, color);
//    //imageStore(cubemap_texture, voxel, vec4(.20f, 0.0f, 0.0f, 1.0f));
//
//    cam_vector = get_camera_vector(atmosphere_state.negative_x);
//    voxel.z = TEXTURE_CUBE_MAP_NEGATIVE_X;
//    color.xyz = compute_atmos_color(cam_vector);
//    imageStore(cubemap_texture, voxel, color);
//    //imageStore(cubemap_texture, voxel, vec4(.0f, .20f, 0.0f, 1.0f));
//
//    cam_vector.xyz = get_camera_vector(atmosphere_state.positive_y);
//    voxel.z = TEXTURE_CUBE_MAP_POSITIVE_Y;
//    color.xyz = compute_atmos_color(cam_vector);
//    imageStore(cubemap_texture, voxel, color);
//    //imageStore(cubemap_texture, voxel, vec4(.0f, 0.0f, .20f, 1.0f));
////
////
////    voxel.z = TEXTURE_CUBE_MAP_NEGATIVE_Y;
////    imageStore(cubemap_texture, voxel, vec4(0.0f, 1.0f, 1.0f, 1.0f));
////    cam_vector.xyz = get_camera_vector(atmosphere_state.negative_y);
////    voxel.z = TEXTURE_CUBE_MAP_NEGATIVE_Y;
////    color.xyz = compute_atmos_color(cam_vector);
////    imageStore(cubemap_texture, voxel, color);
//////
////
////
//    cam_vector.xyz = get_camera_vector(atmosphere_state.positive_z);
//    voxel.z = TEXTURE_CUBE_MAP_POSITIVE_Z;
//    color.xyz = compute_atmos_color(cam_vector);
//    imageStore(cubemap_texture, voxel, color);
//    //imageStore(cubemap_texture, voxel, vec4(.20f, 0.0f, .20f, 1.0f));
//
//    cam_vector.xyz = get_camera_vector(atmosphere_state.negative_z);
//    voxel.z = TEXTURE_CUBE_MAP_NEGATIVE_Z;
//    color.xyz = compute_atmos_color(cam_vector);
//    imageStore(cubemap_texture, voxel, color);
    //imageStore(cubemap_texture, voxel, vec4(.20f, .20f, 0.0f, 1.0f));
}
