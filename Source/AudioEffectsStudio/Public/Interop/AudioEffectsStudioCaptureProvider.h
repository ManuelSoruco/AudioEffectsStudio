// Copyright (c) 2026 AudioFactory. Licensed under the MIT License; see LICENSE.

#pragma once

#include "CoreMinimal.h"
#include "Features/IModularFeatures.h"

class SWindow;

struct FAudioEffectsStudioCaptureTarget
{
	FName Id;
	FString DisplayName;
	int32 FrameWaitCount = 10;
	int32 CropTopPixels = 35;
	int32 PaddingPixels = 20;
	TFunction<TSharedPtr<SWindow>()> OpenWindow;
	TFunction<void(TSharedPtr<SWindow>)> CloseWindow;
};

class IAudioEffectsStudioCaptureProvider : public IModularFeature
{
public:
	static FName GetModularFeatureName()
	{
		static const FName FeatureName(TEXT("AudioEffectsStudio.CaptureProvider"));
		return FeatureName;
	}

	virtual ~IAudioEffectsStudioCaptureProvider() = default;
	virtual FName GetProviderName() const = 0;
	virtual void EnumerateCaptureTargets(TArray<FAudioEffectsStudioCaptureTarget>& OutTargets) const = 0;
};
