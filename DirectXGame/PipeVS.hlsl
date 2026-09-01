struct VSInput
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD0;
};

struct InstanceData
{
    float4x4 worldMatrix;
    int isPowered;
    int mask;
    int isGoal;
    float2 padding;
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

    float4 worldPos = mul(inst.worldMatrix, float4(input.pos, 1.0f));
    output.pos = worldPos;
    output.uv = input.uv;
    output.isPowered = inst.isPowered;
    output.mask = inst.mask;
    output.isGoal = inst.isGoal;

    return output;
}