struct InstanceData
{
    float4x4 worldMatrix;
    int isPowered;
    int mask;
    int isGoal;
    float chargeProgress;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float chargeProgress : TEXCOORD1;
    nointerpolation int isPowered : IS_POWERED;
    nointerpolation int mask : MASK;
    nointerpolation int isGoal : IS_GOAL;
};

StructuredBuffer<InstanceData> g_instanceData : register(t0);

VSOutput main(float3 pos : POSITION, float2 uv : TEXCOORD, uint instanceID : SV_InstanceID)
{
    VSOutput output;
    InstanceData inst = g_instanceData[instanceID];

    output.pos = mul(float4(pos, 1.0f), inst.worldMatrix);

    output.uv = uv;
    output.chargeProgress = inst.chargeProgress;
    output.isPowered = inst.isPowered;
    output.mask = inst.mask;
    output.isGoal = inst.isGoal;

    return output;
}