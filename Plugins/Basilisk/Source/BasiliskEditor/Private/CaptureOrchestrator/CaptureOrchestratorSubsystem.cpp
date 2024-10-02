#include "CaptureOrchestrator/CaptureOrchestratorSubsystem.h"

#include "Dom/JsonObject.h"
#include "HAL/PlatformFilemanager.h"
#include "HttpModule.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "IPAddress.h"
#include "MocapImporter/MocapImporterSubsystem.h"
#include "OSCAddress.h"
#include "OSCClient.h"
#include "OSCManager.h"
#include "OSCMessage.h"
#include "OSCServer.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "SocketSubsystem.h"

#pragma optimize("", off)

const FString UCaptureOrchestratorSubsystem::RokokoApiUrl = TEXT("http://127.0.0.1:14053/v1/1234/");
const float UCaptureOrchestratorSubsystem::DesyncMaxDurationSeconds = 1.0f;
const float UCaptureOrchestratorSubsystem::RecordingRequestedMaxDurationSeconds = 1.0f;
const int32 UCaptureOrchestratorSubsystem::LiveLinkTcpTransferPort = 8888;

// These MUST be static, as these need to be modified by Unreal Remote Control, which can only operate on the default Class Default Object.
EOrchestratorState UCaptureOrchestratorSubsystem::GlobalOrchestratorState = EOrchestratorState::Idle;
FString UCaptureOrchestratorSubsystem::GlobalSceneName = TEXT("SCENE_RENAME_ME");
int32 UCaptureOrchestratorSubsystem::GlobalTakeNumber = 1;
TArray<FString> UCaptureOrchestratorSubsystem::LiveLinkFileNames = {
    // "depth_data.bin",
    // "depth_metadata.mhaical",
    // "video_metadata.json",
    // "audio_metadata.json",
    // "frame_log.csv",
    // "thumbnail.jpg",
    // "take.json",
};

bool UCaptureOrchestratorSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}

void UCaptureOrchestratorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

    IphoneAddress = TEXT("Unknown");
    SceneName = UCaptureOrchestratorSubsystem::GlobalSceneName;
    TakeNumber = UCaptureOrchestratorSubsystem::GlobalTakeNumber;
    TcpListener = new FLiveLinkTcpListener(UCaptureOrchestratorSubsystem::LiveLinkTcpTransferPort);

    CreateOscServer();
}

void UCaptureOrchestratorSubsystem::Deinitialize()
{
    if (TcpListener)
    {
        TcpListener->Stop();
        delete TcpListener;
        TcpListener = nullptr;
    }

	Super::Deinitialize();
}

void UCaptureOrchestratorSubsystem::Tick(float DeltaTime)
{
    // Voice commands use Unreal Remote Command API to modifies these global statics on the Class Default Object.
    // This doesn't reflect on the editor version, so we listen for a change here in tick and update the instance.
    {
        if (CachedOrchestratorState != UCaptureOrchestratorSubsystem::GlobalOrchestratorState)
        {
            OnOrchestratorStateChange(CachedOrchestratorState, UCaptureOrchestratorSubsystem::GlobalOrchestratorState);
        }

        if (SceneName != UCaptureOrchestratorSubsystem::GlobalSceneName)
        {
            SetSceneName(UCaptureOrchestratorSubsystem::GlobalSceneName);
        }

        if (TakeNumber != UCaptureOrchestratorSubsystem::GlobalTakeNumber)
        {
            SetTakeNumber(UCaptureOrchestratorSubsystem::GlobalTakeNumber);
        }
    }

    /*
    if (FaceMocapState != BodyMocapState)
    {
        ElapsedDesyncDuration += DeltaTime;
    }

    if (CachedOrchestratorState == EOrchestratorState::Recording)
    {
        ElapsedRecordingDuration += DeltaTime;
    }

    switch (FaceMocapState)
    {
        case EMocapState::Disconnected:
        {
            break;
        }
        case EMocapState::RecordingRequested:
        {
            break;
        }
        case EMocapState::Idle:
        case EMocapState::Recording:
        default:
        {
            break;
        }
    }

    switch (BodyMocapState)
    {
        case EMocapState::Disconnected:
        {
            break;
        }
        case EMocapState::RecordingRequested:
        {
            break;
        }
        case EMocapState::Idle:
        case EMocapState::Recording:
        default:
        {
            break;
        }
    }*/
}

bool UCaptureOrchestratorSubsystem::IsTickable() const
{
    // Prevent the Class Default Object from ticking, otherwise we basically have two subsystems
    return !IsTemplate(RF_ClassDefaultObject);
}

bool UCaptureOrchestratorSubsystem::IsTickableWhenPaused() const
{
    return true;
}

bool UCaptureOrchestratorSubsystem::IsTickableInEditor() const
{
    return true;
}

TStatId UCaptureOrchestratorSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UCaptureOrchestratorSubsystem, STATGROUP_Tickables);
}

void UCaptureOrchestratorSubsystem::RemoteStartRecording()
{
    UCaptureOrchestratorSubsystem::GlobalOrchestratorState = EOrchestratorState::Recording;
}

void UCaptureOrchestratorSubsystem::RemoteStopRecording()
{
    UCaptureOrchestratorSubsystem::GlobalOrchestratorState = EOrchestratorState::Idle;
}

void UCaptureOrchestratorSubsystem::RemoteRecalibrateMocap()
{
    UCaptureOrchestratorSubsystem::GlobalOrchestratorState = EOrchestratorState::Recalibrating;
}

void UCaptureOrchestratorSubsystem::RemoteSetSceneName(const FString& InSceneName)
{
    UCaptureOrchestratorSubsystem::GlobalSceneName = InSceneName.Replace(TEXT(" "), TEXT("_")).Replace(TEXT("#"), TEXT("_"));
}

void UCaptureOrchestratorSubsystem::RemoteSetTakeNumber(const int32 InTakeNumber)
{
    UCaptureOrchestratorSubsystem::GlobalTakeNumber = InTakeNumber;
}

void UCaptureOrchestratorSubsystem::StartRecording()
{
    if (CachedOrchestratorState != EOrchestratorState::Recording)
    {
        UE_LOG(LogTemp, Warning, TEXT("Attempting to start recording from an invalid state."));
        return;
    }

    ElapsedRecordingDuration = 0.0f;

    StartRokokoCapture();
    StartLiveLinkCapture();

    UCaptureOrchestratorSubsystem::GlobalTakeNumber++;
}

void UCaptureOrchestratorSubsystem::StopRecording()
{
    StopRokokoCapture();
    StopLiveLinkCapture();
}

void UCaptureOrchestratorSubsystem::RecalibrateMocap()
{
    RecalibrateRokokoCapture();
}

void UCaptureOrchestratorSubsystem::SetSceneName(const FString& InSceneName)
{
    SceneName = InSceneName;
    RenameLiveLinkScene(UCaptureOrchestratorSubsystem::GlobalSceneName);
}

FText UCaptureOrchestratorSubsystem::GetSceneName() const
{
	return FText::FromString(UCaptureOrchestratorSubsystem::GlobalSceneName);
}

void UCaptureOrchestratorSubsystem::SetTakeNumber(const int32 InTakeNumber)
{
    TakeNumber = InTakeNumber;
    SetLiveLinkTake(UCaptureOrchestratorSubsystem::GlobalTakeNumber);
}

int32 UCaptureOrchestratorSubsystem::GetTakeNumber() const
{
    return UCaptureOrchestratorSubsystem::GlobalTakeNumber;
}

FText UCaptureOrchestratorSubsystem::GetIphoneIpAddress() const
{
    return FText::FromString(IphoneAddress);
}

FText UCaptureOrchestratorSubsystem::GetIphoneStatus() const
{
    return MocapStateToText(FaceMocapState);
}

FText UCaptureOrchestratorSubsystem::GetRokokoStatus() const
{
    return MocapStateToText(FaceMocapState);
}

FText UCaptureOrchestratorSubsystem::GetDesktopStatus() const
{
    switch (CachedOrchestratorState)
    {
        case EOrchestratorState::Idle: return FText::FromString(TEXT("Idle"));
        case EOrchestratorState::Recording: return FText::FromString(TEXT("Recording"));
        default: return FText::FromString(TEXT("Unknown"));
    }
}

FText UCaptureOrchestratorSubsystem::MocapStateToText(EMocapState InMocapState) const
{
    switch (FaceMocapState)
    {
        case EMocapState::Disconnected: return FText::FromString(TEXT("Disconnected"));
        case EMocapState::Idle: return FText::FromString(TEXT("Idle"));
        case EMocapState::RecordingRequested: return FText::FromString(TEXT("Recording Requested"));
        case EMocapState::Recording: return FText::FromString(TEXT("Recording"));
        default: return FText::FromString(TEXT("Unknown"));
    }
}

int32 UCaptureOrchestratorSubsystem::GetPhoneOscPort()
{
    return PhoneOscPort;
}

int32 UCaptureOrchestratorSubsystem::GetDesktopOscPort()
{
    return DesktopOscPort;
}

void UCaptureOrchestratorSubsystem::SetDesktopOscPort(int32 InDesktopOscPort)
{
    if (DesktopOscPort != InDesktopOscPort)
    {
        DesktopOscPort = InDesktopOscPort;

        // Recreate the server on the new port
        CreateOscServer();
    }
}

void UCaptureOrchestratorSubsystem::OnOrchestratorStateChange(EOrchestratorState InPreviousState, EOrchestratorState InNewState)
{
    CachedOrchestratorState = InNewState;

    switch (InPreviousState)
    {
        case EOrchestratorState::Recording:
        {
            StopRecording();
            break;
        }
        case EOrchestratorState::Recalibrating:
        case EOrchestratorState::Idle:
        default:
        {
            break;
        }
    }

    switch (InNewState)
    {
        case EOrchestratorState::Recording:
        {
            StartRecording();
            break;
        }
        case EOrchestratorState::Recalibrating:
        {
            RecalibrateMocap();
            break;
        }
        case EOrchestratorState::Idle:
        default:
        {
            break;
        }
    }
}

void UCaptureOrchestratorSubsystem::StartRokokoCapture()
{
    /*
    if (BodyMocapState != EMocapState::Idle)
    {
        UE_LOG(LogTemp, Warning, TEXT("Unable to request Rokoko capture."));
        return;
    }*/

    FHttpModule* Http = &FHttpModule::Get();
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("filename"), FString::Printf(TEXT("%s_%d"), *UCaptureOrchestratorSubsystem::GlobalSceneName, UCaptureOrchestratorSubsystem::GlobalTakeNumber));

    FString ContentString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ContentString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    Request->OnProcessRequestComplete().BindUObject(this, &UCaptureOrchestratorSubsystem::OnRokokoResponseReceived);
    Request->SetURL(RokokoApiUrl + TEXT("recording/start"));
    Request->SetVerb("POST");
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(ContentString);
    Request->ProcessRequest();
}

void UCaptureOrchestratorSubsystem::StopRokokoCapture()
{
    /*
    if (BodyMocapState != EMocapState::Recording)
    {
        UE_LOG(LogTemp, Warning, TEXT("Attempted to stop Rokoko capture while none was active."));
        return;
    }*/

    FHttpModule* Http = &FHttpModule::Get();
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetBoolField(TEXT("back_to_live"), true);

    FString ContentString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ContentString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    Request->OnProcessRequestComplete().BindUObject(this, &UCaptureOrchestratorSubsystem::OnRokokoResponseReceived);
    Request->SetURL(RokokoApiUrl + TEXT("recording/stop"));
    Request->SetVerb("POST");
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(ContentString);
    Request->ProcessRequest();
}

void UCaptureOrchestratorSubsystem::RecalibrateRokokoCapture()
{
    /*
    if (BodyMocapState != EMocapState::Idle)
    {
        UE_LOG(LogTemp, Warning, TEXT("Unable to request Rokoko calibration."));
        return;
    }*/

    FHttpModule* Http = &FHttpModule::Get();
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetNumberField(TEXT("countdown_delay"), 0.0f); // Instant since this is voice activated

    FString ContentString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ContentString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    Request->OnProcessRequestComplete().BindUObject(this, &UCaptureOrchestratorSubsystem::OnRokokoResponseReceived);
    Request->SetURL(RokokoApiUrl + TEXT("calibrate"));
    Request->SetVerb("POST");
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(ContentString);
    Request->ProcessRequest();
}

void UCaptureOrchestratorSubsystem::OnRokokoResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    if (!bWasSuccessful || !Response.IsValid())
    {
        // No sense in using the Disconnected state for a REST service. Just mark it as idle.
        // BodyMocapState = EMocapState::Idle;
        UE_LOG(LogTemp, Warning, TEXT("Request failed or response was invalid."));
        return;
    }

    // Parse the JSON response
    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());

    if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
    {
        FString Description;
        FString ResponseCode;

        JsonObject->TryGetStringField(TEXT("description"), Description);
        JsonObject->TryGetStringField(TEXT("response_code"), ResponseCode);

        UE_LOG(LogTemp, Log, TEXT("Rokoko response recieved. Description: %s. ResponseCode: %s"), *Description, *ResponseCode);

        if (CachedOrchestratorState == EOrchestratorState::Recalibrating)
        {
            if (Description == TEXT("Calibration started") && ResponseCode == TEXT("OK"))
            {
                UCaptureOrchestratorSubsystem::GlobalOrchestratorState = EOrchestratorState::Idle;
            }
            // This generally occurs when calibration encounters an error, and Rokoko needs to be rebooted
            // Just go back to Idle state until this happens
            else if (ResponseCode == TEXT("CALIBRATION_ALREADY_ONGOING"))
            {
                UCaptureOrchestratorSubsystem::GlobalOrchestratorState = EOrchestratorState::Idle;
            }
        }

        // Check if recording started
        if (CachedOrchestratorState == EOrchestratorState::Recording)
        {
            if (Description != TEXT("Recording started") || ResponseCode != TEXT("OK"))
            {
                UCaptureOrchestratorSubsystem::GlobalOrchestratorState = EOrchestratorState::Idle;
            }
        }
    }
}

void UCaptureOrchestratorSubsystem::OnLiveLinkMessageReceived(const FOSCMessage& InMessage, const FString& InIpAddress, uint16 InPort)
{
    const FString& MessageText = InMessage.GetAddress().GetFullPath();

    UE_LOG(LogTemp, Log, TEXT("Live Link Message Received: %s"), *MessageText);

    // Update desktop => iphone connection if the IP is new
    if (!IphoneAddress.Equals(InIpAddress) || PhoneOscPort != InPort)
    {
        IphoneAddress = InIpAddress;
        PhoneOscPort = InPort;
        OscClient = CreateOscClient();

        // Just assume any message guarantees the iphone is connected
        // if (MessageText == TEXT("/AppActivated"))
        {
            if (FaceMocapState == EMocapState::Disconnected)
            {
                FaceMocapState = EMocapState::Idle;
            }
        }
    }

    if (MessageText == TEXT("/AppDeactivated"))
    {
        FaceMocapState = EMocapState::Disconnected;
    }
    else if (MessageText == TEXT("/RecordStartConfirm"))
    {
        FaceMocapState = EMocapState::Recording;
    }
    else if (MessageText == TEXT("/RecordStopConfirm"))
    {
        FaceMocapState = EMocapState::Idle;

        FString FileName;
        UOSCManager::GetString(InMessage, 2, FileName);

        TransferLiveLinkCapture(FileName);
    }
}

void UCaptureOrchestratorSubsystem::RenameLiveLinkScene(const FString& InSceneName)
{
    if (!IsValid(OscClient))
    {
        OscClient = CreateOscClient();
    }

    if (ensure(IsValid(OscClient)))
    {
        FOSCMessage OscMessage;

        OscMessage.SetAddress(FOSCAddress(TEXT("/Slate")));
        OscMessage = UOSCManager::AddString(OscMessage, UCaptureOrchestratorSubsystem::GlobalSceneName); // Slate name

        OscClient->SendOSCMessage(OscMessage);
    }
}

void UCaptureOrchestratorSubsystem::SetLiveLinkTake(int32 InTake)
{
    if (!IsValid(OscClient))
    {
        OscClient = CreateOscClient();
    }

    if (ensure(IsValid(OscClient)))
    {
        FOSCMessage OscMessage;

        OscMessage.SetAddress(FOSCAddress(TEXT("/Take")));
        OscMessage = UOSCManager::AddInt32(OscMessage, UCaptureOrchestratorSubsystem::GlobalTakeNumber - 1); // Take number

        OscClient->SendOSCMessage(OscMessage);
    }
}

void UCaptureOrchestratorSubsystem::StartLiveLinkCapture()
{
    if (!IsValid(OscClient))
    {
        OscClient = CreateOscClient();
    }

    FaceMocapState = EMocapState::RecordingRequested;

    if (ensure(IsValid(OscClient)))
    {
        FOSCMessage OscMessage;

        OscMessage.SetAddress(FOSCAddress(TEXT("/RecordStart")));
        OscMessage = UOSCManager::AddString(OscMessage, UCaptureOrchestratorSubsystem::GlobalSceneName); // Slate name
        OscMessage = UOSCManager::AddInt32(OscMessage, UCaptureOrchestratorSubsystem::GlobalTakeNumber - 1); // Take number

        OscClient->SendOSCMessage(OscMessage);
    }
}

void UCaptureOrchestratorSubsystem::StopLiveLinkCapture()
{
    if (!IsValid(OscClient))
    {
        OscClient = CreateOscClient();
    }

    if (ensure(IsValid(OscClient)))
    {
        FOSCMessage OscMessage;

        OscMessage.SetAddress(FOSCAddress(TEXT("/RecordStop")));

        OscClient->SendOSCMessage(OscMessage);
    }
}

void UCaptureOrchestratorSubsystem::TransferLiveLinkCapture(const FString& InFileName, int32 InRetryCount)
{
    if (InFileName.IsEmpty())
    {
        ShowLiveLinkVideo();
        return;
    }

    CurrentFileIndex = -1;
    StartNextFileTransfer(FPaths::GetPath(InFileName), FPaths::GetCleanFilename(InFileName));
}

void UCaptureOrchestratorSubsystem::StartNextFileTransfer(const FString& InDirectory, const FString& InFileName, int32 InRetryCount)
{
    UE_LOG(LogTemp, Log, TEXT("Starting File Transfer: %s"), *(InDirectory / InFileName));

    if (!IsValid(OscClient))
    {
        OscClient = CreateOscClient();
    }
    
    // Turn off video so that we know file transfer is in progress
    HideLiveLinkVideo();

    if (ensure(IsValid(OscClient)))
    {
        FString FileName = InDirectory / InFileName;
        FString IPAddress;
        bool bCanBindAll;
        FOSCMessage OscMessage;

        // Get the local host IP
        ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
        TSharedRef<FInternetAddr> LocalAddr = SocketSubsystem->GetLocalHostAddr(*GLog, bCanBindAll);

        if (LocalAddr->IsValid())
        {
            IPAddress = LocalAddr->ToString(false);
        }

        FString IPAndPort = IPAddress + FString::Printf(TEXT(":%d"), LiveLinkTcpTransferPort);

        OscMessage.SetAddress(FOSCAddress(TEXT("/Transport")));
        OscMessage = UOSCManager::AddString(OscMessage, IPAndPort);
        OscMessage = UOSCManager::AddString(OscMessage, FileName);

        if (ensure(TcpListener))
        {
            if (TObjectPtr<UMocapImporterSubsystem> MocapImporter = GEditor->GetEditorSubsystem<UMocapImporterSubsystem>(); ensure(MocapImporter))
            {
                TcpListener->SetSleepTime(FTimespan::FromSeconds(1.0 * (double)InRetryCount));
                TcpListener->SetImportDirectory(MocapImporter->GetTripletImportPath());
                TcpListener->SetFileName(FileName);
                TcpListener->OnFileTransferComplete.BindUObject(this, &UCaptureOrchestratorSubsystem::OnFileTransferComplete, InRetryCount);
            }
        }

        OscClient->SendOSCMessage(OscMessage);
    }
}

void UCaptureOrchestratorSubsystem::OnFileTransferComplete(const FString& InFileName, int32 InRetryCount)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    const int32 MaxRetries = 4;

    FString ImportedFilePath;

    if (TObjectPtr<UMocapImporterSubsystem> MocapImporter = GEditor->GetEditorSubsystem<UMocapImporterSubsystem>(); ensure(MocapImporter))
    {
        ImportedFilePath = MocapImporter->GetTripletImportPath() / FPaths::GetCleanFilename(InFileName);
    }

    if (PlatformFile.FileSize(*ImportedFilePath) <= 0 && InRetryCount < MaxRetries)
    {
        StartNextFileTransfer(FPaths::GetPath(InFileName), FPaths::GetCleanFilename(InFileName), ++InRetryCount);
        return;
    }

    CurrentFileIndex++;

    // Start async file processors as the relevant data comes in
    if (ImportedFilePath.EndsWith(TEXT(".mov")))
    {
        AsyncTask(ENamedThreads::GameThread, [ImportedFilePath]()
        {
            if (TObjectPtr<UMocapImporterSubsystem> MocapImporter = GEditor->GetEditorSubsystem<UMocapImporterSubsystem>(); ensure(MocapImporter))
            {
                MocapImporter->ImportAudioMocap(*ImportedFilePath);
            }
        });
    }
    else if (ImportedFilePath.EndsWith(TEXT(".mhaical")))
    {
        AsyncTask(ENamedThreads::GameThread, [ImportedFilePath]()
        {
            if (TObjectPtr<UMocapImporterSubsystem> MocapImporter = GEditor->GetEditorSubsystem<UMocapImporterSubsystem>(); ensure(MocapImporter))
            {
                MocapImporter->ImportMetahumanAiCalibration(*ImportedFilePath);
            }
        });
    }

    if (UCaptureOrchestratorSubsystem::LiveLinkFileNames.IsValidIndex(CurrentFileIndex))
    {
        StartNextFileTransfer(FPaths::GetPath(InFileName), UCaptureOrchestratorSubsystem::LiveLinkFileNames[CurrentFileIndex]);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("Live Link file transfers complete"));

        ShowLiveLinkVideo();
    }
}

void UCaptureOrchestratorSubsystem::HideLiveLinkVideo()
{
    if (!IsValid(OscClient))
    {
        OscClient = CreateOscClient();
    }

    // Turn off video so that we know file transfer has started
    if (ensure(IsValid(OscClient)))
    {
        FOSCMessage OscMessage;
        OscMessage.SetAddress(FOSCAddress(TEXT("/VideoDisplayOff")));
        OscClient->SendOSCMessage(OscMessage);
    }
}

void UCaptureOrchestratorSubsystem::ShowLiveLinkVideo()
{
    if (!IsValid(OscClient))
    {
        OscClient = CreateOscClient();
    }

    // Turn on video so that we know file transfer is done
    if (ensure(IsValid(OscClient)))
    {
        FOSCMessage OscMessage;
        OscMessage.SetAddress(FOSCAddress(TEXT("/VideoDisplayOn")));
        OscClient->SendOSCMessage(OscMessage);
    }
}

FString UCaptureOrchestratorSubsystem::GetLocalIP()
{
    static FString LocalIp = TEXT("");

    if (!LocalIp.IsEmpty())
    {
        return LocalIp;
    }

    TArray<TSharedPtr<FInternetAddr>> LocalAddresses;
    ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalAdapterAddresses(LocalAddresses);
    for (const TSharedPtr<FInternetAddr>& Addr : LocalAddresses)
    {
        if (Addr->IsValid())
        {
            LocalIp = Addr->ToString(false);
        }
    }

    return LocalIp;
}

void UCaptureOrchestratorSubsystem::CreateOscServer()
{
    if (OscServer)
    {
        return;
    }

    OscServer = UOSCManager::CreateOSCServer(TEXT("0.0.0.0"), DesktopOscPort, false, true, TEXT("LiveLinkServer"), this);
    OscServer->OnOscMessageReceivedNative.AddUObject(this, &UCaptureOrchestratorSubsystem::OnLiveLinkMessageReceived);
    OscServer->SetTickInEditor(true);
    OscServer->Listen();
}

TObjectPtr<UOSCClient> UCaptureOrchestratorSubsystem::CreateOscClient()
{
    TObjectPtr<UOSCClient> NewOscClient = UOSCManager::CreateOSCClient(*IphoneAddress, PhoneOscPort, TEXT(""), this);

    return NewOscClient;
}

#pragma optimize("", on)
