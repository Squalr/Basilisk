#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "Networking.h"

class FLiveLinkTcpListener : public FRunnable
{
public:
	FLiveLinkTcpListener(int32 InPort);
	virtual ~FLiveLinkTcpListener();

	void SetImportDirectory(const FString& InDirectory);
	void SetFileName(const FString& InFileName);
	void SetSleepTime(FTimespan InSleepTime);

	// FRunnable interface
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;

	DECLARE_DELEGATE_OneParam(FOnFileTransferComplete, const FString& /*FileName*/);
	FOnFileTransferComplete OnFileTransferComplete;

private:
	void HandleLiveLinkData(FSocket* ConnectionSocket);
	const FIPv4Endpoint& GetLocalEndpoint() const;
	FSocket* GetSocket() const;
	bool IsActive() const;
	FOnTcpListenerConnectionAccepted& OnConnectionAccepted();

	bool bDeleteSocket = false;
	FIPv4Endpoint Endpoint;
	FTimespan SleepTime;
	FSocket* Socket = nullptr;
	bool bStopping = false;
	bool bSocketReusable = true;
	FRunnableThread* Thread = nullptr;
	FString ImportDirectoryRoot;
	FString FileName;

	DECLARE_DELEGATE_RetVal_TwoParams(bool, FOnTcpListenerConnectionAccepted, FSocket*, const FIPv4Endpoint&)
	FOnTcpListenerConnectionAccepted ConnectionAcceptedDelegate;
};
