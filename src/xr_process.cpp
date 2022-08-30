#define XR_NULL_ASYNC_REQUEST_ID_FB 0

#include <iostream>
#include <vector>
#include <format>
#include <optional>
#include <thread>
#include <filesystem>
#include <fstream>
#include <format>
#include <array>

#include "Game.hpp"
#include "GraphicsManager.hpp"
#include "GraphicsManager_Vulkan.hpp"
#include "utils.hpp"

#include "xr_linear_helper.h"

#define XR_CHK_ERR(f) if (auto result = f; XR_FAILED(result)){ throw std::runtime_error(std::format("Err: {}, {} {}", to_string(result), __LINE__, #f)); }

auto SpaceToPose(xr::Space space, xr::Space baseSpace, xr::Time time) {
    auto location = space.locateSpace(baseSpace, time);

    if ((location.locationFlags & xr::SpaceLocationFlagBits::PositionValid) &&
        (location.locationFlags & xr::SpaceLocationFlagBits::OrientationValid)) {
        Game::Pose pose;
        pose.pos = toG(location.pose.position);
        pose.ori = toG(location.pose.orientation);
        return std::make_optional(pose);
    }
    else {
        return std::optional<Game::Pose>(std::nullopt);
    }
}

class App
{
    auto requiredExtensions() {
        auto requiredExts = GetGraphicsExtension_Vulkan();
        return requiredExts;
    }

    void showPlatformInfo() {
        std::cout << "Platform Infos:" << std::endl;

        auto apiLayers = xr::enumerateApiLayerPropertiesToVector();
        uint32_t apiLayerCnt = apiLayers.size();

        std::cout << "Api Layers: " << apiLayerCnt << std::endl;
        for (const auto& apiLayer : apiLayers) {
            std::cout << std::format("{} v{}, spec: {}", apiLayer.layerName, apiLayer.layerVersion, to_string(apiLayer.specVersion)) << std::endl;
        }

        auto extProps = xr::enumerateInstanceExtensionPropertiesToVector(nullptr);
        uint32_t extPropCnt = extProps.size();

        auto requiredExts = requiredExtensions();

        std::cout << "Extensions: " << extPropCnt << std::endl;
        for (const auto& extProp : extProps) {
            std::cout << std::format("{} v{}", extProp.extensionName, extProp.extensionVersion) << std::endl;

            if (extProp.extensionName == std::string(XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME)) {
                extInfo.vive_tracker = true;
            }

            auto it = std::find_if(requiredExts.begin(), requiredExts.end(), [&](const char* extName) {
                return std::string(extName) == extProp.extensionName;
            });
            if (it != requiredExts.end()) {
                requiredExts.erase(it);
            }
        }

        if (!requiredExts.empty()) {
            std::string s = "Error: Required Extension is not supported / ";
            for (const auto& ext : requiredExts) {
                s += ext;
                s += ", ";
            }
            s.pop_back();
            s.pop_back();
            throw std::runtime_error(s);
        }
    }

    struct ExtInfo {
        bool vive_tracker = false;
    } extInfo;

    xr::UniqueInstance instance;
    xr::SystemId systemId;
    std::unique_ptr<IGraphicsManager> graphicsManager;
    xr::UniqueSession session;

    std::vector<Swapchain> swapchains;
    xr::UniqueSpace appSpace;
    xr::UniqueSpace viewSpace;
    xr::UniqueSpace stageSpace;
    std::vector<XrView> views;

    xr::UniqueActionSet actionSet;
    struct HandActions {
        xr::UniqueAction pose;
        xr::UniqueAction trigger;
        xr::UniqueAction haptics;
        xr::Path subActionPath[2];
        xr::UniqueSpace space[2];
    } handActions;

    //xr::UniqueActionSet trackerActionSet;
    //struct TrackerActions {
    //    xr::Path subActionPath;
    //    xr::UniqueAction pose;
    //    xr::UniqueAction haptics;
    //    xr::UniqueSpace space;
    //} trackerActions;

    class HandVibrationProvider : public Game::IVibrationProvider {
        const int i;
        const App* app;
    public:
        HandVibrationProvider(int i, App* app) : i(i), app(app) {}
        void vibrate(float a) override {
            xr::HapticVibration vibration{};
            vibration.amplitude = a;
            vibration.duration = xr::Duration::minHaptic();
            vibration.frequency = XR_FREQUENCY_UNSPECIFIED;

            xr::HapticActionInfo hapticActionInfo{};
            hapticActionInfo.action = app->handActions.haptics.get();
            hapticActionInfo.subactionPath = app->handActions.subActionPath[i];

            XR_CHK_ERR(app->session->applyHapticFeedback(hapticActionInfo, reinterpret_cast<XrHapticBaseHeader*>(&vibration)));
        }
    };
    std::optional<HandVibrationProvider> handVibProvider[2];

    xr::EventDataBuffer evBuf;
    bool shouldExit = false;
    bool session_running = false;

    Game::GameData gameData;

    void CreateInstance() {
        std::vector<const char*> layers = {};
        std::vector<const char*> exts = requiredExtensions();
        if (extInfo.vive_tracker)
            exts.push_back(XR_HTCX_VIVE_TRACKER_INTERACTION_EXTENSION_NAME);

        xr::InstanceCreateInfo instanceCreateInfo{};
        strcpy_s(instanceCreateInfo.applicationInfo.applicationName, "XRTest");
        strcpy_s(instanceCreateInfo.applicationInfo.engineName, "XRTest");
        instanceCreateInfo.applicationInfo.apiVersion = xr::Version::current();
        instanceCreateInfo.enabledApiLayerCount = layers.size();
        instanceCreateInfo.enabledApiLayerNames = layers.data();
        instanceCreateInfo.enabledExtensionCount = exts.size();
        instanceCreateInfo.enabledExtensionNames = exts.data();

        instance = xr::createInstanceUnique(instanceCreateInfo);
        if (instance == XR_NULL_HANDLE)
            throw std::runtime_error("instance creation error");

        auto prop = instance->getInstanceProperties();
        std::cout << std::format("Runtime: {} v{}", prop.runtimeName, to_string(prop.runtimeVersion)) << std::endl;
    }

    void InitializeSystem() {
        xr::SystemGetInfo sysGetInfo{};
        sysGetInfo.formFactor = xr::FormFactor::HeadMountedDisplay;

        systemId = instance->getSystem(sysGetInfo);
        if (systemId == XR_NULL_SYSTEM_ID)
            throw std::runtime_error("system get error");

        auto props = instance->getSystemProperties(systemId);

        std::cout << std::format("SystemID: {:X}, FormFactor: {}", this->systemId.get(), to_string(sysGetInfo.formFactor)) << std::endl;
        std::cout << std::format("SystemName: {}, vendorID: {:X}", props.systemName, props.vendorId) << std::endl;
        std::cout << std::format("Max Layers: {}, Max Size: {}x{}", props.graphicsProperties.maxLayerCount, props.graphicsProperties.maxSwapchainImageWidth, props.graphicsProperties.maxSwapchainImageHeight) << std::endl;
        std::cout << std::format("Tracking: position/{}, orientation/{}", props.trackingProperties.positionTracking.get(), props.trackingProperties.orientationTracking.get()) << std::endl;
    }

    void InitializeSession() {
        auto graphicsBinding = graphicsManager->getXrGraphicsBinding();

        xr::SessionCreateInfo sessionCreateInfo{};
        sessionCreateInfo.systemId = systemId;
        sessionCreateInfo.next = graphicsBinding.get();

        session = instance->createSessionUnique(sessionCreateInfo);
        std::cout << "Session created" << std::endl;

        auto spaces = session->enumerateReferenceSpacesToVector();

        std::cout << "Reference spaces:" << std::endl;
        for (const auto space : spaces) {
            std::cout << to_string(space) << std::endl;
        }
    }

    void CreateReferenceSpace() {
        {
            xr::ReferenceSpaceCreateInfo createInfo;
            xr::Posef id{};
            id.orientation.w = 1;
            createInfo.poseInReferenceSpace = id;
            createInfo.referenceSpaceType = xr::ReferenceSpaceType::Local;

            appSpace = session->createReferenceSpaceUnique(createInfo);
        }
        {
            xr::ReferenceSpaceCreateInfo createInfo;
            xr::Posef id{};
            id.orientation.w = 1;
            createInfo.poseInReferenceSpace = id;
            createInfo.referenceSpaceType = xr::ReferenceSpaceType::View;

            viewSpace = session->createReferenceSpaceUnique(createInfo);
        }
        {
            xr::ReferenceSpaceCreateInfo createInfo;
            xr::Posef id{};
            id.orientation.w = 1;
            createInfo.poseInReferenceSpace = id;
            createInfo.referenceSpaceType = xr::ReferenceSpaceType::Stage;

            stageSpace = session->createReferenceSpaceUnique(createInfo);
        }
    }

    void CreateSwapchain() {
        auto swapchainFmts = session->enumerateSwapchainFormatsToVector();
        auto swapchainFmtCnt = swapchainFmts.size();

        auto selectedSwapchainFmt = graphicsManager->chooseImageFormat(swapchainFmts);

        std::cout << "Avilable Swapchain Format x" << swapchainFmtCnt << std::endl;
        for (int i = 0; const auto & swapchainFmt : swapchainFmts) {
            std::cout << std::format("{}{}{}",
                swapchainFmt == selectedSwapchainFmt ? "[" : "",
                swapchainFmt,
                swapchainFmt == selectedSwapchainFmt ? "]" : "") << std::endl;
        }

        {
            auto configViews = instance->enumerateViewConfigurationsToVector(systemId);
            uint32_t viewCnt = configViews.size();

            std::cout << "ViewType x" << viewCnt << std::endl;
            for (int i = 0; const auto & configView : configViews) {
                auto blendModes = instance->enumerateEnvironmentBlendModesToVector(systemId, configView);
                uint32_t blendModeCnt = blendModes.size();

                std::cout << "ViewType " << i << std::endl;
                std::cout << "BlendMode x" << blendModeCnt << std::endl;
                for (const auto blendMode : blendModes) {
                    std::cout << to_string(blendMode) << std::endl;
                }
                i++;
            }
        }

        auto configViews = instance->enumerateViewConfigurationViewsToVector(systemId, xr::ViewConfigurationType::PrimaryStereo);
        auto viewCnt = configViews.size();

        this->views.resize(viewCnt);

        std::cout << "View x" << viewCnt << std::endl;

        for (int i = 0; const auto & configView : configViews) {
            std::cout << "View " << i << ":" << std::endl;
            std::cout << std::format("Size: typ/{}x{}, max/{}x{}",
                configView.recommendedImageRectWidth, configView.recommendedImageRectHeight,
                configView.maxImageRectWidth, configView.maxImageRectHeight) << std::endl;
            std::cout << std::format("Samples: typ/{}, max/{}",
                configView.recommendedSwapchainSampleCount, configView.maxSwapchainSampleCount) << std::endl;

            xr::SwapchainCreateInfo createInfo{};
            createInfo.arraySize = 1;
            createInfo.format = selectedSwapchainFmt;
            createInfo.width = configView.recommendedImageRectWidth;
            createInfo.height = configView.recommendedImageRectHeight;
            createInfo.mipCount = 1;
            createInfo.faceCount = 1;
            createInfo.sampleCount = configView.recommendedSwapchainSampleCount;
            createInfo.usageFlags = xr::SwapchainUsageFlagBits::ColorAttachment/* | xr::SwapchainUsageFlagBits::Sampled*/;

            Swapchain swapchain;
            swapchain.handle = session->createSwapchainUnique(createInfo);
            swapchain.extent.width = createInfo.width;
            swapchain.extent.height = createInfo.height;

            swapchains.emplace_back(std::move(swapchain));

            i++;
        }
        graphicsManager->InitializeRenderTargets(swapchains, selectedSwapchainFmt);
    }

    void InitializeAction() {
        handActions.subActionPath[0] = instance->stringToPath("/user/hand/left");
        handActions.subActionPath[1] = instance->stringToPath("/user/hand/right");
        //trackerActions.subActionPath = instance->stringToPath("/user/vive_tracker_htcx/role/chest");

        {
            xr::ActionSetCreateInfo createInfo{};
            strcpy(createInfo.actionSetName, "gameplay");
            strcpy(createInfo.localizedActionSetName, "Gameplay");
            createInfo.priority = 0;
            actionSet = instance->createActionSetUnique(createInfo);
        }

        //{
        //    xr::ActionSetCreateInfo createInfo{};
        //    strcpy(createInfo.actionSetName, "gameplay_track");
        //    strcpy(createInfo.localizedActionSetName, "Gameplay track");
        //    createInfo.priority = 0;
        //    trackerActionSet = instance->createActionSetUnique(createInfo);
        //}

        auto DefineAction = [&](std::string name, std::string lName, xr::ActionType type, xr::UniqueAction& action) {
            xr::ActionCreateInfo actionInfo{};
            actionInfo.actionType = type;
            strcpy(actionInfo.actionName, name.c_str());
            strcpy(actionInfo.localizedActionName, lName.c_str());
            actionInfo.countSubactionPaths = std::size(handActions.subActionPath);
            actionInfo.subactionPaths = handActions.subActionPath;
            action = actionSet->createActionUnique(actionInfo);
        };
        //auto DefineTrackerAction = [&](std::string name, std::string lName, xr::ActionType type, xr::UniqueAction& action) {
        //    xr::ActionCreateInfo actionInfo{};
        //    actionInfo.actionType = type;
        //    strcpy(actionInfo.actionName, name.c_str());
        //    strcpy(actionInfo.localizedActionName, lName.c_str());
        //    actionInfo.countSubactionPaths = 1;
        //    actionInfo.subactionPaths = &trackerActions.subActionPath;
        //    action = trackerActionSet->createActionUnique(actionInfo);
        //};
        auto ConfigAction = [&](std::string profile, std::initializer_list<std::pair<const xr::UniqueAction&, std::string>> bindingsDat) {
            auto profilePath = instance->stringToPath(profile.c_str());
            xr::InteractionProfileSuggestedBinding suggestedBindings{};

            std::vector<xr::ActionSuggestedBinding> bindings(bindingsDat.size());
            std::transform(bindingsDat.begin(), bindingsDat.end(), bindings.begin(), [&](const std::pair<const xr::UniqueAction&, std::string>& config) {
                auto path = instance->stringToPath(config.second.c_str());
                return xr::ActionSuggestedBinding{ config.first.get(), path};
                });
            suggestedBindings.interactionProfile = profilePath;
            suggestedBindings.suggestedBindings = bindings.data();
            suggestedBindings.countSuggestedBindings = bindings.size();
            instance->suggestInteractionProfileBindings(suggestedBindings);
        };

        DefineAction("hand_pose", "Hand Pose", xr::ActionType::PoseInput, handActions.pose);
        DefineAction("trigger", "Trigger", xr::ActionType::BooleanInput, handActions.trigger);
        DefineAction("haptics", "Haptics", xr::ActionType::VibrationOutput, handActions.haptics);
        //DefineTrackerAction("tracker_pose", "Tracker Pose", xr::ActionType::PoseInput, trackerActions.pose);
        //DefineTrackerAction("tracker_haptics", "Tracker Haptics", xr::ActionType::VibrationOutput, trackerActions.haptics);

        ConfigAction("/interaction_profiles/khr/simple_controller", {
                { handActions.pose, "/user/hand/left/input/aim/pose" },
                { handActions.pose, "/user/hand/right/input/aim/pose" },
                { handActions.trigger, "/user/hand/left/input/select/click" },
                { handActions.trigger, "/user/hand/right/input/select/click" },
                { handActions.haptics, "/user/hand/left/output/haptic" },
                { handActions.haptics, "/user/hand/right/output/haptic" },
            });
        ConfigAction("/interaction_profiles/valve/index_controller", {
                { handActions.pose, "/user/hand/left/input/aim/pose" },
                { handActions.pose, "/user/hand/right/input/aim/pose" },
                { handActions.trigger, "/user/hand/left/input/trigger/click" },
                { handActions.trigger, "/user/hand/right/input/trigger/click" },
                { handActions.haptics, "/user/hand/left/output/haptic" },
                { handActions.haptics, "/user/hand/right/output/haptic" },
            });
        ConfigAction("/interaction_profiles/google/daydream_controller", {
                { handActions.pose, "/user/hand/left/input/aim/pose" },
                { handActions.pose, "/user/hand/right/input/aim/pose" },
                { handActions.trigger, "/user/hand/left/input/select/click" },
                { handActions.trigger, "/user/hand/right/input/select/click" },
            });
        ConfigAction("/interaction_profiles/htc/vive_controller", {
                { handActions.pose, "/user/hand/left/input/aim/pose" },
                { handActions.pose, "/user/hand/right/input/aim/pose" },
                { handActions.trigger, "/user/hand/left/input/trigger/click" },
                { handActions.trigger, "/user/hand/right/input/trigger/click" },
                { handActions.haptics, "/user/hand/left/output/haptic" },
                { handActions.haptics, "/user/hand/right/output/haptic" },
            });
        ConfigAction("/interaction_profiles/microsoft/motion_controller", {
                { handActions.pose, "/user/hand/left/input/aim/pose" },
                { handActions.pose, "/user/hand/right/input/aim/pose" },
                { handActions.trigger, "/user/hand/left/input/squeeze/click" },
                { handActions.trigger, "/user/hand/right/input/squeeze/click" },
                { handActions.haptics, "/user/hand/left/output/haptic" },
                { handActions.haptics, "/user/hand/right/output/haptic" },
            });
        ConfigAction("/interaction_profiles/oculus/go_controller", {
                { handActions.pose, "/user/hand/left/input/aim/pose" },
                { handActions.pose, "/user/hand/right/input/aim/pose" },
                { handActions.trigger, "/user/hand/left/input/trigger/click" },
                { handActions.trigger, "/user/hand/right/input/trigger/click" },
            });
        ConfigAction("/interaction_profiles/oculus/touch_controller", {
                { handActions.pose, "/user/hand/left/input/aim/pose" },
                { handActions.pose, "/user/hand/right/input/aim/pose" },
                { handActions.trigger, "/user/hand/left/input/trigger/touch" },
                { handActions.trigger, "/user/hand/right/input/trigger/touch" },
                { handActions.haptics, "/user/hand/left/output/haptic" },
                { handActions.haptics, "/user/hand/right/output/haptic" },
            });
        //ConfigAction("/interaction_profiles/htc/vive_tracker_htcx", {
        //        { trackerActions.pose, "/user/vive_tracker_htcx/role/chest/input/grip/pose" },
        //        { trackerActions.haptics, "/user/vive_tracker_htcx/role/chest/output/haptic" },
        //    });

        for(int i = 0; i < 2; i++)
        {
            xr::ActionSpaceCreateInfo actionSpaceInfo{};
            actionSpaceInfo.action = handActions.pose.get();
            actionSpaceInfo.poseInActionSpace.orientation.w = 1.0f;
            actionSpaceInfo.subactionPath = handActions.subActionPath[i];
            handActions.space[i] = session->createActionSpaceUnique(actionSpaceInfo);
        }

        //{
        //    xr::ActionSpaceCreateInfo actionSpaceInfo{};
        //    actionSpaceInfo.action = trackerActions.pose.get();
        //    actionSpaceInfo.poseInActionSpace.orientation.w = 1.0f;
        //    actionSpaceInfo.subactionPath = trackerActions.subActionPath;
        //    trackerActions.space = session->createActionSpaceUnique(actionSpaceInfo);
        //}

        auto actionSets = { actionSet.get(), /*trackerActionSet.get()*/ };

        xr::SessionActionSetsAttachInfo attachInfo{};
        attachInfo.countActionSets = actionSets.size();
        attachInfo.actionSets = actionSets.begin();
        session->attachSessionActionSets(attachInfo);

        for (int i = 0; i < 2; i++) {
            handVibProvider[i].emplace(i, this);
            gameData.handVib[i].emplace(std::ref<Game::IVibrationProvider>(handVibProvider[i].value()));
        }
    }

    void HandleSessionStateChange(const xr::EventDataSessionStateChanged& ev) {
        if (ev.session != this->session.get()) {
            std::cout << "Event from Unknown Session" << std::endl;
            return;
        }
        switch (ev.state)
        {
        case xr::SessionState::Ready:
        {
            xr::SessionBeginInfo beginInfo;
            beginInfo.primaryViewConfigurationType = xr::ViewConfigurationType::PrimaryStereo;
            session->beginSession(beginInfo);
            session_running = true;
            std::cout << "session began" << std::endl;
            break;
        }
        case xr::SessionState::Stopping:
        {
            session->endSession();
            session_running = false;
            std::cout << "session ended" << std::endl;
            break;
        }
        case xr::SessionState::Exiting:
        case xr::SessionState::LossPending:
            //session_running = false;
            shouldExit = true;
            break;
        default:
            break;
        }
    }

    bool PollOneEvent() {
        auto result = instance->pollEvent(evBuf);
        switch (result)
        {
        case xr::Result::Success:
            if (this->evBuf.type == xr::StructureType::EventDataEventsLost) {
                auto& eventsLost = *reinterpret_cast<xr::EventDataEventsLost*>(&this->evBuf);
                std::cout << "Event Lost: " << eventsLost.lostEventCount << std::endl;
            }
            return true;
        case xr::Result::EventUnavailable:
            return false;
        default:
            throw std::runtime_error(std::format("failed to poll event: {}", to_string(result)));
        }
    }

    void PollEvent() {
        while (PollOneEvent()) {
            std::cout << std::format("Event: {}", to_string(this->evBuf.type)) << std::endl;
            switch (this->evBuf.type)
            {
            case xr::StructureType::EventDataSessionStateChanged:
            {
                const auto& ev = *reinterpret_cast<xr::EventDataSessionStateChanged*>(&this->evBuf);
                std::cout << "State -> " << to_string(ev.state) << std::endl;
                HandleSessionStateChange(ev);
                break;
            }
            case xr::StructureType::EventDataViveTrackerConnectedHTCX: {
                const auto& viveTrackerConnected =
                    *reinterpret_cast<xr::EventDataViveTrackerConnectedHTCX*>(&this->evBuf);
                std::cout << instance->pathToString(viveTrackerConnected.paths->persistentPath) << std::endl;
                std::cout << instance->pathToString(viveTrackerConnected.paths->rolePath) << std::endl;
                break;
            }
            default:
                break;
            }

        }
    }

    void PollAction() {
        xr::ActiveActionSet activeActionSet;
        activeActionSet.actionSet = actionSet.get();
        activeActionSet.subactionPath = xr::Path(XR_NULL_PATH);

        //xr::ActiveActionSet activeActionSet2;
        //activeActionSet2.actionSet = trackerActionSet.get();
        //activeActionSet2.subactionPath = xr::Path(XR_NULL_PATH);

        auto activeActionSets = { activeActionSet, /*activeActionSet2*/ };

        xr::ActionsSyncInfo syncInfo{};
        syncInfo.countActiveActionSets = activeActionSets.size();
        syncInfo.activeActionSets = activeActionSets.begin();
        XR_CHK_ERR(session->syncActions(syncInfo));

        static bool oldState[2] = {};

        for (int i = 0; i < 2; i++) {
            xr::ActionStateGetInfo getInfo{};
            getInfo.action = handActions.trigger.get();
            getInfo.subactionPath = handActions.subActionPath[i];
            auto trigState = session->getActionStateBoolean(getInfo);

            if (trigState.isActive && trigState.currentState && !oldState[i]) {
                
                //xr::HapticActionInfo hapticActionInfo2{};
                //hapticActionInfo2.action = trackerActions.haptics.get();
                //hapticActionInfo2.subactionPath = trackerActions.subActionPath;

                //std::cout << instance->pathToString(trackerActions.subActionPath) << std::endl;

                //XR_CHK_ERR(session->applyHapticFeedback(hapticActionInfo2, reinterpret_cast<XrHapticBaseHeader*>(&vibration)));

                gameData.trigger[i] = true;
            }
            else {
                gameData.trigger[i] = false;
            }
            oldState[i] = trigState.currentState && trigState.isActive;
        }

        for(int i = 0; i < 2; i++)
        {
            xr::ActionStateGetInfo getInfo{};
            getInfo.action = handActions.pose.get();
            getInfo.subactionPath = handActions.subActionPath[i];
            auto poseState = session->getActionStatePose(getInfo);
        }
        //{
        //    xr::ActionStateGetInfo getInfo{};
        //    getInfo.action = trackerActions.pose.get();
        //    getInfo.subactionPath = trackerActions.subActionPath;
        //    auto poseState = session->getActionStatePose(getInfo);
        //}
    }

    void RenderFrame() {
        xr::FrameWaitInfo frameWaitInfo;
        auto frameState = session->waitFrame(frameWaitInfo);

        xr::FrameBeginInfo beginInfo{};
        XR_CHK_ERR(session->beginFrame(beginInfo));

        constexpr auto max_layers_num = 1;
        constexpr auto max_views_num = 2;

        std::array<xr::CompositionLayerBaseHeader*, max_layers_num> layers{};
        xr::CompositionLayerProjection layer{};
        std::array<xr::CompositionLayerProjectionView, max_views_num> projectionViews{};

        xr::FrameEndInfo endInfo;
        endInfo.displayTime = frameState.predictedDisplayTime;
        endInfo.environmentBlendMode = xr::EnvironmentBlendMode::Opaque;
        endInfo.layerCount = 0;
        endInfo.layers = layers.data();

        if (frameState.shouldRender) {
            xr::ViewLocateInfo locateInfo;
            locateInfo.viewConfigurationType = xr::ViewConfigurationType::PrimaryStereo;
            locateInfo.displayTime = frameState.predictedDisplayTime;
            locateInfo.space = this->appSpace.get();

            XrViewState viewState{ XR_TYPE_VIEW_STATE };
            uint32_t viewCountOutput;

            XR_CHK_ERR(session->locateViews(locateInfo, &viewState, this->views.size(), &viewCountOutput, views.data()));
            if (viewCountOutput != views.size())
                throw std::runtime_error("Failed to locate views: viewCountOutput != views.size()");

            for (int i = 0; i < 2; i++) {
                gameData.handPoses[i] = SpaceToPose(handActions.space[i].get(), appSpace.get(), frameState.predictedDisplayTime);
            }

            gameData.viewPose = SpaceToPose(viewSpace.get(), appSpace.get(), frameState.predictedDisplayTime);
            gameData.stagePose = SpaceToPose(stageSpace.get(), appSpace.get(), frameState.predictedDisplayTime);

            //renderDat.track = SpaceToPose(trackerActions.space.get(), appSpace.get(), frameState.predictedDisplayTime);

            gameData.dt = long double(frameState.predictedDisplayPeriod.get()) / 1'000'000'000;

            Game::proc(gameData);

            for (uint32_t i = 0; const auto & swapchain : swapchains) {
                const auto& swapchain = swapchains[i].handle;

                xr::SwapchainImageAcquireInfo acquireInfo;
                auto imageIndex = swapchain->acquireSwapchainImage(acquireInfo);

                xr::SwapchainImageWaitInfo waitInfo;
                waitInfo.timeout = xr::Duration::infinite();
                swapchain->waitSwapchainImage(waitInfo);

                projectionViews[i].type = xr::StructureType::CompositionLayerProjectionView;
                projectionViews[i].pose = views[i].pose;
                projectionViews[i].fov = views[i].fov;
                projectionViews[i].subImage.swapchain = swapchain.get();
                projectionViews[i].subImage.imageRect.offset = xr::Offset2Di{ 0, 0 };
                projectionViews[i].subImage.imageRect.extent = swapchains[i].extent;

                graphicsManager->render(i, imageIndex, projectionViews[i]);

                xr::SwapchainImageReleaseInfo releaseInfo;
                swapchain->releaseSwapchainImage(releaseInfo);

                i++;
            }

            layer.space = appSpace.get();
            layer.layerFlags = {};
            layer.viewCount = swapchains.size();
            layer.views = projectionViews.data();
            layers[endInfo.layerCount++] = reinterpret_cast<xr::CompositionLayerBaseHeader*>(&layer);
        }

        session->endFrame(endInfo);
    }

    int cnt = 0;

public:
    App() {
        showPlatformInfo();
        CreateInstance();
        InitializeSystem();
        graphicsManager = CreateGraphicsManager_Vulkan(instance.get(), systemId);

        InitializeSession();
        InitializeAction();
        CreateSwapchain();

        CreateReferenceSpace();

        graphicsManager->PrepareResources();
    }

    void MainLoop() {
        while (!shouldExit) {
            PollEvent();
            if (session_running) {
                PollAction();
                RenderFrame();
            }
            else {
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }
        }
    }

    ~App() {
        std::cout << "Session destroyed" << std::endl;
    }
};

int main()
{
    try {
        std::cout << "hello" << std::endl;
        App app{};
        app.MainLoop();
    }
    catch (std::exception e) {
        std::cout << e.what() << std::endl;
    }
}
