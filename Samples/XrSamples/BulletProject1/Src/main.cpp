#define _HAS_STD_BYTE 0    // WindowsのbyteとC++のbyteの衝突を防ぐ
#define NOMINMAX           // min/max関数の衝突を防ぐ

#include <windows.h>
#include <cstdint>
#include <cstdio>
// ... (以降そのまま)
#include <algorithm>
#include <openxr/openxr.h>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>

#include "XrApp.h"
// --- 以降はそのまま ---


// --- Bullet Physics のインクルード ---
#include <btBulletDynamicsCommon.h>
// -------------------------------------
#include "Input/SkeletonRenderer.h"
#include "Input/ControllerRenderer.h"
#include "Input/TinyUI.h"
#include "Input/AxisRenderer.h"
#include "Render/SimpleBeamRenderer.h"

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
        boxShape_(nullptr) {
        // 背景色
        BackgroundColor = OVR::Vector4f(1.00f, 0.95f, 0.00f, 1.0f);
        OpenXRVersion = XR_API_VERSION_1_0;
    }

    // GetExtensions() は基底クラス(XrApp)の標準機能だけで十分なため、関数ごと丸ごと削除しました。
    // 同様に createPCMSamples と SupportsParametricHaptics も削除しました。

    // Returns a map from interaction profile paths to vectors of suggested bindings.
    // xrSuggestInteractionProfileBindings() is called once for each interaction profile path in the
    // returned map.
    // Apps are encouraged to suggest bindings for every device/interaction profile they support.
    // Overridden to add support for the touch_pro interaction profile
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

    // Must return true if the application initializes successfully.
    virtual bool AppInit(const xrJava* context) override {
        // UIシステムの初期化は今後のために残す
        if (false == ui_.Init(context, GetFileSys())) {
            ALOG("TinyUI::Init FAILED.");
            return false;
        }

        // --- Bullet Physics の初期化をここで行うことも可能ですが、
        // --- 空間のセットアップなどは SessionInit() で行うのが通例です。
        // -----------------------------------------------------------------

        // ▼ 追加：物理演算の箱の代わりとなる、空中に浮かぶ「的」を作成！ ▼
        physicsBoxUI_ = ui_.AddLabel("Physics Box", { 0.0f, 3.0f, -2.0f }, { 500.0f, 500.0f });
        return true;

        return true;
    }

    virtual void AppShutdown(const xrJava* context) override {
        /// unhook extensions

        OVRFW::XrApp::AppShutdown(context);
        ui_.Shutdown();
    }

    virtual bool SessionInit() override {
        /// Use LocalSpace instead of Stage Space.
        CurrentSpace = LocalSpace;
        /// Init session bound objects
        if (false == controllerRenderL_.Init(true)) {
            ALOG("AppInit::Init L controller renderer FAILED.");
            return false;
        }
        if (false == controllerRenderR_.Init(false)) {
            ALOG("AppInit::Init R controller renderer FAILED.");
            return false;
        }
        beamRenderer_.Init(GetFileSys(), nullptr, OVR::Vector4f(1.0f), 1.0f);

        ALOG("Size of btScalar: %d bytes", (int)sizeof(btScalar));
        ALOG("Size of btRigidBody: %d bytes", (int)sizeof(btRigidBody));

        // --- Bullet Physics の初期化 ---
        collisionConfiguration_ = new btDefaultCollisionConfiguration();
        dispatcher_ = new btCollisionDispatcher(collisionConfiguration_);
        overlappingPairCache_ = new btDbvtBroadphase();
        solver_ = new btSequentialImpulseConstraintSolver();
        dynamicsWorld_ = new btDiscreteDynamicsWorld(dispatcher_, overlappingPairCache_, solver_, collisionConfiguration_);
        dynamicsWorld_->setGravity(btVector3(0.0f, -9.8f, 0.0f));

        // 箱の作成
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

        dynamicsWorld_->addRigidBody(fallingCube_);
        ALOG("Bullet Box Added to World!");
        // --------------------------------
        return true;
    }

    virtual void SessionEnd() override {
        controllerRenderL_.Shutdown();
        controllerRenderR_.Shutdown();
        beamRenderer_.Shutdown();

        // --- Bullet Physics の後片付け ---
        // ▼今回追加: ワールドから箱を取り除き、メモリを解放▼
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
        if (dynamicsWorld_) {
            delete dynamicsWorld_;
            delete solver_;
            delete overlappingPairCache_;
            delete dispatcher_;
            delete collisionConfiguration_;
            dynamicsWorld_ = nullptr;
        }
        // ---------------------------------
    }

    virtual void Update(const OVRFW::ovrApplFrameIn& in) override {
        if (dynamicsWorld_) {
            dynamicsWorld_->stepSimulation(in.DeltaSeconds, 10);
        }

        // ▼ 追加：Bullet Physicsの箱の座標を、VRのパネルに完全同期させる！ ▼
        if (fallingCube_ && physicsBoxUI_) {
            btTransform trans;
            fallingCube_->getMotionState()->getWorldTransform(trans);

            // Bulletの座標と回転を、VR用のPose(姿勢)に変換
            OVR::Posef pose;
            pose.Translation = OVR::Vector3f(trans.getOrigin().x(), trans.getOrigin().y(), trans.getOrigin().z());
            pose.Rotation = OVR::Quatf(trans.getRotation().x(), trans.getRotation().y(), trans.getRotation().z(), trans.getRotation().w());

            // VR空間のパネルの位置と傾きを更新
            physicsBoxUI_->SetLocalPose(pose);
        }

        // ... (これより下のコントローラーの入力取得などはそのまま残す) ...

        // --- 必要な入力の取得 ---
        // 箱を出す、リセットするなどの操作のために基本的なボタン入力だけ取得しておきます
        const auto buttonA = GetActionStateBoolean(ButtonAAction);
        const auto buttonB = GetActionStateBoolean(ButtonBAction);
        const auto buttonX = GetActionStateBoolean(ButtonXAction);
        const auto buttonY = GetActionStateBoolean(ButtonYAction);

        // --- UIとコントローラーのトラッキング更新 ---
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

    // Render eye buffers while running
    virtual void Render(const OVRFW::ovrApplFrameIn& in, OVRFW::ovrRendererOutput& out) override {
        // ▼ 追加：UIを描画する（これだけで物理エンジンと同期したパネルが描画されます！） ▼
        ui_.Render(in, out);

        /// Render controllers
        if (in.LeftRemoteTracked) {
            controllerRenderL_.Render(out.Surfaces);
        }
        if (in.RightRemoteTracked) {
            controllerRenderR_.Render(out.Surfaces);
        }
        /// Render beams
        beamRenderer_.Render(in, out);
    }


public:
private:
    // --- 必須の描画・UIシステム ---
    OVRFW::ControllerRenderer controllerRenderL_;
    OVRFW::ControllerRenderer controllerRenderR_;
    OVRFW::TinyUI ui_;
    OVRFW::SimpleBeamRenderer beamRenderer_;
    std::vector<OVRFW::ovrBeamRenderer::handle_t> beams_;

    // UIのラグシミュレーション（念のため残す）
    bool delayUI_ = false;

    // --- Bullet Physics 用の変数 ---
    btDefaultCollisionConfiguration* collisionConfiguration_;
    btCollisionDispatcher* dispatcher_;
    btBroadphaseInterface* overlappingPairCache_;
    btSequentialImpulseConstraintSolver* solver_;
    btDiscreteDynamicsWorld* dynamicsWorld_;

    btRigidBody* fallingCube_;
    btCollisionShape* boxShape_;
    // ---------------------------------
        // ▼ これを追加！ ▼
    OVRFW::VRMenuObject* physicsBoxUI_ = nullptr;
};

ENTRY_POINT(XrControllersApp)
