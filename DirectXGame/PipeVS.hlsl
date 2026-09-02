struct VSInput
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD0;
};

struct InstanceData
{
    float4x4 worldMatrix; // 64 bytes
    int isPowered; // 4 bytes
    int mask; // 4 bytes
    int isGoal; // 4 bytes
    float padding; // 4 bytes
};

StructuredBuffer<InstanceData> gInstanceData : register(t0);

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    nointerpolation int isPowered : TEXCOORD1;
    nointerpolation int mask : MASK;
    nointerpolation int isGoal : IS_GOAL;
};

VSOutput main(VSInput input, uint instanceID : SV_InstanceID)
{
    VSOutput output;
    InstanceData inst = gInstanceData[instanceID];

    // Åö èCê≥ÅFmul(Vector, Matrix) ÇÃèáÇ…Ç∑ÇÈ
    float4 worldPos = mul(float4(input.pos, 1.0f), inst.worldMatrix);

    output.pos = worldPos;
    output.uv = input.uv;
    output.isPowered = inst.isPowered;
    output.mask = inst.mask;
    output.isGoal = inst.isGoal;

    return output;
}