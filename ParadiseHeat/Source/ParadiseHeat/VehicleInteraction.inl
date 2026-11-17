#include "Environment/CityImpactComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraActor.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "Environment/TrafficOccupantPose.inl"
#include "CarMirrors.inl"
#include "CarEntryPose.inl"

// Door derivatives retain the original material slots and skeleton. Only interactive
// cars use LOD0 so a closed-door lower LOD cannot appear during the opening sequence.
static UStaticMeshComponent* InstallEntryDoor(AActor* Actor)
{
 TArray<UStaticMeshComponent*> Existing;Actor->GetComponents(Existing);
 for(auto* C:Existing)if(C->GetFName()==TEXT("InteractiveDriverDoor"))return C;
 USkinnedMeshComponent* Body=nullptr;
 TArray<USkinnedMeshComponent*> Bodies;Actor->GetComponents(Bodies);
 for(auto* C:Bodies)if(GetNameSafe(C->GetSkinnedAsset()).Contains(TEXT("Sedan"))||GetNameSafe(C->GetSkinnedAsset()).Contains(TEXT("SUV"))){Body=C;break;}
 if(!Body)return nullptr;
 const bool SUV=GetNameSafe(Body->GetSkinnedAsset()).Contains(TEXT("SUV"));
 const FString Kind=SUV?TEXT("SUV"):TEXT("Sedan");
 auto* Mesh=LoadObject<USkeletalMesh>(nullptr,*FString::Printf(TEXT("/Game/Vehicles/Player/Entry/SK_%s_Entry.SK_%s_Entry"),*Kind,*Kind));
 auto* DoorMesh=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Vehicles/Player/Entry/SM_%s_DriverDoor.SM_%s_DriverDoor"),*Kind,*Kind));
 if(!Mesh||!DoorMesh)return nullptr;
 if(auto* Skeletal=Cast<USkeletalMeshComponent>(Body))Skeletal->SetSkeletalMesh(Mesh);
 else if(auto* Pose=Cast<UPoseableMeshComponent>(Body))Pose->SetSkinnedAssetAndUpdate(Mesh);
 Body->SetForcedLOD(1);
 auto* Door=NewObject<UStaticMeshComponent>(Actor,TEXT("InteractiveDriverDoor"));Actor->AddInstanceComponent(Door);
 Door->SetupAttachment(Body);Door->SetStaticMesh(DoorMesh);Door->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Door->SetRelativeLocation(SUV?FVector(82,-87,105):FVector(88,-86,85));
 for(int32 I=0;I<Body->GetNumMaterials();++I)Door->SetMaterial(I,Body->GetMaterial(I));
 Door->RegisterComponent();
 if(auto* WheelMesh=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Vehicles/Player/Entry/SM_%s_SteeringWheel.SM_%s_SteeringWheel"),*Kind,*Kind)))
 {
  auto* Wheel=NewObject<UStaticMeshComponent>(Actor,TEXT("PlayerSteeringWheel"));Actor->AddInstanceComponent(Wheel);Wheel->SetupAttachment(Body);Wheel->SetStaticMesh(WheelMesh);Wheel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Wheel->SetRelativeLocation(SUV?FVector(33.35934f,-37.43258f,102.89562f):FVector(40.84144f,-38.94663f,81.24230f));
  for(int32 I=0;I<Body->GetNumMaterials();++I)Wheel->SetMaterial(I,Body->GetMaterial(I));Wheel->RegisterComponent();
 }
 return Door;
}

bool AParadiseHeatPlayerController::FindVehicleExit(AActor* Vehicle,FVector& Location) const
{
 if(!Vehicle)return false;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(VehicleExit),false,Vehicle);
 if(GetPawn())Q.AddIgnoredActor(GetPawn());
 if(CarHuman)Q.AddIgnoredActor(CarHuman);
 if(PreparedVehicle)Q.AddIgnoredActor(PreparedVehicle);
 float Width=140.f;
 if(auto* Box=Cast<UBoxComponent>(Vehicle->GetRootComponent()))Width=Box->GetScaledBoxExtent().Y+75.f;
 for(float Side:{-1.f,1.f}) for(float Offset:{0.f,-180.f,180.f})
 {
  const FVector P=Vehicle->GetActorLocation()+Vehicle->GetActorRightVector()*Width*Side+Vehicle->GetActorForwardVector()*Offset;
  FHitResult Hit;
  if(!GetWorld()->LineTraceSingleByChannel(Hit,P+FVector(0,0,250),P-FVector(0,0,450),ECC_Visibility,Q)||Hit.ImpactNormal.Z<.7f)continue;
  const FVector Candidate=Hit.ImpactPoint+FVector(0,0,100);
  if(!GetWorld()->OverlapBlockingTestByChannel(Candidate,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(36,96),Q)){Location=Candidate;return true;}
 }
 return false;
}

void AParadiseHeatPlayerController::BeginVehicleEntry(AActor* Vehicle)
{
 ACharacter* Human=Cast<ACharacter>(GetPawn());
 if(!Human||Human->GetMesh()->IsSimulatingPhysics()||!Vehicle||!FindVehicleExit(Vehicle,EntryLocation))return;
 const bool IsBike=Vehicle->ActorHasTag(TEXT("AmbientMotorcycle"));
 if(IsBike)EntryLocation=Human->GetActorLocation();
 EntryAnimationClass=Human->GetMesh()->GetAnimClass();
 // Validate/spawn the replacement before touching the ambient driver.
 if(!Vehicle->IsA<APawn>())
 {
  FActorSpawnParameters P;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  P.Owner=this;
  if(Vehicle->ActorHasTag(TEXT("AmbientMotorcycle")))
   PreparedVehicle=GetWorld()->SpawnActor<AParadiseHeatMotorcycle>(AParadiseHeatMotorcycle::StaticClass(),Vehicle->GetActorLocation(),FRotator(0,Vehicle->GetActorRotation().Yaw,0),P);
  else
  {
   UPoseableMeshComponent* Body=Vehicle->FindComponentByClass<UPoseableMeshComponent>();
   const FString Mesh=Body?GetNameSafe(Body->GetSkinnedAsset()):TEXT("");
   const FString Kind=Mesh.Contains(TEXT("SUV"))?TEXT("SUV"):Mesh.Contains(TEXT("BoxTruck"))?TEXT("BoxTruck"):TEXT("Sedan");
   UClass* Class=LoadClass<APawn>(nullptr,*FString::Printf(TEXT("/Game/VehicleVarietyVol2/Blueprints/%s/BP_%s_Chaos.BP_%s_Chaos_C"),*Kind,*Kind,*Kind));
   if(!Class)return;
   float Height=65.f;if(auto* Box=Cast<UBoxComponent>(Vehicle->GetRootComponent()))Height=Box->GetScaledBoxExtent().Z;
   PreparedVehicle=GetWorld()->SpawnActor<APawn>(Class,Vehicle->GetActorLocation()-FVector(0,0,Height-5.f),Vehicle->GetActorRotation(),P);
   if(PreparedVehicle)
   {
    auto* Move=PreparedVehicle->FindComponentByClass<UChaosWheeledVehicleMovementComponent>();
    if(!Move){PreparedVehicle->Destroy();PreparedVehicle=nullptr;return;}
    Move->SetHandbrakeInput(true);
    if(auto* Actual=PreparedVehicle->FindComponentByClass<USkeletalMeshComponent>())
     if(Body)for(int32 I=0;I<Body->GetNumMaterials();++I)Actual->SetMaterial(I,Body->GetMaterial(I));
   }
  }
  if(!PreparedVehicle)return;
  PreparedVehicle->SetActorHiddenInGame(true);PreparedVehicle->SetActorEnableCollision(false);
  PreparedVehicle->SetActorTickEnabled(false);
  if(auto* Body=Cast<UPrimitiveComponent>(PreparedVehicle->GetRootComponent()))Body->SetSimulatePhysics(false);
 }
 else PreparedVehicle=Cast<APawn>(Vehicle);
 if(!Vehicle->ActorHasTag(TEXT("AmbientMotorcycle")))InstallEntryDoor(Vehicle);
 Vehicle->Tags.AddUnique(TEXT("VehicleEntryPending"));
 PendingVehicle=Vehicle;EntryStarted=GetWorld()->GetTimeSeconds();bDriverEjected=false;
 {
  const FVector Focus=Vehicle->GetActorLocation()+FVector(0,0,50);
  const FVector CameraPosition=Focus-Vehicle->GetActorRightVector()*390.f-Vehicle->GetActorForwardVector()*170.f+FVector(0,0,150);
  auto* Camera=GetWorld()->SpawnActor<ACameraActor>(CameraPosition,(Focus-CameraPosition).Rotation());
  if(Camera){Camera->GetCameraComponent()->SetFieldOfView(70);EntryCamera=Camera;SetViewTargetWithBlend(Camera,.25f);}
 }
 Human->GetCharacterMovement()->StopMovementImmediately();Human->GetCharacterMovement()->DisableMovement();
 bEntryApproaching=!IsBike;
 EntryApproachWaypoints.Reset();
 if(bEntryApproaching)
 {
  // Route around the vehicle's front/rear corner instead of through its body.
  const FTransform Frame(FRotator(0,Vehicle->GetActorRotation().Yaw,0),Vehicle->GetActorLocation());
  const FVector Start=Frame.InverseTransformPosition(Human->GetActorLocation());
  const FVector End=Frame.InverseTransformPosition(EntryLocation);
  float HalfLength=Vehicle->ActorHasTag(TEXT("AmbientMotorcycle"))?110.f:245.f;
  if(auto* Box=Cast<UBoxComponent>(Vehicle->GetRootComponent()))HalfLength=FMath::Max(HalfLength,Box->GetScaledBoxExtent().X);
  const float Side=End.Y<0?-1.f:1.f;
  const float LaneY=Side*FMath::Max(FMath::Abs(End.Y),165.f);
  auto AddWaypoint=[&](FVector P){P.Z=Start.Z;EntryApproachWaypoints.Add(Frame.TransformPosition(P));};
  if(Start.Y*Side<130.f)
  {
   const float EndX=(Start.X>=0?1.f:-1.f)*(HalfLength+90.f);
   if(FMath::Abs(Start.X)<HalfLength+65.f)AddWaypoint(FVector(EndX,Start.Y,Start.Z));
   AddWaypoint(FVector(FMath::Abs(Start.X)>HalfLength+65.f?Start.X:EndX,LaneY,Start.Z));
  }
  AddWaypoint(FVector(End.X,LaneY,Start.Z));
  EntryApproachWaypoints.Add(EntryLocation);
  UE_LOG(LogParadiseHeat,Display,TEXT("CAR_APPROACH_ROUTE waypoints=%d start=%s"),EntryApproachWaypoints.Num(),*Start.ToString());
  if(auto* Walk=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/ScannedPlayer/Animations/Nathan_MF_Unarmed_Walk_Fwd.Nathan_MF_Unarmed_Walk_Fwd")))Human->GetMesh()->PlayAnimation(Walk,true);
 }
 else
 {
  Human->SetActorRotation(FRotator(0,(Vehicle->GetActorLocation()-EntryLocation).Rotation().Yaw,0));
  bool Occupied=false;TArray<UPoseableMeshComponent*> Riders;Vehicle->GetComponents(Riders);
  for(auto* Rider:Riders)if(Rider->GetFName()==TEXT("TrafficOccupant")&&Rider->IsVisible()&&!Rider->bHiddenInGame)Occupied=true;
  if(Occupied)
  {if(auto* Shove=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/ScannedPlayer/Animations/Nathan_MM_Attack_01.Nathan_MM_Attack_01")))Human->GetMesh()->PlayAnimation(Shove,false);}
  else {bDriverEjected=true;FinishVehicleEntry();}
 }
 UE_LOG(LogParadiseHeat,Display,TEXT("VEHICLE_ENTRY_BEGIN %s"),*Vehicle->GetName());
}

void AParadiseHeatPlayerController::EjectTrafficDriver(AActor* Vehicle)
{
 UPoseableMeshComponent* Driver=nullptr;
 TArray<UPoseableMeshComponent*> Components;Vehicle->GetComponents(Components);
 for(auto* C:Components)if(C->GetFName()==TEXT("TrafficOccupant")){Driver=C;break;}
 if(!Driver||!Driver->IsVisible()||Driver->bHiddenInGame||Vehicle->ActorHasTag(TEXT("DriverEjected")))return;
 USkeletalMesh* Mesh=Cast<USkeletalMesh>(Driver->GetSkinnedAsset());if(!Mesh)return;
 // Use a validated exit beside the vehicle, offset from the player.
 FVector Exit=EntryLocation;FVector Other;
 if(FindVehicleExit(Vehicle,Other))Exit=Other;
 FActorSpawnParameters P;P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
 ACharacter* NPC=GetWorld()->SpawnActor<ACharacter>(ACharacter::StaticClass(),Exit+Vehicle->GetActorForwardVector()*130.f,Vehicle->GetActorRotation(),P);
 if(!NPC)return;
 NPC->Tags.Add(TEXT("EjectedDriver"));NPC->GetCapsuleComponent()->SetCapsuleSize(34,96);
 NPC->GetMesh()->SetSkeletalMesh(Mesh);NPC->GetMesh()->SetRelativeLocationAndRotation(FVector(0,0,-96),FRotator(0,-90,0));
 NPC->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 auto* Move=NPC->GetCharacterMovement();Move->bRunPhysicsWithNoController=true;Move->bOrientRotationToMovement=true;Move->MaxWalkSpeed=420;Move->SetMovementMode(MOVE_Falling);
 if(Vehicle->ActorHasTag(TEXT("AmbientMotorcycle")))
 {
  // Throw from the occupied seat, away from the approaching player, then recover.
  NPC->SetActorLocation(Driver->GetComponentLocation()+FVector(0,0,96),false,nullptr,ETeleportType::TeleportPhysics);
  NPC->Tags.AddUnique(TEXT("TrafficCrashSurvivor"));
  if(auto* Idle=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/ScannedPlayer/Animations/Nathan_MM_Idle.Nathan_MM_Idle")))NPC->GetMesh()->PlayAnimation(Idle,true);
  NPC->GetMesh()->TickAnimation(0.f,false);NPC->GetMesh()->RefreshBoneTransforms();
  const FVector Away=(Vehicle->GetActorLocation()-EntryLocation).GetSafeNormal2D();
  if(UCityImpactComponent::KnockDown(NPC,Away*380.f+FVector(0,0,100)))
  {Vehicle->Tags.AddUnique(TEXT("DriverEjected"));Driver->SetVisibility(false,true);NPC->SetLifeSpan(35.f);UE_LOG(LogParadiseHeat,Display,TEXT("BIKE_TAKEOVER_THROW %s"),*NPC->GetName());return;}
 }
 // Use a full-body stumble; additive hit clips require a base-pose animation graph.
 if(auto* Hit=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/ScannedPlayer/Animations/Nathan_MM_Land.Nathan_MM_Land")))NPC->GetMesh()->PlayAnimation(Hit,false);
 NPC->LaunchCharacter((Exit-Vehicle->GetActorLocation()).GetSafeNormal2D()*180+FVector(0,0,120),true,true);
 if(auto* Voice=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/Pedestrians/PedestrianHurt.PedestrianHurt")))UGameplayStatics::PlaySoundAtLocation(this,Voice,NPC->GetActorLocation(),.9f);
 EscapingDrivers.Add(NPC);DriverHitTimes.Add(GetWorld()->GetTimeSeconds());NPC->SetLifeSpan(35.f);
 Vehicle->Tags.AddUnique(TEXT("DriverEjected"));Driver->SetVisibility(false,true);
 UE_LOG(LogParadiseHeat,Display,TEXT("DRIVER_EJECTED mesh=%s npc=%s"),*Mesh->GetName(),*NPC->GetName());
}

void AParadiseHeatPlayerController::FinishVehicleEntry()
{
 ACharacter* Human=Cast<ACharacter>(GetPawn());
 if(!Human||!IsValid(PendingVehicle)||!IsValid(PreparedVehicle))
 {
  if(Human)Human->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
  if(IsValid(PendingVehicle))PendingVehicle->Tags.Remove(TEXT("VehicleEntryPending"));
  if(IsValid(PreparedVehicle)&&PreparedVehicle!=PendingVehicle)PreparedVehicle->Destroy();
  PendingVehicle=nullptr;PreparedVehicle=nullptr;return;
 }
 APawn* Target=PreparedVehicle;
 if(EntryCamera){EntryCamera->Destroy();EntryCamera=nullptr;SetViewTarget(Human);}
 if(EntryVisual){EntryVisual->DestroyComponent();EntryVisual=nullptr;}
 Human->GetMesh()->SetVisibility(true,true);
 if(PendingVehicle!=Target)
 {
  if(auto* Route=Cast<AParadiseHeatCityRoute>(PendingVehicle->GetOwner()))Route->ReleaseTraffic(PendingVehicle);
  PendingVehicle->Destroy();
 }
 PendingVehicle=nullptr;PreparedVehicle=nullptr;
 Target->Tags.Remove(TEXT("VehicleEntryPending"));Target->SetActorHiddenInGame(false);Target->SetActorEnableCollision(true);
 Target->SetActorTickEnabled(true);
 if(auto* Body=Cast<UPrimitiveComponent>(Target->GetRootComponent())){Body->SetSimulatePhysics(true);Body->WakeAllRigidBodies();}
 Human->GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
 if(EntryAnimationClass)Human->GetMesh()->SetAnimInstanceClass(EntryAnimationClass);
 Human->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
 if(auto* Bike=Cast<AParadiseHeatMotorcycle>(Target)){Bike->Mount(this,Human);return;}
 UCityImpactComponent::Install(Target);
 CarHuman=Human;DrivenCar=Target;Target->Tags.AddUnique(TEXT("PlayerUsableCar"));
 Human->GetCharacterMovement()->StopMovementImmediately();Human->GetCharacterMovement()->DisableMovement();
 Human->SetActorHiddenInGame(true);Human->SetActorEnableCollision(false);
 auto* Body=Target->FindComponentByClass<USkeletalMeshComponent>();
 if(auto* Door=InstallEntryDoor(Target))
 {
  Door->SetRelativeRotation(FRotator::ZeroRotator);
  UE_LOG(LogParadiseHeat,Display,TEXT("DRIVING_DOOR_CLOSED yaw=%.1f"),Door->GetRelativeRotation().Yaw);
 }
 if(Body)
 {
  if(GetNameSafe(Body->GetSkeletalMeshAsset()).Contains(TEXT("BoxTruck")))
   if(auto* Controls=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Vehicles/Player/Entry/SK_BoxTruck_Entry.SK_BoxTruck_Entry"))){Body->SetSkeletalMesh(Controls);Body->SetForcedLOD(1);}
  CarDriverVisual=NewObject<UPoseableMeshComponent>(Target,TEXT("PlayerCarDriver"));Target->AddInstanceComponent(CarDriverVisual);CarDriverVisual->SetupAttachment(Body);CarDriverVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);CarDriverVisual->RegisterComponent();
  PoseTrafficOccupant(CarDriverVisual,Human->GetMesh()->GetSkeletalMeshAsset(),false);
  FVector Seat(-10,-37,50);const FString Name=GetNameSafe(Body->GetSkeletalMeshAsset());
  if(Name.Contains(TEXT("SUV")))Seat=FVector(0,-40,62);
  if(Name.Contains(TEXT("BoxTruck")))Seat=FVector(120,-43,100);
  CarDriverVisual->SetRelativeScale3D(FVector(.9f));
  const auto Bounds=Body->GetSkeletalMeshAsset()->GetBounds();
  Seat.Z=FMath::Min(Seat.Z,float(Bounds.Origin.Z+Bounds.BoxExtent.Z-38-CarDriverVisual->GetSocketTransform(TEXT("head"),RTS_Component).GetLocation().Z*.9));
  if(Body->DoesSocketExist(TEXT("SteeringWheel")))
   CarWheelTarget=(Body->GetSocketTransform(TEXT("SteeringWheel"),RTS_Component).GetLocation()-Seat)/.9f;
  else CarWheelTarget=FVector(44,0,50);
  const FString WheelKind=Name.Contains(TEXT("SUV"))?TEXT("SUV"):Name.Contains(TEXT("BoxTruck"))?TEXT("BoxTruck"):TEXT("Sedan");
  CarWheelAxis=WheelKind==TEXT("SUV")?FVector(.944f,0,-.329f):WheelKind==TEXT("BoxTruck")?FVector(.658f,0,-.753f):FVector(.889f,0,-.458f);
  if(auto* WheelMesh=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Vehicles/Player/Entry/SM_%s_SteeringWheel.SM_%s_SteeringWheel"),*WheelKind,*WheelKind)))
  {
   TArray<UStaticMeshComponent*> ExistingWheels;Target->GetComponents(ExistingWheels);
   for(auto* C:ExistingWheels)if(C->GetFName()==TEXT("PlayerSteeringWheel"))CarSteeringWheel=C;
   if(!CarSteeringWheel){CarSteeringWheel=NewObject<UStaticMeshComponent>(Target,TEXT("PlayerSteeringWheel"));Target->AddInstanceComponent(CarSteeringWheel);CarSteeringWheel->SetupAttachment(Body);}
   CarSteeringWheel->SetStaticMesh(WheelMesh);CarSteeringWheel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   const FVector Pivot=WheelKind==TEXT("SUV")?FVector(33.35934f,-37.43258f,102.89562f):WheelKind==TEXT("BoxTruck")?FVector(133.14851f,-73.15317f,172.03037f):FVector(40.84144f,-38.94663f,81.24230f);
   CarSteeringWheel->SetRelativeLocation(Pivot);CarWheelTarget=(Pivot-Seat)/.9f;
   for(int32 I=0;I<Body->GetNumMaterials();++I)CarSteeringWheel->SetMaterial(I,Body->GetMaterial(I));
   CarSteeringWheel->SetRelativeRotation(FRotator::ZeroRotator);CarSteeringWheel->RegisterComponent();
  }
  PoseTrafficOccupant(CarDriverVisual,Human->GetMesh()->GetSkeletalMeshAsset(),false,CarWheelTarget,0,CarWheelAxis);
  CarDriverVisual->SetRelativeScale3D(FVector(.9f));
  CarDriverVisual->SetRelativeLocation(Seat);
  if(auto* FullBodyMesh=Human->GetMesh()->GetSkeletalMeshAsset())
  {
   CockpitBody=NewObject<UPoseableMeshComponent>(Target,TEXT("PlayerCockpitBody"));Target->AddInstanceComponent(CockpitBody);CockpitBody->SetupAttachment(Body);
   CockpitBody->SetSkinnedAssetAndUpdate(FullBodyMesh);CockpitBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);CockpitBody->SetOnlyOwnerSee(true);CockpitBody->SetCastShadow(false);CockpitBody->RegisterComponent();
   CockpitBody->SetRelativeTransform(CarDriverVisual->GetRelativeTransform());CockpitBody->BoneSpaceTransforms=CarDriverVisual->BoneSpaceTransforms;CockpitBody->MarkRefreshTransformDirty();CockpitBody->RefreshBoneTransforms();
   
   CarDriverVisual->SetOwnerNoSee(true);
  }
  LastDriverSteering=0;
  UE_LOG(LogParadiseHeat,Display,TEXT("CAR_SEAT_POSE wheel=%s leftHand=%s leftFoot=%s"),*CarWheelTarget.ToString(),*CarDriverVisual->GetSocketTransform(TEXT("hand_l"),RTS_Component).GetLocation().ToString(),*CarDriverVisual->GetSocketTransform(TEXT("foot_l"),RTS_Component).GetLocation().ToString());
 }
 TArray<UCameraComponent*> Cameras;Target->GetComponents(Cameras);for(auto* C:Cameras)C->Deactivate();
 auto* Camera=NewObject<UCameraComponent>(Target,TEXT("PlayerDrivingCamera"));Target->AddInstanceComponent(Camera);Camera->SetupAttachment(Target->GetRootComponent());
 FVector Eye(30,-37,115);
 if(CarDriverVisual)
 {
  Eye=Target->GetActorTransform().InverseTransformPosition(CarDriverVisual->GetSocketTransform(TEXT("head"),RTS_World).GetLocation())+FVector(16,0,12);
 }
 Camera->SetRelativeLocation(Eye);Camera->bUsePawnControlRotation=true;Camera->SetFieldOfView(126);Camera->RegisterComponent();Camera->Activate();
 Possess(Target);Target->DisableInput(this);SetControlRotation(FRotator(-24,Target->GetActorRotation().Yaw+8,0));
 if(PlayerCameraManager){PlayerCameraManager->ViewPitchMin=-24;PlayerCameraManager->ViewPitchMax=20;}
 SavedGlassMeshes.Reset();SavedGlassMaterials.Reset();SavedGlassSlots.Reset();
 TArray<UMeshComponent*> CabinMeshes;Target->GetComponents(CabinMeshes);
 if(auto* Clear=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Vehicles/Player/M_CockpitClearGlass.M_CockpitClearGlass")))
  for(auto* M:CabinMeshes)for(int32 I=0;I<M->GetNumMaterials();++I)if(auto* Mat=M->GetMaterial(I))if(Mat->GetName().Contains(TEXT("Glass"),ESearchCase::IgnoreCase))
  {SavedGlassMeshes.Add(M);SavedGlassSlots.Add(I);SavedGlassMaterials.Add(Mat);M->SetMaterial(I,Clear);}
 CreateCarMirrors();
 if(auto* Move=Target->FindComponentByClass<UChaosWheeledVehicleMovementComponent>()){Move->bReverseAsBrake=false;Move->SetUseAutomaticGears(true);Move->SetTargetGear(1,true);Move->SetHandbrakeInput(false);}
 if(auto* Sound=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/Vehicles/PrototypeCarEngine.PrototypeCarEngine")))
 {
  CarEngineAudio=NewObject<UAudioComponent>(Target,TEXT("PlayerCarEngine"));Target->AddInstanceComponent(CarEngineAudio);CarEngineAudio->SetupAttachment(Target->GetRootComponent());
  CarEngineAudio->bAutoActivate=false;CarEngineAudio->bIsUISound=false;CarEngineAudio->SetSound(Sound);CarEngineAudio->RegisterComponent();
  CarEnginePitch=.8f;CarEngineAudio->SetPitchMultiplier(CarEnginePitch);CarEngineAudio->SetVolumeMultiplier(.286f);CarEngineAudio->Play();
  UE_LOG(LogParadiseHeat,Display,TEXT("CAR_ENGINE_STARTED playing=%d"),CarEngineAudio->IsPlaying());
 }
 UE_LOG(LogParadiseHeat,Display,TEXT("CAR_POSSESSED %s"),*Target->GetName());
}

void AParadiseHeatPlayerController::ExitCar()
{
 if(!DrivenCar||!CarHuman)return;
 auto* Move=DrivenCar->FindComponentByClass<UChaosWheeledVehicleMovementComponent>();
 if(DrivenCar->GetVelocity().Size2D()>120){if(Move){Move->SetThrottleInput(0);Move->SetBrakeInput(1);}return;}
 FVector Exit;if(!FindVehicleExit(DrivenCar,Exit))return;
 if(Move){Move->SetThrottleInput(0);Move->SetBrakeInput(1);Move->SetHandbrakeInput(true);}
 for(int32 I=0;I<SavedGlassMeshes.Num();++I)
  if(IsValid(SavedGlassMeshes[I])&&SavedGlassMaterials[I])SavedGlassMeshes[I]->SetMaterial(SavedGlassSlots[I],SavedGlassMaterials[I]);
 UE_LOG(LogParadiseHeat,Display,TEXT("CAR_GLASS_RESTORED slots=%d"),SavedGlassMeshes.Num());
 SavedGlassMeshes.Reset();SavedGlassMaterials.Reset();SavedGlassSlots.Reset();
 if(CarDriverVisual){CarDriverVisual->DestroyComponent();CarDriverVisual=nullptr;}
 RemoveCarMirrors();
 if(CockpitBody){CockpitBody->DestroyComponent();CockpitBody=nullptr;}
 if(CarEngineAudio){CarEngineAudio->Stop();CarEngineAudio->DestroyComponent();CarEngineAudio=nullptr;}
 CarSteeringWheel=nullptr; // Keep the physical wheel visible in the parked car.
 TArray<USpringArmComponent*> Booms;DrivenCar->GetComponents(Booms);for(auto* B:Booms)if(B->GetName().StartsWith(TEXT("PlayerDrivingBoom")))B->DestroyComponent();
 TArray<UCameraComponent*> Cameras;DrivenCar->GetComponents(Cameras);for(auto* C:Cameras)if(C->GetName().StartsWith(TEXT("PlayerDrivingCamera")))C->DestroyComponent();
 ACharacter* Human=CarHuman;Human->SetActorLocation(Exit);Human->SetActorRotation(FRotator(0,DrivenCar->GetActorRotation().Yaw,0));
 Human->SetActorHiddenInGame(false);Human->SetActorEnableCollision(true);Human->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
 DrivenCar=nullptr;CarHuman=nullptr;Possess(Human);
 if(PlayerCameraManager){PlayerCameraManager->ViewPitchMin=-65;PlayerCameraManager->ViewPitchMax=8;}
 UE_LOG(LogParadiseHeat,Display,TEXT("CAR_EXIT safe=%s"),*Exit.ToString());
}

void AParadiseHeatPlayerController::TickVehicleInteraction(float Dt)
{
 if(bCursorReleased)return;
 const float Now=GetWorld()->GetTimeSeconds();
 if(FParse::Param(FCommandLine::Get(),TEXT("VehicleEntryAudit")))TickVehicleEntryAudit();
 for(int32 I=EscapingDrivers.Num()-1;I>=0;--I)
 {
  auto* NPC=EscapingDrivers[I].Get();if(!NPC){EscapingDrivers.RemoveAt(I);DriverHitTimes.RemoveAt(I);continue;}
  if(Now-DriverHitTimes[I]<.8f)continue;
  if(!NPC->ActorHasTag(TEXT("DriverRunning")))
  {
   NPC->Tags.Add(TEXT("DriverRunning"));
   if(auto* Run=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/ScannedPlayer/Animations/Nathan_MF_Unarmed_Jog_Fwd.Nathan_MF_Unarmed_Jog_Fwd")))NPC->GetMesh()->PlayAnimation(Run,true);
   NPC->GetMesh()->SetPlayRate(1.2f);
  }
  if(GetPawn())NPC->AddMovementInput((NPC->GetActorLocation()-GetPawn()->GetActorLocation()).GetSafeNormal2D(),1,true);
 }
 if(PendingVehicle)
 {
  auto* ApproachingHuman=Cast<ACharacter>(GetPawn());
  if(bEntryApproaching&&ApproachingHuman)
  {
   while(EntryApproachWaypoints.Num()>1&&FVector::Dist2D(EntryApproachWaypoints[0],ApproachingHuman->GetActorLocation())<10.f)EntryApproachWaypoints.RemoveAt(0);
   const FVector Goal=EntryApproachWaypoints.IsEmpty()?EntryLocation:EntryApproachWaypoints[0];
   const FVector Delta=Goal-ApproachingHuman->GetActorLocation();
   if(Delta.Size2D()>8.f)
   {
    if(VehicleAuditStage==1&&Now-EntryStarted>.5f&&!PendingVehicle->ActorHasTag(TEXT("ApproachAudit")))
    {PendingVehicle->Tags.Add(TEXT("ApproachAudit"));FScreenshotRequest::RequestScreenshot(TEXT("/private/tmp/entry-walking.png"),false,false);}
    FHitResult Hit;
    const FVector Step=Delta.GetSafeNormal()*FMath::Min(Delta.Size(),135.f*Dt);
    ApproachingHuman->SetActorRotation(FMath::RInterpTo(ApproachingHuman->GetActorRotation(),FRotator(0,Delta.Rotation().Yaw,0),Dt,8));
    ApproachingHuman->SetActorLocation(ApproachingHuman->GetActorLocation()+Step,true,&Hit);
    if(Now-EntryStarted<12.f)return;
    // A blocked approach must not teleport through the car or surrounding objects.
    ApproachingHuman->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    ApproachingHuman->GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    if(EntryAnimationClass)ApproachingHuman->GetMesh()->SetAnimInstanceClass(EntryAnimationClass);
    PendingVehicle->Tags.Remove(TEXT("VehicleEntryPending"));
    if(PreparedVehicle&&PreparedVehicle!=PendingVehicle)PreparedVehicle->Destroy();
    if(EntryCamera){EntryCamera->Destroy();EntryCamera=nullptr;SetViewTarget(ApproachingHuman);}
    PendingVehicle=nullptr;PreparedVehicle=nullptr;bEntryApproaching=false;
    UE_LOG(LogParadiseHeat,Warning,TEXT("VEHICLE_ENTRY_APPROACH_BLOCKED"));return;
   }
   ApproachingHuman->SetActorRotation(FRotator(0,(PendingVehicle->GetActorLocation()-EntryLocation).Rotation().Yaw,0));
   if(auto* Idle=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/ScannedPlayer/Animations/Nathan_MM_Idle.Nathan_MM_Idle")))ApproachingHuman->GetMesh()->PlayAnimation(Idle,true);
   if(PendingVehicle->ActorHasTag(TEXT("AmbientMotorcycle")))
    if(auto* Shove=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/ScannedPlayer/Animations/Nathan_MM_Attack_01.Nathan_MM_Attack_01")))ApproachingHuman->GetMesh()->PlayAnimation(Shove,false);
   bEntryApproaching=false;EntryStarted=Now;
   UE_LOG(LogParadiseHeat,Display,TEXT("VEHICLE_ENTRY_AT_DOOR"));return;
  }
  const float Age=Now-EntryStarted;
  if(VehicleAuditStage==1&&Age>1.05f&&!PendingVehicle->ActorHasTag(TEXT("DoorAuditCaptured")))
  {PendingVehicle->Tags.Add(TEXT("DoorAuditCaptured"));FScreenshotRequest::RequestScreenshot(TEXT("/private/tmp/door-open-audit.png"),false,false);}
  if(VehicleAuditStage==1&&Age>3.5f&&!PendingVehicle->ActorHasTag(TEXT("SeatAuditCaptured")))
  {PendingVehicle->Tags.Add(TEXT("SeatAuditCaptured"));FScreenshotRequest::RequestScreenshot(TEXT("/private/tmp/door-seat-audit.png"),false,false);}
  if(VehicleAuditStage==1&&Age>2.5f&&!PendingVehicle->ActorHasTag(TEXT("FirstFootAudit")))
  {PendingVehicle->Tags.Add(TEXT("FirstFootAudit"));FScreenshotRequest::RequestScreenshot(TEXT("/private/tmp/entry-first-foot.png"),false,false);}
  if(VehicleAuditStage==1&&Age>4.15f&&!PendingVehicle->ActorHasTag(TEXT("SecondFootAudit")))
  {PendingVehicle->Tags.Add(TEXT("SecondFootAudit"));FScreenshotRequest::RequestScreenshot(TEXT("/private/tmp/entry-second-foot.png"),false,false);}
  const bool Bike=PendingVehicle->ActorHasTag(TEXT("AmbientMotorcycle"));
  if(Bike)
  {
   if(!bDriverEjected&&Age>.45f){EjectTrafficDriver(PendingVehicle);bDriverEjected=true;}
   if(Age>1.3f)FinishVehicleEntry();
   return;
  }
  if(auto* Door=InstallEntryDoor(PendingVehicle))
  {
   const float Open=Age<4.6f?FMath::SmoothStep(.5f,1.2f,Age):1.f-FMath::SmoothStep(4.6f,5.4f,Age);
   Door->SetRelativeRotation(FRotator(0,Open*68.f,0));
  }
  if(!bDriverEjected&&Age>1.25f){EjectTrafficDriver(PendingVehicle);bDriverEjected=true;}
  auto* Human=Cast<ACharacter>(GetPawn());
  if(Human&&Age>.05f&&!EntryVisual)
  {
   USkinnedMeshComponent* Body=nullptr;TArray<USkinnedMeshComponent*> Bodies;PendingVehicle->GetComponents(Bodies);
   for(auto* C:Bodies)if(C->GetFName()!=TEXT("TrafficOccupant")){Body=C;break;}
   if(Body)
   {
    EntryVisual=NewObject<UPoseableMeshComponent>(PendingVehicle,TEXT("EntryPlayerVisual"));PendingVehicle->AddInstanceComponent(EntryVisual);
    EntryVisual->SetupAttachment(Body);EntryVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);EntryVisual->RegisterComponent();
    FVector Seat(-10,-37,50);const FString Name=GetNameSafe(Body->GetSkinnedAsset());
    if(Name.Contains(TEXT("SUV")))Seat=FVector(0,-40,62);
    if(Name.Contains(TEXT("BoxTruck")))Seat=FVector(120,-43,100);
    const FVector Wheel=Body->DoesSocketExist(TEXT("SteeringWheel"))?(Body->GetSocketTransform(TEXT("SteeringWheel"),RTS_Component).GetLocation()-Seat)/.9f:FVector(44,0,50);
    PoseTrafficOccupant(EntryVisual,Human->GetMesh()->GetSkeletalMeshAsset(),false,Wheel);
    EntryStandingPose.Reset();EntrySeatedPose.Reset();
    const FTransform SeatTransform(FQuat::Identity,Seat,FVector(.9f));
    const auto& Ref=Human->GetMesh()->GetSkeletalMeshAsset()->GetRefSkeleton();
    for(int32 I=0;I<Ref.GetNum();++I)
    {
     const FName Bone=Ref.GetBoneName(I);
     EntryStandingPose.Add(Human->GetMesh()->GetSocketTransform(Bone,RTS_World).GetRelativeTransform(Body->GetComponentTransform()));
     EntrySeatedPose.Add(EntryVisual->GetSocketTransform(Bone,RTS_Component)*SeatTransform);
    }
    Human->GetMesh()->SetVisibility(false,true);
   }
  }
  if(EntryVisual&&Human)
  {
   const auto& Ref=Human->GetMesh()->GetSkeletalMeshAsset()->GetRefSkeleton();
   FVector Handle(20,-95,85);
   if(auto* Door=InstallEntryDoor(PendingVehicle))Handle=EntryVisual->GetComponentTransform().InverseTransformPosition(Door->GetComponentTransform().TransformPosition(FVector(-65,-3,0)));
   // Show only the handle reach and door opening, then cut to the seated view.
   AnimateCarEntry(EntryVisual,Ref,EntryStandingPose,EntrySeatedPose,FMath::Min(Age,1.2f),Handle);
  }
  if(Age>1.5f)FinishVehicleEntry();
  return;
 }
 if(!DrivenCar)return;
 UpdateCarMirrors();
 auto* Move=DrivenCar->FindComponentByClass<UChaosWheeledVehicleMovementComponent>();if(!Move)return;
 const bool CabinTest=FParse::Param(FCommandLine::Get(),TEXT("FullCabinAudit"))&&VehicleAuditStage!=8;
 const bool Forward=!CabinTest&&((VehicleAuditStage==2&&!FParse::Param(FCommandLine::Get(),TEXT("SteeringAudit")))||IsInputKeyDown(EKeys::W)||IsInputKeyDown(EKeys::Up));
 const bool Back=!CabinTest&&(IsInputKeyDown(EKeys::S)||IsInputKeyDown(EKeys::Down));
 const float Speed=Move->GetForwardSpeed();
 float Brake=IsInputKeyDown(EKeys::SpaceBar)?1.f:0.f,Throttle=0;
 if(VehicleAuditStage==3||CabinTest)Brake=1;
 if(Forward){if(Speed<-70)Brake=1;else{if(Move->GetCurrentGear()<1)Move->SetTargetGear(1,true);Throttle=1;}}
 else if(Back){if(Speed>70)Brake=1;else{Move->SetTargetGear(-1,true);Throttle=.45f;}}
 Move->SetThrottleInput(Throttle);Move->SetBrakeInput(Brake);Move->SetHandbrakeInput(CabinTest||IsInputKeyDown(EKeys::SpaceBar));
 float SteeringInput=float(IsInputKeyDown(EKeys::D)||IsInputKeyDown(EKeys::Right))-float(IsInputKeyDown(EKeys::A)||IsInputKeyDown(EKeys::Left));
 if(VehicleAuditStage==2&&FParse::Param(FCommandLine::Get(),TEXT("SteeringAudit")))SteeringInput=Now-VehicleAuditTime<2?-1.f:1.f;
 if(VehicleAuditStage==2&&FParse::Param(FCommandLine::Get(),TEXT("SteeringAudit"))&&Now-VehicleAuditTime>1.5f&&!DrivenCar->ActorHasTag(TEXT("LeftSteerCaptured")))
 {
  DrivenCar->Tags.Add(TEXT("LeftSteerCaptured"));FScreenshotRequest::RequestScreenshot(TEXT("/private/tmp/car-steer-left.png"),false,false);
  UE_LOG(LogParadiseHeat,Display,TEXT("STEERING_CHECK left degrees=%.1f wheel=%d audio=%d pitch=%.2f"),LastDriverSteering,CarSteeringWheel!=nullptr,CarEngineAudio&&CarEngineAudio->IsPlaying(),CarEnginePitch);
 }
 Move->SetSteeringInput(SteeringInput);
 if(CarEngineAudio)
 {
  const float RPM=FMath::Max(850.f,Move->GetEngineRotationSpeed());
  CarEnginePitch=FMath::FInterpTo(CarEnginePitch,FMath::Clamp(.65f+RPM/2600.f+Throttle*.18f,.8f,2.7f),Dt,4.f);
  CarEngineAudio->SetPitchMultiplier(CarEnginePitch);CarEngineAudio->SetVolumeMultiplier(1.3f*(.2f+Throttle*.25f+FMath::Min(FMath::Abs(Speed)/12000.f,.12f)));
 }
 if(CarDriverVisual&&CarHuman)
 {
  const float Steer=FMath::FInterpTo(LastDriverSteering,-SteeringInput*75.f,Dt,6.f);
  if(CarSteeringWheel)CarSteeringWheel->SetRelativeRotation(FQuat(CarWheelAxis.GetSafeNormal(),FMath::DegreesToRadians(Steer)));
  if(FMath::Abs(Steer-LastDriverSteering)>.25f)
  {
   const FTransform Seat=CarDriverVisual->GetRelativeTransform();
   PoseTrafficOccupant(CarDriverVisual,CarHuman->GetMesh()->GetSkeletalMeshAsset(),false,CarWheelTarget,Steer,CarWheelAxis);
   CarDriverVisual->SetRelativeTransform(Seat);
   if(CockpitBody){CockpitBody->BoneSpaceTransforms=CarDriverVisual->BoneSpaceTransforms;CockpitBody->MarkRefreshTransformDirty();CockpitBody->RefreshBoneTransforms();}
   LastDriverSteering=Steer;
  }
 }
 float X=0,Y=0;GetInputMouseDelta(X,Y);if(CabinTest){X=0;Y=0;}FRotator View=GetControlRotation();
 if(FMath::Abs(X)+FMath::Abs(Y)>.01f)
 {
  LastCarLookTime=Now;
  const auto* Settings=UParadiseHeatUserSettings::Get();
  const float Sensitivity=Settings?Settings->GetPointerSensitivity():1.f;
  View.Yaw+=X*Sensitivity;
  View.Pitch=FMath::Clamp(FRotator::NormalizeAxis(View.Pitch)+Y*Sensitivity*((!Settings||Settings->IsPointerYInverted())?-1.f:1.f),-24.f,20.f);
  View.Yaw=DrivenCar->GetActorRotation().Yaw+FMath::Clamp(FMath::FindDeltaAngleDegrees(DrivenCar->GetActorRotation().Yaw,View.Yaw),-85.f,85.f);
 }
 else if(FMath::Abs(Speed)>100&&Now-LastCarLookTime>2.5f)View.Yaw=FMath::RInterpTo(View,FRotator(View.Pitch,DrivenCar->GetActorRotation().Yaw+8,0),Dt,2).Yaw;
 if(VehicleAuditStage>=11&&VehicleAuditStage<=13)View=FRotator(-65,DrivenCar->GetActorRotation().Yaw,0);
 if(VehicleAuditStage>=14&&VehicleAuditStage<=16)View=FRotator(-10,DrivenCar->GetActorRotation().Yaw-65,0);
 if(VehicleAuditStage>=17&&VehicleAuditStage<=19)View=FRotator(15,DrivenCar->GetActorRotation().Yaw+55,0);
 // Enforce cabin limits even after camera recentering or diagnostic overrides.
 View.Pitch=FMath::Clamp(FRotator::NormalizeAxis(View.Pitch),-24.f,20.f);
 View.Yaw=DrivenCar->GetActorRotation().Yaw+FMath::Clamp(FMath::FindDeltaAngleDegrees(DrivenCar->GetActorRotation().Yaw,View.Yaw),-85.f,85.f);
 SetControlRotation(View);
}

void AParadiseHeatPlayerController::TickVehicleEntryAudit()
{
 const float Now=GetWorld()->GetTimeSeconds();
 if(VehicleAuditStage==0&&Now>8)
 {
  for(TActorIterator<AActor> It(GetWorld());It;++It)
  {
   if(!It->ActorHasTag(TEXT("CityTraffic"))||It->ActorHasTag(TEXT("AmbientMotorcycle")))continue;
   auto* Body=It->FindComponentByClass<UPoseableMeshComponent>();if(!Body||!GetNameSafe(Body->GetSkinnedAsset()).Contains(FParse::Param(FCommandLine::Get(),TEXT("SUVEntryAudit"))?TEXT("SUV"):TEXT("Sedan")))continue;
   FVector Exit;if(!FindVehicleExit(*It,Exit))continue;
   FVector Start=Exit-(FParse::Param(FCommandLine::Get(),TEXT("EntryWalkAudit"))?It->GetActorForwardVector()*160.f:FVector::ZeroVector);
   if(FParse::Param(FCommandLine::Get(),TEXT("EntryFrontAudit")))Start=FVector(It->GetActorLocation().X,It->GetActorLocation().Y,Exit.Z)+It->GetActorForwardVector()*350.f;
   GetPawn()->SetActorLocation(Start);BeginVehicleEntry(*It);
   if(PendingVehicle){AuditCar=*It;VehicleAuditStage=1;VehicleAuditTime=Now;break;}
  }
 }
 else if(VehicleAuditStage==1&&DrivenCar)
 {
  AuditCar=DrivenCar;VehicleAuditPosition=DrivenCar->GetActorLocation();VehicleAuditTime=Now;VehicleAuditStage=2;
  SetControlRotation(FRotator(-24,DrivenCar->GetActorRotation().Yaw+8,0));
  FScreenshotRequest::RequestScreenshot(TEXT("/private/tmp/car-entry-audit.png"),false,false);
 }
 else if(VehicleAuditStage==2&&Now-VehicleAuditTime>4)
 {
  const float Distance=FVector::Dist2D(VehicleAuditPosition,DrivenCar->GetActorLocation());
  UE_LOG(LogParadiseHeat,Display,TEXT("ENTRY_AUDIT_DRIVE distance=%.1f speed=%.1f physics=%d"),Distance,DrivenCar->GetVelocity().Size(),Cast<UPrimitiveComponent>(DrivenCar->GetRootComponent())->IsSimulatingPhysics());
  UE_LOG(LogParadiseHeat,Display,TEXT("STEERING_CHECK right degrees=%.1f wheel=%d audio=%d pitch=%.2f"),LastDriverSteering,CarSteeringWheel!=nullptr,CarEngineAudio&&CarEngineAudio->IsPlaying(),CarEnginePitch);
  FScreenshotRequest::RequestScreenshot(TEXT("/private/tmp/car-drive-audit.png"),false,false);
  VehicleAuditStage=3;VehicleAuditTime=Now;
 }
 else if(VehicleAuditStage==3&&DrivenCar&&DrivenCar->GetVelocity().Size2D()<100)
 {
  ExitCar();if(!DrivenCar){VehicleAuditStage=4;VehicleAuditTime=Now;FScreenshotRequest::RequestScreenshot(TEXT("/private/tmp/car-exit-glass.png"),false,false);UE_LOG(LogParadiseHeat,Display,TEXT("ENTRY_AUDIT_EXIT visible=%d"),GetPawn()&&!GetPawn()->IsHidden());}
 }
 else if(VehicleAuditStage==4&&Now-VehicleAuditTime>1&&AuditCar.IsValid())
 {
  BeginVehicleEntry(AuditCar.Get());if(PendingVehicle){VehicleAuditStage=5;VehicleAuditTime=Now;}
 }
 else if(VehicleAuditStage==5&&DrivenCar&&Now-VehicleAuditTime>2)
 {
  if(FParse::Param(FCommandLine::Get(),TEXT("CockpitPreview")))
  {
   VehicleAuditStage=FParse::Param(FCommandLine::Get(),TEXT("FullCabinAudit"))?10:8;VehicleAuditTime=Now;
   FScreenshotRequest::RequestScreenshot(TEXT("/private/tmp/cockpit-final.png"),false,false);
   UE_LOG(LogParadiseHeat,Display,TEXT("COCKPIT_PREVIEW_READY"));return;
  }
  ExitCar();if(!DrivenCar){VehicleAuditStage=6;VehicleAuditTime=Now;UE_LOG(LogParadiseHeat,Display,TEXT("ENTRY_AUDIT_REENTRY_PASS"));}
 }
 else if(VehicleAuditStage==6&&Now-VehicleAuditTime>1)
 {
  for(TActorIterator<AActor> It(GetWorld());It;++It)
  {
   if(!It->ActorHasTag(TEXT("AmbientMotorcycle")))continue;
   FVector Exit;if(!FindVehicleExit(*It,Exit))continue;
   FVector Start=Exit-(FParse::Param(FCommandLine::Get(),TEXT("EntryWalkAudit"))?It->GetActorForwardVector()*160.f:FVector::ZeroVector);
   if(FParse::Param(FCommandLine::Get(),TEXT("EntryFrontAudit")))Start=FVector(It->GetActorLocation().X,It->GetActorLocation().Y,Exit.Z)+It->GetActorForwardVector()*350.f;
   GetPawn()->SetActorLocation(Start);BeginVehicleEntry(*It);
   if(PendingVehicle){VehicleAuditStage=7;VehicleAuditTime=Now;break;}
  }
 }
 else if(VehicleAuditStage==7&&Cast<AParadiseHeatMotorcycle>(GetPawn()))
 {
  UE_LOG(LogParadiseHeat,Display,TEXT("ENTRY_AUDIT_BIKE mounted=%d"),Cast<AParadiseHeatMotorcycle>(GetPawn())->IsMounted());
  FScreenshotRequest::RequestScreenshot(TEXT("/private/tmp/bike-entry-audit.png"),false,false);VehicleAuditStage=8;
 }
 if(DrivenCar&&VehicleAuditStage>=10&&VehicleAuditStage<=19&&Now-VehicleAuditTime>2)
 {
  const float Yaw=DrivenCar->GetActorRotation().Yaw;
  if(VehicleAuditStage==10)SetControlRotation(FRotator(-65,Yaw,0));
  if(VehicleAuditStage==12){FScreenshotRequest::RequestScreenshot(TEXT("/private/tmp/cabin-body.png"),false,false);}
  if(VehicleAuditStage==13)SetControlRotation(FRotator(-10,Yaw-65,0));
  if(VehicleAuditStage==15){FScreenshotRequest::RequestScreenshot(TEXT("/private/tmp/cabin-side.png"),false,false);}
  if(VehicleAuditStage==18)FScreenshotRequest::RequestScreenshot(TEXT("/private/tmp/cabin-rear-mirror.png"),false,false);
  VehicleAuditStage++;VehicleAuditTime=Now;
 }
 if(DrivenCar&&VehicleAuditStage==20&&Now-VehicleAuditTime>2){SetControlRotation(FRotator(-24,DrivenCar->GetActorRotation().Yaw+8,0));VehicleAuditStage=8;}
 if(VehicleAuditStage>0&&VehicleAuditStage<8&&Now-VehicleAuditTime>20)
 {UE_LOG(LogParadiseHeat,Error,TEXT("ENTRY_AUDIT_TIMEOUT stage=%d"),VehicleAuditStage);VehicleAuditStage=9;}
}
