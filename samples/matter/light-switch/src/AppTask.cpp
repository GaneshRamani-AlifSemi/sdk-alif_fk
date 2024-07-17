/* Copyright (C) 2024 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 */

#include "AppTask.h"
#include "AppConfig.h"
#include "LightSwitch.h"
#include "MatterStack.h"

#include <DeviceInfoProviderImpl.h>
#include <app/TestEventTriggerDelegate.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>
#include <lib/support/CHIPMem.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/SafeInt.h>
#include <platform/CHIPDeviceLayer.h>
#include <system/SystemError.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace ::chip;
using namespace ::chip::app;
using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;

namespace
{
constexpr EndpointId kLightDimmerSwitchEndpointId = 1;
constexpr EndpointId kLightEndpointId = 1;

constexpr int kAppEventQueueSize = 10;

K_MSGQ_DEFINE(sAppEventQueue, sizeof(AppEvent), kAppEventQueueSize, alignof(AppEvent));

Identify sIdentify = {kLightEndpointId, AppTask::IdentifyStartHandler, AppTask::IdentifyStopHandler,
		      Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator};

chip::DeviceLayer::DeviceInfoProviderImpl gExampleDeviceInfoProvider;

CHIP_ERROR DevInit()
{
	LightSwitch::GetInstance().Init(kLightDimmerSwitchEndpointId);
	gExampleDeviceInfoProvider.SetStorageDelegate(
		&Server::GetInstance().GetPersistentStorage());
	chip::DeviceLayer::SetDeviceInfoProvider(&gExampleDeviceInfoProvider);
	return CHIP_NO_ERROR;
}
} // namespace

void AppTask::PostEvent(AppEvent * aEvent)
{
    if (!aEvent) {
        return;
    }
        
    if (k_msgq_put(&sAppEventQueue, aEvent, K_NO_WAIT) != 0) {
        LOG_INF("PostEvent fail");
    }
}

void AppTask::DispatchEvent(const AppEvent * aEvent)
{
    if (!aEvent) {
        return;
    }
    if (aEvent->Handler) {
        aEvent->Handler(aEvent);
    } else {
        LOG_INF("Dropping event without handler");
    }
}

void AppTask::GetEvent(AppEvent * aEvent)
{
    k_msgq_get(&sAppEventQueue, aEvent, K_FOREVER);
}

void AppTask::IdentifyStartHandler(Identify *)
{
	AppEvent event;

	event.Type = AppEventType::IdentifyStart;
	event.Handler = [](const AppEvent *) { LOG_INF("Identify start"); };
	PostEvent(&event);
}

void AppTask::IdentifyStopHandler(Identify *)
{
	AppEvent event;

	event.Type = AppEventType::IdentifyStop;
	event.Handler = [](const AppEvent *) { LOG_INF("Identify stop"); };
	PostEvent(&event);
}

void AppTask::StartBLEAdvertisementHandler(const AppEvent *)
{
	if (Server::GetInstance().GetFabricTable().FabricCount() != 0) {
		LOG_INF("Matter service BLE advertising not started - device is already "
			"commissioned");
		return;
	}

	if (ConnectivityMgr().IsBLEAdvertisingEnabled()) {
		LOG_INF("BLE advertising is already enabled");
		return;
	}

	if (Server::GetInstance().GetCommissioningWindowManager().OpenBasicCommissioningWindow() !=
	    CHIP_NO_ERROR) {
		LOG_ERR("OpenBasicCommissioningWindow() failed");
	}
}



CHIP_ERROR AppTask::Init()
{
	/* Initialize Matter stack */
	ReturnErrorOnFailure(MatterStack::Instance().matter_stack_init(DevInit));

	/* Init Light switch endpoint */
	
	

	/* Start Matter sheduler */
	ReturnErrorOnFailure(MatterStack::Instance().matter_stack_start());

	
	return CHIP_NO_ERROR;
}

CHIP_ERROR AppTask::StartApp()
{
	ReturnErrorOnFailure(Init());

	AppEvent event = {};

	while (true) {
		GetEvent(&event);
		DispatchEvent(&event);
	}

	return CHIP_NO_ERROR;
}
