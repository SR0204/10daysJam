struct PipeInstanceData
{
    float4x4 worldMatrix;
    int isPowered;
    int mask;
    float2 padding;
};

StructuredBuffer<PipeInstanceData> gInstanceData : register(t0);

struct VSInput
{
    float3 pos : POSITION;
    float2 uv : TEXCOORD;
    uint instanceID : SV_InstanceID;
};

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    nointerpolation int isPowered : POWERED;
    nointerpolation int mask : MASK;
};

VSOutput main(VSInput input)
{
    VSOutput output;

    PipeInstanceData inst = gInstanceData[input.instanceID];

    output.pos = mul(inst.worldMatrix, float4(input.pos, 1.0f));
    
    output.uv = input.uv;

    output.isPowered = inst.isPowered;
    output.mask = inst.mask;

    return output;
}