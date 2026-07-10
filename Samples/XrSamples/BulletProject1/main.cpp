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

    // Update state
    virtual void Update(const OVRFW::ovrApplFrameIn& in) override {
        
        // --- Bullet Physics の時間を進める ---
        if (dynamicsWorld_) {
            dynamicsWorld_->stepSimulation(in.DeltaSeconds, 10);
        }
        // -------------------------------------
        // ▼今回追加: 箱の座標を取得して出力▼
        if (fallingCube_) {
            btTransform trans;
            fallingCube_->getMotionState()->getWorldTransform(trans);
            // 物理演算された箱のY座標（高さ）をログに流す
            ALOG("Cube Y Position: %f", trans.getOrigin().getY());
        }
        /// Update Input
        {
            /// Trigger Force
            triggerForceL_ = GetActionStateFloat(triggerForceAction_, LeftHandPath).currentState;
            triggerForceR_ = GetActionStateFloat(triggerForceAction_, RightHandPath).currentState;
            /// TrackPad Force
            trackpadForceL_ = GetActionStateFloat(trackpadForceAction_, LeftHandPath).currentState;
            trackpadForceR_ = GetActionStateFloat(trackpadForceAction_, RightHandPath).currentState;
            /// Stylus Force
            stylusForceL_ = GetActionStateFloat(stylusForceAction_, LeftHandPath).currentState;
            stylusForceR_ = GetActionStateFloat(stylusForceAction_, RightHandPath).currentState;
            /// Trigger Curl
            triggerCurlL_ = GetActionStateFloat(triggerCurlAction_, LeftHandPath).currentState;
            triggerCurlR_ = GetActionStateFloat(triggerCurlAction_, RightHandPath).currentState;
            /// Squeeze Curl
            squeezeCurlL_ = GetActionStateFloat(triggerSlideAction_, LeftHandPath).currentState;
            squeezeCurlR_ = GetActionStateFloat(triggerSlideAction_, RightHandPath).currentState;
            /// Proximity
            triggerProxL_ = GetActionStateBoolean(triggerProxAction_, LeftHandPath).currentState;
            triggerProxR_ = GetActionStateBoolean(triggerProxAction_, RightHandPath).currentState;
            thumbFBProxL_ = GetActionStateBoolean(thumbFbProxAction_, LeftHandPath).currentState;
            thumbFBProxR_ = GetActionStateBoolean(thumbFbProxAction_, RightHandPath).currentState;
            // same as above on touch plus
            thumbMetaProxL_ =
                GetActionStateBoolean(thumbMetaProxAction_, LeftHandPath).currentState;
            thumbMetaProxR_ =
                GetActionStateBoolean(thumbMetaProxAction_, RightHandPath).currentState;
            /// Trigger Value
            triggerValueL_ = GetActionStateBoolean(triggerValueAction_, LeftHandPath).currentState;
            triggerValueR_ = GetActionStateBoolean(triggerValueAction_, RightHandPath).currentState;

            /// Trigger Touch
            triggerTouchL_ = GetActionStateBoolean(triggerTouchAction_, LeftHandPath).currentState;
            triggerTouchR_ = GetActionStateBoolean(triggerTouchAction_, RightHandPath).currentState;
            /// Squeeze Value
            squeezeValueL_ = GetActionStateBoolean(squeezeValueAction_, LeftHandPath).currentState;
            squeezeValueR_ = GetActionStateBoolean(squeezeValueAction_, RightHandPath).currentState;
        }

        // we can only request haptic sample rate when the session is in focus
        if (Focused) {
            XrHapticActionInfo hai = {XR_TYPE_HAPTIC_ACTION_INFO, nullptr};
            hai.action = mainHapticAction_;
            hai.subactionPath = LeftHandPath;
            OXR(xrGetDeviceSampleRateFB(Session, &hai, &leftDeviceSampleRate_));

            hai.action = mainHapticAction_;
            hai.subactionPath = RightHandPath;
            OXR(xrGetDeviceSampleRateFB(Session, &hai, &rightDeviceSampleRate_));
        }

        // once per A button press
        const auto buttonA = GetActionStateBoolean(ButtonAAction);
        if (buttonA.currentState == XR_TRUE && buttonA.changedSinceLastSync == XR_TRUE) {
            // Trigger PCM haptics: simple sine wave
            std::vector<float> sineWave =
                createPCMSamples(157, std::size(constantIntensity), constantIntensity, ToXrTime(1));
            VibrateControllerPCM(
                mainHapticAction_, RightHandPath, sineWave.data(), sineWave.size(), 2000.0f);
        }

        // once per B button press
        const auto buttonB = GetActionStateBoolean(ButtonBAction);
        if (buttonB.currentState == XR_TRUE && buttonB.changedSinceLastSync == XR_TRUE) {
            // Trigger AE haptics
            float aeBufferSimple[500]; // 1sec
            for (int i = 0; i < 500; i++) {
                aeBufferSimple[i] = 0.1;
            }

            VibrateControllerAmplitude(
                mainHapticAction_,
                RightHandPath,
                aeBufferSimple,
                std::size(aeBufferSimple),
                0.002f * std::size(aeBufferSimple));
        }

        // once per X button press
        const auto buttonX = GetActionStateBoolean(ButtonXAction);
        if (buttonX.currentState == XR_TRUE && buttonX.changedSinceLastSync == XR_TRUE) {
            // Trigger Localized(thumb) haptics
            VibrateController(thumbHapticAction_, LeftHandPath, 0.1f, 157.0f, 1.0f);
        }

        // once per Y button press
        const auto buttonY = GetActionStateBoolean(ButtonYAction);
        if (buttonY.currentState == XR_TRUE && buttonY.changedSinceLastSync == XR_TRUE) {
            // Trigger Localized(trigger) haptics
            VibrateController(triggerHapticAction_, LeftHandPath, 0.1f, 157.0f, 1.0f);
        }

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

        /// Update labels
        {
            XrInteractionProfileState lIpState{XR_TYPE_INTERACTION_PROFILE_STATE};
            OXR(xrGetCurrentInteractionProfile(Session, LeftHandPath, &lIpState));
            XrInteractionProfileState rIpState{XR_TYPE_INTERACTION_PROFILE_STATE};
            OXR(xrGetCurrentInteractionProfile(Session, LeftHandPath, &rIpState));

            char lBuf[XR_MAX_PATH_LENGTH];
            uint32_t written = 0;
            if (lIpState.interactionProfile != XR_NULL_PATH) {
                OXR(xrPathToString(
                    Instance, lIpState.interactionProfile, XR_MAX_PATH_LENGTH, &written, lBuf));
            }
            if (written == 0) {
                strcpy(lBuf, "<none>");
            }

            char rBuf[XR_MAX_PATH_LENGTH];
            written = 0;
            if (rIpState.interactionProfile != XR_NULL_PATH) {
                OXR(xrPathToString(
                    Instance, rIpState.interactionProfile, XR_MAX_PATH_LENGTH, &written, rBuf));
            }
            if (written == 0) {
                strcpy(rBuf, "<none>");
            }

            std::stringstream ss;
            ss << "Left IP: " << lBuf << std::endl;
            ss << "Right IP: " << rBuf;
            ipText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << std::setprecision(4) << std::fixed;
            ss << triggerForceL_;
            triggerForceLText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << std::setprecision(4) << std::fixed;
            ss << triggerForceR_;
            triggerForceRText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << std::setprecision(4) << std::fixed;
            ss << trackpadForceL_;
            trackpadForceLText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << std::setprecision(4) << std::fixed;
            ss << trackpadForceR_;
            trackpadForceRText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << std::setprecision(4) << std::fixed;
            ss << stylusForceL_;
            stylusForceLText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << std::setprecision(4) << std::fixed;
            ss << stylusForceR_;
            stylusForceRText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << std::setprecision(4) << std::fixed;
            ss << triggerCurlL_;
            triggerCurlLText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << std::setprecision(4) << std::fixed;
            ss << triggerCurlR_;
            triggerCurlRText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << std::setprecision(4) << std::fixed;
            ss << squeezeCurlL_;
            squeezeCurlLText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << std::setprecision(4) << std::fixed;
            ss << squeezeCurlR_;
            squeezeCurlRText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << "PCM Haptic\n[SR: ";
            ss << std::setprecision(1) << std::fixed;
            ss << leftDeviceSampleRate_.sampleRate << ", ";
            ss << rightDeviceSampleRate_.sampleRate << "]";
            pcmHapticText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << std::setprecision(4) << std::fixed;
            ss << triggerProxL_;
            triggerProxLText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << std::setprecision(4) << std::fixed;
            ss << triggerProxR_;
            triggerProxRText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << "_FB: " << thumbFBProxL_;
            thumbFBProxLText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << "_FB: " << thumbFBProxR_;
            thumbFBProxRText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << "_META: " << thumbMetaProxL_;
            thumbMetaProxLText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << "_META: " << thumbMetaProxR_;
            thumbMetaProxRText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << std::setprecision(4) << std::fixed;
            ss << triggerValueL_;
            triggerValueLText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << std::setprecision(4) << std::fixed;
            ss << triggerValueR_;
            triggerValueRText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << std::setprecision(4) << std::fixed;
            ss << triggerTouchL_;
            triggerTouchLText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << std::setprecision(4) << std::fixed;
            ss << triggerTouchR_;
            triggerTouchRText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << std::setprecision(4) << std::fixed;
            ss << squeezeValueL_;
            squeezeValueLText_->SetText(ss.str().c_str());
        }
        {
            std::stringstream ss;
            ss << std::setprecision(4) << std::fixed;
            ss << squeezeValueR_;
            squeezeValueRText_->SetText(ss.str().c_str());
        }

        /*
         */

        ui_.Update(in);
        beamRenderer_.Update(in, ui_.HitTestDevices());

        /// Add some deliberate lag to the app
        if (delayUI_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }
    }

    // Render eye buffers while running
    virtual void Render(const OVRFW::ovrApplFrameIn& in, OVRFW::ovrRendererOutput& out) override {
        /// Render UI
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

    void EnumerateActions() {
        // Enumerate actions
        XrPath actionPathsBuffer[16];
        char stringBuffer[256];
        XrAction actionsToEnumerate[] = {
            /// new actions
            triggerForceAction_,
            thumbMetaProxAction_,
            trackpadForceAction_,
            stylusForceAction_,
            triggerCurlAction_,
            triggerSlideAction_,
            /// existing actions form base class
            IndexTriggerAction,
            GripTriggerAction,
            triggerProxAction_,
            thumbFbProxAction_,
            triggerValueAction_,
            triggerTouchAction_,
            squeezeValueAction_,
        };
        for (size_t i = 0; i < sizeof(actionsToEnumerate) / sizeof(actionsToEnumerate[0]); ++i) {
            XrBoundSourcesForActionEnumerateInfo enumerateInfo = {
                XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO};
            enumerateInfo.action = actionsToEnumerate[i];

            // Get Count
            uint32_t countOutput = 0;
            OXR(xrEnumerateBoundSourcesForAction(
                Session, &enumerateInfo, 0 /* request size */, &countOutput, nullptr));
            ALOGV(
                "xrEnumerateBoundSourcesForAction action=%lld count=%u",
                (long long)enumerateInfo.action,
                countOutput);

            if (countOutput < 16) {
                OXR(xrEnumerateBoundSourcesForAction(
                    Session, &enumerateInfo, 16, &countOutput, actionPathsBuffer));
                for (uint32_t a = 0; a < countOutput; ++a) {
                    XrInputSourceLocalizedNameGetInfo nameGetInfo = {
                        XR_TYPE_INPUT_SOURCE_LOCALIZED_NAME_GET_INFO};
                    nameGetInfo.sourcePath = actionPathsBuffer[a];
                    nameGetInfo.whichComponents = XR_INPUT_SOURCE_LOCALIZED_NAME_USER_PATH_BIT |
                        XR_INPUT_SOURCE_LOCALIZED_NAME_INTERACTION_PROFILE_BIT |
                        XR_INPUT_SOURCE_LOCALIZED_NAME_COMPONENT_BIT;

                    uint32_t stringCount = 0u;
                    OXR(xrGetInputSourceLocalizedName(
                        Session, &nameGetInfo, 0, &stringCount, nullptr));
                    if (stringCount < 256) {
                        OXR(xrGetInputSourceLocalizedName(
                            Session, &nameGetInfo, 256, &stringCount, stringBuffer));
                        char pathStr[256];
                        uint32_t strLen = 0;
                        OXR(xrPathToString(
                            Instance,
                            actionPathsBuffer[a],
                            (uint32_t)sizeof(pathStr),
                            &strLen,
                            pathStr));
                        ALOGV(
                            "Xr##  -> path = %lld `%s` -> `%s`",
                            (long long)actionPathsBuffer[a],
                            pathStr,
                            stringBuffer);
                    }
                }
            }
        }
    }

    void VibrateController(
        const XrAction& action,
        const XrPath& subactionPath,
        float duration,
        float frequency,
        float amplitude) {
        // fire haptics using output action
        XrHapticVibration v{XR_TYPE_HAPTIC_VIBRATION, nullptr};
        v.amplitude = amplitude;
        v.duration = ToXrTime(duration);
        v.frequency = frequency;
        XrHapticActionInfo hai = {XR_TYPE_HAPTIC_ACTION_INFO, nullptr};
        hai.action = action;
        hai.subactionPath = subactionPath;
        OXR(xrApplyHapticFeedback(Session, &hai, (const XrHapticBaseHeader*)&v));
    }

    void VibrateControllerAmplitude(
        const XrAction& action,
        const XrPath& subactionPath,
        const float* envelope,
        const size_t envelopeSize,
        const float durationSecs) {
        /// fill in the amplitude buffer
        std::vector<float> amplitudes(envelope, envelope + envelopeSize);
        // fire haptics using output action
        XrHapticAmplitudeEnvelopeVibrationFB v{
            XR_TYPE_HAPTIC_AMPLITUDE_ENVELOPE_VIBRATION_FB, nullptr};
        v.duration = ToXrTime(durationSecs);
        v.amplitudeCount = (uint32_t)envelopeSize;
        v.amplitudes = amplitudes.data();
        XrHapticActionInfo hai = {XR_TYPE_HAPTIC_ACTION_INFO, nullptr};
        hai.action = action;
        hai.subactionPath = subactionPath;
        OXR(xrApplyHapticFeedback(Session, &hai, (const XrHapticBaseHeader*)&v));
    }

    void VibrateControllerPCM(
        const XrAction& action,
        const XrPath& subactionPath,
        const float* buffer,
        const size_t bufferSize,
        float sampleRate) {
        // stream and sleep on a separate thread,
        // so that we don't lock up the entire app
        std::thread t([action, subactionPath, buffer, bufferSize, sampleRate, this]() {
        /// fill in the amplitude buffer
        std::vector<float> pcmBuffer(bufferSize);
        for (size_t i = 0; i < bufferSize; ++i) {
            pcmBuffer[i] = buffer[i];
        }
        // fire haptics using output action
        XrHapticPcmVibrationFB v{XR_TYPE_HAPTIC_PCM_VIBRATION_FB, nullptr};
        v.sampleRate = sampleRate;
        v.bufferSize = bufferSize;
        v.buffer = pcmBuffer.data();
        uint32_t samplesUsed = 0;
        v.samplesConsumed = &samplesUsed;
        v.append = XR_FALSE;
        XrHapticActionInfo hai = {XR_TYPE_HAPTIC_ACTION_INFO, nullptr};
        hai.action = action;
        hai.subactionPath = subactionPath;
        OXR(xrApplyHapticFeedback(Session, &hai, (const XrHapticBaseHeader*)&v));
        samplesUsed = *(v.samplesConsumed);
        ALOG("Initial Haptics PCM Buffer Count Output: %d", samplesUsed);
        uint32_t totalSamplesUsed = samplesUsed;
        while (totalSamplesUsed < bufferSize) {
            ALOG("TotalSamplesUsed: %d", totalSamplesUsed);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            int newBufferSize = bufferSize - totalSamplesUsed;
            pcmBuffer.resize(newBufferSize);
            for (int i = 0; i < newBufferSize; ++i) {
                pcmBuffer[i] = buffer[i + totalSamplesUsed];
            }
            v.bufferSize = newBufferSize;
            v.buffer = pcmBuffer.data();
            v.append = XR_TRUE;
            OXR(xrApplyHapticFeedback(Session, &hai, (const XrHapticBaseHeader*)&v));
            samplesUsed = *(v.samplesConsumed);
            if (samplesUsed == 0) {
                ALOG("No samples used; stopping logging.");
                break;
            }
            totalSamplesUsed += samplesUsed;
            ALOG("Haptics PCM Buffer Count Output: %d", *(v.samplesConsumed));
        }
        });
        t.detach();
    }

    void VibrateControllerParametric(
        const XrAction& action,
        const XrPath& subactionPath)
    {
        if (!SupportsParametricHaptics()) {
            return;
        }

        const std::vector<XrHapticParametricPointEXTX1> amplitudePoints{{
            {0, 0.0}, {100000000, 1.0}, {270000000, 1.0}, {480000000, 0.7}, {750000000, 0.6},
            {1000000000, 1.0}}};
         const std::vector<XrHapticParametricPointEXTX1> frequencyPoints{{
            {0, 1.0}, {1000000000, 0.0}}};
         const std::vector<XrHapticParametricTransientEXTX1> transients{{
            {600000000, 1.0, 1.0}}};

        XrHapticParametricVibrationEXTX1 parametricVibration{
            XR_TYPE_HAPTIC_PARAMETRIC_VIBRATION_EXTX1};
        parametricVibration.amplitudePointCount = amplitudePoints.size();
        parametricVibration.amplitudePoints = amplitudePoints.data();
        parametricVibration.frequencyPointCount = frequencyPoints.size();
        parametricVibration.frequencyPoints = frequencyPoints.data();
        parametricVibration.transientCount = transients.size();
        parametricVibration.transients = transients.data();
        parametricVibration.minFrequencyHz = XR_FREQUENCY_UNSPECIFIED;
        parametricVibration.maxFrequencyHz = XR_FREQUENCY_UNSPECIFIED;
        parametricVibration.streamFrameType = XR_HAPTIC_PARAMETRIC_STREAM_FRAME_TYPE_NONE_EXTX1;
        XrHapticActionInfo hapticActionInfo{XR_TYPE_HAPTIC_ACTION_INFO};
        hapticActionInfo.action = action;
        hapticActionInfo.subactionPath = subactionPath;
        OXR(xrApplyHapticFeedback(
            Session, &hapticActionInfo, reinterpret_cast<const XrHapticBaseHeader*>(&parametricVibration)));
    }

    void StopHapticEffect(const XrAction& action, const XrPath& subactionPath) {
        XrHapticActionInfo hai = {XR_TYPE_HAPTIC_ACTION_INFO, nullptr};
        hai.action = action;
        hai.subactionPath = subactionPath;
        OXR(xrStopHapticFeedback(Session, &hai));
    }

   public:
   private:
    OVRFW::ControllerRenderer controllerRenderL_;
    OVRFW::ControllerRenderer controllerRenderR_;
    OVRFW::TinyUI ui_;
    OVRFW::SimpleBeamRenderer beamRenderer_;
    std::vector<OVRFW::ovrBeamRenderer::handle_t> beams_;

    OVRFW::VRMenuObject* bigText_ = nullptr;
    OVRFW::VRMenuObject* ipText_ = nullptr;
    XrAction triggerForceAction_ = XR_NULL_HANDLE;
    float triggerForceL_ = 0.0f;
    float triggerForceR_ = 0.0f;
    OVRFW::VRMenuObject* triggerForceLText_ = nullptr;
    OVRFW::VRMenuObject* triggerForceRText_ = nullptr;

    XrAction trackpadForceAction_ = XR_NULL_HANDLE;
    float trackpadForceL_ = 0.0f;
    float trackpadForceR_ = 0.0f;
    OVRFW::VRMenuObject* trackpadForceLText_ = nullptr;
    OVRFW::VRMenuObject* trackpadForceRText_ = nullptr;

    XrAction stylusForceAction_ = XR_NULL_HANDLE;
    float stylusForceL_ = 0.0f;
    float stylusForceR_ = 0.0f;
    OVRFW::VRMenuObject* stylusForceLText_ = nullptr;
    OVRFW::VRMenuObject* stylusForceRText_ = nullptr;

    XrAction triggerCurlAction_ = XR_NULL_HANDLE;
    float triggerCurlL_ = 0.0f;
    float triggerCurlR_ = 0.0f;
    OVRFW::VRMenuObject* triggerCurlLText_ = nullptr;
    OVRFW::VRMenuObject* triggerCurlRText_ = nullptr;

    XrAction triggerSlideAction_ = XR_NULL_HANDLE;
    float squeezeCurlL_ = 0.0f;
    float squeezeCurlR_ = 0.0f;
    OVRFW::VRMenuObject* squeezeCurlLText_ = nullptr;
    OVRFW::VRMenuObject* squeezeCurlRText_ = nullptr;

    XrDevicePcmSampleRateGetInfoFB rightDeviceSampleRate_{
        XR_TYPE_DEVICE_PCM_SAMPLE_RATE_GET_INFO_FB};
    XrDevicePcmSampleRateGetInfoFB leftDeviceSampleRate_{
        XR_TYPE_DEVICE_PCM_SAMPLE_RATE_GET_INFO_FB};
    OVRFW::VRMenuObject* pcmHapticText_ = nullptr;

    XrAction mainHapticAction_ = XR_NULL_HANDLE;
    XrAction triggerHapticAction_ = XR_NULL_HANDLE;
    XrAction thumbHapticAction_ = XR_NULL_HANDLE;

    // Proximity
    XrAction triggerProxAction_ = XR_NULL_HANDLE;
    bool triggerProxL_ = false;
    bool triggerProxR_ = false;
    OVRFW::VRMenuObject* triggerProxLText_ = nullptr;
    OVRFW::VRMenuObject* triggerProxRText_ = nullptr;

    XrAction thumbFbProxAction_ = XR_NULL_HANDLE;
    bool thumbFBProxL_ = false;
    bool thumbFBProxR_ = false;
    OVRFW::VRMenuObject* thumbFBProxLText_ = nullptr;
    OVRFW::VRMenuObject* thumbFBProxRText_ = nullptr;
    XrAction thumbMetaProxAction_ = XR_NULL_HANDLE;
    bool thumbMetaProxL_ = false;
    bool thumbMetaProxR_ = false;
    OVRFW::VRMenuObject* thumbMetaProxLText_ = nullptr;
    OVRFW::VRMenuObject* thumbMetaProxRText_ = nullptr;

    // Trigger Value
    XrAction triggerValueAction_ = XR_NULL_HANDLE;
    bool triggerValueL_ = false;
    bool triggerValueR_ = false;
    OVRFW::VRMenuObject* triggerValueLText_ = nullptr;
    OVRFW::VRMenuObject* triggerValueRText_ = nullptr;

    // Trigger Touch
    XrAction triggerTouchAction_ = XR_NULL_HANDLE;
    bool triggerTouchL_ = false;
    bool triggerTouchR_ = false;
    OVRFW::VRMenuObject* triggerTouchLText_ = nullptr;
    OVRFW::VRMenuObject* triggerTouchRText_ = nullptr;

    // Squeeze Value
    XrAction squeezeValueAction_ = XR_NULL_HANDLE;
    bool squeezeValueL_ = false;
    bool squeezeValueR_ = false;
    OVRFW::VRMenuObject* squeezeValueLText_ = nullptr;
    OVRFW::VRMenuObject* squeezeValueRText_ = nullptr;

    /// UI lag
    bool delayUI_ = false;

    // --- Bullet Physics 用の変数 ---
    btDefaultCollisionConfiguration* collisionConfiguration_;
    btCollisionDispatcher* dispatcher_;
    btBroadphaseInterface* overlappingPairCache_;
    btSequentialImpulseConstraintSolver* solver_;
    btDiscreteDynamicsWorld* dynamicsWorld_;
    // ---------------------------------

    btRigidBody* fallingCube_;
    btCollisionShape* boxShape_;
    // ---------------------------------
};

ENTRY_POINT(XrControllersApp)
