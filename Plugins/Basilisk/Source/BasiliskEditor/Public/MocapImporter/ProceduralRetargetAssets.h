#pragma once

#include "EditorSubsystem.h"

#include "RigEditor/SIKRigOutputLog.h"
#include "RigEditor/IKRigAutoCharacterizer.h"
#include "RigEditor/IKRigAutoFBIK.h"

class UIKRigDefinition;
class UIKRetargeter;

struct FProceduralRetargetAssets : FGCObject
{
	FProceduralRetargetAssets();

	// FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("FProceduralRetargetAssets"); }
	// END FGCObject

	void AutoGenerateIKRigAsset(USkeletalMesh* Mesh, ERetargetSourceOrTarget SourceOrTarget);
	void AutoGenerateIKRetargetAsset();

	// resulting assets used to retarget animation, or export to disk
	TObjectPtr<UIKRigDefinition> SourceIKRig;
	TObjectPtr<UIKRigDefinition> TargetIKRig;
	TObjectPtr<UIKRetargeter> Retargeter;

	// results of characterizing for error reporting
	FAutoCharacterizeResults SourceCharacterizationResults;
	FAutoCharacterizeResults TargetCharacterizationResults;
	FAutoFBIKResults SourceIKResults;
	FAutoFBIKResults TargetIKResults;

private:
	void CreateNewRetargetAsset();
};
