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
        physicsBoxUI_(nullptr) {

        // VR空間のクリア（背景）カラーを設定 (RGBA: 黄色)
        BackgroundColor = OVR::Vector4f(1.00f, 0.95f, 0.00f, 1.0f);
        OpenXRVersion = XR_API_VERSION_1_0;
    }

    // ------------------------------------------------------------------------
    // 入力アクションのマッピング設定 (GetSuggestedBindings)
    // Questコントローラーの各ボタンやスティックの入力をOpenXRのアクションに割り当てる
    // ------------------------------------------------------------------------
    std::unordered_map<XrPath, std::vector<XrActionSuggestedBinding>> GetSuggestedBindings(XrInstance instance) override {
        XrPath touchInteractionProfile = XR_NULL_PATH;
        OXR(xrStringToPath(instance, "/interaction_profiles/meta/touch_controller_quest_2", &touchInteractionProfile));
        XrPath touchProInteractionProfile = XR_NULL_PATH;
        OXR(xrStringToPath(instance, "/interaction_profiles/meta/touch_pro_controller", &touchProInteractionProfile));
        XrPath touchPlusInteractionProfile = XR_NULL_PATH;
        OXR(xrStringToPath(instance, "/interaction_profiles/meta/touch_plus_controller", &touchPlusInteractionProfile));

        std::vector<XrActionSuggestedBinding> baseTouchBindings{};
        // ポーズ（位置・回転）トラッキング
        baseTouchBindings.emplace_back(ActionSuggestedBinding(AimPoseAction, "/user/hand/left/input/aim/pose"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(AimPoseAction, "/user/hand/right/input/aim/pose"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(GripPoseAction, "/user/hand/left/input/grip/pose"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(GripPoseAction, "/user/hand/right/input/grip/pose"));

        // アナログスティック
        baseTouchBindings.emplace_back(ActionSuggestedBinding(JoystickAction, "/user/hand/left/input/thumbstick"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(JoystickAction, "/user/hand/right/input/thumbstick"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(thumbstickClickAction, "/user/hand/left/input/thumbstick/click"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(thumbstickClickAction, "/user/hand/right/input/thumbstick/click"));

        // トリガー＆グリップ
        baseTouchBindings.emplace_back(ActionSuggestedBinding(IndexTriggerAction, "/user/hand/left/input/trigger/value"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(IndexTriggerAction, "/user/hand/right/input/trigger/value"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(GripTriggerAction, "/user/hand/left/input/squeeze/value"));
        baseTouchBindings.emplace_back(ActionSuggestedBinding(GripTriggerAction, "/user/hand/right/input/squeeze/value"));

        // ボタン類
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
    // プログラム起動時に1度だけ呼ばれる（描画・UIシステムの初期化など）
    // ------------------------------------------------------------------------
    virtual bool AppInit(const xrJava* context) override {
        // VR用UIフレームワーク (TinyUI) の初期化
        if (false == ui_.Init(context, GetFileSys())) {
            ALOG("TinyUI::Init FAILED.");
            return false;
        }

        // 物理演算された箱の位置に重ねて表示するためのラベル(UIパネル)を作成
        // 初期位置: X=0m, Y=3m, Z=-2m（前方2m、高さ3m）
        physicsBoxUI_ = ui_.AddLabel("Physics Box", { 0.0f, 3.0f, -2.0f }, { 500.0f, 500.0f });
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
    // ヘッドセットが接続され、VR空間の追跡が始まったタイミングで実行される
    // ------------------------------------------------------------------------
    virtual bool SessionInit() override {
        // VR空間の基準座標系を LocalSpace（頭位置基準）に設定
        CurrentSpace = LocalSpace;

        // 左右のコントローラー描画モデルの初期化
        if (false == controllerRenderL_.Init(true)) {
            ALOG("AppInit::Init L controller renderer FAILED.");
            return false;
        }
        if (false == controllerRenderR_.Init(false)) {
            ALOG("AppInit::Init R controller renderer FAILED.");
            return false;
        }

        // コントローラーから伸びるビーム（光線）描画の初期化
        beamRenderer_.Init(GetFileSys(), nullptr, OVR::Vector4f(1.0f), 1.0f);

        // --------------------------------------------------------------------
        // Bullet Physics ワールドの構築
        // --------------------------------------------------------------------
        // 衝突判定の設定・ディスパッチャ・広域アルゴリズム・ソルバーの構築
        collisionConfiguration_ = new btDefaultCollisionConfiguration();
        dispatcher_ = new btCollisionDispatcher(collisionConfiguration_);
        overlappingPairCache_ = new btDbvtBroadphase();
        solver_ = new btSequentialImpulseConstraintSolver();

        // 物理ワールドの生成
        dynamicsWorld_ = new btDiscreteDynamicsWorld(dispatcher_, overlappingPairCache_, solver_, collisionConfiguration_);

        // 重力を設定 (Y軸負の方向へ -9.8 m/s^2)
        dynamicsWorld_->setGravity(btVector3(0.0f, -9.8f, 0.0f));
        ALOG("Bullet Physics World Initialized!");

        // --------------------------------------------------------------------
        // 物理オブジェクト（落下する箱）の追加
        // --------------------------------------------------------------------
        // 1. 形状の定義（幅1m x 高さ1m x 奥行1m -> 半径 0.5m のボックス）
        boxShape_ = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));

        // 2. 初期トランスフォーム（位置と回転）の設定
        btTransform startTransform;
        startTransform.setIdentity();
        startTransform.setOrigin(btVector3(0.0f, 3.0f, -2.0f)); // 頭上3m、前方2m

        // 3. 質量と慣性モーメントの計算
        btScalar mass(1.0f); // 質量 1kg
        btVector3 localInertia(0.0f, 0.0f, 0.0f);
        boxShape_->calculateLocalInertia(mass, localInertia);

        // 4. MotionState (位置追跡用) と RigidBody (剛体) の生成
        btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, boxShape_, localInertia);
        fallingCube_ = new btRigidBody(rbInfo);

        // 5. 剛体を物理ワールドに追加
        dynamicsWorld_->addRigidBody(fallingCube_);
        ALOG("Bullet Box Added to World!");

        return true;
    }

    // ------------------------------------------------------------------------
    // VRセッション終了処理 (SessionEnd)
    // メモリリーク（解放忘れ）を防ぐための後片付け
    // ------------------------------------------------------------------------
    virtual void SessionEnd() override {
        controllerRenderL_.Shutdown();
        controllerRenderR_.Shutdown();
        beamRenderer_.Shutdown();

        // 物理オブジェクトの破棄
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

        // 物理ワールドの破棄（生成の逆順で解放）
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
        // 1. 物理シミュレーションを1ステップ進める (時間の経過を計算)
        if (dynamicsWorld_) {
            dynamicsWorld_->stepSimulation(in.DeltaSeconds, 10);
        }

        // 2. 物理演算された箱の位置・回転を取得し、VR空間の描画オブジェクト(UIパネル)に同期
        if (fallingCube_ && physicsBoxUI_) {
            btTransform trans;
            fallingCube_->getMotionState()->getWorldTransform(trans);

            // Bulletの座標・回転(btTransform) を Meta OpenXR用の姿勢(OVR::Posef) に変換
            OVR::Posef pose;
            pose.Translation = OVR::Vector3f(trans.getOrigin().x(), trans.getOrigin().y(), trans.getOrigin().z());
            pose.Rotation = OVR::Quatf(trans.getRotation().x(), trans.getRotation().y(), trans.getRotation().z(), trans.getRotation().w());

            // UIパネルの位置・傾きを物理オブジェクトの計算結果に追従させる
            physicsBoxUI_->SetLocalPose(pose);
        }

        // 3. コントローラーのボタン入力取得（今後の拡張用）
        const auto buttonA = GetActionStateBoolean(ButtonAAction);
        const auto buttonB = GetActionStateBoolean(ButtonBAction);
        const auto buttonX = GetActionStateBoolean(ButtonXAction);
        const auto buttonY = GetActionStateBoolean(ButtonYAction);

        // 4. UIのレイキャスト（光線判定）およびコントローラー描画位置の更新
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
        // UIパネルの描画（位置が物理演算と同期しているため、箱のように動いて見える）
        ui_.Render(in, out);

        // 左右のコントローラーの描画
        if (in.LeftRemoteTracked) {
            controllerRenderL_.Render(out.Surfaces);
        }
        if (in.RightRemoteTracked) {
            controllerRenderR_.Render(out.Surfaces);
        }

        // コントローラー光線の描画
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
    btDefaultCollisionConfiguration* collisionConfiguration_; // 衝突判定の詳細設定
    btCollisionDispatcher* dispatcher_;                      // 衝突判定の分配管理
    btBroadphaseInterface* overlappingPairCache_;             // 広域の当たり判定キャッシュ
    btSequentialImpulseConstraintSolver* solver_;            // 物理制約・拘束ソルバー
    btDiscreteDynamicsWorld* dynamicsWorld_;                  // 物理シミュレーションワールド本体

    // 物理オブジェクト
    btRigidBody* fallingCube_;                               // 落下する箱の剛体ポインタ
    btCollisionShape* boxShape_;                             // 箱の形状ポインタ

    // VR空間上の見た目用オブジェクト
    OVRFW::VRMenuObject* physicsBoxUI_;                     // 箱の位置に重ねるUIパネル
};

// OpenXRエントリーポイント（メイン関数の生成）
ENTRY_POINT(XrControllersApp)