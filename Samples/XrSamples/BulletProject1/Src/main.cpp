// ============================================================================
// ストラックアウト VR アプリケーション (Bullet Physics 学習用・最適化版)
// ============================================================================
#define _HAS_STD_BYTE 0
#define NOMINMAX

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <vector>
#include <memory>
#include <fstream> // ファイル読み込み用
#include <openxr/openxr.h>
#include <sstream>

#include "XrApp.h"
#include <btBulletDynamicsCommon.h> // Bullet Physics の基本ヘッダー

#include "Input/ControllerRenderer.h"
#include "Input/TinyUI.h"
#include "Render/SimpleBeamRenderer.h"
#include "Render/GeometryRenderer.h"
#include "Render/GeometryBuilder.h"
#include "Render/SimpleGlbRenderer.h"

// ----------------------------------------------------------------------------
// 各種オブジェクトを管理するための構造体
// 物理モデル(btRigidBody)と描画モデル(Renderer)を紐付けて管理します。
// ----------------------------------------------------------------------------

// 投げるボールを管理する構造体
struct BallItem {
    btRigidBody* body;                         // 物理演算用の剛体
    btCollisionShape* shape;                   // 衝突判定の形状（コライダー）
    OVRFW::SimpleGlbRenderer* glbRenderer;     // GLBモデル（本命の描画）
    OVRFW::GeometryRenderer* fallbackRenderer; // 代替モデル（読み込み失敗時）
    float lifeTime;                            // ボールの生存時間（自動削除用）
};

// ストラックアウトの的（パネル）を管理する構造体
struct PanelItem {
    btRigidBody* body;
    btCollisionShape* shape;
    btHingeConstraint* hinge;                  // 蝶番（ヒンジ）のように動かすための拘束（ジョイント）
    OVRFW::SimpleGlbRenderer* glbRenderer;
    OVRFW::GeometryRenderer* fallbackRenderer;
    bool isKnockedDown;                        // 倒れたかどうかのフラグ
    btVector3 initialPos;                      // リセット用に初期位置を保持
};

// パネルを支える外枠を管理する構造体
struct FrameItem {
    btRigidBody* body;
    btCollisionShape* shape;
    OVRFW::GeometryRenderer* renderer;
};

// 装飾用の木を管理する構造体（物理判定を持たず、描画のみ行う）
struct TreeItem {
    OVRFW::SimpleGlbRenderer* glbRenderer;
    OVRFW::GeometryRenderer* fallbackRenderer;
    OVR::Posef initialPose;                    // 配置する座標
};

// ============================================================================
// メインアプリケーションクラス
// ============================================================================
class XrControllersApp : public OVRFW::XrApp {
public:
    XrControllersApp() : OVRFW::XrApp() {
        BackgroundColor = OVR::Vector4f(0.6f, 0.8f, 1.0f, 1.0f); // 空色
        OpenXRVersion = XR_API_VERSION_1_0;
    }

    // --- バイナリファイルを読み込むヘルパー関数（GLBモデル読み込み用） ---
    std::vector<uint8_t> ReadFileBuffer(const std::string& path) {
        std::vector<uint8_t> buffer;
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (file) {
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            buffer.resize(size);
            file.read((char*)buffer.data(), size);
        }
        return buffer;
    }

    virtual bool AppInit(const xrJava* context) override {
        if (!ui_.Init(context, GetFileSys())) return false;
        // スコア表示用UIの初期化
        scoreLabel_ = ui_.AddLabel("SCORE: 0", { 0.0f, 2.0f, -4.0f }, { 800.0f, 200.0f });
        return true;
    }

    // ------------------------------------------------------------------------
    // [Bullet] クリーンアップ（メモリ解放）処理
    // ------------------------------------------------------------------------
    virtual void AppShutdown(const xrJava* context) override {
        floorRenderer_.Shutdown();

        // --- 描画モデルの解放 ---
        if (basketGlbRenderer_) { basketGlbRenderer_->Shutdown(); delete basketGlbRenderer_; basketGlbRenderer_ = nullptr; }
        if (basketFallbackRenderer_) { basketFallbackRenderer_->Shutdown(); delete basketFallbackRenderer_; basketFallbackRenderer_ = nullptr; }

        for (auto& b : balls_) {
            if (b.glbRenderer) { b.glbRenderer->Shutdown(); delete b.glbRenderer; }
            if (b.fallbackRenderer) { b.fallbackRenderer->Shutdown(); delete b.fallbackRenderer; }
        }
        for (auto& p : panels_) {
            if (p.glbRenderer) { p.glbRenderer->Shutdown(); delete p.glbRenderer; }
            if (p.fallbackRenderer) { p.fallbackRenderer->Shutdown(); delete p.fallbackRenderer; }
        }
        for (auto& f : frames_) { f.renderer->Shutdown(); delete f.renderer; }
        for (auto& t : trees_) {
            if (t.glbRenderer) { t.glbRenderer->Shutdown(); delete t.glbRenderer; }
            if (t.fallbackRenderer) { t.fallbackRenderer->Shutdown(); delete t.fallbackRenderer; }
        }

        OVRFW::XrApp::AppShutdown(context);
        ui_.Shutdown();
    }

    // 代替の箱モデルを生成するヘルパー関数
    OVRFW::GeometryRenderer* CreateBoxRenderer(OVR::Vector3f scale, OVR::Vector4f color) {
        OVRFW::GeometryRenderer* renderer = new OVRFW::GeometryRenderer();
        OVRFW::GeometryBuilder builder;
        builder.Add(OVRFW::BuildUnitCubeDescriptor(), OVRFW::GeometryBuilder::kInvalidIndex, color);
        renderer->ChannelControl = OVR::Vector4f(1, 1, 1, 1);
        renderer->Init(builder.ToGeometryDescriptor());
        renderer->SetScale(scale);
        return renderer;
    }

    // キネマティック剛体（重力の影響を受けず、プログラムから座標を強制指定する剛体＝プレイヤーの手）を生成
    btRigidBody* CreateKinematicBody(btCollisionShape* shape) {
        btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, new btDefaultMotionState(), shape, btVector3(0, 0, 0));
        btRigidBody* body = new btRigidBody(rbInfo);
        // キネマティックオブジェクトとしてフラグを設定
        body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        // スリープ（計算の最適化による停止）を無効化し、常に判定を取り続ける
        body->setActivationState(DISABLE_DEACTIVATION);
        return body;
    }

    // ------------------------------------------------------------------------
    // [Bullet] 物理ワールドの構築と初期化
    // ------------------------------------------------------------------------
    virtual bool SessionInit() override {
        CurrentSpace = LocalSpace;
        if (!controllerRenderL_.Init(true) || !controllerRenderR_.Init(false)) return false;
        beamRenderer_.Init(GetFileSys(), nullptr, OVR::Vector4f(1.0f), 1.0f);

        // --- 1. 物理演算の基本セットアップ ---
        collisionConfiguration_ = new btDefaultCollisionConfiguration();
        dispatcher_ = new btCollisionDispatcher(collisionConfiguration_);
        overlappingPairCache_ = new btDbvtBroadphase();
        solver_ = new btSequentialImpulseConstraintSolver();
        dynamicsWorld_ = new btDiscreteDynamicsWorld(dispatcher_, overlappingPairCache_, solver_, collisionConfiguration_);
        dynamicsWorld_->setGravity(btVector3(0.0f, -9.8f, 0.0f));

        // --- 2. 床（地面）の生成 ---
        groundShape_ = new btStaticPlaneShape(btVector3(0, 1, 0), 0.0f);
        btTransform groundTrans; groundTrans.setIdentity(); groundTrans.setOrigin(btVector3(0, -1.5f, 0));

        groundRigidBody_ = new btRigidBody(0.0f, new btDefaultMotionState(groundTrans), groundShape_, btVector3(0, 0, 0));
        groundRigidBody_->setRestitution(0.6f);
        groundRigidBody_->setFriction(0.8f);
        dynamicsWorld_->addRigidBody(groundRigidBody_);

        // 描画用の床
        OVRFW::GeometryBuilder floorBuilder;
        floorBuilder.Add(OVRFW::BuildUnitCubeDescriptor(), OVRFW::GeometryBuilder::kInvalidIndex, { 0.2f, 0.8f, 0.2f, 1.0f });
        floorRenderer_.ChannelControl = OVR::Vector4f(1, 1, 1, 1);
        floorRenderer_.Init(floorBuilder.ToGeometryDescriptor());
        floorRenderer_.SetScale({ 20.0f, 0.1f, 20.0f });

        // --- 3. プレイヤーの手の生成 ---
        handShape_ = new btSphereShape(0.1f); // 半径10cmの球体判定
        leftHandRb_ = CreateKinematicBody(handShape_);
        rightHandRb_ = CreateKinematicBody(handShape_);
        dynamicsWorld_->addRigidBody(leftHandRb_);
        dynamicsWorld_->addRigidBody(rightHandRb_);

        // --- 4. バスケット（ボール湧きエリア）の生成 ---
        spawnAreaPos_ = { 0.6f, 0.0f, -0.5f };
        basketGlbBuffer_ = ReadFileBuffer("assets/basket.glb");
        if (basketGlbBuffer_.empty()) basketGlbBuffer_ = ReadFileBuffer("../../../../XrSamples/BulletProject1/assets/basket.glb");
        if (basketGlbBuffer_.empty()) basketGlbBuffer_ = ReadFileBuffer("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/XrSamples/BulletProject1/assets/basket.glb");

        if (!basketGlbBuffer_.empty()) {
            basketGlbRenderer_ = new OVRFW::SimpleGlbRenderer();
            if (basketGlbRenderer_->Init(basketGlbBuffer_)) {
                // C++の機能で明るさをMAXにする
                basketGlbRenderer_->AmbientLightColor = OVR::Vector3f(1.0f, 1.0f, 1.0f);
            }
            else {
                delete basketGlbRenderer_; basketGlbRenderer_ = nullptr;
                basketFallbackRenderer_ = CreateBoxRenderer({ spawnAreaSize_, spawnAreaSize_, spawnAreaSize_ }, { 0.0f, 1.0f, 0.5f, 0.3f });
            }
        }
        else {
            basketFallbackRenderer_ = CreateBoxRenderer({ spawnAreaSize_, spawnAreaSize_, spawnAreaSize_ }, { 0.0f, 1.0f, 0.5f, 0.3f });
        }

        // --- 5. ストラックアウトの的（ジョイント拘束付き剛体）の生成 ---
        std::vector<uint8_t> panelGlbBuffer = ReadFileBuffer("assets/panel_strike_out.glb");
        if (panelGlbBuffer.empty()) panelGlbBuffer = ReadFileBuffer("../../../../XrSamples/BulletProject1/assets/panel_strike_out.glb");
        if (panelGlbBuffer.empty()) panelGlbBuffer = ReadFileBuffer("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/XrSamples/BulletProject1/assets/panel_strike_out.glb");
        bool hasPanelGlb = !panelGlbBuffer.empty();

        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                float px = (col - 1) * 0.45f;
                float py = 0.0f + row * 0.45f;
                float pz = -4.0f;

                btBoxShape* shape = new btBoxShape(btVector3(0.2f, 0.2f, 0.025f));
                btTransform trans; trans.setIdentity(); trans.setOrigin(btVector3(px, py, pz));

                btVector3 inertia(0, 0, 0);
                float mass = 1.0f;
                shape->calculateLocalInertia(mass, inertia);
                btRigidBody* body = new btRigidBody(mass, new btDefaultMotionState(trans), shape, inertia);

                btHingeConstraint* hinge = new btHingeConstraint(*body, btVector3(0, -0.2f, 0), btVector3(-1.0f, 0.0f, 0.0f));
                hinge->setLimit(0.0f, M_PI / 2.0f);

                dynamicsWorld_->addRigidBody(body);
                dynamicsWorld_->addConstraint(hinge, true);

                PanelItem item;
                item.body = body; item.shape = shape; item.hinge = hinge;
                item.isKnockedDown = false;
                item.initialPos = btVector3(px, py, pz);
                item.glbRenderer = nullptr; item.fallbackRenderer = nullptr;

                if (hasPanelGlb) {
                    item.glbRenderer = new OVRFW::SimpleGlbRenderer();
                    if (item.glbRenderer->Init(panelGlbBuffer)) {
                        // C++の機能で明るさをMAXにする
                        item.glbRenderer->AmbientLightColor = OVR::Vector3f(1.0f, 1.0f, 1.0f);
                    }
                    else {
                        delete item.glbRenderer; item.glbRenderer = nullptr;
                        item.fallbackRenderer = CreateBoxRenderer({ 0.4f, 0.4f, 0.05f }, { 1.0f, 1.0f, 1.0f, 1.0f });
                    }
                }
                else {
                    item.fallbackRenderer = CreateBoxRenderer({ 0.4f, 0.4f, 0.05f }, { 1.0f, 1.0f, 1.0f, 1.0f });
                }
                panels_.push_back(item);
            }
        }

        // --- ボール用モデルデータの事前読み込み ---
        ballGlbBuffer_ = ReadFileBuffer("assets/ball.glb");
        if (ballGlbBuffer_.empty()) ballGlbBuffer_ = ReadFileBuffer("../../../../XrSamples/BulletProject1/assets/ball.glb");
        if (ballGlbBuffer_.empty()) ballGlbBuffer_ = ReadFileBuffer("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/XrSamples/BulletProject1/assets/ball.glb");

        // --- 6. 的の外枠（静的剛体）の生成 ---
        auto CreateFrame = [&](OVR::Vector3f pos, OVR::Vector3f size) {
            btBoxShape* shape = new btBoxShape(btVector3(size.x / 2, size.y / 2, size.z / 2));
            btTransform trans; trans.setIdentity(); trans.setOrigin(btVector3(pos.x, pos.y, pos.z));
            btRigidBody* body = new btRigidBody(0.0f, new btDefaultMotionState(trans), shape, btVector3(0, 0, 0));
            body->setRestitution(0.4f);
            dynamicsWorld_->addRigidBody(body);

            FrameItem item; item.body = body; item.shape = shape;
            item.renderer = CreateBoxRenderer(size, { 0.2f, 0.2f, 0.2f, 1.0f });
            frames_.push_back(item);
            };
        CreateFrame({ -0.75f,  0.45f, -4.05f }, { 0.1f,  1.4f, 0.2f }); // 左枠
        CreateFrame({ 0.75f,  0.45f, -4.05f }, { 0.1f,  1.4f, 0.2f }); // 右枠
        CreateFrame({ 0.0f,   1.20f, -4.05f }, { 1.6f,  0.1f, 0.2f }); // 上枠
        CreateFrame({ 0.0f,  -0.30f, -4.05f }, { 1.6f,  0.1f, 0.2f }); // 下枠

        // --- 7. 装飾用の木（物理演算なし・グラフィックのみ）の生成 ---
        treeGlbBuffer_ = ReadFileBuffer("assets/tree.glb");
        if (treeGlbBuffer_.empty()) treeGlbBuffer_ = ReadFileBuffer("../../../../XrSamples/BulletProject1/assets/tree.glb");
        if (treeGlbBuffer_.empty()) treeGlbBuffer_ = ReadFileBuffer("C:/Users/hiten/source/repos/HitenKato/Meta-OpenXR-SDK/Samples/XrSamples/BulletProject1/assets/tree.glb");

        // ★修正：木のX座標を ±4.0f 程度に広げ、Z座標も散らして配置
        std::vector<OVR::Vector3f> treePositions = {
            { -4.0f, -1.55f, -1.0f }, // 左手前
            {  4.0f, -1.55f, -1.0f }, // 右手前
            { -4.5f, -1.55f, -3.5f }, // 左奥
            {  4.5f, -1.55f, -3.5f }  // 右奥
        };

        for (const auto& pos : treePositions) {
            TreeItem tree;
            tree.initialPose = OVR::Posef(OVR::Quatf::Identity(), pos);
            tree.glbRenderer = nullptr; tree.fallbackRenderer = nullptr;

            if (!treeGlbBuffer_.empty()) {
                tree.glbRenderer = new OVRFW::SimpleGlbRenderer();
                if (tree.glbRenderer->Init(treeGlbBuffer_)) {
                    // C++の機能で明るさをMAXにする
                    tree.glbRenderer->AmbientLightColor = OVR::Vector3f(1.0f, 1.0f, 1.0f);
                }
                else {
                    delete tree.glbRenderer; tree.glbRenderer = nullptr;
                    tree.fallbackRenderer = CreateBoxRenderer({ 0.4f, 2.0f, 0.4f }, { 0.1f, 0.6f, 0.2f, 1.0f });
                    tree.initialPose.Translation.y += 1.0f;
                }
            }
            else {
                tree.fallbackRenderer = CreateBoxRenderer({ 0.4f, 2.0f, 0.4f }, { 0.1f, 0.6f, 0.2f, 1.0f });
                tree.initialPose.Translation.y += 1.0f;
            }
            trees_.push_back(tree);
        }

        return true;
    }

    virtual void SessionEnd() override {
    }

    // ------------------------------------------------------------------------
    // [Bullet] 動的剛体（ボール）の動的生成
    // ------------------------------------------------------------------------
    btRigidBody* SpawnBall(OVR::Vector3f pos) {
        btCollisionShape* ballShape = new btSphereShape(0.05f); // 半径5cmの球体判定
        btTransform trans; trans.setIdentity(); trans.setOrigin(btVector3(pos.x, pos.y, pos.z));

        // 質量0.2kg(200g)の動的剛体として設定
        btVector3 inertia(0, 0, 0);
        ballShape->calculateLocalInertia(0.2f, inertia);
        btRigidBody* body = new btRigidBody(0.2f, new btDefaultMotionState(trans), ballShape, inertia);
        body->setRestitution(0.8f); // 弾みやすくする
        dynamicsWorld_->addRigidBody(body);

        BallItem ball;
        ball.body = body; ball.shape = ballShape;
        ball.lifeTime = 6.0f; // 6秒後に自動削除
        ball.glbRenderer = nullptr; ball.fallbackRenderer = nullptr;

        if (!ballGlbBuffer_.empty()) {
            ball.glbRenderer = new OVRFW::SimpleGlbRenderer();
            if (ball.glbRenderer->Init(ballGlbBuffer_)) {
                // C++の機能で明るさをMAXにする
                ball.glbRenderer->AmbientLightColor = OVR::Vector3f(1.0f, 1.0f, 1.0f);
            }
            else {
                delete ball.glbRenderer; ball.glbRenderer = nullptr;
                ball.fallbackRenderer = CreateBoxRenderer({ 0.1f, 0.1f, 0.1f }, { 1.0f, 0.5f, 0.0f, 1.0f });
            }
        }
        else {
            ball.fallbackRenderer = CreateBoxRenderer({ 0.1f, 0.1f, 0.1f }, { 1.0f, 0.5f, 0.0f, 1.0f });
        }

        balls_.push_back(ball);
        return body;
    }

    // ------------------------------------------------------------------------
    // [Bullet] 6DoF拘束によるオブジェクトのグラブ（掴む）処理
    // ------------------------------------------------------------------------
    void HandleGrab(bool isGrip, bool& isGrabbing, btGeneric6DofConstraint*& constraint, btRigidBody*& grabBody, btRigidBody* handRb, OVR::Posef handWorldPose, OVR::Vector3f prevHandPos, float deltaTime) {
        if (isGrip && !isGrabbing) {
            btRigidBody* targetBall = nullptr;
            float minDist = 0.2f;

            // 既存のボールとの距離判定
            for (auto& ball : balls_) {
                btTransform t; ball.body->getMotionState()->getWorldTransform(t);
                float dist = (OVR::Vector3f(t.getOrigin().x(), t.getOrigin().y(), t.getOrigin().z()) - handWorldPose.Translation).Length();
                if (dist < minDist && ball.body != leftGrabBody_ && ball.body != rightGrabBody_) {
                    minDist = dist; targetBall = ball.body;
                }
            }

            // 近くにボールが無く、かつバスケットの範囲内であれば新規生成
            if (!targetBall && (handWorldPose.Translation - spawnAreaPos_).Length() < spawnAreaSize_) {
                targetBall = SpawnBall(handWorldPose.Translation);
            }

            // ボールを掴んだ場合、手とボールを「剛体拘束（ジョイント）」で結合する
            if (targetBall) {
                isGrabbing = true;
                grabBody = targetBall;
                btTransform frameInHand = handRb->getWorldTransform().inverse() * grabBody->getWorldTransform();
                btTransform frameInBox = btTransform::getIdentity();

                // 6自由度すべて（移動3軸、回転3軸）を完全にロックすることで、「手で持っている状態」を作る
                constraint = new btGeneric6DofConstraint(*handRb, *grabBody, frameInHand, frameInBox, true);
                constraint->setLinearLowerLimit(btVector3(0, 0, 0)); constraint->setLinearUpperLimit(btVector3(0, 0, 0));
                constraint->setAngularLowerLimit(btVector3(0, 0, 0)); constraint->setAngularUpperLimit(btVector3(0, 0, 0));

                dynamicsWorld_->addConstraint(constraint, true);
                grabBody->activate(true); // スリープ状態から復帰させる
            }
        }
        else if (!isGrip && isGrabbing) {
            // リリース（手放す）処理：拘束を削除し、初速を与える
            isGrabbing = false;
            if (constraint) { dynamicsWorld_->removeConstraint(constraint); delete constraint; constraint = nullptr; }
            if (grabBody) {
                grabBody->activate(true);
                // 手の移動差分から速度を計算し、ボールに付与して投げる
                OVR::Vector3f vel = (handWorldPose.Translation - prevHandPos) / std::max(deltaTime, 0.001f);
                grabBody->setLinearVelocity(btVector3(vel.x * 1.5f, vel.y * 1.5f, vel.z * 1.5f));
                grabBody = nullptr;
            }
        }
    }

    // ========================================================================
    // 毎フレームの更新処理（ロジックと物理演算の実行）
    // ========================================================================
    virtual void Update(const OVRFW::ovrApplFrameIn& in) override {
        // --- 1. プレイヤーの移動と回転 ---
        if (std::abs(in.RightRemoteJoystick.x) > 0.1f) {
            playerYaw_ -= in.RightRemoteJoystick.x * 2.0f * in.DeltaSeconds;
        }
        OVR::Quatf yawQuat(OVR::Vector3f(0, 1, 0), playerYaw_);

        if (std::abs(in.LeftRemoteJoystick.x) > 0.1f || std::abs(in.LeftRemoteJoystick.y) > 0.1f) {
            OVR::Vector3f forward = yawQuat.Rotate(OVR::Vector3f(0, 0, -1));
            OVR::Vector3f right = yawQuat.Rotate(OVR::Vector3f(1, 0, 0));
            playerPosition_ += (forward * in.LeftRemoteJoystick.y + right * in.LeftRemoteJoystick.x) * 3.0f * in.DeltaSeconds;
        }
        playerPose_ = OVR::Posef(yawQuat, playerPosition_);

        // --- 2. プレイヤーの手の物理座標更新 ---
        OVR::Posef leftWorldPose = playerPose_ * in.LeftRemotePose;
        OVR::Posef rightWorldPose = playerPose_ * in.RightRemotePose;

        // キネマティック剛体の座標は手動で上書きする
        auto UpdateHand = [&](btRigidBody* rb, OVR::Posef pose) {
            btTransform trans; trans.setOrigin(btVector3(pose.Translation.x, pose.Translation.y, pose.Translation.z));
            trans.setRotation(btQuaternion(pose.Rotation.x, pose.Rotation.y, pose.Rotation.z, pose.Rotation.w));
            rb->getMotionState()->setWorldTransform(trans); rb->setWorldTransform(trans);
            };
        UpdateHand(leftHandRb_, leftWorldPose);
        UpdateHand(rightHandRb_, rightWorldPose);

        // --- 3. ボールのグラブ判定 ---
        HandleGrab(in.LeftRemoteGripTrigger > 0.5f, isLeftGrabbing_, leftGrabConstraint_, leftGrabBody_, leftHandRb_, leftWorldPose, leftHandPrevPos_, in.DeltaSeconds);
        HandleGrab(in.RightRemoteGripTrigger > 0.5f, isRightGrabbing_, rightGrabConstraint_, rightGrabBody_, rightHandRb_, rightWorldPose, rightHandPrevPos_, in.DeltaSeconds);

        // --------------------------------------------------------------------
        // [Bullet] シミュレーションの進行 (Step Simulation)
        // --------------------------------------------------------------------
        if (dynamicsWorld_) {
            dynamicsWorld_->stepSimulation(in.DeltaSeconds, 10);
        }

        // --- 4. スコア判定 ---
        int knockedDownCount = 0;
        for (auto& panel : panels_) {
            if (!panel.isKnockedDown) {
                // ヒンジの角度を取得し、一定以上傾いていたら「倒れた」とみなす
                if (std::abs(panel.hinge->getHingeAngle()) > 0.5f) {
                    panel.isKnockedDown = true;
                    currentScore_ += 10;

                    if (panel.glbRenderer) panel.glbRenderer->AmbientLightColor = OVR::Vector3f(1.0f, 0.3f, 0.3f);
                    if (panel.fallbackRenderer) panel.fallbackRenderer->DiffuseColor = OVR::Vector4f(1.0f, 0.3f, 0.3f, 1.0f);

                    char buf[64];
                    sprintf(buf, "SCORE: %d", currentScore_);
                    scoreLabel_->SetText(buf);
                }
            }
            if (panel.isKnockedDown) knockedDownCount++;
        }

        if (knockedDownCount == 9 && currentScore_ == 90) {
            scoreLabel_->SetText("PERFECT CLEAR!");
        }

        // --- 5. リセット機能（Aボタン） ---
        bool isButtonAPressed = (in.AllButtons & OVRFW::ovrApplFrameIn::kButtonA) != 0;
        static bool prevA = false;
        if (isButtonAPressed && !prevA) {
            currentScore_ = 0;
            scoreLabel_->SetText("SCORE: 0");

            if (leftGrabConstraint_) { dynamicsWorld_->removeConstraint(leftGrabConstraint_); delete leftGrabConstraint_; leftGrabConstraint_ = nullptr; leftGrabBody_ = nullptr; isLeftGrabbing_ = false; }
            if (rightGrabConstraint_) { dynamicsWorld_->removeConstraint(rightGrabConstraint_); delete rightGrabConstraint_; rightGrabConstraint_ = nullptr; rightGrabBody_ = nullptr; isRightGrabbing_ = false; }

            // 物理ワールドから剛体を削除し、メモリを解放する
            for (auto& ball : balls_) {
                dynamicsWorld_->removeRigidBody(ball.body);
                delete ball.body->getMotionState(); delete ball.body; delete ball.shape;
                if (ball.glbRenderer) { ball.glbRenderer->Shutdown(); delete ball.glbRenderer; }
                if (ball.fallbackRenderer) { ball.fallbackRenderer->Shutdown(); delete ball.fallbackRenderer; }
            }
            balls_.clear();

            // 的の位置・角度・速度を初期状態にリセット
            for (auto& panel : panels_) {
                panel.isKnockedDown = false;
                if (panel.glbRenderer) panel.glbRenderer->AmbientLightColor = OVR::Vector3f(1.0f, 1.0f, 1.0f);
                if (panel.fallbackRenderer) panel.fallbackRenderer->DiffuseColor = OVR::Vector4f(1.0f, 1.0f, 1.0f, 1.0f);

                btTransform t; t.setIdentity(); t.setOrigin(panel.initialPos);
                panel.body->setWorldTransform(t); panel.body->getMotionState()->setWorldTransform(t);
                panel.body->setLinearVelocity(btVector3(0, 0, 0)); panel.body->setAngularVelocity(btVector3(0, 0, 0));
                panel.body->clearForces(); panel.body->activate(true);
            }
        }
        prevA = isButtonAPressed;

        // --- 6. ボールの自動削除（ガベージコレクション） ---
        for (auto it = balls_.begin(); it != balls_.end(); ) {
            if (it->body != leftGrabBody_ && it->body != rightGrabBody_) {
                it->lifeTime -= in.DeltaSeconds;
            }

            if (it->lifeTime < 0.0f) {
                dynamicsWorld_->removeRigidBody(it->body);
                delete it->body->getMotionState(); delete it->body; delete it->shape;

                if (it->glbRenderer) { it->glbRenderer->Shutdown(); delete it->glbRenderer; }
                if (it->fallbackRenderer) { it->fallbackRenderer->Shutdown(); delete it->fallbackRenderer; }

                it = balls_.erase(it);
            }
            else {
                ++it;
            }
        }

        // --------------------------------------------------------------------
        // [Bullet] 描画用モデルへの座標同期
        // 物理エンジンの演算結果(MotionState)を取得し、グラフィックに反映します。
        // --------------------------------------------------------------------
        for (auto& ball : balls_) {
            btTransform t; ball.body->getMotionState()->getWorldTransform(t);
            OVR::Posef pose(OVR::Quatf(t.getRotation().x(), t.getRotation().y(), t.getRotation().z(), t.getRotation().w()),
                OVR::Vector3f(t.getOrigin().x(), t.getOrigin().y(), t.getOrigin().z()));
            OVR::Posef relativePose = playerPose_.Inverted() * pose;

            if (ball.glbRenderer) ball.glbRenderer->Update(relativePose);
            if (ball.fallbackRenderer) {
                ball.fallbackRenderer->SetPose(relativePose);
                ball.fallbackRenderer->Update();
            }
        }

        for (auto& panel : panels_) {
            btTransform t; panel.body->getMotionState()->getWorldTransform(t);
            OVR::Posef pose(OVR::Quatf(t.getRotation().x(), t.getRotation().y(), t.getRotation().z(), t.getRotation().w()),
                OVR::Vector3f(t.getOrigin().x(), t.getOrigin().y(), t.getOrigin().z()));
            OVR::Posef relativePose = playerPose_.Inverted() * pose;

            if (panel.glbRenderer) panel.glbRenderer->Update(relativePose);
            if (panel.fallbackRenderer) {
                panel.fallbackRenderer->SetPose(relativePose);
                panel.fallbackRenderer->Update();
            }
        }

        for (auto& frame : frames_) {
            btTransform t; frame.body->getMotionState()->getWorldTransform(t);
            OVR::Posef pose(OVR::Quatf(t.getRotation().x(), t.getRotation().y(), t.getRotation().z(), t.getRotation().w()),
                OVR::Vector3f(t.getOrigin().x(), t.getOrigin().y(), t.getOrigin().z()));
            frame.renderer->SetPose(playerPose_.Inverted() * pose);
            frame.renderer->Update();
        }

        OVR::Posef basketPose = playerPose_.Inverted() * OVR::Posef(OVR::Quatf::Identity(), spawnAreaPos_);
        if (basketGlbRenderer_) basketGlbRenderer_->Update(basketPose);
        if (basketFallbackRenderer_) {
            basketFallbackRenderer_->SetPose(basketPose);
            basketFallbackRenderer_->Update();
        }

        for (auto& tree : trees_) {
            OVR::Posef relativePose = playerPose_.Inverted() * tree.initialPose;
            if (tree.glbRenderer) tree.glbRenderer->Update(relativePose);
            if (tree.fallbackRenderer) {
                tree.fallbackRenderer->SetPose(relativePose);
                tree.fallbackRenderer->Update();
            }
        }

        OVR::Posef floorWorldPose = OVR::Posef::Identity();
        floorWorldPose.Translation = { 0.0f, -1.55f, 0.0f };
        floorRenderer_.SetPose(playerPose_.Inverted() * floorWorldPose);
        floorRenderer_.Update();

        leftHandPrevPos_ = leftWorldPose.Translation;
        rightHandPrevPos_ = rightWorldPose.Translation;

        ui_.HitTestDevices().clear();
        if (in.LeftRemoteTracked) controllerRenderL_.Update(in.LeftRemotePose);
        if (in.RightRemoteTracked) controllerRenderR_.Update(in.RightRemotePose);
        ui_.Update(in);
        beamRenderer_.Update(in, ui_.HitTestDevices());
    }

    // ========================================================================
    // 描画処理（グラフィックパイプラインへの送信）
    // ========================================================================
    virtual void Render(const OVRFW::ovrApplFrameIn& in, OVRFW::ovrRendererOutput& out) override {
        ui_.Render(in, out);
        floorRenderer_.Render(out.Surfaces);

        if (basketGlbRenderer_) basketGlbRenderer_->Render(out.Surfaces);
        if (basketFallbackRenderer_) basketFallbackRenderer_->Render(out.Surfaces);

        for (auto& tree : trees_) {
            if (tree.glbRenderer) tree.glbRenderer->Render(out.Surfaces);
            if (tree.fallbackRenderer) tree.fallbackRenderer->Render(out.Surfaces);
        }

        for (auto& ball : balls_) {
            if (ball.glbRenderer) ball.glbRenderer->Render(out.Surfaces);
            if (ball.fallbackRenderer) ball.fallbackRenderer->Render(out.Surfaces);
        }

        for (auto& panel : panels_) {
            if (panel.glbRenderer) panel.glbRenderer->Render(out.Surfaces);
            if (panel.fallbackRenderer) panel.fallbackRenderer->Render(out.Surfaces);
        }

        for (auto& frame : frames_) frame.renderer->Render(out.Surfaces);

        if (in.LeftRemoteTracked) controllerRenderL_.Render(out.Surfaces);
        if (in.RightRemoteTracked) controllerRenderR_.Render(out.Surfaces);
        beamRenderer_.Render(in, out);
    }

private:
    OVRFW::ControllerRenderer controllerRenderL_;
    OVRFW::ControllerRenderer controllerRenderR_;
    OVRFW::TinyUI ui_;
    OVRFW::SimpleBeamRenderer beamRenderer_;

    OVRFW::GeometryRenderer floorRenderer_;
    OVRFW::VRMenuObject* scoreLabel_ = nullptr;

    OVRFW::SimpleGlbRenderer* basketGlbRenderer_ = nullptr;
    OVRFW::GeometryRenderer* basketFallbackRenderer_ = nullptr;
    std::vector<uint8_t> basketGlbBuffer_;

    // Bullet Physicsの基本コンポーネント
    btDefaultCollisionConfiguration* collisionConfiguration_;
    btCollisionDispatcher* dispatcher_;
    btBroadphaseInterface* overlappingPairCache_;
    btSequentialImpulseConstraintSolver* solver_;
    btDiscreteDynamicsWorld* dynamicsWorld_;

    btRigidBody* groundRigidBody_;
    btCollisionShape* groundShape_;

    btRigidBody* leftHandRb_;
    btRigidBody* rightHandRb_;
    btCollisionShape* handShape_;

    std::vector<BallItem> balls_;
    std::vector<PanelItem> panels_;
    std::vector<FrameItem> frames_;
    std::vector<uint8_t> ballGlbBuffer_;

    std::vector<TreeItem> trees_;
    std::vector<uint8_t> treeGlbBuffer_;

    OVR::Posef playerPose_ = OVR::Posef::Identity();
    float playerYaw_ = 0.0f;
    OVR::Vector3f playerPosition_ = { 0.0f, 0.0f, 0.0f };

    bool isLeftGrabbing_ = false;
    bool isRightGrabbing_ = false;
    btGeneric6DofConstraint* leftGrabConstraint_ = nullptr;
    btGeneric6DofConstraint* rightGrabConstraint_ = nullptr;
    btRigidBody* leftGrabBody_ = nullptr;
    btRigidBody* rightGrabBody_ = nullptr;

    OVR::Vector3f leftHandPrevPos_;
    OVR::Vector3f rightHandPrevPos_;

    OVR::Vector3f spawnAreaPos_ = { 0.6f, 0.0f, -0.5f };
    float spawnAreaSize_ = 0.3f;
    int currentScore_ = 0;
};

ENTRY_POINT(XrControllersApp)