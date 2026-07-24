// ============================================================================
// ストラックアウト VR アプリケーション (プロトタイプ版)
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
#include <btBulletDynamicsCommon.h>

#include "Input/ControllerRenderer.h"
#include "Input/TinyUI.h"
#include "Render/SimpleBeamRenderer.h"
#include "Render/GeometryRenderer.h"
#include "Render/GeometryBuilder.h"

// ★追加：GLBモデルを描画するためのヘッダー
#include "Render/SimpleGlbRenderer.h"

// ----------------------------------------------------------------------------
// 各種オブジェクトを管理するための構造体
// ----------------------------------------------------------------------------
struct BallItem {
    btRigidBody* body;
    btCollisionShape* shape;
    OVRFW::GeometryRenderer* renderer;
    float lifeTime;
};

struct PanelItem {
    btRigidBody* body;
    btCollisionShape* shape;
    btHingeConstraint* hinge;

    // ★追加：GLBモデル用のレンダラーと、読み込み失敗時用の代替レンダラー
    OVRFW::SimpleGlbRenderer* glbRenderer;
    OVRFW::GeometryRenderer* fallbackRenderer;

    bool isKnockedDown;
    btVector3 initialPos;
};

struct FrameItem {
    btRigidBody* body;
    btCollisionShape* shape;
    OVRFW::GeometryRenderer* renderer;
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

    // --- バイナリファイルを読み込むヘルパー関数 ---
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

        scoreLabel_ = ui_.AddLabel("SCORE: 0", { 0.0f, 2.0f, -4.0f }, { 800.0f, 200.0f });
        return true;
    }

    virtual void AppShutdown(const xrJava* context) override {
        floorRenderer_.Shutdown();
        if (spawnAreaRenderer_) { spawnAreaRenderer_->Shutdown(); delete spawnAreaRenderer_; spawnAreaRenderer_ = nullptr; }
        for (auto& b : balls_) { b.renderer->Shutdown(); delete b.renderer; }

        // ★修正：パネルのレンダラー解放処理
        for (auto& p : panels_) {
            if (p.glbRenderer) { p.glbRenderer->Shutdown(); delete p.glbRenderer; }
            if (p.fallbackRenderer) { p.fallbackRenderer->Shutdown(); delete p.fallbackRenderer; }
        }

        for (auto& f : frames_) { f.renderer->Shutdown(); delete f.renderer; }
        OVRFW::XrApp::AppShutdown(context);
        ui_.Shutdown();
    }

    OVRFW::GeometryRenderer* CreateBoxRenderer(OVR::Vector3f scale, OVR::Vector4f color) {
        OVRFW::GeometryRenderer* renderer = new OVRFW::GeometryRenderer();
        OVRFW::GeometryBuilder builder;
        builder.Add(OVRFW::BuildUnitCubeDescriptor(), OVRFW::GeometryBuilder::kInvalidIndex, color);
        renderer->ChannelControl = OVR::Vector4f(1, 1, 1, 1);
        renderer->Init(builder.ToGeometryDescriptor());
        renderer->SetScale(scale);
        return renderer;
    }

    btRigidBody* CreateKinematicBody(btCollisionShape* shape) {
        btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, new btDefaultMotionState(), shape, btVector3(0, 0, 0));
        btRigidBody* body = new btRigidBody(rbInfo);
        body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        body->setActivationState(DISABLE_DEACTIVATION);
        return body;
    }

    virtual bool SessionInit() override {
        CurrentSpace = LocalSpace;
        if (!controllerRenderL_.Init(true) || !controllerRenderR_.Init(false)) return false;
        beamRenderer_.Init(GetFileSys(), nullptr, OVR::Vector4f(1.0f), 1.0f);

        // 1. 床の生成 
        OVRFW::GeometryBuilder floorBuilder;
        floorBuilder.Add(OVRFW::BuildUnitCubeDescriptor(), OVRFW::GeometryBuilder::kInvalidIndex, { 0.2f, 0.8f, 0.2f, 1.0f });
        floorRenderer_.ChannelControl = OVR::Vector4f(1, 1, 1, 1);
        floorRenderer_.Init(floorBuilder.ToGeometryDescriptor());
        floorRenderer_.SetScale({ 20.0f, 0.1f, 20.0f });

        // 2. 物理エンジンの初期化
        collisionConfiguration_ = new btDefaultCollisionConfiguration();
        dispatcher_ = new btCollisionDispatcher(collisionConfiguration_);
        overlappingPairCache_ = new btDbvtBroadphase();
        solver_ = new btSequentialImpulseConstraintSolver();
        dynamicsWorld_ = new btDiscreteDynamicsWorld(dispatcher_, overlappingPairCache_, solver_, collisionConfiguration_);
        dynamicsWorld_->setGravity(btVector3(0.0f, -9.8f, 0.0f));

        groundShape_ = new btStaticPlaneShape(btVector3(0, 1, 0), 0.0f);
        btTransform groundTrans; groundTrans.setIdentity(); groundTrans.setOrigin(btVector3(0, -1.5f, 0));
        groundRigidBody_ = new btRigidBody(0.0f, new btDefaultMotionState(groundTrans), groundShape_, btVector3(0, 0, 0));
        groundRigidBody_->setRestitution(0.6f); groundRigidBody_->setFriction(0.8f);
        dynamicsWorld_->addRigidBody(groundRigidBody_);

        handShape_ = new btSphereShape(0.1f);
        leftHandRb_ = CreateKinematicBody(handShape_);
        rightHandRb_ = CreateKinematicBody(handShape_);
        dynamicsWorld_->addRigidBody(leftHandRb_);
        dynamicsWorld_->addRigidBody(rightHandRb_);

        // 3. ボールの無限湧きエリア
        spawnAreaPos_ = { 0.6f, 0.0f, -0.5f };
        spawnAreaRenderer_ = CreateBoxRenderer({ spawnAreaSize_, spawnAreaSize_, spawnAreaSize_ }, { 0.0f, 1.0f, 0.5f, 0.3f });

        // ★追加：GLBファイルの読み込み（パスが見つかるまでフォールバック探索）
        std::vector<uint8_t> panelGlbBuffer = ReadFileBuffer("assets/panel_strike_out.glb");
        if (panelGlbBuffer.empty()) {
            panelGlbBuffer = ReadFileBuffer("../../../../XrSamples/BulletProject1/assets/panel_strike_out.glb");
        }
        if (panelGlbBuffer.empty()) {
            // 絶対パスでの最終フォールバック
            panelGlbBuffer = ReadFileBuffer("C:/Users/hiten/source/repos/Meta-OpenXR-SDK/Samples/XrSamples/BulletProject1/assets/panel_strike_out.glb");
        }
        bool hasGlb = !panelGlbBuffer.empty();

        // 4. ストラックアウトの「的（パネル）」の生成
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                float px = (col - 1) * 0.45f;
                float py = 0.0f + row * 0.45f;
                float pz = -4.0f;

                btBoxShape* shape = new btBoxShape(btVector3(0.2f, 0.2f, 0.025f));
                btTransform trans; trans.setIdentity(); trans.setOrigin(btVector3(px, py, pz));
                btVector3 inertia(0, 0, 0);
                shape->calculateLocalInertia(1.0f, inertia);
                btRigidBody* body = new btRigidBody(1.0f, new btDefaultMotionState(trans), shape, inertia);

                btHingeConstraint* hinge = new btHingeConstraint(*body, btVector3(0, -0.2f, 0), btVector3(-1.0f, 0.0f, 0.0f));
                hinge->setLimit(0.0f, M_PI / 2.0f);

                dynamicsWorld_->addRigidBody(body);
                dynamicsWorld_->addConstraint(hinge, true);

                PanelItem item;
                item.body = body; item.shape = shape; item.hinge = hinge;
                item.isKnockedDown = false;
                item.initialPos = btVector3(px, py, pz);

                // ★修正：GLBが読み込めていればSimpleGlbRendererを、ダメなら従来の箱を生成
                item.glbRenderer = nullptr;
                item.fallbackRenderer = nullptr;
                if (hasGlb) {
                    item.glbRenderer = new OVRFW::SimpleGlbRenderer();
                    bool loaded = item.glbRenderer->Init(panelGlbBuffer); // 読み込み成功かをチェック

                    if (loaded) {
                        item.glbRenderer->AmbientLightColor = OVR::Vector3f(1.0f, 1.0f, 1.0f);
                    }
                    else {
                        // 失敗時は破棄して代替の箱を出す
                        delete item.glbRenderer;
                        item.glbRenderer = nullptr;
                        item.fallbackRenderer = CreateBoxRenderer({ 0.4f, 0.4f, 0.05f }, { 1.0f, 1.0f, 1.0f, 1.0f });
                    }
                }
                else {
                    item.fallbackRenderer = CreateBoxRenderer({ 0.4f, 0.4f, 0.05f }, { 1.0f, 1.0f, 1.0f, 1.0f });
                }

                panels_.push_back(item);
            }
        }

        // 5. ストラックアウトの「枠（フレーム）」の生成
        auto CreateFrame = [&](OVR::Vector3f pos, OVR::Vector3f size) {
            btBoxShape* shape = new btBoxShape(btVector3(size.x / 2, size.y / 2, size.z / 2));
            btTransform trans; trans.setIdentity(); trans.setOrigin(btVector3(pos.x, pos.y, pos.z));
            btRigidBody* body = new btRigidBody(0.0f, new btDefaultMotionState(trans), shape, btVector3(0, 0, 0));
            body->setRestitution(0.4f); dynamicsWorld_->addRigidBody(body);

            FrameItem item; item.body = body; item.shape = shape;
            item.renderer = CreateBoxRenderer(size, { 0.2f, 0.2f, 0.2f, 1.0f });
            frames_.push_back(item);
            };
        CreateFrame({ -0.75f,  0.45f, -4.05f }, { 0.1f,  1.4f, 0.2f }); // 左枠
        CreateFrame({ 0.75f,  0.45f, -4.05f }, { 0.1f,  1.4f, 0.2f }); // 右枠
        CreateFrame({ 0.0f,   1.20f, -4.05f }, { 1.6f,  0.1f, 0.2f }); // 上枠
        CreateFrame({ 0.0f,  -0.30f, -4.05f }, { 1.6f,  0.1f, 0.2f }); // 下枠

        return true;
    }

    virtual void SessionEnd() override {
        // (省略)
    }

    btRigidBody* SpawnBall(OVR::Vector3f pos) {
        btBoxShape* ballShape = new btBoxShape(btVector3(0.05f, 0.05f, 0.05f));
        btTransform trans; trans.setIdentity(); trans.setOrigin(btVector3(pos.x, pos.y, pos.z));
        btVector3 inertia(0, 0, 0);
        ballShape->calculateLocalInertia(0.2f, inertia);
        btRigidBody* body = new btRigidBody(0.2f, new btDefaultMotionState(trans), ballShape, inertia);
        body->setRestitution(0.8f);
        dynamicsWorld_->addRigidBody(body);

        BallItem ball;
        ball.body = body; ball.shape = ballShape;
        ball.renderer = CreateBoxRenderer({ 0.1f, 0.1f, 0.1f }, { 1.0f, 0.5f, 0.0f, 1.0f });
        ball.lifeTime = 6.0f;
        balls_.push_back(ball);
        return body;
    }

    void HandleGrab(bool isGrip, bool& isGrabbing, btGeneric6DofConstraint*& constraint, btRigidBody*& grabBody, btRigidBody* handRb, OVR::Posef handWorldPose, OVR::Vector3f prevHandPos, float deltaTime) {
        if (isGrip && !isGrabbing) {
            btRigidBody* targetBall = nullptr;
            float minDist = 0.2f;
            for (auto& ball : balls_) {
                btTransform t; ball.body->getMotionState()->getWorldTransform(t);
                float dist = (OVR::Vector3f(t.getOrigin().x(), t.getOrigin().y(), t.getOrigin().z()) - handWorldPose.Translation).Length();
                if (dist < minDist && ball.body != leftGrabBody_ && ball.body != rightGrabBody_) {
                    minDist = dist; targetBall = ball.body;
                }
            }

            if (!targetBall && (handWorldPose.Translation - spawnAreaPos_).Length() < spawnAreaSize_) {
                targetBall = SpawnBall(handWorldPose.Translation);
            }

            if (targetBall) {
                isGrabbing = true;
                grabBody = targetBall;
                btTransform frameInHand = handRb->getWorldTransform().inverse() * grabBody->getWorldTransform();
                btTransform frameInBox = btTransform::getIdentity();
                constraint = new btGeneric6DofConstraint(*handRb, *grabBody, frameInHand, frameInBox, true);

                constraint->setLinearLowerLimit(btVector3(0, 0, 0)); constraint->setLinearUpperLimit(btVector3(0, 0, 0));
                constraint->setAngularLowerLimit(btVector3(0, 0, 0)); constraint->setAngularUpperLimit(btVector3(0, 0, 0));

                dynamicsWorld_->addConstraint(constraint, true);
                grabBody->activate(true);
            }
        }
        else if (!isGrip && isGrabbing) {
            isGrabbing = false;
            if (constraint) { dynamicsWorld_->removeConstraint(constraint); delete constraint; constraint = nullptr; }
            if (grabBody) {
                grabBody->activate(true);
                OVR::Vector3f vel = (handWorldPose.Translation - prevHandPos) / std::max(deltaTime, 0.001f);
                grabBody->setLinearVelocity(btVector3(vel.x * 1.5f, vel.y * 1.5f, vel.z * 1.5f));
                grabBody = nullptr;
            }
        }
    }

    virtual void Update(const OVRFW::ovrApplFrameIn& in) override {
        // --- 1. 移動と回転 ---
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

        // --- 2. 手の物理座標更新 ---
        OVR::Posef leftWorldPose = playerPose_ * in.LeftRemotePose;
        OVR::Posef rightWorldPose = playerPose_ * in.RightRemotePose;

        auto UpdateHand = [&](btRigidBody* rb, OVR::Posef pose) {
            btTransform trans; trans.setOrigin(btVector3(pose.Translation.x, pose.Translation.y, pose.Translation.z));
            trans.setRotation(btQuaternion(pose.Rotation.x, pose.Rotation.y, pose.Rotation.z, pose.Rotation.w));
            rb->getMotionState()->setWorldTransform(trans); rb->setWorldTransform(trans);
            };
        UpdateHand(leftHandRb_, leftWorldPose);
        UpdateHand(rightHandRb_, rightWorldPose);

        // --- 3. ボールのグラブ（掴み）判定 ---
        HandleGrab(in.LeftRemoteGripTrigger > 0.5f, isLeftGrabbing_, leftGrabConstraint_, leftGrabBody_, leftHandRb_, leftWorldPose, leftHandPrevPos_, in.DeltaSeconds);
        HandleGrab(in.RightRemoteGripTrigger > 0.5f, isRightGrabbing_, rightGrabConstraint_, rightGrabBody_, rightHandRb_, rightWorldPose, rightHandPrevPos_, in.DeltaSeconds);

        // --- 4. 物理エンジンのステップ実行 ---
        if (dynamicsWorld_) {
            dynamicsWorld_->stepSimulation(in.DeltaSeconds, 10);
        }

        // --- 5. スコア判定とクリア判定 ---
        int knockedDownCount = 0;
        for (auto& panel : panels_) {
            if (!panel.isKnockedDown) {
                if (std::abs(panel.hinge->getHingeAngle()) > 0.5f) {
                    panel.isKnockedDown = true;
                    currentScore_ += 10;

                    // ▼ 修正：当たった時は、明るく鮮やかな赤色に発光させる
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

        // --- 6. リセット機能（Aボタン） ---
        bool isButtonAPressed = (in.AllButtons & OVRFW::ovrApplFrameIn::kButtonA) != 0;
        static bool prevA = false;
        if (isButtonAPressed && !prevA) {
            currentScore_ = 0;
            scoreLabel_->SetText("SCORE: 0");

            if (leftGrabConstraint_) { dynamicsWorld_->removeConstraint(leftGrabConstraint_); delete leftGrabConstraint_; leftGrabConstraint_ = nullptr; leftGrabBody_ = nullptr; isLeftGrabbing_ = false; }
            if (rightGrabConstraint_) { dynamicsWorld_->removeConstraint(rightGrabConstraint_); delete rightGrabConstraint_; rightGrabConstraint_ = nullptr; rightGrabBody_ = nullptr; isRightGrabbing_ = false; }
            for (auto& ball : balls_) {
                dynamicsWorld_->removeRigidBody(ball.body);
                delete ball.body->getMotionState(); delete ball.body; delete ball.shape;
                ball.renderer->Shutdown(); delete ball.renderer;
            }
            balls_.clear();

            for (auto& panel : panels_) {
                panel.isKnockedDown = false;
                // 色を元に戻す（明るさMAXの状態を維持する）
                if (panel.glbRenderer) panel.glbRenderer->AmbientLightColor = OVR::Vector3f(1.0f, 1.0f, 1.0f);
                if (panel.fallbackRenderer) panel.fallbackRenderer->DiffuseColor = OVR::Vector4f(1.0f, 1.0f, 1.0f, 1.0f);

                btTransform t; t.setIdentity(); t.setOrigin(panel.initialPos);
                // ... 省略 ...
                panel.body->setWorldTransform(t); panel.body->getMotionState()->setWorldTransform(t);
                panel.body->setLinearVelocity(btVector3(0, 0, 0)); panel.body->setAngularVelocity(btVector3(0, 0, 0));
                panel.body->clearForces(); panel.body->activate(true);
            }
        }
        prevA = isButtonAPressed;

        // --- 7. ボールの自動削除 ---
        for (auto it = balls_.begin(); it != balls_.end(); ) {
            if (it->body != leftGrabBody_ && it->body != rightGrabBody_) {
                it->lifeTime -= in.DeltaSeconds;
            }

            if (it->lifeTime < 0.0f) {
                dynamicsWorld_->removeRigidBody(it->body);
                delete it->body->getMotionState(); delete it->body; delete it->shape;
                it->renderer->Shutdown(); delete it->renderer;
                it = balls_.erase(it);
            }
            else {
                ++it;
            }
        }

        // --- 8. 描画用モデルの座標同期 ---
        for (auto& ball : balls_) {
            btTransform t; ball.body->getMotionState()->getWorldTransform(t);
            OVR::Posef pose(OVR::Quatf(t.getRotation().x(), t.getRotation().y(), t.getRotation().z(), t.getRotation().w()),
                OVR::Vector3f(t.getOrigin().x(), t.getOrigin().y(), t.getOrigin().z()));
            ball.renderer->SetPose(playerPose_.Inverted() * pose);
            ball.renderer->Update();
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

        OVR::Posef floorWorldPose = OVR::Posef::Identity();
        floorWorldPose.Translation = { 0.0f, -1.55f, 0.0f };
        floorRenderer_.SetPose(playerPose_.Inverted() * floorWorldPose);
        floorRenderer_.Update();

        if (spawnAreaRenderer_) {
            spawnAreaRenderer_->SetPose(playerPose_.Inverted() * OVR::Posef(OVR::Quatf::Identity(), spawnAreaPos_));
            spawnAreaRenderer_->Update();
        }

        leftHandPrevPos_ = leftWorldPose.Translation;
        rightHandPrevPos_ = rightWorldPose.Translation;

        ui_.HitTestDevices().clear();
        if (in.LeftRemoteTracked) controllerRenderL_.Update(in.LeftRemotePose);
        if (in.RightRemoteTracked) controllerRenderR_.Update(in.RightRemotePose);
        ui_.Update(in);
        beamRenderer_.Update(in, ui_.HitTestDevices());
    }

    virtual void Render(const OVRFW::ovrApplFrameIn& in, OVRFW::ovrRendererOutput& out) override {
        ui_.Render(in, out);

        floorRenderer_.Render(out.Surfaces);
        if (spawnAreaRenderer_) spawnAreaRenderer_->Render(out.Surfaces);

        for (auto& ball : balls_) ball.renderer->Render(out.Surfaces);

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
    OVRFW::GeometryRenderer* spawnAreaRenderer_ = nullptr;
    OVRFW::VRMenuObject* scoreLabel_ = nullptr;

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