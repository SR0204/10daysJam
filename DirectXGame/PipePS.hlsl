struct PSInput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    nointerpolation int isPowered : POWERED;
    nointerpolation int mask : MASK;
};

float4 main(PSInput input) : SV_TARGET
{
    float2 p = input.uv - 0.5f;

    // 通電: 水色, 非通電: グレー
    float3 activeColor = (input.isPowered != 0) ? float3(0.0f, 0.8f, 1.0f) : float3(0.5f, 0.5f, 0.5f);
    float3 bgColor = float3(0.2f, 0.2f, 0.25f);

    float pipeWidth = 0.12f;
    bool isPipe = false;

    // 中央結合部
    if (input.mask != 0)
    {
        if (abs(p.x) < pipeWidth && abs(p.y) < pipeWidth)
        {
            isPipe = true;
        }
    }

    int m = input.mask;

    // ★ Board.h の Enum 定義（UP=1, RIGHT=2, DOWN=4, LEFT=8）に完全一致させる
    // 1 (UP)    : 上方向 (p.y <= 0)
    if ((m & 1) != 0 && abs(p.x) < pipeWidth && p.y <= 0.0f)
        isPipe = true;

    // 2 (RIGHT) : 右方向 (p.x >= 0)
    if ((m & 2) != 0 && abs(p.y) < pipeWidth && p.x >= 0.0f)
        isPipe = true;

    // 4 (DOWN)  : 下方向 (p.y >= 0)
    if ((m & 4) != 0 && abs(p.x) < pipeWidth && p.y >= 0.0f)
        isPipe = true;

    // 8 (LEFT)  : 左方向 (p.x <= 0)
    if ((m & 8) != 0 && abs(p.y) < pipeWidth && p.x <= 0.0f)
        isPipe = true;

    float3 finalColor = isPipe ? activeColor : bgColor;

    // 外枠
    if (abs(p.x) > 0.48f || abs(p.y) > 0.48f)
    {
        finalColor = float3(0.1f, 0.1f, 0.12f);
    }

    return float4(finalColor, 1.0f);
}