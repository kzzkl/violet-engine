struct irradiance_data
{
    uint width;
    uint height;
};

ConstantBuffer<irradiance_data> data : register(b0, space0);
TextureCube environment_map : register(t1, space0);
SamplerState environment_sampler : register(s1, space0);

struct vs_in
{
    uint vertex_id : SV_VertexID;
};

struct vs_out
{
    float4 position : SV_POSITION;
    
    float3 right : NORMAL0;
    float3 left : NORMAL1;
    float3 top : NORMAL2;
    float3 bottom : NORMAL3;
    float3 front : NORMAL4;
    float3 back : NORMAL5;
};

struct ps_out
{
    float4 right : SV_TARGET0;
    float4 left : SV_TARGET1;
    float4 top : SV_TARGET2;
    float4 bottom : SV_TARGET3;
    float4 front : SV_TARGET4;
    float4 back : SV_TARGET5;
};

vs_out vs_main(vs_in input)
{
    const float2 ndc[4] = {
        float2(1.0, 1.0),
        float2(-1.0, 1.0),
        float2(-1.0, -1.0),
        float2(1.0, -1.0),
    };

    const float3 vertices[8] = {
        float3(1.0, 1.0, -1.0),
        float3(1.0, 1.0, 1.0),
        float3(1.0, -1.0, 1.0),
        float3(1.0, -1.0, -1.0),
        float3(-1.0, 1.0, 1.0),
        float3(-1.0, 1.0, -1.0),
        float3(-1.0, -1.0, -1.0),
        float3(-1.0, -1.0, 1.0)};

    const int indices[36] = {
        0, 1, 2, 2, 3, 0, // +x
        4, 5, 6, 6, 7, 4, // -x
        0, 5, 4, 4, 1, 0, // +y
        2, 7, 6, 6, 3, 2, // -y
        1, 4, 7, 7, 2, 1, // +z
        5, 0, 3, 3, 6, 5  // -z
    };

    const float3 normals[6] = {
        float3(1.0, 0.0, 0.0),
        float3(-1.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0),
        float3(0.0, -1.0, 0.0),
        float3(0.0, 0.0, 1.0),
        float3(0.0, 0.0, -1.0)
    };

    vs_out output;
    output.position = float4(ndc[indices[input.vertex_id]], 0.0, 1.0);

    output.right = vertices[indices[input.vertex_id + 0]];
    output.left = vertices[indices[input.vertex_id + 6]];
    output.top = vertices[indices[input.vertex_id + 12]];
    output.bottom = vertices[indices[input.vertex_id + 18]];
    output.front = vertices[indices[input.vertex_id + 24]];
    output.back = vertices[indices[input.vertex_id + 30]];

    return output;
}

float4 sample_irradiance(float3 N)
{
	float3 up = float3(0.0, 1.0, 0.0);
	float3 right = normalize(cross(up, N));
	up = cross(N, right);

    const float PI = 3.141592654;
	const float TWO_PI = PI * 2.0;
	const float HALF_PI = PI * 0.5;

    float delta_phi = TWO_PI / data.width;
    float delta_theta = HALF_PI / data.height;

	float3 color = float3(0.0, 0.0, 0.0);
	uint sample_count = 0;
	for (float phi = 0.0; phi < TWO_PI; phi += delta_phi) {
		for (float theta = 0.0; theta < HALF_PI; theta += delta_theta) {
			float3 temp = cos(phi) * right + sin(phi) * up;
			float3 sample_normal = cos(theta) * N + sin(theta) * temp;
			color += environment_map.Sample(environment_sampler, sample_normal).rgb * cos(theta) * sin(theta);
			++sample_count;
		}
	}
	return float4(PI * color / float(sample_count), 1.0);
}

ps_out ps_main(vs_out input)
{
    ps_out output;
    output.right = sample_irradiance(normalize(input.right));
    output.left = sample_irradiance(normalize(input.left));
    output.top = sample_irradiance(normalize(input.top));
    output.bottom = sample_irradiance(normalize(input.bottom));
    output.front = sample_irradiance(normalize(input.front));
    output.back = sample_irradiance(normalize(input.back));

    return output;
}