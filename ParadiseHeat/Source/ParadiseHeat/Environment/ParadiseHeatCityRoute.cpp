#include "ParadiseHeatCityRoute.h"
#include "Misc/PackageName.h"
#include "CityImpactComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SpotLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Engine/World.h"
#include "Engine/Blueprint.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "ParadiseHeat.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "Sound/SoundBase.h"
#include "TrafficOccupantPose.inl"

static void AddTrafficOccupant(AActor* Vehicle, int32 Variant, bool Bike, FVector Seat, float CabinCeiling=0.f)
{
 const TCHAR* Names[]={TEXT("rp_eric_rigged_001_Mobile_ue4"),TEXT("rp_manuel_rigged_001_Mobile_ue4"),TEXT("rp_sophia_rigged_003_Mobile_ue4")};
 const FString Name=Names[Variant%UE_ARRAY_COUNT(Names)];
 USkeletalMesh* Mesh=LoadObject<USkeletalMesh>(nullptr,*FString::Printf(TEXT("/Game/Scanned3DPeoplePack/RP_Character/%s/%s.%s"),*Name,*Name,*Name));
 if(!Mesh)return;
 auto* Person=NewObject<UPoseableMeshComponent>(Vehicle,TEXT("TrafficOccupant"));
 Vehicle->AddInstanceComponent(Person);Person->SetupAttachment(Vehicle->GetRootComponent());
 Person->SetCollisionEnabled(ECollisionEnabled::NoCollision);Person->RegisterComponent();
 if(!PoseTrafficOccupant(Person,Mesh,Bike)){Person->DestroyComponent();return;}
 if(!Bike)
 {
  Person->SetRelativeScale3D(FVector(.9f));
  // Include scalp/hair clearance; people have different torso lengths.
  const float HeadTop=Person->GetSocketTransform(TEXT("head"),RTS_Component).GetLocation().Z*.9f+20.f;
  Seat.Z=FMath::Min(Seat.Z,CabinCeiling-HeadTop);
 }
 Person->SetRelativeLocation(Seat);Person->SetComponentTickEnabled(false);
 UE_LOG(LogParadiseHeat,Display,TEXT("TRAFFIC_OCCUPANT vehicle=%s bike=%d mesh=%s seat=%s"),*Vehicle->GetName(),Bike,*Name,*Seat.ToString());
}

AParadiseHeatCityRoute::AParadiseHeatCityRoute()
{
 Path=CreateDefaultSubobject<USplineComponent>(TEXT("Lane")); RootComponent=Path;
 PrimaryActorTick.bCanEverTick=true;
 PrimaryActorTick.TickInterval=.05f;
}
void AParadiseHeatCityRoute::BeginPlay()
{
 Super::BeginPlay();
 PedestrianMeshes.RemoveAll([](const USkeletalMesh* Mesh){return !Mesh||Mesh->GetName().Contains(TEXT("nathan"),ESearchCase::IgnoreCase);});
 SetActorTickInterval(bPedestrians?0.f:.05f);
 Agents.SetNum(Population); Distances.SetNum(Population);
 for(int32 i=0;i<Population;++i) Distances[i]=Path->GetSplineLength()*(i+.17f)/FMath::Max(1,Population);
}
AActor* AParadiseHeatCityRoute::SpawnAgent(int32 Slot,const FVector& P,const FRotator& R)
{
 FActorSpawnParameters Params; Params.Owner=this; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
 if(bPedestrians)
 {
  int32 Count=0; for(TActorIterator<ACharacter> It(GetWorld());It;++It) if(It->ActorHasTag(TEXT("CityPedestrian"))) ++Count;
  const bool bScanned=PedestrianMeshes.Num()>0 && PedestrianWalk && PedestrianIdle;
  if(Count>=28 || !bScanned) return nullptr;
  ACharacter* C=GetWorld()->SpawnActor<ACharacter>(ACharacter::StaticClass(),P+FVector(0,0,108),R,Params);
  if(!C) return nullptr;
  C->Tags.Add(TEXT("CityPedestrian"));
  C->AutoPossessPlayer=EAutoReceiveInput::Disabled;
  C->GetCharacterMovement()->bRunPhysicsWithNoController=true;
  C->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
  C->GetCharacterMovement()->Activate(true);
  C->GetCharacterMovement()->AddTickPrerequisiteActor(this);
  C->GetCharacterMovement()->MaxWalkSpeed=TravelSpeed;
  C->GetCharacterMovement()->bOrientRotationToMovement=true;
  C->GetCharacterMovement()->RotationRate=FRotator(0,240,0);
  int32 Index=0, Least=MAX_int32;
  for(int32 Candidate=0;Candidate<PedestrianMeshes.Num();++Candidate)
  {
   int32 Uses=0;
   for(TActorIterator<ACharacter> It(GetWorld());It;++It)
    if(It->ActorHasTag(TEXT("ScannedPedestrian")) && It->GetMesh()->GetSkeletalMeshAsset()==PedestrianMeshes[Candidate]) ++Uses;
   if(Uses<Least) { Least=Uses; Index=Candidate; }
  }
  C->GetCapsuleComponent()->SetCapsuleSize(34.f,96.f);
  C->GetMesh()->SetSkeletalMesh(PedestrianMeshes[Index]);
  C->GetMesh()->SetRelativeLocationAndRotation(FVector(0,0,-96),FRotator(0,-90,0));
  C->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  C->GetMesh()->PlayAnimation(PedestrianWalk,true);
  C->GetMesh()->SetPosition(FMath::FRand()*PedestrianWalk->GetPlayLength(),false);
  C->GetMesh()->SetForcedLOD(0);
  C->Tags.Add(TEXT("ScannedPedestrian"));
  UE_LOG(LogParadiseHeat,Display,TEXT("SCANNED_PERSON_SPAWN id=%s mesh=%s"),*C->GetName(),*GetNameSafe(PedestrianMeshes[Index]));
  return C;
 }
 USkeletalMesh* SelectedTrafficMesh=TrafficSkeletalMesh;
 if(TrafficSkeletalMeshes.Num()>0)
 {
  const int32 Start=(Slot+GetUniqueID())%TrafficSkeletalMeshes.Num();
  for(int32 Offset=0;Offset<TrafficSkeletalMeshes.Num();++Offset)
   if(USkeletalMesh* Candidate=TrafficSkeletalMeshes[(Start+Offset)%TrafficSkeletalMeshes.Num()]) { SelectedTrafficMesh=Candidate; break; }
 }
 // Use ordinary traffic when the optional motorcycle artwork is not installed.
 const bool Bike=(Slot+GetUniqueID())%4==0 && FPackageName::DoesPackageExist(TEXT("/Game/Vehicles/Motorcycles/Triumph/PreparedAssembly/RideBody"));
 if(!Bike && !CarBody && !SelectedTrafficMesh) return nullptr;
 AActor* A=GetWorld()->SpawnActor<AActor>(AActor::StaticClass(),P,R,Params);
 A->Tags.Add(TEXT("CityTraffic"));
 auto* Root=NewObject<UBoxComponent>(A,TEXT("TrafficBounds")); A->AddInstanceComponent(Root); A->SetRootComponent(Root);
 const FBoxSphereBounds VehicleBounds=SelectedTrafficMesh?SelectedTrafficMesh->GetBounds():FBoxSphereBounds(FVector::ZeroVector,FVector(240,115,65),275.f);
 FVector VehicleExtent=Bike?FVector(100,38,51):FVector(FMath::Max(200.f,VehicleBounds.BoxExtent.X),FMath::Max(90.f,VehicleBounds.BoxExtent.Y),FMath::Max(50.f,VehicleBounds.BoxExtent.Z));
 const FString CollisionType=GetNameSafe(SelectedTrafficMesh);
 // The full asset bounds include the roof; using them as one box makes the
 // character stand at roof height above the hood. Split hood and cabin instead.
 const bool Sedan=CollisionType.Contains(TEXT("Sedan"));
 const bool SUV=CollisionType.Contains(TEXT("SUV"));
 if(!Bike && (Sedan||SUV))VehicleExtent.Z=Sedan?40.f:52.f;
 Root->SetBoxExtent(VehicleExtent); Root->SetCollisionProfileName(TEXT("BlockAllDynamic")); Root->SetGenerateOverlapEvents(false); Root->RegisterComponent();
 UCityImpactComponent::Install(A);
 A->SetActorLocationAndRotation(P+FVector(0,0,VehicleExtent.Z+5.f),R);
 if(!Bike && (Sedan||SUV))
 {
  auto* Cabin=NewObject<UBoxComponent>(A,TEXT("WalkableCabin"));A->AddInstanceComponent(Cabin);Cabin->SetupAttachment(Root);
  Cabin->SetBoxExtent(Sedan?FVector(85,73,30):FVector(135,84,35));
  Cabin->SetRelativeLocation(Sedan?FVector(-27,0,70):FVector(-65,0,88));
  Cabin->SetCollisionEnabled(ECollisionEnabled::QueryOnly);Cabin->SetCollisionResponseToAllChannels(ECR_Ignore);
  Cabin->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block);Cabin->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
  Cabin->SetCanEverAffectNavigation(false);Cabin->RegisterComponent();
 }
 auto Part=[A,Root](UStaticMesh* Mesh,const TCHAR* Name,FVector Position)
 {
  auto* C=NewObject<UStaticMeshComponent>(A,Name); A->AddInstanceComponent(C); C->SetupAttachment(Root);
  C->SetStaticMesh(Mesh); C->SetCollisionEnabled(ECollisionEnabled::NoCollision); C->SetRelativeLocation(Position); C->SetCastShadow(false); C->RegisterComponent(); return C;
 };
 if(Bike)
 {
  A->Tags.Add(TEXT("AmbientMotorcycle"));
  auto LoadBike=[](const TCHAR* Name){return LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Vehicles/Motorcycles/Triumph/PreparedAssembly/%s.%s"),Name,Name));};
  auto* Body=Part(LoadBike(TEXT("RideBody")),TEXT("BikeBody"),FVector(0,0,-51));
  Body->SetRelativeRotation(FRotator(0,180,0));Body->SetCastShadow(true);
  for(int32 Wheel=0;Wheel<2;++Wheel)
  {
   auto* W=Part(LoadBike(Wheel==0?TEXT("FrontWheel"):TEXT("RearWheel")),Wheel==0?TEXT("BikeFrontWheel"):TEXT("BikeRearWheel"),FVector(Wheel==0?64.874f:-65.525f,0,Wheel==0?-22.167f:-23.023f));
   W->SetRelativeRotation(FRotator(0,180,0));W->ComponentTags.Add(TEXT("BikeWheel"));W->SetCastShadow(true);
  }
  AddTrafficOccupant(A,Slot+GetUniqueID(),true,FVector::ZeroVector);
 }
 else if(SelectedTrafficMesh)
 {
  auto* Body=NewObject<UPoseableMeshComponent>(A,TEXT("TrafficVehicleBody"));A->AddInstanceComponent(Body);Body->SetupAttachment(Root);
  Body->SetSkinnedAssetAndUpdate(SelectedTrafficMesh);Body->SetRelativeLocation(FVector(0,0,-VehicleExtent.Z));Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);Body->RegisterComponent();
  static const FLinearColor Paints[]={FLinearColor(.04f,.12f,.55f),FLinearColor(.62f,.025f,.02f),FLinearColor(.02f,.28f,.07f),FLinearColor(.72f,.72f,.68f),FLinearColor(.08f,.08f,.09f),FLinearColor(.82f,.82f,.78f)};
  const FLinearColor Paint=Paints[(Slot+GetUniqueID())%UE_ARRAY_COUNT(Paints)];
  for(int32 MaterialIndex=0;MaterialIndex<Body->GetNumMaterials();++MaterialIndex)
  {
   UMaterialInterface* Material=Body->GetMaterial(MaterialIndex); if(!Material) continue;
   const FString Name=Material->GetName();
   if(Name.Contains(TEXT("Body"),ESearchCase::IgnoreCase)||Name.Contains(TEXT("Exterior"),ESearchCase::IgnoreCase))
   {
    auto* Dynamic=Body->CreateDynamicMaterialInstance(MaterialIndex,Material);
    if(Dynamic) Dynamic->SetVectorParameterValue(TEXT("01 - Diffuse"),Paint);
   }
  }
  for(int32 i=0;i<TrafficParts.Num();++i) if(TrafficParts[i] && TrafficPartTransforms.IsValidIndex(i))
  {
   auto* C=Part(TrafficParts[i],*FString::Printf(TEXT("Mz3Part%d"),i),FVector::ZeroVector);
   FTransform T=TrafficPartTransforms[i];T.AddToTranslation(FVector(0,0,-VehicleExtent.Z));C->SetRelativeTransform(T);C->SetCastShadow(true);
  }
  UE_LOG(LogParadiseHeat,Display,TEXT("VEHICLE_VARIETY_SPAWN id=%s mesh=%s parts=%d"),*A->GetName(),*GetNameSafe(SelectedTrafficMesh),TrafficParts.Num());
 }
 else
 {
 Part(CarBody,TEXT("Body"),FVector(0,0,-65));
 if(CarGlass) Part(CarGlass,TEXT("Glass"),FVector(0,0,-65));
 if(CarWheel) for(int32 i=0;i<4;++i)
 {
  auto* W=Part(CarWheel,*FString::Printf(TEXT("Wheel%d"),i),FVector(i<2?140:-140,i%2?82:-82,-25.7));
  W->ComponentTags.Add(TEXT("TrafficWheel"));
  if(i%2==0) W->ComponentTags.Add(TEXT("LeftWheel"));
 }
 }
 if(!Bike)
 {
  const FString Type=GetNameSafe(SelectedTrafficMesh);
  // Seat coordinates relative to the imported vehicle's ground-level origin.
  FVector Seat(12,-37,50);
  if(Type.Contains(TEXT("SUV")))Seat=FVector(20,-40,62);
  if(Type.Contains(TEXT("BoxTruck")))Seat=FVector(140,-43,100);
  Seat.Z-=VehicleExtent.Z;
  const float Roof=VehicleBounds.Origin.Z+VehicleBounds.BoxExtent.Z-VehicleExtent.Z-18.f;
  AddTrafficOccupant(A,Slot+GetUniqueID(),false,Seat,Roof);
 }
 for(int32 Side:{-1,1})
 {
  auto* Light=NewObject<USpotLightComponent>(A); A->AddInstanceComponent(Light); Light->SetupAttachment(Root);
  Light->SetRelativeLocation(FVector(200,Side*60,-15)); Light->SetIntensity(18000); Light->SetAttenuationRadius(2400);
  Light->SetInnerConeAngle(12); Light->SetOuterConeAngle(32); Light->SetCastShadows(false); Light->RegisterComponent();
 }
 return A;
}
void AParadiseHeatCityRoute::Tick(float DeltaSeconds)
{
 Super::Tick(DeltaSeconds); Age+=DeltaSeconds;
 const APawn* Player=UGameplayStatics::GetPlayerPawn(this,0); if(!Player) return;
 const float Length=Path->GetSplineLength(); if(Length<100) return;
 for(int32 i=0;i<Agents.Num();++i)
 {
  if(IsValid(Agents[i]) && (Agents[i]->ActorHasTag(TEXT("VehicleEntryPending"))||Agents[i]->ActorHasTag(TEXT("ImpactAuditParked")))) continue;
  if(IsValid(Agents[i]) && Agents[i]->ActorHasTag(TEXT("ImpactWreck")))
  {
   if(FVector::DistSquared2D(Player->GetActorLocation(),Agents[i]->GetActorLocation())>FMath::Square(24000.f)){Agents[i]->Destroy();Agents[i]=nullptr;}
   continue;
  }
  const float WalkDirection=(i%2==0)?1.f:-1.f;
  const float WalkSpeed=TravelSpeed*(.82f+.06f*(i%6));
  const auto RoutePoint=[&](float Distance)
  {
   const float Wrapped=FMath::Fmod(Distance+Length,Length);
   FVector Point=Path->GetLocationAtDistanceAlongSpline(Wrapped,ESplineCoordinateSpace::World);
   if(bPedestrians) Point+=Path->GetRightVectorAtDistanceAlongSpline(Wrapped,ESplineCoordinateSpace::World)*(WalkDirection*55.f);
   return Point;
  };
  FVector P=RoutePoint(Distances[i]);
  const float Range=FVector::DistSquared2D(Player->GetActorLocation(),P);
  if(IsValid(Agents[i]) && Range>FMath::Square(bPedestrians?16000.f:24000.f)) { Agents[i]->Destroy(); Agents[i]=nullptr; }
  if(!IsValid(Agents[i]) && Range<FMath::Square(bPedestrians?11000.f:20000.f))
   {
   FRotator Heading=Path->GetRotationAtDistanceAlongSpline(Distances[i],ESplineCoordinateSpace::World);
   if(bPedestrians && WalkDirection<0) Heading.Yaw+=180;
   Agents[i]=SpawnAgent(i,P,Heading);
  }
  if(auto* C=Cast<ACharacter>(Agents[i]))
  {
   if(C->ActorHasTag(TEXT("ImpactRagdoll"))){FleeUntil.Add(C,Age+10.f);continue;}
   // Nearby impacts interrupt the route and send the pedestrian away from danger.
   bool bHit=false;
   FVector Threat=Player->GetActorLocation();
   if(Age>HitCooldown.FindRef(C))
   {
    const FVector Delta=C->GetActorLocation()-Threat;
    if(auto* PC=UGameplayStatics::GetPlayerController(this,0))
     if(Cast<ACharacter>(Player)&&PC->WasInputKeyJustPressed(EKeys::F)&&Delta.Size2D()<190.f)
     {
      bHit=true;
      for(TActorIterator<ACharacter> It(GetWorld());It;++It)
       if(*It!=C&&It->ActorHasTag(TEXT("CityPedestrian"))&&FVector::DistSquared2D(It->GetActorLocation(),Threat)<Delta.SizeSquared2D())bHit=false;
     }

    if(bHit)
    {
     if(ACharacter* Human=Cast<ACharacter>(const_cast<APawn*>(Player)))
      if(UAnimSequence* Punch=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/ScannedPlayer/Animations/Nathan_MM_Attack_01.Nathan_MM_Attack_01")))
      {
       auto* Mesh=Human->GetMesh();UClass* AnimClass=Mesh->GetAnimClass();
       Human->SetActorRotation(Delta.Rotation());
       Mesh->PlayAnimation(Punch,false);
       FTimerHandle Handle;TWeakObjectPtr<USkeletalMeshComponent> WeakMesh=Mesh;
       GetWorld()->GetTimerManager().SetTimer(Handle,[WeakMesh,AnimClass](){if(WeakMesh.IsValid()){WeakMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);WeakMesh->SetAnimInstanceClass(AnimClass);}},Punch->GetPlayLength(),false);
      }
     HitCooldown.Add(C,Age+1.f);FleeUntil.Add(C,Age+12.f);HurtUntil.Add(C,Age+.8f);
     if(auto* Voice=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/Pedestrians/PedestrianHurt.PedestrianHurt")))
      UGameplayStatics::PlaySoundAtLocation(this,Voice,C->GetActorLocation(),.8f,FMath::FRandRange(.95f,1.05f));
     C->GetCharacterMovement()->StopMovementImmediately();
     if(auto* HitAnim=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/ScannedPlayer/Animations/Nathan_MM_HitReact_Front_Med_01.Nathan_MM_HitReact_Front_Med_01")))C->GetMesh()->PlayAnimation(HitAnim,false);
     C->LaunchCharacter(Delta.GetSafeNormal2D()*420.f+FVector(0,0,180),true,true);
     UE_LOG(LogParadiseHeat,Display,TEXT("PEDESTRIAN_REACTION %s"),*C->GetName());
    }
   }
   if(Age<FleeUntil.FindRef(C))
   {
    if(Age<HurtUntil.FindRef(C))continue;
    C->GetCharacterMovement()->MaxWalkSpeed=420.f;
    C->AddMovementInput((C->GetActorLocation()-Threat).GetSafeNormal2D(),1.f,true);
    if(auto* Run=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/ScannedPlayer/Animations/Nathan_MF_Unarmed_Jog_Fwd.Nathan_MF_Unarmed_Jog_Fwd")))
     if(auto* Node=C->GetMesh()->GetSingleNodeInstance())
      if(Node->GetCurrentAsset()!=Run) C->GetMesh()->PlayAnimation(Run,true);
    C->GetMesh()->SetPlayRate(1.2f);
    continue;
   }
   if(FleeUntil.Contains(C))
   {
    Distances[i]=Path->GetDistanceAlongSplineAtLocation(C->GetActorLocation(),ESplineCoordinateSpace::World);
    FleeUntil.Remove(C);
   }
   const FVector Target=RoutePoint(Distances[i]+WalkDirection*100);
   const FVector Direction=(Target-C->GetActorLocation()).GetSafeNormal2D();
   C->GetCharacterMovement()->MaxWalkSpeed=WalkSpeed;
   // Brief staggered pauses and yielding to nearby people, including the player.
   bool bWaiting=false;
   for(TActorIterator<ACharacter> It(GetWorld());It && !bWaiting;++It)
   {
    if(*It==C) continue;
    const FVector Delta=It->GetActorLocation()-C->GetActorLocation();
    const float Ahead=FVector::DotProduct(Delta,Direction);
    if(Ahead>0 && Ahead<115 && (Delta-Direction*Ahead).Size2D()<65)
    {
     // Opposing walkers step to their own right instead of both waiting forever.
     const FVector Side=FVector::CrossProduct(FVector::UpVector,Direction);
     C->AddMovementInput(Side,.7f,true);
     // Keep walking around people instead of freezing beside them.
    }
   }
   if(!bWaiting) C->AddMovementInput(Direction,1.f,true);
   // Advance the route only when the actual walking character reaches it, so blocked people never slide/teleport.
   if(C->ActorHasTag(TEXT("ScannedPedestrian")))
   {
    const float Speed=C->GetVelocity().Size2D();
    auto* Desired=Speed>10.f?PedestrianWalk.Get():PedestrianIdle.Get();
    if(auto* Node=C->GetMesh()->GetSingleNodeInstance())
    {
     if(Node->GetCurrentAsset()!=Desired) C->GetMesh()->PlayAnimation(Desired,true);
     C->GetMesh()->SetPlayRate(Speed>10.f?FMath::Clamp(Speed/145.f,.25f,1.5f):1.f);
    }
   }
   if(!bWaiting && FVector::DistSquared2D(C->GetActorLocation(),P)<FMath::Square(160.f)) Distances[i]=FMath::Fmod(Distances[i]+WalkDirection*WalkSpeed*DeltaSeconds+Length,Length);
  }
  else
  {
   bool Blocked=false;
   if(IsValid(Agents[i]))
   {
    const FVector Forward=Path->GetDirectionAtDistanceAlongSpline(Distances[i],ESplineCoordinateSpace::World);
    const FVector Start=P+FVector(0,0,110)+Forward*280;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(CityTraffic),false,Agents[i]); Query.AddIgnoredActor(this);
    FHitResult Hit;
    Blocked=GetWorld()->SweepSingleByChannel(Hit,Start,Start+Forward*300,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(55),Query);
    if(Blocked && Age>=NextSample && Age<65 && FParse::Param(FCommandLine::Get(),TEXT("CityLifeAudit")))
     UE_LOG(LogParadiseHeat,Display,TEXT("TRAFFIC_BLOCK id=%s other=%s component=%s penetrating=%d normal=%s"),*Agents[i]->GetName(),*GetNameSafe(Hit.GetActor()),*GetNameSafe(Hit.GetComponent()),Hit.bStartPenetrating,*Hit.ImpactNormal.ToString());
   }
   if(!Blocked) Distances[i]=FMath::Fmod(Distances[i]+TravelSpeed*DeltaSeconds,Length);
   if(IsValid(Agents[i]))
   {
    const FVector Next=Path->GetLocationAtDistanceAlongSpline(Distances[i],ESplineCoordinateSpace::World);
    const FRotator Heading=Path->GetRotationAtDistanceAlongSpline(Distances[i],ESplineCoordinateSpace::World);
    float VehicleHeight=65.f;
    if(auto* Bounds=Cast<UBoxComponent>(Agents[i]->GetRootComponent())) VehicleHeight=Bounds->GetUnscaledBoxExtent().Z;
    const FVector TrafficVelocity=Blocked?FVector::ZeroVector:Heading.Vector()*TravelSpeed;
    FHitResult Contact;
    Agents[i]->SetActorLocationAndRotation(Next+FVector(0,0,VehicleHeight+5.f),FMath::RInterpTo(Agents[i]->GetActorRotation(),Heading,DeltaSeconds,8),true,&Contact);
    if(Contact.bBlockingHit)
    {
     UCityImpactComponent::Install(Agents[i])->Contact(Contact.GetActor(),Contact.GetComponent(),Contact,TrafficVelocity);
     if(!Agents[i]->ActorHasTag(TEXT("ImpactWreck")))Distances[i]=Path->GetDistanceAlongSplineAtLocation(Agents[i]->GetActorLocation(),ESplineCoordinateSpace::World);
    }
    if(auto* Body=Agents[i]->FindComponentByClass<UPoseableMeshComponent>())
    {
     const float Angle=FMath::RadiansToDegrees(Distances[i]/31.f);
     for(int32 BoneIndex=0;BoneIndex<Body->GetNumBones();++BoneIndex)
     {
      const FName Bone=Body->GetBoneName(BoneIndex);
      if(Bone.ToString().Contains(TEXT("wheel"),ESearchCase::IgnoreCase))
       Body->SetBoneRotationByName(Bone,FRotator(-Angle,0,0),EBoneSpaces::ComponentSpace);
     }
    }
    TArray<UStaticMeshComponent*> Parts; Agents[i]->GetComponents(Parts);
    for(auto* Part:Parts) if(Part->ComponentHasTag(TEXT("BikeWheel"))) Part->SetRelativeRotation(FRotator(FMath::RadiansToDegrees(Distances[i]/28.f),180,0));
    for(auto* Part:Parts) if(Part->ComponentHasTag(TEXT("TrafficWheel"))) Part->SetRelativeRotation(FRotator((Part->ComponentHasTag(TEXT("LeftWheel"))?1.f:-1.f)*FMath::RadiansToDegrees(Distances[i]/39.27f),Part->ComponentHasTag(TEXT("LeftWheel"))?180.f:0.f,0));
   }
  }
 }
 if(Age>=NextSample && Age<65 && FParse::Param(FCommandLine::Get(),TEXT("CityLifeAudit")))
 {
  NextSample+=10;
  for(auto Agent:Agents) if(IsValid(Agent))
  {
   float Foot=0; if(auto* C=Cast<ACharacter>(Agent)) Foot=C->GetMesh()->GetSocketTransform(TEXT("foot_l"),RTS_Component).GetLocation().X;
   UE_LOG(LogParadiseHeat,Display,TEXT("CITY_LIFE_AUDIT id=%s kind=%s t=%.1f x=%.1f y=%.1f z=%.1f speed=%.1f foot=%.1f"),*Agent->GetName(),bPedestrians?TEXT("person"):TEXT("car"),Age,Agent->GetActorLocation().X,Agent->GetActorLocation().Y,Agent->GetActorLocation().Z,Agent->GetVelocity().Size(),Foot);
  }
 }
 if(!bReported && Age>35)
 {
  bReported=true;
  for(auto Agent:Agents) if(IsValid(Agent))
  {
   UE_LOG(LogParadiseHeat,Display,TEXT("City life sample type=%s pos=%s velocity=%.1f"), bPedestrians?TEXT("pedestrian"):TEXT("traffic"),*Agent->GetActorLocation().ToString(),Agent->GetVelocity().Size());
  }
  int32 Active=0; for(auto A:Agents) if(IsValid(A)) ++Active;
  UE_LOG(LogParadiseHeat,Display,TEXT("City life route %s type=%s active=%d slots=%d length=%.0f speed=%.0f"),*GetName(),bPedestrians?TEXT("walking"):TEXT("ambient traffic"),Active,Population,Length,TravelSpeed);
 }
}
void AParadiseHeatCityRoute::EndPlay(const EEndPlayReason::Type Reason)
{
 for(auto A:Agents) if(IsValid(A)) A->Destroy(); Super::EndPlay(Reason);
}

void AParadiseHeatCityRoute::ReleaseTraffic(AActor* Vehicle)
{
 const int32 Slot=Agents.IndexOfByKey(Vehicle);
 if(Slot!=INDEX_NONE)
 {
  Agents[Slot]=nullptr;
  Distances[Slot]=FMath::Fmod(Distances[Slot]+Path->GetSplineLength()*.5f,Path->GetSplineLength());
 }
}

int32 AParadiseHeatCityRoute::ConfigureLegacyTraffic(UBlueprint* Blueprint)
{
#if WITH_EDITOR
 if(!Blueprint || !Blueprint->SimpleConstructionScript) return 0;
 TrafficParts.Reset();TrafficPartTransforms.Reset();
 TFunction<void(USCS_Node*,const FTransform&)> Visit;
 Visit=[&](USCS_Node* Node,const FTransform& Parent)
 {
  if(!Node) return;
  FTransform World=Parent;
  if(auto* Scene=Cast<USceneComponent>(Node->ComponentTemplate)) World=Scene->GetRelativeTransform()*Parent;
  if(auto* Mesh=Cast<UStaticMeshComponent>(Node->ComponentTemplate))
   if(Mesh->GetStaticMesh()) { TrafficParts.Add(Mesh->GetStaticMesh());TrafficPartTransforms.Add(World); }
  for(auto* Child:Node->GetChildNodes()) Visit(Child,World);
 };
 for(auto* Root:Blueprint->SimpleConstructionScript->GetRootNodes()) Visit(Root,FTransform::Identity);
#endif
 return TrafficParts.Num();
}
