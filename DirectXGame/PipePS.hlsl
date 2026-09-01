struct VSOutput
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD0;
    nointerpolation int isPowered : TEXCOORD1;
    nointerpolation int mask : MASK;
    nointerpolation int isGoal : IS_GOAL;
};

static const int DIR_UP = 1;
static const int DIR_RIGHT = 2;
static const int DIR_DOWN = 4;
static const int DIR_LEFT = 8;

float4 main(VSOutput input) : SV_TARGET
{
    float2 p = input.uv - float2(0.5f, 0.5f);
    float width = 0.15f; // パイプの太さ

    bool draw = false;

    // 中央交差点
    if (abs(p.x) <= width && abs(p.y) <= width)
    {
        draw = true;
    }
    // 上方向
    if ((input.mask & DIR_UP) && p.y < -width && abs(p.x) <= width)
    {
        draw = true;
    }
    // 右方向
    if ((input.mask & DIR_RIGHT) && p.x > width && abs(p.y) <= width)
    {
        draw = true;
    }
    // 下方向
    if ((input.mask & DIR_DOWN) && p.y > width && abs(p.x) <= width)
    {
        draw = true;
    }
    // 左方向
    if ((input.mask & DIR_LEFT) && p.x < -width && abs(p.y) <= width)
    {
        draw = true;
    }

    if (!draw)
    {
        discard;
    }

    // ★ 色の設定
    float4 color = float4(0.3f, 0.3f, 0.3f, 1.0f); // 未通電（暗いグレー）

    if (input.isGoal == 1)
    {
        // ゴール地点の色（通電前：黄色 / 通電時：赤ピンク）
        if (input.isPowered == 1)
        {
            color = float4(1.0f, 0.1f, 0.4f, 1.0f);
        }
        else
        {
            color = float4(1.0f, 0.8f, 0.0f, 1.0f);
        }
    }
    else if (input.isPowered == 1)
    {
        // 通常マスの通電色（水色）
        color = float4(0.0f, 0.9f, 1.0f, 1.0f);
    }

    return color;
}