override saturation: f32 = 1.08;
override contrast: f32 = 1.04;
override warmth: f32 = 0.02;

fn post(input: PostInput) -> vec4<f32> {
    let luminance = dot(input.color.rgb, vec3<f32>(0.2126, 0.7152, 0.0722));
    let colour = mix(vec3<f32>(luminance), input.color.rgb, saturation);
    let contrasted = (colour - vec3<f32>(0.5)) * contrast + vec3<f32>(0.5);
    let warmed = contrasted + vec3<f32>(warmth, warmth * 0.2, -warmth);
    return vec4<f32>(clamp(warmed, vec3<f32>(0.0), vec3<f32>(1.0)), input.color.a);
}
