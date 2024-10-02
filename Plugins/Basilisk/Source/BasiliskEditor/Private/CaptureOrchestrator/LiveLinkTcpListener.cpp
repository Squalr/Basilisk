#include "CaptureOrchestrator/LiveLinkTcpListener.h"

#include "HAL/RunnableThread.h"
#include "Async/Async.h"

#pragma optimize("", off)

FLiveLinkTcpListener::FLiveLinkTcpListener(int32 InPort)
{
    FIPv4Address Addr;
    FIPv4Address::Parse(TEXT("0.0.0.0"), Addr);
    Endpoint = FIPv4Endpoint(Addr, InPort);

    Thread = FRunnableThread::Create(this, TEXT("FLiveLinkTcpListener"), 8 * 1024, TPri_Normal);
}

FLiveLinkTcpListener::~FLiveLinkTcpListener()
{
    Stop();
    if (Thread != nullptr)
    {
        Thread->Kill(true);
        delete Thread;
    }
}

void FLiveLinkTcpListener::SetImportDirectory(const FString& InDirectory)
{
    ImportDirectoryRoot = InDirectory;
}

void FLiveLinkTcpListener::SetFileName(const FString& InFileName)
{
    FileName = InFileName;
}

void FLiveLinkTcpListener::SetSleepTime(FTimespan InSleepTime)
{
    SleepTime = InSleepTime;
}

bool FLiveLinkTcpListener::Init()
{
    if (Socket == nullptr)
    {
        Socket = FTcpSocketBuilder(TEXT("FTcpListener server"))
            .AsReusable(bSocketReusable)
            .BoundToEndpoint(Endpoint)
            .Listening(8)
            .WithSendBufferSize(2 * 1024 * 1024);
    }

    return (Socket != nullptr);
}

uint32 FLiveLinkTcpListener::Run()
{
    TSharedRef<FInternetAddr> RemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

    while (!bStopping)
    {
        bool Pending;

        FPlatformProcess::Sleep(SleepTime.GetSeconds());

        if (Socket->WaitForPendingConnection(Pending, SleepTime) && Pending)
        {
            FSocket* ConnectionSocket = Socket->Accept(*RemoteAddress, TEXT("FTcpListener client"));
            if (ConnectionSocket != nullptr)
            {
                // New method for processing incoming data from Live Link
                HandleLiveLinkData(ConnectionSocket);
            }
        }
    }

    return 0;
}

void FLiveLinkTcpListener::Stop()
{
    bStopping = true;
}

void FLiveLinkTcpListener::HandleLiveLinkData(FSocket* ConnectionSocket)
{
    if (!ensure(!FileName.IsEmpty()))
    {
        return;
    }

    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    PlatformFile.CreateDirectoryTree(*ImportDirectoryRoot);
    FString FilePath = ImportDirectoryRoot / FPaths::GetCleanFilename(FileName);
    IFileHandle* FileHandle = PlatformFile.OpenWrite(*FilePath);

    if (!FileHandle)
    {
        return;
    }

    int32 TotalBytesRead = 0;
    int32 BytesRead = 0;
    int32 FileSize = 0;

    ConnectionSocket->Recv((uint8*)&FileSize, sizeof(int32), BytesRead);
    FileSize = ByteSwap(FileSize);

    if (FileSize > 0)
    {
        TArray<uint8> DataBuffer;
        const int32 ChunkSize = 8192;
        DataBuffer.SetNumUninitialized(ChunkSize);

        while (TotalBytesRead < FileSize)
        {
            int32 Remaining = FMath::Min(ChunkSize, FileSize - TotalBytesRead);
            if (ConnectionSocket->Recv(DataBuffer.GetData(), Remaining, BytesRead))
            {
                FileHandle->Write(DataBuffer.GetData(), BytesRead);
                TotalBytesRead += BytesRead;
            }
            else
            {
                break;
            }
        }
    }

    delete FileHandle;

    if (OnFileTransferComplete.IsBound())
    {
        OnFileTransferComplete.Execute(FileName);
    }
}

const FIPv4Endpoint& FLiveLinkTcpListener::GetLocalEndpoint() const
{
    return Endpoint;
}

FSocket* FLiveLinkTcpListener::GetSocket() const
{
    return Socket;
}

bool FLiveLinkTcpListener::IsActive() const
{
    return ((Socket != nullptr) && !bStopping);
}

FOnTcpListenerConnectionAccepted& FLiveLinkTcpListener::OnConnectionAccepted()
{
    return ConnectionAcceptedDelegate;
}

#pragma optimize("", on)
