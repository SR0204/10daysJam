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

    // ★ 1. マスのグリッド枠線の判定 (UVのフチから2%の位置に線を引く)
    float gridThickness = 0.02f;
    bool isGridBorder = (input.uv.x < gridThickness || input.uv.x > (1.0f - gridThickness) ||
                         input.uv.y < gridThickness || input.uv.y > (1.0f - gridThickness));

    // ★ 2. パイプの形状判定
    bool isPipe = false;

    if (abs(p.x) <= width && abs(p.y) <= width)
        isPipe = true;
    if ((input.mask & DIR_UP) && p.y <= 0.0f && abs(p.x) <= width)
        isPipe = true;
    if ((input.mask & DIR_RIGHT) && p.x >= 0.0f && abs(p.y) <= width)
        isPipe = true;
    if ((input.mask & DIR_DOWN) && p.y >= 0.0f && abs(p.x) <= width)
        isPipe = true;
    if ((input.mask & DIR_LEFT) && p.x <= 0.0f && abs(p.y) <= width)
        isPipe = true;

    // ★ 3. 背景色とグリッド色
    float4 bgColor = float4(0.35f, 0.35f, 0.4f, 1.0f); // 基本のグレー背景
    float4 gridColor = float4(0.2f, 0.2f, 0.25f, 1.0f); // 枠線の濃いグレー

    if (input.isGoal == 1)
    {
        bgColor = (input.isPowered == 1) ? float4(1.0f, 0.1f, 0.4f, 1.0f) : float4(1.0f, 0.8f, 0.0f, 1.0f);
    }

    // パイプの色
    float4 pipeColor = (input.isPowered == 1) ? float4(0.0f, 0.9f, 1.0f, 1.0f) : float4(0.75f, 0.75f, 0.75f, 1.0f);

    // 描画の優先度: パイプ > グリッド線 > 背景
    if (isPipe)
    {
        return pipeColor;
    }
    else if (isGridBorder)
    {
        return gridColor;
    }

    return bgColor;
}