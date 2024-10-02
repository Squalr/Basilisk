#include "MocapImporter/ProceduralRetargetAssets.h"

#include "Retargeter/RetargetOps/PinBoneOp.h"
#include "Retargeter/RetargetOps/RootMotionGeneratorOp.h"
#include "RigEditor/IKRigController.h"

FProceduralRetargetAssets::FProceduralRetargetAssets()
{
	FName Name = FName("BatchRetargeter");
	Name = MakeUniqueObjectName(GetTransientPackage(), UIKRetargeter::StaticClass(), Name, EUniqueObjectNameOptions::GloballyUnique);
	Retargeter = NewObject<UIKRetargeter>(GetTransientPackage(), Name, RF_Public | RF_Standalone);
}

void FProceduralRetargetAssets::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(SourceIKRig);
	Collector.AddReferencedObject(TargetIKRig);
	Collector.AddReferencedObject(Retargeter);
}

void FProceduralRetargetAssets::AutoGenerateIKRigAsset(USkeletalMesh* Mesh, ERetargetSourceOrTarget SourceOrTarget)
{
	TObjectPtr<UIKRigDefinition>* IKRig = SourceOrTarget == ERetargetSourceOrTarget::Source ? &SourceIKRig : &TargetIKRig;
	FName AssetName = SourceOrTarget == ERetargetSourceOrTarget::Source ? FName("BatchRetargetSourceIKRig") : FName("BatchRetargetTargetIKRig");
	AssetName = MakeUniqueObjectName(GetTransientPackage(), UIKRigDefinition::StaticClass(), AssetName, EUniqueObjectNameOptions::GloballyUnique);
	*IKRig = NewObject<UIKRigDefinition>(GetTransientPackage(), AssetName, RF_Public | RF_Standalone);

	if (!Mesh)
	{
		return;
	}

	// auto-setup retarget chains and IK for the given mesh
	const UIKRigController* Controller = UIKRigController::GetController(*IKRig);

	Controller->SetSkeletalMesh(Mesh);
	FAutoCharacterizeResults& CharacterizationResults = SourceOrTarget == ERetargetSourceOrTarget::Source ? SourceCharacterizationResults : TargetCharacterizationResults;
	Controller->AutoGenerateRetargetDefinition(CharacterizationResults);
	Controller->SetRetargetDefinition(CharacterizationResults.AutoRetargetDefinition.RetargetDefinition);
	FAutoFBIKResults IKResults = SourceOrTarget == ERetargetSourceOrTarget::Source ? SourceIKResults : TargetIKResults;
	Controller->AutoGenerateFBIK(IKResults);

	// assign to the retargeter
	const UIKRetargeterController* RetargetController = UIKRetargeterController::GetController(Retargeter);
	FScopedReinitializeIKRetargeter ReinitializeRetargeter(RetargetController);
	RetargetController->SetIKRig(SourceOrTarget, *IKRig);
}

void FProceduralRetargetAssets::AutoGenerateIKRetargetAsset()
{
	if (!(SourceIKRig && SourceIKRig->GetPreviewMesh() && TargetIKRig && TargetIKRig->GetPreviewMesh()))
	{
		return;
	}

	const UIKRetargeterController* RetargetController = UIKRetargeterController::GetController(Retargeter);

	FScopedReinitializeIKRetargeter Reinitialize(RetargetController);

	// re-assign both IK Rigs
	RetargetController->SetIKRig(ERetargetSourceOrTarget::Source, SourceIKRig);
	RetargetController->SetIKRig(ERetargetSourceOrTarget::Target, TargetIKRig);

	// reset and regenerate the target retarget pose
	const FName TargetRetargetPoseName = RetargetController->GetCurrentRetargetPoseName(ERetargetSourceOrTarget::Target);
	RetargetController->ResetRetargetPose(TargetRetargetPoseName, TArray<FName>() /* all bones if empty */, ERetargetSourceOrTarget::Target);
	RetargetController->AutoAlignAllBones(ERetargetSourceOrTarget::Target);

	// templates records a list of bones to exclude from the auto pose, so we reset them now; immediately after auto-aligning the whole skeleton.
	// by default this is used on biped feet where we want the feet to remain at their flat / default retarget pose
	const TArray<FName>& BonesToExcludeFromAutoPose = TargetCharacterizationResults.AutoRetargetDefinition.BonesToExcludeFromAutoPose;
	if (!BonesToExcludeFromAutoPose.IsEmpty())
	{
		const FName CurrentPose = RetargetController->GetCurrentRetargetPoseName(ERetargetSourceOrTarget::Target);
		RetargetController->ResetRetargetPose(
			CurrentPose,
			TargetCharacterizationResults.AutoRetargetDefinition.BonesToExcludeFromAutoPose,
			ERetargetSourceOrTarget::Target);
	}

	// disable IK pass because auto-IK is not yet robust enough, still useful for manual editing though
	FRetargetGlobalSettings GlobalSettings = RetargetController->GetGlobalSettings();
	GlobalSettings.bEnableIK = false;
	RetargetController->SetGlobalSettings(GlobalSettings);

	// clear the op stack and regenerate it
	RetargetController->RemoveAllOps();

	// add a root motion generator (but only if the skeleton has a root bone separate from the pelvis/retarget root)
	const FName TargetSkeletonRoot = TargetIKRig->GetSkeleton().BoneNames[0];
	const FName TargetPelvis = TargetCharacterizationResults.AutoRetargetDefinition.RetargetDefinition.RootBone;
	if (TargetSkeletonRoot != TargetPelvis)
	{
		// add a root motion op to the post retarget op stack
		const int32 RootOpIndex = RetargetController->AddRetargetOp(URootMotionGeneratorOp::StaticClass());

		// if the source does not have a dedicated root bone, configure the root motion to be generated from the pelvis motion on target
		const FName SourceSkeletonRoot = SourceIKRig->GetSkeleton().BoneNames[0];
		const FName SourcePelvis = SourceCharacterizationResults.AutoRetargetDefinition.RetargetDefinition.RootBone;
		if (SourceSkeletonRoot == SourcePelvis)
		{
			if (URootMotionGeneratorOp* RootOp = Cast<URootMotionGeneratorOp>(RetargetController->GetRetargetOpAtIndex(RootOpIndex)))
			{
				RootOp->RootMotionSource = ERootMotionSource::GenerateFromTargetPelvis;
				RootOp->RootHeightSource = ERootMotionHeightSource::SnapToGround;
				RootOp->bRotateWithPelvis = true;
				RootOp->bMaintainOffsetFromPelvis = true;
			}
		}
	}

	// setup a "Pin Bone Op" to pin IK bones to the new target locations
	const int32 PinBonesOpIndex = RetargetController->AddRetargetOp(UPinBoneOp::StaticClass());
	UPinBoneOp* PinBoneOp = CastChecked<UPinBoneOp>(RetargetController->GetRetargetOpAtIndex(PinBonesOpIndex));
	PinBoneOp->bMaintainOffset = false;
	const TArray<FBoneToPin>& AllBonesToPin = TargetCharacterizationResults.AutoRetargetDefinition.BonesToPin.GetBonesToPin();
	for (const FBoneToPin& BonesToPin : AllBonesToPin)
	{
		PinBoneOp->BonesToPin.Emplace(BonesToPin.BoneToPin, BonesToPin.BoneToPinTo);
	}
}
