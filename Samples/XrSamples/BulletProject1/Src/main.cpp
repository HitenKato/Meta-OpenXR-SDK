// ============================================================================
// プリプロセッサマクロ定義 (マクロ・型衝突防止)
// ============================================================================
#define _HAS_STD_BYTE 0
#define NOMINMAX

#include <windows.h>
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
        groundShape_(nullptr),
        leftHandRb_(nullptr),
        rightHandRb_(nullptr),
        handShape_(nullptr) {

        BackgroundColor = OVR::Vector4f(0.20f, 0.20f, 0.20f, 1.0f);
        OpenXRVersion = XR_API_VERSION_1_0;
    }

    virtual bool AppInit(const xrJava* context) override {
        if (false == ui_.Init(context, GetFileSys())) {
            return false;
        }
        return true;
    }

    virtual void AppShutdown(const xrJava* context) override {
        boxRenderer_.Shutdown();
        floorRenderer_.Shutdown();
        OVRFW::XrApp::AppShutdown(context);
        ui_.Shutdown();
    }

    // キネマティック（手）用の剛体を生成するヘルパー関数
    btRigidBody* CreateKinematicBody(btCollisionShape* shape) {
        btTransform startTransform;
        startTransform.setIdentity();
        btDefaultMotionState* motionState = new btDefaultMotionState(startTransform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0f, motionState, shape, btVector3(0, 0, 0));
        btRigidBody* body = new btRigidBody(rbInfo);

        // プログラムから強制的に座標を動かすためのキネマティック設定
        body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        body->setActivationState(DISABLE_DEACTIVATION);
        return body;
    }

    virtual bool SessionInit() override {
        CurrentSpace = LocalSpace;

        if (false == controllerRenderL_.Init(true)) return false;
        if (false == controllerRenderR_.Init(false)) return false;
        beamRenderer_.Init(GetFileSys(), nullptr, OVR::Vector4f(1.0f), 1.0f);

        // --- 3Dモデル（描画用オブジェクト）の生成 ---
        OVRFW::GeometryBuilder boxBuilder;
        boxBuilder.Add(OVRFW::BuildUnitCubeDescriptor(), OVRFW::GeometryBuilder::kInvalidIndex, { 1.0f, 0.5f, 0.0f, 1.0f });
        boxRenderer_.ChannelControl = OVR::Vector4f(1, 1, 1, 1);
        boxRenderer_.Init(boxBuilder.ToGeometryDescriptor());
        boxRenderer_.SetScale({ 1.0f, 1.0f, 1.0f });

        OVRFW::GeometryBuilder floorBuilder;
        floorBuilder.Add(OVRFW::BuildUnitCubeDescriptor(), OVRFW::GeometryBuilder::kInvalidIndex, { 0.3f, 0.3f, 0.3f, 1.0f });
        floorRenderer_.ChannelControl = OVR::Vector4f(1, 1, 1, 1);
        floorRenderer_.Init(floorBuilder.ToGeometryDescriptor());
        floorRenderer_.SetScale({ 10.0f, 0.1f, 10.0f });

        // --- Bullet Physics ワールドの構築 ---
        collisionConfiguration_ = new btDefaultCollisionConfiguration();
        dispatcher_ = new btCollisionDispatcher(collisionConfiguration_);
        overlappingPairCache_ = new btDbvtBroadphase();
        solver_ = new btSequentialImpulseConstraintSolver();

        dynamicsWorld_ = new btDiscreteDynamicsWorld(dispatcher_, overlappingPairCache_, solver_, collisionConfiguration_);
        dynamicsWorld_->setGravity(btVector3(0.0f, -9.8f, 0.0f));

        // 1. 静的な「床」
        groundShape_ = new btStaticPlaneShape(btVector3(0.0f, 1.0f, 0.0f), 0.0f);
        btTransform groundTransform;
        groundTransform.setIdentity();
        groundTransform.setOrigin(btVector3(0.0f, -1.5f, 0.0f));
        btRigidBody::btRigidBodyConstructionInfo groundRbInfo(0.0f, new btDefaultMotionState(groundTransform), groundShape_, btVector3(0, 0, 0));
        groundRigidBody_ = new btRigidBody(groundRbInfo);
        groundRigidBody_->setRestitution(0.6f);
        groundRigidBody_->setFriction(0.8f);
        dynamicsWorld_->addRigidBody(groundRigidBody_);

        // 2. 落下する「箱」
        boxShape_ = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
        btTransform startTransform;
        startTransform.setIdentity();
        startTransform.setOrigin(btVector3(0.0f, 1.5f, -2.0f));
        btVector3 localInertia(0, 0, 0);
        boxShape_->calculateLocalInertia(1.0f, localInertia);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(1.0f, new btDefaultMotionState(startTransform), boxShape_, localInertia);
        fallingCube_ = new btRigidBody(rbInfo);
        fallingCube_->setRestitution(0.6f);
        fallingCube_->setFriction(0.8f);
        dynamicsWorld_->addRigidBody(fallingCube_);

        // 3. 左右の手の当たり判定（半径10cmの見えない球）
        handShape_ = new btSphereShape(0.1f);
        leftHandRb_ = CreateKinematicBody(handShape_);
        rightHandRb_ = CreateKinematicBody(handShape_);
        dynamicsWorld_->addRigidBody(leftHandRb_);
        dynamicsWorld_->addRigidBody(rightHandRb_);

        return true;
    }

    virtual void SessionEnd() override {
        controllerRenderL_.Shutdown();
        controllerRenderR_.Shutdown();
        beamRenderer_.Shutdown();

        // 拘束（グラブ）の解除
        if (leftGrabConstraint_) { dynamicsWorld_->removeConstraint(leftGrabConstraint_); delete leftGrabConstraint_; }
        if (rightGrabConstraint_) { dynamicsWorld_->removeConstraint(rightGrabConstraint_); delete rightGrabConstraint_; }

        auto safeDeleteBody = [&](btRigidBody*& body) {
            if (body) {
                dynamicsWorld_->removeRigidBody(body);
                delete body->getMotionState();
                delete body;
                body = nullptr;
            }
            };
        safeDeleteBody(fallingCube_);
        safeDeleteBody(groundRigidBody_);
        safeDeleteBody(leftHandRb_);
        safeDeleteBody(rightHandRb_);

        if (boxShape_) { delete boxShape_; boxShape_ = nullptr; }
        if (groundShape_) { delete groundShape_; groundShape_ = nullptr; }
        if (handShape_) { delete handShape_; handShape_ = nullptr; }

        if (dynamicsWorld_) {
            delete dynamicsWorld_; delete solver_; delete overlappingPairCache_; delete dispatcher_; delete collisionConfiguration_;
            dynamicsWorld_ = nullptr;
        }
    }

    virtual void Update(const OVRFW::ovrApplFrameIn& in) override {
        // ---------------------------------------------------------
        // 1. 視点移動と回転 (Locomotion)
        // ---------------------------------------------------------
        if (std::abs(in.RightRemoteJoystick.x) > 0.1f) {
            playerYaw_ -= in.RightRemoteJoystick.x * 2.0f * in.DeltaSeconds;
        }
        OVR::Quatf yawQuat(OVR::Vector3f(0, 1, 0), playerYaw_);

        if (std::abs(in.LeftRemoteJoystick.x) > 0.1f || std::abs(in.LeftRemoteJoystick.y) > 0.1f) {
            OVR::Vector3f forward = yawQuat.Rotate(OVR::Vector3f(0, 0, -1)); // 修正：Transform -> Rotate
            OVR::Vector3f right = yawQuat.Rotate(OVR::Vector3f(1, 0, 0));    // 修正：Transform -> Rotate
            playerPosition_ += (forward * in.LeftRemoteJoystick.y + right * in.LeftRemoteJoystick.x) * 3.0f * in.DeltaSeconds;
        }
        playerPose_ = OVR::Posef(yawQuat, playerPosition_);

        // ---------------------------------------------------------
        // 2. 左右の手のワールド座標計算と物理剛体の更新
        // ---------------------------------------------------------
        OVR::Posef leftWorldPose = playerPose_ * in.LeftRemotePose;
        OVR::Posef rightWorldPose = playerPose_ * in.RightRemotePose;

        btTransform leftTrans;
        leftTrans.setOrigin(btVector3(leftWorldPose.Translation.x, leftWorldPose.Translation.y, leftWorldPose.Translation.z));
        leftTrans.setRotation(btQuaternion(leftWorldPose.Rotation.x, leftWorldPose.Rotation.y, leftWorldPose.Rotation.z, leftWorldPose.Rotation.w));
        leftHandRb_->getMotionState()->setWorldTransform(leftTrans);
        leftHandRb_->setWorldTransform(leftTrans);

        btTransform rightTrans;
        rightTrans.setOrigin(btVector3(rightWorldPose.Translation.x, rightWorldPose.Translation.y, rightWorldPose.Translation.z));
        rightTrans.setRotation(btQuaternion(rightWorldPose.Rotation.x, rightWorldPose.Rotation.y, rightWorldPose.Rotation.z, rightWorldPose.Rotation.w));
        rightHandRb_->getMotionState()->setWorldTransform(rightTrans);
        rightHandRb_->setWorldTransform(rightTrans);

        // ---------------------------------------------------------
        // 3. Bullet 6自由度拘束（Constraint）を用いたグラブ処理
        // ---------------------------------------------------------
        bool gripLeft = in.LeftRemoteGripTrigger > 0.5f;
        bool gripRight = in.RightRemoteGripTrigger > 0.5f;

        btTransform boxTrans;
        fallingCube_->getMotionState()->getWorldTransform(boxTrans);
        OVR::Vector3f boxPos(boxTrans.getOrigin().x(), boxTrans.getOrigin().y(), boxTrans.getOrigin().z());

        float grabRadius = 3.4f; // ★空振りを防ぐため、判定半径を40cmに拡大

        // --- 左手グラブ ---
        if (gripLeft && !leftGrabConstraint_ && !rightGrabConstraint_) {
            if ((boxPos - leftWorldPose.Translation).Length() < grabRadius) {
                // 掴んだ瞬間に手と箱の「相対位置」を計算し、6自由度拘束(Constraint)でガッチリ固定する
                btTransform frameInHand = leftHandRb_->getWorldTransform().inverse() * fallingCube_->getWorldTransform();
                btTransform frameInBox = btTransform::getIdentity();

                leftGrabConstraint_ = new btGeneric6DofConstraint(*leftHandRb_, *fallingCube_, frameInHand, frameInBox, true);

                // 自由度をすべて0にして完全固定（ボンドでくっつけた状態）
                leftGrabConstraint_->setLinearLowerLimit(btVector3(0, 0, 0));
                leftGrabConstraint_->setLinearUpperLimit(btVector3(0, 0, 0));
                leftGrabConstraint_->setAngularLowerLimit(btVector3(0, 0, 0));
                leftGrabConstraint_->setAngularUpperLimit(btVector3(0, 0, 0));

                dynamicsWorld_->addConstraint(leftGrabConstraint_, true);
                fallingCube_->activate(true);
            }
        }
        else if (!gripLeft && leftGrabConstraint_) {
            // 左手から離す（拘束を破壊）
            dynamicsWorld_->removeConstraint(leftGrabConstraint_);
            delete leftGrabConstraint_;
            leftGrabConstraint_ = nullptr;
            fallingCube_->activate(true);

            // 手の速度を箱に与えて投げる
            OVR::Vector3f vel = (leftWorldPose.Translation - leftHandPrevPos_) / std::max(in.DeltaSeconds, 0.001f);
            fallingCube_->setLinearVelocity(btVector3(vel.x, vel.y, vel.z));
        }

        // --- 右手グラブ ---
        if (gripRight && !rightGrabConstraint_ && !leftGrabConstraint_) {
            if ((boxPos - rightWorldPose.Translation).Length() < grabRadius) {
                btTransform frameInHand = rightHandRb_->getWorldTransform().inverse() * fallingCube_->getWorldTransform();
                btTransform frameInBox = btTransform::getIdentity();

                rightGrabConstraint_ = new btGeneric6DofConstraint(*rightHandRb_, *fallingCube_, frameInHand, frameInBox, true);
                rightGrabConstraint_->setLinearLowerLimit(btVector3(0, 0, 0));
                rightGrabConstraint_->setLinearUpperLimit(btVector3(0, 0, 0));
                rightGrabConstraint_->setAngularLowerLimit(btVector3(0, 0, 0));
                rightGrabConstraint_->setAngularUpperLimit(btVector3(0, 0, 0));

                dynamicsWorld_->addConstraint(rightGrabConstraint_, true);
                fallingCube_->activate(true);
            }
        }
        else if (!gripRight && rightGrabConstraint_) {
            // 右手から離す
            dynamicsWorld_->removeConstraint(rightGrabConstraint_);
            delete rightGrabConstraint_;
            rightGrabConstraint_ = nullptr;
            fallingCube_->activate(true);

            OVR::Vector3f vel = (rightWorldPose.Translation - rightHandPrevPos_) / std::max(in.DeltaSeconds, 0.001f);
            fallingCube_->setLinearVelocity(btVector3(vel.x, vel.y, vel.z));
        }

        // ---------------------------------------------------------
        // 4. Aボタン/トリガーで箱の位置をリセット
        // ---------------------------------------------------------
        bool isTriggerPressed = (in.RightRemoteIndexTrigger > 0.5f);
        bool isButtonAPressed = (in.AllButtons & OVRFW::ovrApplFrameIn::kButtonA) != 0;
        static bool prevAction = false;
        bool currentAction = isTriggerPressed || isButtonAPressed;

        if (currentAction && !prevAction && fallingCube_) {
            // 掴んでいる最中にリセットされた場合は拘束を強制解除する
            if (leftGrabConstraint_) { dynamicsWorld_->removeConstraint(leftGrabConstraint_); delete leftGrabConstraint_; leftGrabConstraint_ = nullptr; }
            if (rightGrabConstraint_) { dynamicsWorld_->removeConstraint(rightGrabConstraint_); delete rightGrabConstraint_; rightGrabConstraint_ = nullptr; }

            btTransform trans;
            trans.setIdentity();
            trans.setOrigin(btVector3(0.0f, 1.5f, -2.0f));

            btQuaternion quat;
            quat.setEuler(0.8f, 0.5f, 0.3f);
            trans.setRotation(quat);

            fallingCube_->setWorldTransform(trans);
            fallingCube_->getMotionState()->setWorldTransform(trans);
            fallingCube_->setLinearVelocity(btVector3(0, 0, 0));
            fallingCube_->setAngularVelocity(btVector3(0, 0, 0));
            fallingCube_->clearForces();
            fallingCube_->activate(true);
        }
        prevAction = currentAction;

        // ---------------------------------------------------------
        // 5. 物理エンジンの更新と描画モデルへの同期
        // ---------------------------------------------------------
        if (dynamicsWorld_) {
            dynamicsWorld_->stepSimulation(in.DeltaSeconds, 10);
        }

        fallingCube_->getMotionState()->getWorldTransform(boxTrans);
        OVR::Posef boxWorldPose(
            OVR::Quatf(boxTrans.getRotation().x(), boxTrans.getRotation().y(), boxTrans.getRotation().z(), boxTrans.getRotation().w()),
            OVR::Vector3f(boxTrans.getOrigin().x(), boxTrans.getOrigin().y(), boxTrans.getOrigin().z())
        );
        boxRenderer_.SetPose(playerPose_.Inverted() * boxWorldPose);
        boxRenderer_.Update();

        OVR::Posef floorWorldPose = OVR::Posef::Identity();
        floorWorldPose.Translation = { 0.0f, -1.55f, 0.0f };
        floorRenderer_.SetPose(playerPose_.Inverted() * floorWorldPose);
        floorRenderer_.Update();

        // 過去の座標を保存（投げる速度の計算用）
        leftHandPrevPos_ = leftWorldPose.Translation;
        rightHandPrevPos_ = rightWorldPose.Translation;

        // --- コントローラー描画の更新 ---
        ui_.HitTestDevices().clear();
        if (in.LeftRemoteTracked) controllerRenderL_.Update(in.LeftRemotePose);
        if (in.RightRemoteTracked) controllerRenderR_.Update(in.RightRemotePose);

        ui_.Update(in);
        beamRenderer_.Update(in, ui_.HitTestDevices());
    }

    virtual void Render(const OVRFW::ovrApplFrameIn& in, OVRFW::ovrRendererOutput& out) override {
        ui_.Render(in, out);

        boxRenderer_.Render(out.Surfaces);
        floorRenderer_.Render(out.Surfaces);

        if (in.LeftRemoteTracked) controllerRenderL_.Render(out.Surfaces);
        if (in.RightRemoteTracked) controllerRenderR_.Render(out.Surfaces);
        beamRenderer_.Render(in, out);
    }

private:
    OVRFW::ControllerRenderer controllerRenderL_;
    OVRFW::ControllerRenderer controllerRenderR_;
    OVRFW::TinyUI ui_;
    OVRFW::SimpleBeamRenderer beamRenderer_;

    OVRFW::GeometryRenderer boxRenderer_;
    OVRFW::GeometryRenderer floorRenderer_;

    // --- Bullet Physics ---
    btDefaultCollisionConfiguration* collisionConfiguration_;
    btCollisionDispatcher* dispatcher_;
    btBroadphaseInterface* overlappingPairCache_;
    btSequentialImpulseConstraintSolver* solver_;
    btDiscreteDynamicsWorld* dynamicsWorld_;

    btRigidBody* fallingCube_;
    btCollisionShape* boxShape_;
    btRigidBody* groundRigidBody_;
    btCollisionShape* groundShape_;

    // コントローラーの物理判定と拘束（グラブ）用
    btRigidBody* leftHandRb_;
    btRigidBody* rightHandRb_;
    btCollisionShape* handShape_;
    btGeneric6DofConstraint* leftGrabConstraint_ = nullptr;
    btGeneric6DofConstraint* rightGrabConstraint_ = nullptr;

    // --- ロコモーション用変数 ---
    OVR::Posef playerPose_ = OVR::Posef::Identity();
    float playerYaw_ = 0.0f;
    OVR::Vector3f playerPosition_ = { 0.0f, 0.0f, 0.0f };

    OVR::Vector3f leftHandPrevPos_;
    OVR::Vector3f rightHandPrevPos_;
};

ENTRY_POINT(XrControllersApp)