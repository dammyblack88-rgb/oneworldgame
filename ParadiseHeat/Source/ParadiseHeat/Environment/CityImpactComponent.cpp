#include "CityImpactComponent.h"
#include "Animation/AnimSequence.h"
#include "ParadiseHeatCityRoute.h"
#include "Components/SplineComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "ParadiseHeat.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

UCityImpactComponent::UCityImpactComponent(){PrimaryComponentTick.bCanEverTick=true;}
UCityImpactComponent* UCityImpactComponent::Install(AActor* Actor)
{
 if(!Actor)return nullptr;
 if(auto* Existing=Actor->FindComponentByClass<UCityImpactComponent>())return Existing;
 auto* C=NewObject<UCityImpactComponent>(Actor);Actor->AddInstanceComponent(C);C->RegisterComponent();
 if(!Cast<ACharacter>(Actor))if(auto* Root=Cast<UPrimitiveComponent>(Actor->GetRootComponent()))
 {Root->SetNotifyRigidBodyCollision(true);Root->SetUseCCD(true);Root->OnComponentHit.AddDynamic(C,&UCityImpactComponent::OnHit);}
 return C;
}
bool UCityImpactComponent::KnockDown(ACharacter* Human,FVector Velocity)
{
 if(!Human||Human->GetMesh()->IsSimulatingPhysics()||!Human->GetMesh()->GetPhysicsAsset())return false;
 auto* C=Install(Human);const float Now=Human->GetWorld()->GetTimeSeconds();
 if(Now<C->Cooldown)return false;
 C->Cooldown=Now+7.f;C->RecoverAt=Now+4.f;C->bFallen=true;
 C->RecoveryOrigin=Human->GetActorLocation();C->SavedMesh=Human->GetMesh()->GetRelativeTransform();
 Human->Tags.AddUnique(TEXT("ImpactRagdoll"));
 Human->GetCharacterMovement()->StopMovementImmediately();Human->GetCharacterMovement()->DisableMovement();
 Human->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 auto* Mesh=Human->GetMesh();Mesh->SetCollisionProfileName(TEXT("Ragdoll"));Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 Mesh->SetUseCCD(true);Mesh->SetAllBodiesSimulatePhysics(true);Mesh->SetSimulatePhysics(true);
 Velocity=Velocity.GetClampedToMaxSize(1600.f);Velocity.Z=FMath::Clamp(Velocity.Z+100.f,60.f,250.f);
 Mesh->SetAllPhysicsLinearVelocity(Velocity);
 // Vehicle knockdowns are silent; deliberate punches and ejections own their reaction audio.
 UE_LOG(LogParadiseHeat,Display,TEXT("IMPACT_PERSON down=%s speed=%.1f"),*Human->GetName(),Velocity.Size());return true;
}
void UCityImpactComponent::ActivateWreck(FVector Velocity,FVector ImpactVelocity)
{
 auto* Actor=GetOwner();
 if(!Actor->ActorHasTag(TEXT("CityTraffic"))||Actor->ActorHasTag(TEXT("ImpactWreck")))return;
 auto* Root=Cast<UPrimitiveComponent>(Actor->GetRootComponent());if(!Root)return;
 Actor->Tags.AddUnique(TEXT("ImpactWreck"));
 const bool Bike=Actor->ActorHasTag(TEXT("AmbientMotorcycle"));
 Root->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Root->SetMassOverrideInKg(NAME_None,Bike?240.f:1400.f,true);
 Root->SetLinearDamping(.35f);Root->SetAngularDamping(1.5f);Root->SetEnableGravity(true);Root->SetSimulatePhysics(true);
 Root->SetPhysicsLinearVelocity(Velocity.GetClampedToMaxSize(2200.f));Root->WakeAllRigidBodies();
 if(Bike)
 {
  Root->AddAngularImpulseInRadians(Actor->GetActorForwardVector()*2.5f,NAME_None,true);
  TArray<UPoseableMeshComponent*> Parts;Actor->GetComponents(Parts);
  for(auto* Part:Parts)if(Part->GetFName()==TEXT("TrafficOccupant")&&Part->IsVisible()&&!Part->bHiddenInGame&&!Actor->ActorHasTag(TEXT("DriverEjected")))
  {
   FActorSpawnParameters P;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
   auto* Rider=Actor->GetWorld()->SpawnActor<ACharacter>(Actor->GetActorLocation()+FVector(0,0,110),Actor->GetActorRotation(),P);
   if(Rider)
   {
    Rider->Tags.AddUnique(TEXT("TrafficCrashSurvivor"));
    Rider->GetCapsuleComponent()->SetCapsuleSize(34.f,96.f);
    Rider->GetCharacterMovement()->bRunPhysicsWithNoController=true;
    Rider->GetCharacterMovement()->bOrientRotationToMovement=true;
    Rider->GetCharacterMovement()->RotationRate=FRotator(0,300,0);
    Rider->GetMesh()->SetSkeletalMesh(Cast<USkeletalMesh>(Part->GetSkinnedAsset()));
    Rider->GetMesh()->SetRelativeLocationAndRotation(FVector(0,0,-96),FRotator(0,-90,0));
    if(auto* Idle=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/ScannedPlayer/Animations/Nathan_MM_Idle.Nathan_MM_Idle")))Rider->GetMesh()->PlayAnimation(Idle,true);
    Rider->GetMesh()->TickAnimation(0.f,false);Rider->GetMesh()->RefreshBoneTransforms();
    if(KnockDown(Rider,Velocity+ImpactVelocity)){Actor->Tags.AddUnique(TEXT("DriverEjected"));Part->SetVisibility(false,true);Rider->SetLifeSpan(45.f);}else Rider->Destroy();
   }
  }
 }
 UE_LOG(LogParadiseHeat,Display,TEXT("IMPACT_WRECK actor=%s bike=%d speed=%.1f"),*Actor->GetName(),Bike,Velocity.Size());
}
void UCityImpactComponent::OnHit(UPrimitiveComponent* Body,AActor* Other,UPrimitiveComponent* OtherBody,FVector Impulse,const FHitResult& Hit)
{Contact(Other,OtherBody,Hit,PreviousVelocity);}
void UCityImpactComponent::Contact(AActor* Other,UPrimitiveComponent* OtherBody,const FHitResult& Hit,FVector Velocity)
{
 if(!Other||Other==GetOwner()||Hit.bStartPenetrating||FMath::Abs(Hit.ImpactNormal.Z)>.7f)return;
 // Routine spline traffic yields on contact. Only player vehicles or already
 // simulated wrecks initiate physical crashes; route corrections are not impacts.
 if(GetOwner()->ActorHasTag(TEXT("CityTraffic")) && !GetOwner()->ActorHasTag(TEXT("ImpactWreck"))
  && !(Cast<APawn>(Other)&&Cast<APawn>(Other)->IsPlayerControlled()) && !Other->ActorHasTag(TEXT("ImpactWreck")))return;
 // A pedestrian walking into a parked vehicle is not a vehicle strike.
 if(Cast<ACharacter>(Other)&&(Velocity.SizeSquared2D()<FMath::Square(450.f)||Hit.bStartPenetrating))return;
 if(GetOwner()->ActorHasTag(TEXT("VehicleEntryPending"))||Other->ActorHasTag(TEXT("VehicleEntryPending")))return;
 const float Now=GetWorld()->GetTimeSeconds();if(Now<Cooldown)return;
 const FVector OtherVelocity=OtherBody?OtherBody->GetComponentVelocity():Other->GetVelocity();
 const float Closing=FMath::Max(0.f,float(-FVector::DotProduct(Velocity-OtherVelocity,Hit.ImpactNormal)));
 if(Closing<280.f)return;
 Cooldown=Now+.65f;
 if(auto* Human=Cast<ACharacter>(Other))KnockDown(Human,Velocity*.65f);
 ActivateWreck(Velocity);
 if(Other->ActorHasTag(TEXT("CityTraffic")))
 {
  const bool NewWreck=!Other->ActorHasTag(TEXT("ImpactWreck"));
  auto* Response=Install(Other);Response->ActivateWreck(OtherVelocity,Velocity*.4f);
  if(NewWreck&&OtherBody&&OtherBody->IsSimulatingPhysics())
  {
   const float Mass=Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent())->GetMass();
   const float OtherMass=OtherBody->GetMass();
   const float ReducedMass=Mass*OtherMass/FMath::Max(1.f,Mass+OtherMass);
   OtherBody->AddImpulseAtLocation(-Hit.ImpactNormal*Closing*ReducedMass*.6f,Hit.ImpactPoint);
  }
 }
 UE_LOG(LogParadiseHeat,Display,TEXT("IMPACT_CONTACT owner=%s other=%s closing=%.1f"),*GetOwner()->GetName(),*Other->GetName(),Closing);
}
void UCityImpactComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Tick)
{
 Super::TickComponent(Dt,Type,Tick);
 // Opt-in integration check: launch actual traffic bodies into a car, bike and pedestrian.
 if(FParse::Param(FCommandLine::Get(),TEXT("CityImpactAudit")))
 {
  static TWeakObjectPtr<AActor> TestCar;
  static TWeakObjectPtr<AActor> PreviousTarget;
  static int32 Stage=0;
  static float NextTest=12.f;
  if(!TestCar.IsValid()&&GetOwner()->ActorHasTag(TEXT("CityTraffic"))&&!GetOwner()->ActorHasTag(TEXT("AmbientMotorcycle")))
   TestCar=GetOwner();
  if(TestCar.Get()==GetOwner()&&Stage<3&&GetWorld()->GetTimeSeconds()>NextTest)
  {
   if(PreviousTarget.IsValid())PreviousTarget->Destroy();
   AActor* Target=nullptr;
   for(TActorIterator<AActor> It(GetWorld());It;++It)
   {
    if(*It==GetOwner()||It->ActorHasTag(TEXT("ImpactWreck")))continue;
    const bool Match=Stage==2?It->ActorHasTag(TEXT("CityPedestrian")):
      (It->ActorHasTag(TEXT("CityTraffic"))&&(It->ActorHasTag(TEXT("AmbientMotorcycle"))==(Stage==1)));
    if(Match){Target=*It;break;}
   }
   if(Target)
   {
    const FVector Direction=GetOwner()->GetActorForwardVector().GetSafeNormal2D();
    FVector Origin=GetOwner()->GetActorLocation();
    Target->Tags.AddUnique(TEXT("ImpactAuditParked"));
    float Height=Origin.Z;
    if(auto* Human=Cast<ACharacter>(Target))
    {Human->GetCharacterMovement()->StopMovementImmediately();Height=Origin.Z+Human->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()-55;}
    Target->SetActorLocationAndRotation(FVector(Origin.X,Origin.Y,Height)+Direction*750,GetOwner()->GetActorRotation(),false,nullptr,ETeleportType::TeleportPhysics);
    ActivateWreck(Direction*1300);
    if(auto* Root=Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent())){Root->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);Root->SetPhysicsLinearVelocity(Direction*1300);}
    UE_LOG(LogParadiseHeat,Display,TEXT("IMPACT_AUDIT stage=%d source=%s target=%s"),Stage,*GetOwner()->GetName(),*Target->GetName());
    PreviousTarget=Target;
    ++Stage;NextTest=GetWorld()->GetTimeSeconds()+10.f;
   }
  }
 }
 if(auto* Root=Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()))
 {
  PreviousVelocity=Root->GetComponentVelocity();
  // A riderless motorcycle cannot balance on its upright box collider.
  // Continue tipping until its side contacts the ground; gravity handles settling.
  if(GetOwner()->ActorHasTag(TEXT("AmbientMotorcycle"))&&GetOwner()->ActorHasTag(TEXT("ImpactWreck"))
   &&!GetOwner()->ActorHasTag(TEXT("VehicleEntryPending"))&&Root->IsSimulatingPhysics()
   &&FVector::DotProduct(GetOwner()->GetActorUpVector(),FVector::UpVector)>.25f)
  {
   const FVector Axis=GetOwner()->GetActorForwardVector();
   const FVector Angular=Root->GetPhysicsAngularVelocityInRadians();
   Root->SetPhysicsAngularVelocityInRadians(Angular+Axis*(2.5f-FVector::DotProduct(Angular,Axis)));
   Root->WakeAllRigidBodies();
  }
 }
 if(!bFallen)
 {
  if(FleeUntil>0)if(auto* Survivor=Cast<ACharacter>(GetOwner()))
  {
   if(GetWorld()->GetTimeSeconds()<FleeUntil)Survivor->AddMovementInput(FleeDirection,1.f,true);
   else
   {
    Survivor->GetCharacterMovement()->StopMovementImmediately();
    if(auto* Idle=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/ScannedPlayer/Animations/Nathan_MM_Idle.Nathan_MM_Idle")))Survivor->GetMesh()->PlayAnimation(Idle,true);
    FleeUntil=0;
   }
  }
  return;
 }
 if(GetWorld()->GetTimeSeconds()<RecoverAt)return;
 auto* Human=Cast<ACharacter>(GetOwner());if(!Human)return;
 auto* Mesh=Human->GetMesh();FVector Center=Mesh->GetSocketLocation(TEXT("pelvis"));
 if(FVector::DistSquared(Center,RecoveryOrigin)>FMath::Square(2000.f))Center=RecoveryOrigin;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(ImpactRecovery),false,Human);FVector Safe;bool Found=false;
 for(int32 I=0;I<12;++I)
 {
  FVector Start=FVector(Center.X,Center.Y,RecoveryOrigin.Z+500)+FRotator(0,I*60.f,0).Vector()*(I/6+1)*110;
  FHitResult Ground;
  if(!GetWorld()->LineTraceSingleByChannel(Ground,Start,Start-FVector(0,0,1500),ECC_Visibility,Q)||Ground.ImpactNormal.Z<.7f)continue;
  Safe=Ground.ImpactPoint+FVector(0,0,Human->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+5);
  if(!GetWorld()->OverlapBlockingTestByChannel(Safe,FQuat::Identity,ECC_Pawn,Human->GetCapsuleComponent()->GetCollisionShape(),Q)){Found=true;break;}
 }
 if(!Found){RecoverAt=GetWorld()->GetTimeSeconds()+.5f;return;}
 Mesh->SetAllBodiesSimulatePhysics(false);Mesh->SetSimulatePhysics(false);Mesh->AttachToComponent(Human->GetCapsuleComponent(),FAttachmentTransformRules::KeepRelativeTransform);
 Mesh->SetRelativeTransform(SavedMesh);Mesh->SetCollisionProfileName(TEXT("CharacterMesh"));
 Human->SetActorLocation(Safe,false,nullptr,ETeleportType::TeleportPhysics);Human->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 if(Human->ActorHasTag(TEXT("TrafficCrashSurvivor")))
 {
  float Nearest=MAX_flt;FVector Destination=Safe+Human->GetActorRightVector()*800;
  for(TActorIterator<AParadiseHeatCityRoute> It(GetWorld());It;++It)if(It->bPedestrians)
  {
   const FVector Point=It->Path->FindLocationClosestToWorldLocation(Safe,ESplineCoordinateSpace::World);
   const float Distance=FVector::DistSquared2D(Point,Safe);
   if(Distance<Nearest){Nearest=Distance;Destination=Point;}
  }
  FleeDirection=(Destination-Safe).GetSafeNormal2D();
  FleeUntil=GetWorld()->GetTimeSeconds()+FMath::Clamp(FVector::Dist2D(Safe,Destination)/320.f,.5f,8.f);
  Human->GetCharacterMovement()->MaxWalkSpeed=320.f;
  if(auto* Run=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/ScannedPlayer/Animations/Nathan_MF_Unarmed_Jog_Fwd.Nathan_MF_Unarmed_Jog_Fwd")))Mesh->PlayAnimation(Run,true);
 }
 Human->GetCharacterMovement()->SetMovementMode(MOVE_Falling);Human->Tags.Remove(TEXT("ImpactRagdoll"));bFallen=false;
 UE_LOG(LogParadiseHeat,Display,TEXT("IMPACT_PERSON recovered=%s survivor=%d animationMode=%d fleeing=%d"),*Human->GetName(),Human->ActorHasTag(TEXT("TrafficCrashSurvivor")),int32(Mesh->GetAnimationMode()),FleeUntil>GetWorld()->GetTimeSeconds());
}
