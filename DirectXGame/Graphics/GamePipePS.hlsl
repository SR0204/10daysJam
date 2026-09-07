Texture2D<float4> g_texOff : register(t1);
Texture2D<float4> g_texOn : register(t2);
SamplerState g_sampler : register(s0);

struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    float chargeProgress : TEXCOORD1;
    nointerpolation int isPowered : IS_POWERED;
    nointerpolation int mask : MASK;
    nointerpolation int isGoal : IS_GOAL;
};

float4 main(VSOutput input) : SV_TARGET
{
    float4 colorOff = g_texOff.Sample(g_sampler, input.uv);
    float4 colorOn = g_texOn.Sample(g_sampler, input.uv);

    float4 pipeColor = lerp(colorOff, colorOn, input.chargeProgress);

    // グリッド線（外枠）の判定
    float border = 0.02f;
    if (input.uv.x < border || input.uv.x > (1.0f - border) ||
        input.uv.y < border || input.uv.y > (1.0f - border))
    {
        // ★ ゲームプレイ中専用：黒色 (0, 0, 0, 1)
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    return pipeColor;
}