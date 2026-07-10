#include <cstdint>
#include <cstdio>
#include <algorithm>
#include <openxr/openxr.h>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>

#include "XrApp.h"
#include "utils.h" // 授業の便利関数

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
    XrControllersApp() : OVRFW::XrApp() {
        // 背景色はお好みで変更可能です（R, G, B, A）
        BackgroundColor = OVR::Vector4f(0.60f, 0.95f, 0.4f, 1.0f);
        OpenXRVersion = XR_API_VERSION_1_0;
    }

    // GetExtensions() は基底クラス(XrApp)の標準機能だけで十分なため、関数ごと丸ごと削除しました。
    // 同様に createPCMSamples と SupportsParametricHaptics も削除しました。

    // Returns a map from interaction profile paths to vectors of suggested bindings.
    // xrSuggestInteractionProfileBindings() is called once for each interaction profile path in the
    // returned map.
    // Apps are encouraged to suggest bindings for every device/interaction profile they support.
    // Overridden to add support for the touch_pro interaction profile
    std::unordered_map<XrPath, std::vector<XrActionSuggestedBinding>> GetSuggestedBindings(
        XrInstance instance) override {
        //    …/input/trackpad/x
        //    …/input/trackpad/y
        //    …/input/trackpad/force
        //    …/input/stylus/force
        //    …/input/trigger/curl
        //    …/input/trigger/slide
        //    …/output/trigger_haptic
        //    …/output/thumb_haptic

        XrPath handSubactionPaths[2] = {LeftHandPath, RightHandPath};

        trackpadForceAction_ = CreateAction(
            BaseActionSet,
            XR_ACTION_TYPE_FLOAT_INPUT,
            "the_trackpad_force",
            nullptr,
            2,
            handSubactionPaths);
        triggerForceAction_ = CreateAction(
            BaseActionSet,
            XR_ACTION_TYPE_FLOAT_INPUT,
            "trigger_force",
            nullptr,
            2,
            handSubactionPaths);
        stylusForceAction_ = CreateAction(
            BaseActionSet,
            XR_ACTION_TYPE_FLOAT_INPUT,
            "the_stylus_force",
            nullptr,
            2,
            handSubactionPaths);
        triggerCurlAction_ = CreateAction(
            BaseActionSet,
            XR_ACTION_TYPE_FLOAT_INPUT,
            "the_trigger_curl",
            nullptr,
            2,
            handSubactionPaths);
        triggerSlideAction_ = CreateAction(
            BaseActionSet,
            XR_ACTION_TYPE_FLOAT_INPUT,
            "the_trigger_slide",
            nullptr,
            2,
            handSubactionPaths);

        /// haptics
        mainHapticAction_ = CreateAction(
            BaseActionSet,
            XR_ACTION_TYPE_VIBRATION_OUTPUT,
            "the_main_haptic",
            nullptr,
            2,
            handSubactionPaths);
        triggerHapticAction_ = CreateAction(
            BaseActionSet,
            XR_ACTION_TYPE_VIBRATION_OUTPUT,
            "the_trigger_haptic",
            nullptr,
            2,
            handSubactionPaths);
        thumbHapticAction_ = CreateAction(
            BaseActionSet,
            XR_ACTION_TYPE_VIBRATION_OUTPUT,
            "the_thumb_haptic",
            nullptr,
            2,
            handSubactionPaths);

        // Proximity
        triggerProxAction_ = CreateAction(
            BaseActionSet,
            XR_ACTION_TYPE_BOOLEAN_INPUT,
            "trigger_prox",
            nullptr,
            2,
            handSubactionPaths);
        thumbFbProxAction_ = CreateAction(
            BaseActionSet,
            XR_ACTION_TYPE_BOOLEAN_INPUT,
            "thumb_fb_prox",
            nullptr,
            2,
            handSubactionPaths);
        thumbMetaProxAction_ = CreateAction(
            BaseActionSet,
            XR_ACTION_TYPE_BOOLEAN_INPUT,
            "thumb_meta_prox",
            nullptr,
            2,
            handSubactionPaths);

        // Trigger Value
        triggerValueAction_ = CreateAction(
            BaseActionSet,
            XR_ACTION_TYPE_BOOLEAN_INPUT,
            "trigger_value",
            nullptr,
            2,
            handSubactionPaths);

        // Trigger Touch
        triggerTouchAction_ = CreateAction(
            BaseActionSet,
            XR_ACTION_TYPE_BOOLEAN_INPUT,
            "trigger_touch",
            nullptr,
            2,
            handSubactionPaths);

        // Squeeze Value
        squeezeValueAction_ = CreateAction(
            BaseActionSet,
            XR_ACTION_TYPE_BOOLEAN_INPUT,
            "squeeze_value",
            nullptr,
            2,
            handSubactionPaths);

        XrPath touchInteractionProfile = XR_NULL_PATH;
        OXR(xrStringToPath(
            instance, "/interaction_profiles/meta/touch_controller_quest_2", &touchInteractionProfile));

        XrPath touchProInteractionProfile = XR_NULL_PATH;
        OXR(xrStringToPath(
            instance,
            "/interaction_profiles/meta/touch_pro_controller",
            &touchProInteractionProfile));
        XrPath touchPlusInteractionProfile = XR_NULL_PATH;
        OXR(xrStringToPath(
            instance,
            "/interaction_profiles/meta/touch_plus_controller",
            &touchPlusInteractionProfile));

        std::vector<XrActionSuggestedBinding> baseTouchBindings{};

        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(AimPoseAction, "/user/hand/left/input/aim/pose"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(AimPoseAction, "/user/hand/right/input/aim/pose"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(GripPoseAction, "/user/hand/left/input/grip/pose"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(GripPoseAction, "/user/hand/right/input/grip/pose"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(JoystickAction, "/user/hand/left/input/thumbstick"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(JoystickAction, "/user/hand/right/input/thumbstick"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(thumbstickClickAction, "/user/hand/left/input/thumbstick/click"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(thumbstickClickAction, "/user/hand/right/input/thumbstick/click"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(IndexTriggerAction, "/user/hand/left/input/trigger/value"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(IndexTriggerAction, "/user/hand/right/input/trigger/value"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(GripTriggerAction, "/user/hand/left/input/squeeze/value"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(GripTriggerAction, "/user/hand/right/input/squeeze/value"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(ButtonAAction, "/user/hand/right/input/a/click"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(ButtonBAction, "/user/hand/right/input/b/click"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(ButtonXAction, "/user/hand/left/input/x/click"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(ButtonYAction, "/user/hand/left/input/y/click"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(ButtonMenuAction, "/user/hand/left/input/menu/click"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(ThumbStickTouchAction, "/user/hand/left/input/thumbstick/touch"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(ThumbStickTouchAction, "/user/hand/right/input/thumbstick/touch"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(ThumbRestTouchAction, "/user/hand/left/input/thumbrest/touch"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(ThumbRestTouchAction, "/user/hand/right/input/thumbrest/touch"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(TriggerTouchAction, "/user/hand/left/input/trigger/touch"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(TriggerTouchAction, "/user/hand/right/input/trigger/touch"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(mainHapticAction_, "/user/hand/left/output/haptic"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(mainHapticAction_, "/user/hand/right/output/haptic"));

        // Thumb rest proximity
        baseTouchBindings.emplace_back(ActionSuggestedBinding(
            thumbFbProxAction_, "/user/hand/left/input/thumb_resting_surfaces/proximity"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(
            thumbFbProxAction_, "/user/hand/right/input/thumb_resting_surfaces/proximity"));

        // Trigger Value
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(triggerValueAction_, "/user/hand/left/input/trigger/value"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(triggerValueAction_, "/user/hand/right/input/trigger/value"));

        // Trigger Touch
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(triggerTouchAction_, "/user/hand/left/input/trigger/touch"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(triggerTouchAction_, "/user/hand/right/input/trigger/touch"));

        // Trigger Proximity
        baseTouchBindings.emplace_back(ActionSuggestedBinding(
            triggerProxAction_, "/user/hand/left/input/trigger/proximity"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(
            triggerProxAction_, "/user/hand/right/input/trigger/proximity"));

        // Squeeze Value
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(squeezeValueAction_, "/user/hand/left/input/squeeze/value"));
        baseTouchBindings.emplace_back(
            ActionSuggestedBinding(squeezeValueAction_, "/user/hand/right/input/squeeze/value"));

        // Copy(construct) base paths since these interaction profiles are similar
        // NOTE: Only bindings that are different from touch controller will be listed here.
        // Bindings that also exist on touch controller will be copied from baseTouchBindings
        std::vector<XrActionSuggestedBinding> touchProBindings(baseTouchBindings);

        // We are assuming that every touch binding exists for touch pro here
        // if that is not the case they need to be removed from the vector

        touchProBindings.emplace_back(
            ActionSuggestedBinding(trackpadForceAction_, "/user/hand/left/input/thumbrest/force"));
        touchProBindings.emplace_back(
            ActionSuggestedBinding(trackpadForceAction_, "/user/hand/right/input/thumbrest/force"));
        touchProBindings.emplace_back(
            ActionSuggestedBinding(stylusForceAction_, "/user/hand/left/input/stylus/force"));
        touchProBindings.emplace_back(
            ActionSuggestedBinding(stylusForceAction_, "/user/hand/right/input/stylus/force"));
        touchProBindings.emplace_back(
            ActionSuggestedBinding(triggerCurlAction_, "/user/hand/left/input/trigger_curl/value"));
        touchProBindings.emplace_back(
            ActionSuggestedBinding(triggerCurlAction_, "/user/hand/right/input/trigger_curl/value"));
        touchProBindings.emplace_back(
            ActionSuggestedBinding(triggerSlideAction_, "/user/hand/left/input/trigger_slide/value"));
        touchProBindings.emplace_back(
            ActionSuggestedBinding(triggerSlideAction_, "/user/hand/right/input/trigger_slide/value"));
        touchProBindings.emplace_back(ActionSuggestedBinding(
            triggerHapticAction_, "/user/hand/left/output/haptic_trigger"));
        touchProBindings.emplace_back(ActionSuggestedBinding(
            triggerHapticAction_, "/user/hand/right/output/haptic_trigger"));
        touchProBindings.emplace_back(
            ActionSuggestedBinding(thumbHapticAction_, "/user/hand/left/output/haptic_thumb"));
        touchProBindings.emplace_back(
            ActionSuggestedBinding(thumbHapticAction_, "/user/hand/right/output/haptic_thumb"));

        // Copy(construct) base paths since these interaction profiles are similar
        // NOTE: Only bindings that are different from touch controller will be listed here.
        // Bindings that also exist on touch controller will be copied from baseTouchBindings
        std::vector<XrActionSuggestedBinding> touchPlusBindings(baseTouchBindings);
        touchPlusBindings.emplace_back(
            ActionSuggestedBinding(triggerForceAction_, "/user/hand/left/input/trigger/force"));
        touchPlusBindings.emplace_back(
            ActionSuggestedBinding(triggerForceAction_, "/user/hand/right/input/trigger/force"));
        touchPlusBindings.emplace_back(
            ActionSuggestedBinding(triggerCurlAction_, "/user/hand/left/input/trigger_curl/value"));
        touchPlusBindings.emplace_back(
            ActionSuggestedBinding(triggerCurlAction_, "/user/hand/right/input/trigger_curl/value"));
        touchPlusBindings.emplace_back(ActionSuggestedBinding(
            triggerSlideAction_, "/user/hand/left/input/trigger_slide/value"));
        touchPlusBindings.emplace_back(ActionSuggestedBinding(
            triggerSlideAction_, "/user/hand/right/input/trigger_slide/value"));

        std::unordered_map<XrPath, std::vector<XrActionSuggestedBinding>> allSuggestedBindings;
        allSuggestedBindings[touchInteractionProfile] = baseTouchBindings;
        allSuggestedBindings[touchProInteractionProfile] = touchProBindings;
        allSuggestedBindings[touchPlusInteractionProfile] = touchPlusBindings;
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

        // --- デバッグ用のラベルを1つだけ残す（不要ならこれも消してOK） ---
        bigText_ = ui_.AddLabel("Bullet Physics Test", { 0.0f, 0.0f, -2.0f }, { 1000.0f, 100.0f });
        // -----------------------------------------------------------------

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

        /// enumerate all actions
        EnumerateActions();

        // --- Bullet Physics の初期化 ---
        collisionConfiguration_ = new btDefaultCollisionConfiguration();
        dispatcher_ = new btCollisionDispatcher(collisionConfiguration_);
        overlappingPairCache_ = new btDbvtBroadphase();
        solver_ = new btSequentialImpulseConstraintSolver();
        dynamicsWorld_ = new btDiscreteDynamicsWorld(dispatcher_, overlappingPairCache_, solver_, collisionConfiguration_);
        dynamicsWorld_->setGravity(btVector3(0, -9.8f, 0)); // 重力を設定
        ALOG("Bullet Physics World Initialized!");
        // --------------------------------
        // 
        // 1. 箱の形を作る（XYZそれぞれ幅1m。Bulletでは半分のサイズを指定するため0.5f）
        boxShape_ = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));

        // 2. 初期位置を設定（目の前2m、高さ3mの空中に配置）
        btTransform startTransform;
        startTransform.setIdentity();
        startTransform.setOrigin(btVector3(0.0f, 3.0f, -2.0f));

        // 3. 質量（1kg）と慣性を設定
        btScalar mass(1.0f);
        btVector3 localInertia(0, 0, 0);
        boxShape_->calculateLocalInertia(mass, localInertia);

        // 4. 剛体（RigidBody）を作成してワールドに追加
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

        // --- Bullet Physics の時間を進める ---
        if (dynamicsWorld_) {
            dynamicsWorld_->stepSimulation(in.DeltaSeconds, 10);
        }

        // 箱の座標を取得して出力
        if (fallingCube_) {
            btTransform trans;
            fallingCube_->getMotionState()->getWorldTransform(trans);
            // ALOG("Cube Y Position: %f", trans.getOrigin().getY()); // ログが重い場合はコメントアウト
        }

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
        /// Render UI
        // ui_.Render(in, out); // UIは消したのでコメントアウト

        /// Render controllers
        if (in.LeftRemoteTracked) {
            controllerRenderL_.Render(out.Surfaces);
        }
        if (in.RightRemoteTracked) {
            controllerRenderR_.Render(out.Surfaces);
        }

        /// Render beams
        beamRenderer_.Render(in, out);

        // ▼今回追加：Bullet Physics の世界をVR空間に描画する▼
        // out.Surfaces の数（通常は両目で2つ）だけ描画を繰り返す
        for (int eye = 0; eye < out.NumSurfaces; ++eye) {
            // 現在描画しようとしている目のカメラ情報（ProjectionとView行列）を取得
            const OVRFW::ovrRendererOutput::ovrSurface& surf = out.Surfaces[eye];

            // OpenGLの行列計算モードを切り替え、カメラ情報をセット
            glMatrixMode(GL_PROJECTION);
            glLoadMatrixf((const GLfloat*)&surf.ProjectionMatrix);

            glMatrixMode(GL_MODELVIEW);
            glLoadMatrixf((const GLfloat*)&surf.ViewMatrix);

            // 授業の utils.h にある描画関数を呼び出す！
            // この中で、g_dynamicsworld (今回は dynamicsWorld_) の全オブジェクトがスキャンされ、
            // GetOpenGLMatrix() で変換されたのち、DrawCubeVBO() などで描画されます。
            glPushMatrix();

            // ※注意: utils.h の DrawBulletObjects は g_dynamicsworld というグローバル変数を
            // 参照する作りになっているため、もしこのクラス内で dynamicsWorld_ という名前で
            // 作っている場合は、g_dynamicsworld = dynamicsWorld_; と代入しておく必要があります。
            extern btDynamicsWorld* g_dynamicsworld;
            g_dynamicsworld = dynamicsWorld_;

            DrawBulletObjects();

            glPopMatrix();
        }
        // ▲ここまで▲
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
};

ENTRY_POINT(XrControllersApp)
