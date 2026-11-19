#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <cmath>
#include <string>
#include <vector>

// Math & Color Helpers (Matching Three.js Conventions)
static Color HexColor(unsigned int hex, float alpha = 1.0f) {
    return Color{
        static_cast<unsigned char>((hex >> 16) & 0xFF),
        static_cast<unsigned char>((hex >> 8) & 0xFF),
        static_cast<unsigned char>(hex & 0xFF),
        static_cast<unsigned char>(Clamp(alpha, 0.0f, 1.0f) * 255.0f)
    };
}

// Three.js Euler(x, y, z, 'XYZ') intrinsic rotation -> Quaternion
static Quaternion QuaternionFromEulerXYZ(float x, float y, float z) {
    float c1 = cosf(x * 0.5f), s1 = sinf(x * 0.5f);
    float c2 = cosf(y * 0.5f), s2 = sinf(y * 0.5f);
    float c3 = cosf(z * 0.5f), s3 = sinf(z * 0.5f);
    return Quaternion{
        s1 * c2 * c3 + c1 * s2 * s3,
        c1 * s2 * c3 - s1 * c2 * s3,
        c1 * c2 * s3 + s1 * s2 * c3,
        c1 * c2 * c3 - s1 * s2 * s3
    };
}

// Multiply a 3D point by a 4x4 Matrix
static Vector3 TransformPoint(Vector3 v, const Matrix& m) {
    return Vector3Transform(v, m);
}

// Transform a normal vector by the upper-left 3x3 of a matrix and normalize
static Vector3 TransformNormal(Vector3 n, const Matrix& normalMat) {
    Vector3 res = {
        normalMat.m0 * n.x + normalMat.m4 * n.y + normalMat.m8  * n.z,
        normalMat.m1 * n.x + normalMat.m5 * n.y + normalMat.m9  * n.z,
        normalMat.m2 * n.x + normalMat.m6 * n.y + normalMat.m10 * n.z
    };
    return Vector3Normalize(res);
}

// Apply Three.js FogExp2(0x0f172a, 0.05) + Dual Directional & Specular Lighting
static Color ComputeLitFogColor(
    Vector3 worldPos,
    Vector3 worldNormal,
    Vector3 camPos,
    Color baseColor,
    bool twoSided,
    bool unlit = false
) {
    static const Color fogColor = HexColor(0x0f172a, 1.0f);
    static const Vector3 light1 = Vector3Normalize({ 5.0f,  5.0f,  5.0f });
    static const Vector3 light2 = Vector3Normalize({-5.0f, -5.0f, -5.0f });
    float dist = Vector3Distance(camPos, worldPos);
    float fogFactor = expf(-0.05f * dist * 0.05f * dist);
    fogFactor = Clamp(fogFactor, 0.0f, 1.0f);
    float r = baseColor.r / 255.0f;
    float g = baseColor.g / 255.0f;
    float b = baseColor.b / 255.0f;

    if (!unlit) {
        Vector3 V = Vector3Normalize(Vector3Subtract(camPos, worldPos));
        Vector3 N = worldNormal;
        if (twoSided && Vector3DotProduct(N, V) < 0.0f) {
            N = Vector3Negate(N);
        }

        float diff1 = fmaxf(Vector3DotProduct(N, light1), 0.0f) * 1.1f;
        float diff2 = fmaxf(Vector3DotProduct(N, light2), 0.0f) * 0.4f;
        float ambient = 0.4f;

        // Blinn-Phong specular highlight to emulate MeshPhysicalMaterial roughness 0.1
        Vector3 H1 = Vector3Normalize(Vector3Add(light1, V));
        float spec = powf(fmaxf(Vector3DotProduct(N, H1), 0.0f), 64.0f) * 0.45f;
        float light = ambient + diff1 + diff2;
        r = fminf(r * light + spec, 1.0f);
        g = fminf(g * light + spec, 1.0f);
        b = fminf(b * light + spec, 1.0f);
    }

    float fr = fogColor.r / 255.0f;
    float fg = fogColor.g / 255.0f;
    float fb = fogColor.b / 255.0f;
    
    r = fr + (r - fr) * fogFactor;
    g = fg + (g - fg) * fogFactor;
    b = fb + (b - fb) * fogFactor;

    return Color{
        static_cast<unsigned char>(Clamp(r, 0.0f, 1.0f) * 255.0f),
        static_cast<unsigned char>(Clamp(g, 0.0f, 1.0f) * 255.0f),
        static_cast<unsigned char>(Clamp(b, 0.0f, 1.0f) * 255.0f),
        baseColor.a
    };
}

// Scene Geometry Renderers under Composite Transform M = U * Sigma * V^T
static void DrawDeformedSphereAndWireframe(
    const Matrix& M,
    const Matrix& normalMat,
    Vector3 camPos,
    int rings = 32,
    int slices = 32
) {
    Color sphereBase = HexColor(0x0ea5e9, 0.35f);
    Color wireBase   = HexColor(0x38bdf8, 0.60f);

    // Sphere topology is constant while the transformation changes every frame.
    // Cache the local-space points so trigonometric functions run only when the
    // requested resolution changes.
    static std::vector<Vector3> cachedPoints;
    static int cachedRings = 0;
    static int cachedSlices = 0;
    if (cachedRings != rings || cachedSlices != slices) {
        cachedPoints.resize(static_cast<size_t>(rings + 1) * static_cast<size_t>(slices + 1));
        for (int i = 0; i <= rings; ++i) {
            float phi = PI * static_cast<float>(i) / static_cast<float>(rings);
            float sinPhi = sinf(phi);
            float cosPhi = cosf(phi);
            for (int j = 0; j <= slices; ++j) {
                float theta = 2.0f * PI * static_cast<float>(j) / static_cast<float>(slices);
                cachedPoints[static_cast<size_t>(i) * static_cast<size_t>(slices + 1) + static_cast<size_t>(j)] = {
                    sinPhi * cosf(theta),
                    cosPhi,
                    sinPhi * sinf(theta)
                };
            }
        }
        cachedRings = rings;
        cachedSlices = slices;
    }

    auto spherePoint = [&](int i, int j) -> const Vector3& {
        return cachedPoints[
            static_cast<size_t>(i) * static_cast<size_t>(slices + 1) +
            static_cast<size_t>(j)
        ];
    };

    //Two-pass translucent rendering (back faces then front faces) for clean alpha
    rlDisableDepthMask();
    rlDisableBackfaceCulling();
    rlBegin(RL_TRIANGLES);
    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < slices; ++j) {
            Vector3 n00 = spherePoint(i, j);
            Vector3 n10 = spherePoint(i + 1, j);
            Vector3 n11 = spherePoint(i + 1, j + 1);
            Vector3 n01 = spherePoint(i, j + 1);
            Vector3 p00 = TransformPoint(n00, M);
            Vector3 p10 = TransformPoint(n10, M);
            Vector3 p11 = TransformPoint(n11, M);
            Vector3 p01 = TransformPoint(n01, M);
            Vector3 wn00 = TransformNormal(n00, normalMat);
            Vector3 wn10 = TransformNormal(n10, normalMat);
            Vector3 wn11 = TransformNormal(n11, normalMat);
            Vector3 wn01 = TransformNormal(n01, normalMat);
            Color c00 = ComputeLitFogColor(p00, wn00, camPos, sphereBase, true, false);
            Color c10 = ComputeLitFogColor(p10, wn10, camPos, sphereBase, true, false);
            Color c11 = ComputeLitFogColor(p11, wn11, camPos, sphereBase, true, false);
            Color c01 = ComputeLitFogColor(p01, wn01, camPos, sphereBase, true, false);
            rlColor4ub(c00.r, c00.g, c00.b, c00.a); rlVertex3f(p00.x, p00.y, p00.z);
            rlColor4ub(c10.r, c10.g, c10.b, c10.a); rlVertex3f(p10.x, p10.y, p10.z);
            rlColor4ub(c11.r, c11.g, c11.b, c11.a); rlVertex3f(p11.x, p11.y, p11.z);
            rlColor4ub(c00.r, c00.g, c00.b, c00.a); rlVertex3f(p00.x, p00.y, p00.z);
            rlColor4ub(c11.r, c11.g, c11.b, c11.a); rlVertex3f(p11.x, p11.y, p11.z);
            rlColor4ub(c01.r, c01.g, c01.b, c01.a); rlVertex3f(p01.x, p01.y, p01.z);
        }
    }
    rlEnd();

    // Wireframe overlay
    rlBegin(RL_LINES);
    for (int i = 0; i < rings; ++i) {
        for (int j = 0; j < slices; ++j) {
            Vector3 p00 = TransformPoint(spherePoint(i, j), M);
            Vector3 p10 = TransformPoint(spherePoint(i + 1, j), M);
            Vector3 p01 = TransformPoint(spherePoint(i, j + 1), M);
            Color c00 = ComputeLitFogColor(p00, {0, 1, 0}, camPos, wireBase, false, true);
            Color c10 = ComputeLitFogColor(p10, {0, 1, 0}, camPos, wireBase, false, true);
            Color c01 = ComputeLitFogColor(p01, {0, 1, 0}, camPos, wireBase, false, true);
            rlColor4ub(c00.r, c00.g, c00.b, c00.a); rlVertex3f(p00.x, p00.y, p00.z);
            rlColor4ub(c10.r, c10.g, c10.b, c10.a); rlVertex3f(p10.x, p10.y, p10.z);
            rlColor4ub(c00.r, c00.g, c00.b, c00.a); rlVertex3f(p00.x, p00.y, p00.z);
            rlColor4ub(c01.r, c01.g, c01.b, c01.a); rlVertex3f(p01.x, p01.y, p01.z);
        }
    }
    rlEnd();
    rlEnableDepthMask();
}

static void DrawBasisAxis(const Matrix& M, Vector3 dir, unsigned int hexColor, Vector3 camPos) {
    Color baseCol = HexColor(hexColor, 1.0f);
    // Shaft from (0,0,0) to 1.5 * dir
    Vector3 startLocal = { 0.0f, 0.0f, 0.0f };
    Vector3 endLocal   = Vector3Scale(dir, 1.5f);
    Vector3 startW     = TransformPoint(startLocal, M);
    Vector3 endW       = TransformPoint(endLocal, M);
    Color cStart = ComputeLitFogColor(startW, {0, 1, 0}, camPos, baseCol, false, true);
    Color cEnd   = ComputeLitFogColor(endW,   {0, 1, 0}, camPos, baseCol, false, true);
    rlBegin(RL_LINES);
    rlColor4ub(cStart.r, cStart.g, cStart.b, cStart.a); rlVertex3f(startW.x, startW.y, startW.z);
    rlColor4ub(cEnd.r,   cEnd.g,   cEnd.b,   cEnd.a);   rlVertex3f(endW.x,   endW.y,   endW.z);
    rlEnd();
    // ConeGeometry(0.06, 0.2, 16) centered at 1.5 * dir oriented along dir
    const float radius = 0.06f;
    const float height = 0.20f;
    const int segments = 16;
    // Build orthonormal basis (u, dir, w) in local V-space
    Vector3 upRef = (fabsf(dir.y) < 0.99f) ? Vector3{0.0f, 1.0f, 0.0f} : Vector3{1.0f, 0.0f, 0.0f};
    Vector3 u = Vector3Normalize(Vector3CrossProduct(upRef, dir));
    Vector3 w = Vector3CrossProduct(dir, u);
    Vector3 tipLocal  = Vector3Add(endLocal, Vector3Scale(dir,  height * 0.5f));
    Vector3 baseCenterLocal = Vector3Add(endLocal, Vector3Scale(dir, -height * 0.5f));
    Vector3 tipW  = TransformPoint(tipLocal, M);
    Vector3 baseW = TransformPoint(baseCenterLocal, M);
    Color cTip    = ComputeLitFogColor(tipW, {0, 1, 0}, camPos, baseCol, false, true);
    rlDisableBackfaceCulling();
    rlBegin(RL_TRIANGLES);
    
    for (int i = 0; i < segments; ++i) {
        float a0 = 2.0f * PI * static_cast<float>(i) / segments;
        float a1 = 2.0f * PI * static_cast<float>(i + 1) / segments;
        
        Vector3 r0 = Vector3Add(baseCenterLocal,
            Vector3Add(Vector3Scale(u, radius * cosf(a0)), Vector3Scale(w, radius * sinf(a0))));
        Vector3 r1 = Vector3Add(baseCenterLocal,
            Vector3Add(Vector3Scale(u, radius * cosf(a1)), Vector3Scale(w, radius * sinf(a1))));
        
        Vector3 r0W = TransformPoint(r0, M);
        Vector3 r1W = TransformPoint(r1, M);
        Color cr0 = ComputeLitFogColor(r0W, {0, 1, 0}, camPos, baseCol, false, true);
        Color cr1 = ComputeLitFogColor(r1W, {0, 1, 0}, camPos, baseCol, false, true);

        // Cone side triangle
        rlColor4ub(cTip.r, cTip.g, cTip.b, cTip.a); rlVertex3f(tipW.x, tipW.y, tipW.z);
        rlColor4ub(cr0.r,  cr0.g,  cr0.b,  cr0.a);  rlVertex3f(r0W.x,  r0W.y,  r0W.z);
        rlColor4ub(cr1.r,  cr1.g,  cr1.b,  cr1.a);  rlVertex3f(r1W.x,  r1W.y,  r1W.z);

        // Cone base cap
        rlColor4ub(cEnd.r, cEnd.g, cEnd.b, cEnd.a); rlVertex3f(baseW.x, baseW.y, baseW.z);
        rlColor4ub(cr1.r,  cr1.g,  cr1.b,  cr1.a);  rlVertex3f(r1W.x,   r1W.y,   r1W.z);
        rlColor4ub(cr0.r,  cr0.g,  cr0.b,  cr0.a);  rlVertex3f(r0W.x,   r0W.y,   r0W.z);
    }
    rlEnd();
}

static void DrawGridHelper3D(Vector3 camPos) {
    // GridHelper(10, 10, 0x475569, 0x1e293b) at y = -2
    const float y = -2.0f;
    const float halfSize = 5.0f;
    Color centerCol = HexColor(0x475569, 1.0f);
    Color gridCol   = HexColor(0x1e293b, 1.0f);

    rlBegin(RL_LINES);
    for (int i = -5; i <= 5; ++i) {
        Color base = (i == 0) ? centerCol : gridCol;
        float pos = static_cast<float>(i);
        // Subdivide lines slightly so exponential fog gradients look smooth
        const int segs = 20;
        for (int s = 0; s < segs; ++s) {
            float t0 = -halfSize + (2.0f * halfSize * s) / segs;
            float t1 = -halfSize + (2.0f * halfSize * (s + 1)) / segs;

            Vector3 pA0 = { pos, y, t0 }, pA1 = { pos, y, t1 };
            Color cA0 = ComputeLitFogColor(pA0, {0, 1, 0}, camPos, base, false, true);
            Color cA1 = ComputeLitFogColor(pA1, {0, 1, 0}, camPos, base, false, true);
            rlColor4ub(cA0.r, cA0.g, cA0.b, cA0.a); rlVertex3f(pA0.x, pA0.y, pA0.z);
            rlColor4ub(cA1.r, cA1.g, cA1.b, cA1.a); rlVertex3f(pA1.x, pA1.y, pA1.z);

            Vector3 pB0 = { t0, y, pos }, pB1 = { t1, y, pos };
            Color cB0 = ComputeLitFogColor(pB0, {0, 1, 0}, camPos, base, false, true);
            Color cB1 = ComputeLitFogColor(pB1, {0, 1, 0}, camPos, base, false, true);
            rlColor4ub(cB0.r, cB0.g, cB0.b, cB0.a); rlVertex3f(pB0.x, pB0.y, pB0.z);
            rlColor4ub(cB1.r, cB1.g, cB1.b, cB1.a); rlVertex3f(pB1.x, pB1.y, pB1.z);
        }
    }
    rlEnd();
}

// 2D UI Helpers
static bool DrawStyledButton(Rectangle bounds, const char* label, Color bgNormal, Color bgHover) {
    Vector2 mouse = GetMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, bounds);
    bool clicked = hovered && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);

    DrawRectangleRounded(bounds, 0.35f, 8, hovered ? bgHover : bgNormal);
    DrawRectangleRoundedLinesEx(bounds, 0.35f, 8, 1.0f, HexColor(0xffffff, 0.12f));

    int fontSize = 16;
    int textW = MeasureText(label, fontSize);
    DrawText(
        label,
        static_cast<int>(bounds.x + (bounds.width - textW) * 0.5f),
        static_cast<int>(bounds.y + (bounds.height - fontSize) * 0.5f),
        fontSize,
        WHITE
    );
    return clicked;
}

static int DrawWrappedText(const std::string& text, int x, int y, int maxWidth, int fontSize, int lineSpacing, Color color) {
    std::string word;
    std::string line;
    int curY = y;

    for (size_t i = 0; i <= text.size(); ++i) {
        char c = (i < text.size()) ? text[i] : ' ';
        if (c == ' ' || c == '\n') {
            std::string testLine = line.empty() ? word : (line + " " + word);
            if (MeasureText(testLine.c_str(), fontSize) > maxWidth && !line.empty()) {
                DrawText(line.c_str(), x, curY, fontSize, color);
                curY += fontSize + lineSpacing;
                line = word;
            } else {
                line = testLine;
            }
            word.clear();
            if (c == '\n') {
                DrawText(line.c_str(), x, curY, fontSize, color);
                curY += fontSize + lineSpacing;
                line.clear();
            }
        } else {
            word += c;
        }
    }
    if (!line.empty()) {
        DrawText(line.c_str(), x, curY, fontSize, color);
        curY += fontSize + lineSpacing;
    }
    return curY - y;
}

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(1280, 800, "Singular Value Decomposition Geometric Simulation");
    SetTargetFPS(60);
    
    Vector3 camTarget = { 0.0f, 0.0f, 0.0f };
    float radius = sqrtf(4.0f * 4.0f + 3.0f * 3.0f + 5.0f * 5.0f);
    float theta  = atan2f(4.0f, 5.0f);
    float phi    = acosf(3.0f / radius);

    float targetRadius = radius;
    float targetTheta  = theta;
    float targetPhi    = phi;
    bool isDraggingOrbit = false;

    Camera3D camera = { 0 };
    camera.up = { 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // SVD Transformation Targets
    const Quaternion qIdentity = QuaternionIdentity();
    const Vector3    sIdentity = { 1.0f, 1.0f, 1.0f };

    const Quaternion qV     = QuaternionFromEulerXYZ(PI / 4.0f, PI / 3.0f, 0.0f);
    const Vector3    sSigma = { 2.5f, 1.2f, 0.4f };
    const Quaternion qU     = QuaternionFromEulerXYZ(-PI / 6.0f, -PI / 4.0f, PI / 6.0f);

    Quaternion curV     = qIdentity, targetV     = qIdentity;
    Vector3    curSigma = sIdentity, targetSigma = sIdentity;
    Quaternion curU     = qIdentity, targetU     = qIdentity;

    struct StateText {
        const char* heading;
        const char* body;
    };

    const StateText stateTexts[4] = {
        {
            "State: Initial",
            "A perfect unit sphere representing data/space before transformation. "
            "The colored arrows represent the orthogonal basis vectors (X: Red, Y: Green, Z: Blue)."
        },
        {
            "Step 1: Apply V^T",
            "The orthogonal matrix V^T performs a pure rotation. The sphere remains a sphere, "
            "and the basis vectors rotate. V^T aligns the geometry so that the principal axes "
            "match the global coordinate axes, preparing it for scaling."
        },
        {
            "Step 2: Apply Sigma",
            "The diagonal matrix Sigma applies pure scaling. Notice how the sphere deforms into "
            "an ellipsoid. It stretches strongly along the X-axis (sigma_1 = 2.5), slightly along Y "
            "(sigma_2 = 1.2), and compresses along Z (sigma_3 = 0.4). This reveals the system's "
            "principal gains!"
        },
        {
            "Step 3: Apply U",
            "The orthogonal matrix U performs the final rotation. It takes the deformed ellipsoid "
            "and orients it into its final spatial position. This completes the transformation "
            "A = U * Sigma * V^T."
        }
    };

    int currentStep = 0;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        float lerpAlpha = 1.0f - powf(1.0f - 0.04f, dt * 60.0f);

        // UI Panel bounds (to prevent OrbitControls dragging when clicking buttons)
        Rectangle panelRect = { 24.0f, 24.0f, 320.0f, 440.0f };
        Vector2 mouse = GetMousePosition();
        bool pointerOverUI = CheckCollisionPointRec(mouse, panelRect);

        // OrbitControls Input & Damping (dampingFactor = 0.05)
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && !pointerOverUI) {
            isDraggingOrbit = true;
        }
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            isDraggingOrbit = false;
        }
        if (isDraggingOrbit) {
            Vector2 delta = GetMouseDelta();
            targetTheta -= delta.x * 0.007f;
            targetPhi   -= delta.y * 0.007f;
            targetPhi    = Clamp(targetPhi, 0.05f, PI - 0.05f);
        }
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f && !pointerOverUI) {
            targetRadius = Clamp(targetRadius - wheel * 0.6f, 2.0f, 25.0f);
        }

        float orbitDamp = 1.0f - powf(1.0f - 0.05f, dt * 60.0f);
        theta  += (targetTheta  - theta)  * orbitDamp;
        phi    += (targetPhi    - phi)    * orbitDamp;
        radius += (targetRadius - radius) * orbitDamp;

        camera.position = {
            camTarget.x + radius * sinf(phi) * sinf(theta),
            camTarget.y + radius * cosf(phi),
            camTarget.z + radius * sinf(phi) * cosf(theta)
        };
        camera.target = camTarget;

        // Keyboard shortcuts (0/R = Reset, 1 = V^T, 2 = Sigma, 3 = U)
        if (IsKeyPressed(KEY_ZERO) || IsKeyPressed(KEY_R)) {
            currentStep = 0; targetV = qIdentity; targetSigma = sIdentity; targetU = qIdentity;
        } else if (IsKeyPressed(KEY_ONE)) {
            currentStep = 1; targetV = qV; targetSigma = sIdentity; targetU = qIdentity;
        } else if (IsKeyPressed(KEY_TWO)) {
            currentStep = 2; targetV = qV; targetSigma = sSigma; targetU = qIdentity;
        } else if (IsKeyPressed(KEY_THREE)) {
            currentStep = 3; targetV = qV; targetSigma = sSigma; targetU = qU;
        }

        // Interpolate groupV.quaternion, groupSigma.scale, groupU.quaternion
        curV     = QuaternionSlerp(curV, targetV, lerpAlpha);
        curSigma = Vector3Lerp(curSigma, targetSigma, lerpAlpha);
        curU     = QuaternionSlerp(curU, targetU, lerpAlpha);

        // Build hierarchical matrix M = R(curU) * S(curSigma) * R(curV)
        Matrix matU     = QuaternionToMatrix(curU);
        Matrix matSigma = MatrixScale(curSigma.x, curSigma.y, curSigma.z);
        Matrix matV     = QuaternionToMatrix(curV);

        // Note: Raylib MatrixMultiply(left, right) computes right * left in standard math notation,
        // so to apply V first, then Sigma, then U to column vectors:
        Matrix matUSigma = MatrixMultiply(matSigma, matU);
        Matrix M         = MatrixMultiply(matV, matUSigma);

        // Normal matrix = transpose(inverse(M)) for accurate lighting under non-uniform scaling
        Matrix normalMat = MatrixTranspose(MatrixInvert(M));

        
        // RENDER 3D SCENE!

        
        BeginDrawing();
        ClearBackground(HexColor(0x0f172a, 1.0f));

        BeginMode3D(camera);
            DrawGridHelper3D(camera.position);

            // Draw basis axes and unit sphere transformed by M = U * Sigma * V^T
            DrawBasisAxis(M, { 1.0f, 0.0f, 0.0f }, 0xff4444, camera.position);
            DrawBasisAxis(M, { 0.0f, 1.0f, 0.0f }, 0x44ff44, camera.position);
            DrawBasisAxis(M, { 0.0f, 0.0f, 1.0f }, 0x4444ff, camera.position);

            DrawDeformedSphereAndWireframe(M, normalMat, camera.position, 32, 32);
        EndMode3D();

        DrawRectangleRounded(panelRect, 0.06f, 10, HexColor(0x1e293b, 0.85f));
        DrawRectangleRoundedLinesEx(panelRect, 0.06f, 10, 1.0f, HexColor(0x334155, 0.7f));

        DrawText("SVD Decomposition", 48, 44, 24, HexColor(0x38bdf8, 1.0f));

        float btnX = 48.0f;
        float btnW = 272.0f;
        float btnH = 38.0f;
        float btnY = 86.0f;
        float gap  = 10.0f;

        if (DrawStyledButton({ btnX, btnY, btnW, btnH }, "Reset Sphere",
                             HexColor(0x334155), HexColor(0x475569))) {
            currentStep = 0;
            targetV = qIdentity; targetSigma = sIdentity; targetU = qIdentity;
        }
        btnY += btnH + gap;

        if (DrawStyledButton({ btnX, btnY, btnW, btnH }, "Step 1: Apply V^T (Rotation)",
                             HexColor(0x4f46e5), HexColor(0x6366f1))) {
            currentStep = 1;
            targetV = qV; targetSigma = sIdentity; targetU = qIdentity;
        }
        btnY += btnH + gap;

        if (DrawStyledButton({ btnX, btnY, btnW, btnH }, "Step 2: Apply Sigma (Deformation)",
                             HexColor(0x9333ea), HexColor(0xa855f7))) {
            currentStep = 2;
            targetV = qV; targetSigma = sSigma; targetU = qIdentity;
        }
        btnY += btnH + gap;

        if (DrawStyledButton({ btnX, btnY, btnW, btnH }, "Step 3: Apply U (Final Rotation)",
                             HexColor(0x059669), HexColor(0x10b981))) {
            currentStep = 3;
            targetV = qV; targetSigma = sSigma; targetU = qU;
        }
        btnY += btnH + 18.0f;

        Rectangle explRect = { btnX, btnY, btnW, 158.0f };
        DrawRectangleRounded(explRect, 0.08f, 8, HexColor(0x0f172a, 0.75f));
        DrawRectangleRoundedLinesEx(explRect, 0.08f, 8, 1.0f, HexColor(0x334155, 1.0f));

        DrawText(stateTexts[currentStep].heading,
                 static_cast<int>(explRect.x + 14),
                 static_cast<int>(explRect.y + 12),
                 15, WHITE);

        DrawWrappedText(stateTexts[currentStep].body,
                        static_cast<int>(explRect.x + 14),
                        static_cast<int>(explRect.y + 34),
                        static_cast<int>(explRect.width - 28),
                        14, 5, HexColor(0xcbD5e1, 1.0f));

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
