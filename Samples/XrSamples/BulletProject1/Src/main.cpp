// ============================================================================
// プリプロセッサマクロ定義 (マクロ・型衝突防止)
// ============================================================================
#define _HAS_STD_BYTE 0    // Windows SDKの「byte」と C++17 <cstddef> の「std::byte」の型衝突を防止
#define NOMINMAX           // <windows.h> 内の min/max マクロ定義を無効化（std::min/max との衝突防止）

// ============================================================================
// ヘッダーインクルード
// ============================================================================
#include <windows.h>       // Windows API基本ヘッダー（一番最初に読み込む必要がある）
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <openxr/openxr.h>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>

// Meta OpenXR Framework 基底クラス
#include "XrApp.h"

// Bullet Physics (物理演算ライブラリ)
#include <btBulletDynamicsCommon.h>

// Meta OpenXR Framework レンダリング・入力系コンポーネント
#include "Input/SkeletonRenderer.h"
#include "Input/ControllerRenderer.h"
#include "Input/TinyUI.h"
#include "Input/AxisRenderer.h"
#include "Render/SimpleBeamRenderer.h"

// ============================================================================
// メインアプリケーションクラス定義
// ============================================================================
class XrControllersApp : public OVRFW::XrApp {
public:
    // ------------------------------------------------------------------------
    // コンストラクタ / デストラクタ
    // ------------------------------------------------------------------------
    XrControllersApp()
        : OVRFW::XrApp(),
        collisionConfiguration_(nullptr),
        dispatcher_(nullptr),
        overlappingPairCache_(nullptr),
        solver_(nullptr),
        dynamicsWorld_(nullptr),
        fallingCube_(nullptr),
        boxShape_(nullptr),
        groundRigidBody_(nullptr),
        groundShape_(nullptr),
        physicsBoxUI_(nullptr),
        groundUI_(nullptr) {

        // ▼ 変更：VR空間の背景色を「灰色 (Dark Gray)」に設定 (RGBA: R=0.2, G=0.2, B=0.2, A=1.0)
        BackgroundColor = OVR::Vector4f(0.20f, 0.20f, 0.20f, 1.0f);
        OpenXRVersion = XR_API_VERSION_1_0;
    }

    // ------------------------------------------------------------------------
    // 入力アクションのマッピング設定 (GetSuggestedBindings)
    // ------------------------------------------------------------------------
    std::unordered_map<XrPath, std::vector<XrActionSuggestedBinding>> GetSuggestedBindings(XrInstance instance) override {
        XrPath touchInteractionProfile = XR_NULL_PATH;
        OXR(xrStringToPath(instance, "/interaction_profiles/meta/touch_controller_quest_2", &touchInteractionProfile));
        XrPath touchProInteractionProfile = XR_NULL_PATH;
        OXR(xrStringToPath(instance, "/interaction_profiles/meta/touch_pro_controller", &touchProInteractionProfile));
        XrPath touchPlusInteractionProfile = XR_NULL_PATH;
        OXR(xrStringToPath(instance, "/interaction_profiles/meta/touch_plus_controller", &touchPlusInteractionProfile));

        std::vector<XrActionSuggestedBinding> baseTouchBindings{};
        baseTouchBindings.emplace_back(ActionSuggestedBinding(AimPoseAction, "/user/hand/left/input/aim/pose"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(AimPoseAction, "/user/hand/right/input/aim/pose"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(GripPoseAction, "/user/hand/left/input/grip/pose"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(GripPoseAction, "/user/hand/right/input/grip/pose"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(JoystickAction, "/user/hand/left/input/thumbstick"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(JoystickAction, "/user/hand/right/input/thumbstick"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(thumbstickClickAction, "/user/hand/left/input/thumbstick/click"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(thumbstickClickAction, "/user/hand/right/input/thumbstick/click"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(IndexTriggerAction, "/user/hand/left/input/trigger/value"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(IndexTriggerAction, "/user/hand/right/input/trigger/value"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(GripTriggerAction, "/user/hand/left/input/squeeze/value"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(GripTriggerAction, "/user/hand/right/input/squeeze/value"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(ButtonAAction, "/user/hand/right/input/a/click"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(ButtonBAction, "/user/hand/right/input/b/click"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(ButtonXAction, "/user/hand/left/input/x/click"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(ButtonYAction, "/user/hand/left/input/y/click"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(ButtonMenuAction, "/user/hand/left/input/menu/click"));

        std::unordered_map<XrPath, std::vector<XrActionSuggestedBinding>> allSuggestedBindings;
        allSuggestedBindings[touchInteractionProfile] = baseTouchBindings;
        allSuggestedBindings[touchProInteractionProfile] = baseTouchBindings;
        allSuggestedBindings[touchPlusInteractionProfile] = baseTouchBindings;
        return allSuggestedBindings;
    }

    // ------------------------------------------------------------------------
    // アプリケーション初期化 (AppInit)
    // ------------------------------------------------------------------------
    virtual bool AppInit(const xrJava* context) override {
        // VR用UIフレームワーク (TinyUI) の初期化
        if (false == ui_.Init(context, GetFileSys())) {
            ALOG("TinyUI::Init FAILED.");
            return false;
        }

        // 物理演算された箱の位置に重ねるラベルパネル
        physicsBoxUI_ = ui_.AddLabel("Physics Box", { 0.0f, 3.0f, -2.0f }, { 400.0f, 400.0f });

        // ▼ 追加：床の位置を目視で確認するための「FLOOR」ラベルパネル
        groundUI_ = ui_.AddLabel("=== FLOOR ===", { 0.0f, 0.0f, -2.0f }, { 1000.0f, 200.0f });

        return true;
    }

    // ------------------------------------------------------------------------
    // アプリケーション終了処理 (AppShutdown)
    // ------------------------------------------------------------------------
    virtual void AppShutdown(const xrJava* context) override {
        OVRFW::XrApp::AppShutdown(context);
        ui_.Shutdown();
    }

    // ------------------------------------------------------------------------
    // VRセッション開始時初期化 (SessionInit)
    // ------------------------------------------------------------------------
    virtual bool SessionInit() override {
        CurrentSpace = LocalSpace;

        if (false == controllerRenderL_.Init(true)) {
            ALOG("AppInit::Init L controller renderer FAILED.");
            return false;
        }
        if (false == controllerRenderR_.Init(false)) {
            ALOG("AppInit::Init R controller renderer FAILED.");
            return false;
        }

        beamRenderer_.Init(GetFileSys(), nullptr, OVR::Vector4f(1.0f), 1.0f);

        // --------------------------------------------------------------------
        // Bullet Physics ワールドの構築
        // --------------------------------------------------------------------
        collisionConfiguration_ = new btDefaultCollisionConfiguration();
        dispatcher_ = new btCollisionDispatcher(collisionConfiguration_);
        overlappingPairCache_ = new btDbvtBroadphase();
        solver_ = new btSequentialImpulseConstraintSolver();

        dynamicsWorld_ = new btDiscreteDynamicsWorld(dispatcher_, overlappingPairCache_, solver_, collisionConfiguration_);
        dynamicsWorld_->setGravity(btVector3(0.0f, -9.8f, 0.0f)); // 重力 (-9.8 m/s^2)
        ALOG("Bullet Physics World Initialized!");

        // --------------------------------------------------------------------
        // ▼ 追加：物理オブジェクト（1. 静的な「床」）の追加
        // --------------------------------------------------------------------
        // 上向き（法線ベクトル: (0, 1, 0)）で、原点からの距離が 0.0m の無限平面
        groundShape_ = new btStaticPlaneShape(btVector3(0.0f, 1.0f, 0.0f), 0.0f);

        btTransform groundTransform;
        groundTransform.setIdentity();
        groundTransform.setOrigin(btVector3(0.0f, 0.0f, 0.0f)); // 足元の高さ(Y=0)に設置

        // 静的オブジェクト（動かない物体）のため、質量 mass = 0.0kg に設定
        btScalar groundMass(0.0f);
        btVector3 groundLocalInertia(0.0f, 0.0f, 0.0f);

        btDefaultMotionState* groundMotionState = new btDefaultMotionState(groundTransform);
        btRigidBody::btRigidBodyConstructionInfo groundRbInfo(groundMass, groundMotionState, groundShape_, groundLocalInertia);
        groundRigidBody_ = new btRigidBody(groundRbInfo);

        // 床の跳ね返り係数(Restitution)と摩擦係数(Friction)を設定
        groundRigidBody_->setRestitution(0.5f); // 50% の跳ね返り
        groundRigidBody_->setFriction(0.8f);    // 滑り止め（摩擦）

        dynamicsWorld_->addRigidBody(groundRigidBody_);
        ALOG("Bullet Ground Plane Added to World!");

        // --------------------------------------------------------------------
        // 物理オブジェクト（2. 落下する「箱」）の追加
        // --------------------------------------------------------------------
        boxShape_ = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f)); // 幅1m x 高さ1m x 奥行1m

        btTransform startTransform;
        startTransform.setIdentity();
        startTransform.setOrigin(btVector3(0.0f, 3.0f, -2.0f)); // 高さを3m、前方を2mに設定

        btScalar mass(1.0f); // 質量 1.0kg（動体）
        btVector3 localInertia(0.0f, 0.0f, 0.0f);
        boxShape_->calculateLocalInertia(mass, localInertia);

        btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, boxShape_, localInertia);
        fallingCube_ = new btRigidBody(rbInfo);

        // 箱の跳ね返り係数(Restitution)と摩擦係数(Friction)を設定
        fallingCube_->setRestitution(0.5f); // 床とぶつかった時に跳ね返る
        fallingCube_->setFriction(0.8f);

        dynamicsWorld_->addRigidBody(fallingCube_);
        ALOG("Bullet Box Added to World!");

        return true;
    }

    // ------------------------------------------------------------------------
    // VRセッション終了処理 (SessionEnd)
    // ------------------------------------------------------------------------
    virtual void SessionEnd() override {
        controllerRenderL_.Shutdown();
        controllerRenderR_.Shutdown();
        beamRenderer_.Shutdown();

        // 箱の破棄
        if (fallingCube_) {
            dynamicsWorld_->removeRigidBody(fallingCube_);
            delete fallingCube_->getMotionState();
            delete fallingCube_;
            fallingCube_ = nullptr;
        }
        if (boxShape_) {
            delete boxShape_;
            boxShape_ = nullptr;
        }

        // ▼ 追加：床の破棄
        if (groundRigidBody_) {
            dynamicsWorld_->removeRigidBody(groundRigidBody_);
            delete groundRigidBody_->getMotionState();
            delete groundRigidBody_;
            groundRigidBody_ = nullptr;
        }
        if (groundShape_) {
            delete groundShape_;
            groundShape_ = nullptr;
        }

        // 物理ワールドの破棄
        if (dynamicsWorld_) {
            delete dynamicsWorld_;
            delete solver_;
            delete overlappingPairCache_;
            delete dispatcher_;
            delete collisionConfiguration_;
            dynamicsWorld_ = nullptr;
        }
    }

    // ------------------------------------------------------------------------
    // 毎フレームの物理・位置計算更新 (Update)
    // ------------------------------------------------------------------------
    virtual void Update(const OVRFW::ovrApplFrameIn& in) override {
        // 1. 物理シミュレーションを1ステップ進める
        if (dynamicsWorld_) {
            dynamicsWorld_->stepSimulation(in.DeltaSeconds, 10);
        }

        // --- 必要な入力の取得 ---
        const auto stateA = GetActionStateBoolean(ButtonAAction);
        bool isButtonAPressed = (stateA.isActive && stateA.currentState == XR_TRUE);

        // ▼ 簡易デバッグログを追加 ▼
        if (isButtonAPressed) {
            ALOG("A Button is Pressed!");
        }
        // ▲ ここまで ▲

        // ▼ 追加：Aボタンを押した瞬間、箱を上空にリセットして斜めに落とす！ ▼
        static bool prevButtonA = false; // 前フレームのボタン状態（連打防止）

        // 判定には変換した bool 変数を使います
        if (isButtonAPressed && !prevButtonA && fallingCube_) {
            btTransform trans;
            trans.setIdentity();
            trans.setOrigin(btVector3(0.0f, 3.0f, -2.0f)); // 再び頭上3m、前方2mへ

            // そのまま落とすと真っ直ぐ止まるので、X, Y, Z軸にランダムな傾きをつける
            btQuaternion quat;
            quat.setEuler(0.8f, 0.5f, 0.3f);
            trans.setRotation(quat);

            // 座標と回転を物理エンジンに強制上書き
            fallingCube_->setWorldTransform(trans);
            fallingCube_->getMotionState()->setWorldTransform(trans);

            // 落下速度と回転速度をゼロにリセット
            fallingCube_->setLinearVelocity(btVector3(0, 0, 0));
            fallingCube_->setAngularVelocity(btVector3(0, 0, 0));
            fallingCube_->clearForces();
        }
        prevButtonA = isButtonAPressed; // 記録するのも bool 変数にします

        // 2. 物理演算された「箱」の座標をUIパネルに同期
        if (fallingCube_ && physicsBoxUI_) {
            btTransform trans;
            fallingCube_->getMotionState()->getWorldTransform(trans);

            OVR::Posef pose;
            pose.Translation = OVR::Vector3f(trans.getOrigin().x(), trans.getOrigin().y(), trans.getOrigin().z());
            pose.Rotation = OVR::Quatf(trans.getRotation().x(), trans.getRotation().y(), trans.getRotation().z(), trans.getRotation().w());

            physicsBoxUI_->SetLocalPose(pose);
        }

        // 3. 入力処理およびコントローラーのトラッキング更新
        ui_.HitTestDevices().clear();

        if (in.LeftRemoteTracked) {
            controllerRenderL_.Update(in.LeftRemotePose);
            const bool didPinch = in.LeftRemoteIndexTrigger > 0.25f;
            ui_.AddHitTestRay(in.LeftRemotePointPose, didPinch);
        }
        if (in.RightRemoteTracked) {
            controllerRenderR_.Update(in.RightRemotePose);
            const bool didPinch = in.RightRemoteIndexTrigger > 0.25f;
            ui_.AddHitTestRay(in.RightRemotePointPose, didPinch);
        }

        ui_.Update(in);
        beamRenderer_.Update(in, ui_.HitTestDevices());
    }

    // ------------------------------------------------------------------------
    // 毎フレームの描画処理 (Render)
    // ------------------------------------------------------------------------
    virtual void Render(const OVRFW::ovrApplFrameIn& in, OVRFW::ovrRendererOutput& out) override {
        ui_.Render(in, out);

        if (in.LeftRemoteTracked) {
            controllerRenderL_.Render(out.Surfaces);
        }
        if (in.RightRemoteTracked) {
            controllerRenderR_.Render(out.Surfaces);
        }

        beamRenderer_.Render(in, out);
    }

private:
    // --- VR描画・UI用メンバ変数 ---
    OVRFW::ControllerRenderer controllerRenderL_;
    OVRFW::ControllerRenderer controllerRenderR_;
    OVRFW::TinyUI ui_;
    OVRFW::SimpleBeamRenderer beamRenderer_;
    std::vector<OVRFW::ovrBeamRenderer::handle_t> beams_;
    bool delayUI_ = false;

    // --- Bullet Physics用メンバ変数 ---
    btDefaultCollisionConfiguration* collisionConfiguration_;
    btCollisionDispatcher* dispatcher_;
    btBroadphaseInterface* overlappingPairCache_;
    btSequentialImpulseConstraintSolver* solver_;
    btDiscreteDynamicsWorld* dynamicsWorld_;

    // 物理オブジェクト (箱)
    btRigidBody* fallingCube_;
    btCollisionShape* boxShape_;

    // ▼ 追加：物理オブジェクト (床)
    btRigidBody* groundRigidBody_;
    btCollisionShape* groundShape_;

    // VR空間上の見た目用オブジェクト
    OVRFW::VRMenuObject* physicsBoxUI_;
    OVRFW::VRMenuObject* groundUI_; // ▼ 追加：床位置の標識パネル
};

// OpenXRエントリーポイント
ENTRY_POINT(XrControllersApp)