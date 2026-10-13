#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interfaces/IHttpRequest.h"
#include "AudioCaptureCore.h"
#include "CEDialogue.generated.h"

class ACECustomer;
class SWidget;
class SEditableTextBox;
class IInputProcessor;
class UAudioComponent;
class USoundWaveProcedural;
struct FCECaptureBuffer;
struct FCEChaosEvent;
struct FCEReplyStream;

struct FCEConversationTurn {
    FString PlayerText;
    FString ReplyText;
    FString ProviderReply;
};

UCLASS(ClassGroup=(CeroEstrellas), meta=(BlueprintSpawnableComponent))
class CEROESTRELLAS_API UCEDialogueComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UCEDialogueComponent();
    virtual ~UCEDialogueComponent() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void TickComponent(float Delta, ELevelTick Tick, FActorComponentTickFunction* Function) override;
    bool Open(ACECustomer* Customer);
    void Close();
    bool Submit(const FString& Text,bool bRetry=false);
    void ToggleRecording();
    bool IsOpen() const { return Target.IsValid(); }
    bool IsBusy() const { return bBusy; }
    bool IsEditing() const { return bEditing; }
    FString ConversationLabel() const;
    void ResumePlay();
    void EditConversation();
    FString Status = TEXT("Escribe una frase o pulsa Grabar voz.");
    FString Transcript;
    FString Draft;
    FString LastReply;
    FString LastEmotion;
    FString LastPlayerText;
    FString PreviewReply;
    double FirstTextSeconds = 0;
    double ResponseSeconds = 0;
    int32 ValidReplies = 0;
    int32 PlayedVoices = 0;
    int32 ServiceReplies = 0;
    int32 ServicePlayedVoices = 0;
    int32 DiscardedStaleVoices = 0;
    bool LastVoiceCacheHit = false;
    bool bVoiceEnabled = true;
    bool bRecording = false;
    bool bLastReplyFromOllama = false;
    // Only the automated scenario may inject Escape while external input is isolated.
    bool bSmokeInjectingEscape = false;
    FString Model = TEXT("qwen3:4b-instruct");
    // Shared by runtime and contract tests; reject incomplete/malformed output atomically.
    static bool ValidateReply(const FString& Json, FString& Text, FString& Emotion, float& Anger, bool& Leave);
    void TranscribeWav(const TArray<uint8>& Wav);
    bool SetDraft(const FString& Text);
    bool SubmitDraft();
private:
    TWeakObjectPtr<ACECustomer> Target;
    TSharedPtr<SWidget> Panel;
    TSharedPtr<SEditableTextBox> Input;
    TSharedPtr<IInputProcessor> EscapeProcessor;
    TArray<TSharedPtr<IHttpRequest,ESPMode::ThreadSafe>> Requests;
    TMap<FString,TArray<FCEConversationTurn>> History;
    TArray<FCEChaosEvent> Events;
    TUniquePtr<Audio::FAudioCapture> Capture;
    TSharedPtr<FCECaptureBuffer,ESPMode::ThreadSafe> CaptureData;
    TSharedPtr<FCEReplyStream,ESPMode::ThreadSafe> ReplyStream;
    TArray<uint8> StreamLine;
    FString StreamJson, StreamEnvelope;
    double TurnStartedAt = 0;
    void ConsumeReplyStream();
    UPROPERTY() TObjectPtr<UAudioComponent> Speaker;
    UPROPERTY() TObjectPtr<USoundWaveProcedural> Voice;
    bool bBusy = false;
    bool bEditing = false;
    bool bRecordingOnTheMove = false;
    int32 Generation = 0;
    int32 SpeechGeneration = 0;
    float RecordStartedAt = 0;
    UFUNCTION() void OnChaos(const FCEChaosEvent& Event);
    void StopRecording(bool bSend);
    void CancelRequests();
    void SpeakReply(const FString& Text, int32 Turn);
    void BuildPanel();
    bool TryServiceReply(const FString& Text,int32 Turn);
};
