// ============================================================================
// プリプロセッサマクロ定義 (マクロ・型衝突防止)
// ============================================================================
#define _HAS_STD_BYTE 0    // Windows SDKの「byte」と C++17 <cstddef> の型衝突を防止
#define NOMINMAX           // <windows.h> 内の min/max マクロ定義を無効化

#include <windows.h>       // 一番最初に読み込む
#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <openxr/openxr.h>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>

#include "XrApp.h"
#include <btBulletDynamicsCommon.h>

#include "Input/SkeletonRenderer.h"
#include "Input/ControllerRenderer.h"
#include "Input/TinyUI.h"
#include "Input/AxisRenderer.h"
#include "Render/SimpleBeamRenderer.h"

// ▼ 追加：3D図形描画のためのヘッダーをインクルード
#include "Render/GeometryRenderer.h"
#include "Render/GeometryBuilder.h"

class XrControllersApp : public OVRFW::XrApp {
public:
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
        groundShape_(nullptr) {

        // 背景色（ダークグレー）
        BackgroundColor = OVR::Vector4f(0.20f, 0.20f, 0.20f, 1.0f);
        OpenXRVersion = XR_API_VERSION_1_0;
    }

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

    virtual bool AppInit(const xrJava* context) override {
        if (false == ui_.Init(context, GetFileSys())) {
            ALOG("TinyUI::Init FAILED.");
            return false;
        }

        // --------------------------------------------------------------------
        // ▼ 追加：3D描画用モデルの生成 (GeometryBuilderを使用)
        // --------------------------------------------------------------------
        // 1. 落下する箱の描画設定（オレンジ色）
        OVRFW::GeometryBuilder boxBuilder;
        boxBuilder.Add(OVRFW::BuildUnitCubeDescriptor(), OVRFW::GeometryBuilder::kInvalidIndex, { 1.0f, 0.5f, 0.0f, 1.0f });
        boxRenderer_.Init(boxBuilder.ToGeometryDescriptor());
        boxRenderer_.SetScale({ 1.0f, 1.0f, 1.0f }); // 1m x 1m x 1m

        // 2. 床の描画設定（濃いグレー）
        OVRFW::GeometryBuilder floorBuilder;
        floorBuilder.Add(OVRFW::BuildUnitCubeDescriptor(), OVRFW::GeometryBuilder::kInvalidIndex, { 0.3f, 0.3f, 0.3f, 1.0f });
        floorRenderer_.Init(floorBuilder.ToGeometryDescriptor());
        floorRenderer_.SetScale({ 10.0f, 0.1f, 10.0f }); // 10m x 0.1m x 10m の広大な板にスケール

        return true;
    }

    virtual void AppShutdown(const xrJava* context) override {
        boxRenderer_.Shutdown();
        floorRenderer_.Shutdown();
        OVRFW::XrApp::AppShutdown(context);
        ui_.Shutdown();
    }

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
        dynamicsWorld_->setGravity(btVector3(0.0f, -9.8f, 0.0f));

        // 1. 静的な「床」の追加
        groundShape_ = new btStaticPlaneShape(btVector3(0.0f, 1.0f, 0.0f), 0.0f);
        btTransform groundTransform;
        groundTransform.setIdentity();
        groundTransform.setOrigin(btVector3(0.0f, 0.0f, 0.0f));

        btScalar groundMass(0.0f); // 質量0＝動かない
        btVector3 groundLocalInertia(0.0f, 0.0f, 0.0f);
        btDefaultMotionState* groundMotionState = new btDefaultMotionState(groundTransform);
        btRigidBody::btRigidBodyConstructionInfo groundRbInfo(groundMass, groundMotionState, groundShape_, groundLocalInertia);
        groundRigidBody_ = new btRigidBody(groundRbInfo);

        groundRigidBody_->setRestitution(0.6f); // 60% の跳ね返り
        groundRigidBody_->setFriction(0.8f);
        dynamicsWorld_->addRigidBody(groundRigidBody_);

        // 2. 落下する「箱」の追加
        boxShape_ = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
        btTransform startTransform;
        startTransform.setIdentity();
        startTransform.setOrigin(btVector3(0.0f, 3.0f, -2.0f));

        btScalar mass(1.0f);
        btVector3 localInertia(0.0f, 0.0f, 0.0f);
        boxShape_->calculateLocalInertia(mass, localInertia);

        btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, boxShape_, localInertia);
        fallingCube_ = new btRigidBody(rbInfo);

        fallingCube_->setRestitution(0.6f); // 床とぶつかった時に跳ね返る
        fallingCube_->setFriction(0.8f);
        dynamicsWorld_->addRigidBody(fallingCube_);

        return true;
    }

    virtual void SessionEnd() override {
        controllerRenderL_.Shutdown();
        controllerRenderR_.Shutdown();
        beamRenderer_.Shutdown();

        if (fallingCube_) {
            dynamicsWorld_->removeRigidBody(fallingCube_);
            delete fallingCube_->getMotionState();
            delete fallingCube_;
            fallingCube_ = nullptr;
        }
        if (boxShape_) { delete boxShape_; boxShape_ = nullptr; }

        if (groundRigidBody_) {
            dynamicsWorld_->removeRigidBody(groundRigidBody_);
            delete groundRigidBody_->getMotionState();
            delete groundRigidBody_;
            groundRigidBody_ = nullptr;
        }
        if (groundShape_) { delete groundShape_; groundShape_ = nullptr; }

        if (dynamicsWorld_) {
            delete dynamicsWorld_;
            delete solver_;
            delete overlappingPairCache_;
            delete dispatcher_;
            delete collisionConfiguration_;
            dynamicsWorld_ = nullptr;
        }
    }

    virtual void Update(const OVRFW::ovrApplFrameIn& in) override {
        // 1. 物理シミュレーションを1ステップ進める
        if (dynamicsWorld_) {
            dynamicsWorld_->stepSimulation(in.DeltaSeconds, 10);
        }

        // --- Aボタンで箱をリセットしてバウンドさせる ---
        const auto stateA = GetActionStateBoolean(ButtonAAction);
        bool isButtonAPressed = (stateA.isActive && stateA.currentState == XR_TRUE);
        static bool prevButtonA = false;

        if (isButtonAPressed && !prevButtonA && fallingCube_) {
            btTransform trans;
            trans.setIdentity();
            trans.setOrigin(btVector3(0.0f, 3.0f, -2.0f));

            // ランダムな傾きをつける
            btQuaternion quat;
            quat.setEuler(0.8f, 0.5f, 0.3f);
            trans.setRotation(quat);

            fallingCube_->setWorldTransform(trans);
            fallingCube_->getMotionState()->setWorldTransform(trans);
            fallingCube_->setLinearVelocity(btVector3(0, 0, 0));
            fallingCube_->setAngularVelocity(btVector3(0, 0, 0));
            fallingCube_->clearForces();
        }
        prevButtonA = isButtonAPressed;

        // 2. 物理演算された座標を3D描画モデル(GeometryRenderer)に同期
        if (fallingCube_) {
            btTransform trans;
            fallingCube_->getMotionState()->getWorldTransform(trans);

            OVR::Posef pose;
            pose.Translation = OVR::Vector3f(trans.getOrigin().x(), trans.getOrigin().y(), trans.getOrigin().z());
            pose.Rotation = OVR::Quatf(trans.getRotation().x(), trans.getRotation().y(), trans.getRotation().z(), trans.getRotation().w());

            boxRenderer_.SetPose(pose);
            boxRenderer_.Update();
        }

        // 床の描画位置の更新
        // （0.1mの厚さを持つキューブを潰しているため、Y軸方向に -0.05m 下げて上面を物理の0.0mに合わせる）
        OVR::Posef floorPose = OVR::Posef::Identity();
        floorPose.Translation = { 0.0f, -0.05f, 0.0f };
        floorRenderer_.SetPose(floorPose);
        floorRenderer_.Update();

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

    virtual void Render(const OVRFW::ovrApplFrameIn& in, OVRFW::ovrRendererOutput& out) override {
        ui_.Render(in, out);

        // ▼ 追加：3Dの箱と床を描画リストに追加！
        boxRenderer_.Render(out.Surfaces);
        floorRenderer_.Render(out.Surfaces);

        if (in.LeftRemoteTracked) {
            controllerRenderL_.Render(out.Surfaces);
        }
        if (in.RightRemoteTracked) {
            controllerRenderR_.Render(out.Surfaces);
        }
        beamRenderer_.Render(in, out);
    }

private:
    OVRFW::ControllerRenderer controllerRenderL_;
    OVRFW::ControllerRenderer controllerRenderR_;
    OVRFW::TinyUI ui_;
    OVRFW::SimpleBeamRenderer beamRenderer_;
    std::vector<OVRFW::ovrBeamRenderer::handle_t> beams_;

    // ▼ 追加：3D描画用レンダラー
    OVRFW::GeometryRenderer boxRenderer_;
    OVRFW::GeometryRenderer floorRenderer_;

    // --- Bullet Physics用メンバ変数 ---
    btDefaultCollisionConfiguration* collisionConfiguration_;
    btCollisionDispatcher* dispatcher_;
    btBroadphaseInterface* overlappingPairCache_;
    btSequentialImpulseConstraintSolver* solver_;
    btDiscreteDynamicsWorld* dynamicsWorld_;

    // 物理オブジェクト
    btRigidBody* fallingCube_;
    btCollisionShape* boxShape_;
    btRigidBody* groundRigidBody_;
    btCollisionShape* groundShape_;
};

ENTRY_POINT(XrControllersApp)