#pragma once

#include "EditorSubsystem.h"

#include "CaptureOrchestrator/LiveLinkTcpListener.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

#include "CaptureOrchestratorSubsystem.generated.h"

class UOSCClient;
class UOSCServer;
struct FOSCBundle;
struct FOSCMessage;

UENUM()
enum class EOrchestratorState : uint8
{
	Idle,
	Recalibrating,
	Recording,
};

UENUM()
enum class EMocapState : uint8
{
	Disconnected,
	Idle,
	RecordingRequested,
	Recording,
};

UCLASS()
class UCaptureOrchestratorSubsystem : public UEditorSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	//~UEditorSubsystem interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~End of UEditorSubsystem interface

	//~FTickableGameObject interface
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override;
	virtual bool IsTickableInEditor() const override;
	virtual TStatId GetStatId() const override;
	//~End of FTickableGameObject interface

	// Remote functions for use via Unreal Remote Commands, intended to be triggered via Siri shortcuts
	UFUNCTION(BlueprintCallable)
	void RemoteStartRecording();
	UFUNCTION(BlueprintCallable)
	void RemoteStopRecording();
	UFUNCTION(BlueprintCallable)
	void RemoteRecalibrateMocap();
	UFUNCTION(BlueprintCallable)
	void RemoteSetSceneName(const FString& InSceneName);
	UFUNCTION(BlueprintCallable)
	void RemoteSetTakeNumber(const int32 InTakeNumber);

	void StartRecording();
	void StopRecording();
	void RecalibrateMocap();
	void SetSceneName(const FString& InSceneName);
	FText GetSceneName() const;
	void SetTakeNumber(const int32 InTakeNumber);
	int32 GetTakeNumber() const;
	FText GetIphoneIpAddress() const;
	FText GetIphoneStatus() const;
	FText GetRokokoStatus() const;
	FText GetDesktopStatus() const;
	int32 GetPhoneOscPort();
	int32 GetDesktopOscPort();
	void SetDesktopOscPort(int32 InDesktopOscPort);

private:
	void OnOrchestratorStateChange(EOrchestratorState InPreviousState, EOrchestratorState InNewState);

	FText MocapStateToText(EMocapState InMocapState) const;
	void StartRokokoCapture();
	void StopRokokoCapture();
	void RecalibrateRokokoCapture();
	void OnRokokoResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	UFUNCTION()
	void OnLiveLinkMessageReceived(const FOSCMessage& InMessage, const FString& InIpAddress, uint16 InPort);

	void RenameLiveLinkScene(const FString& InSceneName);
	void SetLiveLinkTake(int32 InTake);
	void StartLiveLinkCapture();
	void StopLiveLinkCapture();
	void TransferLiveLinkCapture(const FString& InFileName, int32 InRetryCount = 0);
	void CreateOscServer();
	TObjectPtr<UOSCClient> CreateOscClient();
	FString GetLocalIP();

	UFUNCTION()
	void StartNextFileTransfer(const FString& InDirectory, const FString& InFileName, int32 InRetryCount = 0);
	UFUNCTION()
	void OnFileTransferComplete(const FString& InFileName, int32 InRetryCount = 0);
	void HideLiveLinkVideo();
	void ShowLiveLinkVideo();

	FLiveLinkTcpListener* TcpListener = nullptr;
	UPROPERTY()
	TObjectPtr<UOSCClient> OscClient;
	UPROPERTY()
	TObjectPtr<UOSCServer> OscServer;
	EOrchestratorState CachedOrchestratorState = EOrchestratorState::Idle;
	EMocapState BodyMocapState = EMocapState::Idle;
	EMocapState FaceMocapState = EMocapState::Disconnected;

	int32 PhoneOscPort = 14785;
	int32 DesktopOscPort = 6000;
	FString IphoneAddress;
	FString SceneName;
	int32 TakeNumber = 0;
	float ElapsedDesyncDuration = 0.0f;
	float ElapsedRecordingDuration = 0.0f;

	int32 CurrentFileIndex = -1;

	static const FString RokokoApiUrl;
	static const float DesyncMaxDurationSeconds;
	static const float RecordingRequestedMaxDurationSeconds;
	static const int32 LiveLinkTcpTransferPort;
	static TArray<FString> LiveLinkFileNames;

	static EOrchestratorState GlobalOrchestratorState;
	static FString GlobalSceneName;
	static int32 GlobalTakeNumber;
};
