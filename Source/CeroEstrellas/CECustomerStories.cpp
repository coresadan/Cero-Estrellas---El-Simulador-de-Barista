#include "CEGameplay.h"
#include "CEVisual.h"
#include "CESocialSpeech.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

namespace {
struct FGuestStory { const TCHAR* Title; const TCHAR* Setup; const TCHAR* Goal; const TCHAR* Good; const TCHAR* Bad; };
const FGuestStory Stories[]={
    {TEXT("ENTREVISTA EN DIEZ MINUTOS"),TEXT("Tengo una entrevista. Mi currículum dice que tolero la presión."),TEXT("Sirve el desayuno y dale ánimos; o arruina su ensayo."),TEXT("Voy a pedir un sueldo digno. Gracias por darme valor."),TEXT("La entrevista será sobre gestión de catástrofes. Tengo experiencia reciente.")},
    {TEXT("CITA A CIEGAS"),TEXT("Mi cita dijo que llevaría una flor. Aquí solo veo gente traumada."),TEXT("Sirve su desayuno y tranquilízale; o sabotea la cita."),TEXT("Si esta cita sale mal, al menos he desayunado bien."),TEXT("La mancha cuenta como flor marrón. Espero que me reconozca.")},
    {TEXT("CRÍTICA EN INCÓGNITO"),TEXT("No soy crítico. Ese cuaderno es para anotar... sentimientos."),TEXT("Sirve bien y descubre su secreto; o consigue una reseña terrible."),TEXT("Anotaré: servicio sorprendentemente competente. No se lo digas a nadie."),TEXT("Cero estrellas. Por fin un restaurante que cumple su promesa.")},
    {TEXT("AUDICIÓN DE ÓPERA"),TEXT("Tengo una audición. Necesito calentar la voz, no la camisa."),TEXT("Sirve y anima al cantante; o transforma su ensayo en un grito."),TEXT("Mi aria se titulará: gracias por no tirarme nada."),TEXT("¡MI CAMISA! Ese agudo me dará el papel protagonista.")},
    {TEXT("POEMA PARA UN EX"),TEXT("Escribo un poema para mi ex. Me falta una palabra que rime con latte."),TEXT("Sirve el desayuno y sugiere un verso; o inspira un poema de odio."),TEXT("Ya tengo el verso: tu ausencia sabe a café. Voy a enviarlo."),TEXT("Ahora mi poema se titula: ojalá te atiendan aquí.")}
};
const TCHAR* Names[]={TEXT("CRIPTO-BRO"),TEXT("HIPSTER PURISTA"),TEXT("INFLUENCER"),TEXT("PUNK DE OFICINA"),TEXT("ASTRONAUTA DE FIN DE SEMANA"),TEXT("DIVA DE LA ÓPERA"),TEXT("MOTORISTA CON PRISA"),TEXT("ABUELA RAVE"),TEXT("MAGO SIN CONTRATO")};
const TCHAR* Voices[]={TEXT("Inversor cripto sarcástico; presumes de NFTs."),TEXT("Hipster pedante del café."),TEXT("Influencer pasivo-agresiva, obsesionada con su outfit."),TEXT("Punk que acaba de salir de una oficina; rebelde, habla de nóminas."),TEXT("Aficionado al espacio; describes el desayuno como una misión espacial."),TEXT("Cantante de ópera dramática; conviertes cualquier molestia en tragedia."),TEXT("Motorista impaciente pero amable si le atienden bien."),TEXT("Abuela aficionada a las raves; humor seco y jerga de discoteca."),TEXT("Mago desempleado; tus trucos fallan, te tomas muy en serio.")};
}

FString ACECustomer::GuestPersonality() const { return LookIndex>=0 ? Voices[LookIndex%9] : Voices[Personality]; }
FString ACECustomer::StoryAnswer() const {
    if(StoryId<0) return TEXT("He venido a desayunar y a tener cinco minutos de paz.");
    return bStoryResolved ? StoryOutcome : !StoryBeat.IsEmpty() ? StoryBeat+(StoryDraft.IsEmpty() ? TEXT("") : TEXT(" Mi verso: ")+StoryDraft) : StorySetup;
}
void ACECustomer::InitStory(int32 ForceStory) {
    if(LookIndex<0) return; // Legacy fixtures have no hidden extra objective.
    DisplayName=Names[LookIndex%9];
    StoryId=ForceStory>=0 ? FMath::Clamp(ForceStory,0,4) : (LookIndex+GetTypeHash(CustomerId)%5)%5;
    const auto& S=Stories[StoryId]; StoryTitle=S.Title; StorySetup=S.Setup; StoryObjective=S.Goal;
    StoryStage=0; StoryBeat=StorySetup; StoryDraft.Empty(); bStoryResolved=false; bStoryHelped=false;
    StoryStartedAt=StoryStageStartedAt=GetWorld()->GetTimeSeconds();
}
void ACECustomer::StoryConversation(const FString& Text) {
    if(StoryId<0 || bStoryResolved) return;
    FString Key=Text.ToLower();
    const bool Help=Key.Contains(TEXT("suerte")) || Key.Contains(TEXT("tranquil")) || Key.Contains(TEXT("ánimo")) || Key.Contains(TEXT("animo")) || Key.Contains(TEXT("puedes")) || Key.Contains(TEXT("crítico")) || Key.Contains(TEXT("critico")) || Key.Contains(TEXT("poema")) || Key.Contains(TEXT("verso"));
    if(Help) bStoryHelped=true;
    if(StoryId==4 && (Key.Contains(TEXT("verso")) || Key.Contains(TEXT("poema")))) {
        int32 Colon; if(Text.FindChar(TEXT(':'),Colon)) StoryDraft=Text.Mid(Colon+1).TrimStartAndEnd().Left(80);
        else StoryDraft=TEXT("Tu ausencia sabe a café; mi paciencia también.");
    }
    if(bStoryHelped && State==ECECustomerState::Eating && StoryStage==1 && GetWorld()->GetTimeSeconds()-StoryStageStartedAt>=4.f) AdvanceStory();
    const bool Insult=Key.Contains(TEXT("idiota")) || Key.Contains(TEXT("ridícul")) || Key.Contains(TEXT("ridicul"));
    if(Insult) ResolveStory(false);
}
void ACECustomer::ResolveStory(bool bSuccess) {
    if(StoryId<0 || bStoryResolved) return;
    bStoryResolved=true; StoryStage=3; StoryOutcome=bSuccess ? Stories[StoryId].Good : Stories[StoryId].Bad;
    auto* Cafe=GetWorld()->GetSubsystem<UCECafeSubsystem>();
    if(bSuccess) ++Cafe->StoriesHelped; else ++Cafe->StoriesRuined;
    Cafe->ViralPoints+=bSuccess ? 80 : 160;
    Cafe->LastStoryOutcome=StoryTitle+TEXT(": ")+StoryOutcome;
    Cafe->Notify((bSuccess ? TEXT("HISTORIA / Has ayudado: ") : TEXT("HISTORIA / Has arruinado: "))+StoryTitle);
    if(bSuccess && State==ECECustomerState::Eating) EatUntil=FMath::Max(EatUntil,GetWorld()->GetTimeSeconds()+7.f);
    StoryBeat=StoryOutcome; Speak(StoryOutcome,7.f);
    // Optional story outcomes do not cause radio calls by themselves.
    Cafe->Observe(TEXT("story_resolved"),CustomerId,StoryTitle,0.f);
}
void ACECustomer::AdvanceStory()
{
    if(StoryId<0 || bStoryResolved || StoryStage>=2) return;
    ++StoryStage; StoryStageStartedAt=GetWorld()->GetTimeSeconds();
    const TCHAR* First[]={TEXT("Me está llamando la empresa. ¿Ensayamos cómo pedir un sueldo?"),TEXT("Mi cita debería estar entrando. ¿Puedes ayudarme a reconocerla?"),TEXT("Voy a anotar cómo atiendes. Sí, ese cuaderno era de un crítico."),TEXT("Voy a ensayar el aria aquí. Nadie ha pedido silencio, ¿verdad?"),TEXT("He abierto el borrador. Puedes proponer un verso: escríbelo tras dos puntos.")};
    const TCHAR* Second[]={TEXT("Ya he desayunado. Ahora voy a devolver la llamada y defender mi sueldo."),TEXT("El desayuno está servido. Mi cita y yo tenemos tema de conversación."),TEXT("Desayuno probado. Estoy terminando la reseña."),TEXT("Con el desayuno y los ánimos, voy a cantar mi última nota."),TEXT("Voy a leer el verso antes de enviárselo a mi ex.")};
    StoryBeat=StoryStage==1 ? First[StoryId] : Second[StoryId]; SocialGestureUntil=StoryStageStartedAt+4.f;
    if(StoryId==0 && StoryStage==1) { PhoneCallUntil=StoryStageStartedAt+6.f; GetWorld()->GetSubsystem<UCECafeSubsystem>()->Sound(TEXT("PhoneRing"),HeadLocation(),.45f); }
    if(StoryId==3) GetWorld()->GetSubsystem<UCECafeSubsystem>()->Sound(TEXT("OperaWarmup"),HeadLocation(),.3f);
    Speak(StoryBeat,6.f);
    GetWorld()->GetSubsystem<UCECafeSubsystem>()->Observe(TEXT("story_stage"),CustomerId,StoryBeat,0.f);
}
void ACECustomer::PairWith(ACECustomer* Other)
{
    if(!Other || Other==this || Other->bLeaving || !SocialPartnerId.IsEmpty() || !Other->SocialPartnerId.IsEmpty()) return;
    SocialPartnerId=Other->CustomerId; Other->SocialPartnerId=CustomerId;
    if(SocialGroup.IsEmpty()) SocialGroup=TEXT("encuentro-")+CustomerId;
    Other->SocialGroup=SocialGroup;
    SocialGestureUntil=Other->SocialGestureUntil=GetWorld()->GetTimeSeconds()+4.f;
    ++GetWorld()->GetSubsystem<UCECafeSubsystem>()->StoryEncounters;
    Speak(StoryId==1 ? TEXT("¡Eres mi cita! Pensé que la flor era el cruasán.") : TEXT("Ven, te guardo conversación. La silla corre por tu cuenta."),5.f);
    CESocial::Reply(Other,TEXT("He encontrado el sitio. ¿El desastre también venía en la reserva?"));
}
void ACECustomer::TickStory(float Delta)
{
    if(StoryId<0 || bStoryResolved) return;
    const float Now=GetWorld()->GetTimeSeconds();
    if(bLeaving) { ResolveStory(false); return; }
    if(State==ECECustomerState::Eating && bStoryHelped) EatUntil=FMath::Max(EatUntil,Now+1.f);
    if(StoryStage==0 && Now-StoryStartedAt>=15.f) AdvanceStory();
    if(StoryStage==1 && State==ECECustomerState::Eating && bStoryHelped && Now-StoryStageStartedAt>=4.f) AdvanceStory();
    if(StoryStage==2 && bStoryHelped) EatUntil=FMath::Max(EatUntil,StoryStageStartedAt+9.f);
    if(StoryStage==2 && Now-StoryStageStartedAt>=8.f && State==ECECustomerState::Eating && bStoryHelped) ResolveStory(true);
}
