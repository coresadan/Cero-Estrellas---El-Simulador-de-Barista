#include "CEGameplay.h"
#include "CEVisual.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"

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
    return bStoryResolved ? StoryOutcome : StorySetup;
}
void ACECustomer::InitStory(int32 ForceStory) {
    if(LookIndex<0) return; // Legacy fixtures have no hidden extra objective.
    DisplayName=Names[LookIndex%9];
    StoryId=ForceStory>=0 ? FMath::Clamp(ForceStory,0,4) : (LookIndex+GetTypeHash(CustomerId)%5)%5;
    const auto& S=Stories[StoryId]; StoryTitle=S.Title; StorySetup=S.Setup; StoryObjective=S.Goal;
    StoryStartedAt=GetWorld()->GetTimeSeconds();
}
void ACECustomer::StoryConversation(const FString& Text) {
    if(StoryId<0 || bStoryResolved) return;
    FString Key=Text.ToLower();
    const bool Help=Key.Contains(TEXT("suerte")) || Key.Contains(TEXT("tranquil")) || Key.Contains(TEXT("ánimo")) || Key.Contains(TEXT("animo")) || Key.Contains(TEXT("puedes")) || Key.Contains(TEXT("crítico")) || Key.Contains(TEXT("critico")) || Key.Contains(TEXT("poema")) || Key.Contains(TEXT("verso"));
    if(Help) bStoryHelped=true;
    const bool Insult=Key.Contains(TEXT("idiota")) || Key.Contains(TEXT("ridícul")) || Key.Contains(TEXT("ridicul"));
    if(Insult) ResolveStory(false);
}
void ACECustomer::ResolveStory(bool bSuccess) {
    if(StoryId<0 || bStoryResolved) return;
    bStoryResolved=true; StoryOutcome=bSuccess ? Stories[StoryId].Good : Stories[StoryId].Bad;
    auto* Cafe=GetWorld()->GetSubsystem<UCECafeSubsystem>();
    if(bSuccess) ++Cafe->StoriesHelped; else ++Cafe->StoriesRuined;
    Cafe->ViralPoints+=bSuccess ? 80 : 160;
    Cafe->LastStoryOutcome=StoryTitle+TEXT(": ")+StoryOutcome;
    Cafe->Notify((bSuccess ? TEXT("HISTORIA / Has ayudado: ") : TEXT("HISTORIA / Has arruinado: "))+StoryTitle);
    Speak(StoryOutcome,7.f);
    // Optional story outcomes do not cause radio calls by themselves.
    Cafe->Observe(TEXT("story_resolved"),CustomerId,StoryTitle,0.f);
}
void ACECustomer::TickStory(float Delta) {
    if(StoryId<0 || bStoryResolved) return;
    if(State==ECECustomerState::Eating && bStoryHelped) ResolveStory(true);
    else if(bLeaving) ResolveStory(false);
}
