#include "ParadiseHeatMotorcycle.h"
#include "Environment/CityImpactComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputCoreTypes.h"
#include "Engine/StaticMesh.h"
#include "Sound/SoundBase.h"
#include "ParadiseHeat.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/PhysicsAsset.h"

AParadiseHeatMotorcycle::AParadiseHeatMotorcycle()
{
 PrimaryActorTick.bCanEverTick=true;
 PhysicsBody=CreateDefaultSubobject<UBoxComponent>(TEXT("PhysicsBody"));RootComponent=PhysicsBody;
 PhysicsBody->SetBoxExtent(FVector(86,19,22));PhysicsBody->SetCollisionProfileName(TEXT("Vehicle"));
 PhysicsBody->SetNotifyRigidBodyCollision(true);
 PhysicsBody->OnComponentHit.AddDynamic(this,&AParadiseHeatMotorcycle::OnCrashHit);
 PhysicsBody->SetSimulatePhysics(true);PhysicsBody->SetEnableGravity(true);PhysicsBody->SetLinearDamping(.06f);PhysicsBody->SetAngularDamping(.8f);PhysicsBody->BodyInstance.bUseCCD=true;
 VisualRoot=CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));VisualRoot->SetupAttachment(PhysicsBody);VisualRoot->SetRelativeRotation(FRotator(0,180,0));VisualRoot->SetRelativeLocation(FVector(0,0,-51));
 BodyMesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));BodyMesh->SetupAttachment(VisualRoot);BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Kickstand=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Kickstand"));Kickstand->SetupAttachment(VisualRoot);Kickstand->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 FrontWheel=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FrontWheel"));FrontWheel->SetupAttachment(VisualRoot);FrontWheel->SetRelativeLocation(FVector(-64.874f,.068f,28.833f));FrontWheel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 RearWheel=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RearWheel"));RearWheel->SetupAttachment(VisualRoot);RearWheel->SetRelativeLocation(FVector(65.525f,0,27.977f));RearWheel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 CameraBoom=CreateDefaultSubobject<USpringArmComponent>(TEXT("RideCameraBoom"));CameraBoom->SetupAttachment(PhysicsBody);CameraBoom->TargetArmLength=430;CameraBoom->SetRelativeLocation(FVector(0,0,105));CameraBoom->bUsePawnControlRotation=true;CameraBoom->bEnableCameraLag=true;CameraBoom->CameraLagSpeed=7;CameraBoom->bInheritRoll=false;
 RideCamera=CreateDefaultSubobject<UCameraComponent>(TEXT("RideCamera"));RideCamera->SetupAttachment(CameraBoom);RideCamera->bUsePawnControlRotation=false;
 EngineAudio=CreateDefaultSubobject<UAudioComponent>(TEXT("EngineAudio"));EngineAudio->SetupAttachment(PhysicsBody);EngineAudio->bAutoActivate=false;EngineAudio->bAllowSpatialization=true;
 RiderVisual=CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("RiderVisual"));RiderVisual->SetupAttachment(PhysicsBody);RiderVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);RiderVisual->SetVisibility(false,true);
 AutoPossessAI=EAutoPossessAI::Disabled;
}
void AParadiseHeatMotorcycle::BeginPlay()
{
 Super::BeginPlay();
 UCityImpactComponent::Install(this);
 BodyMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Vehicles/Motorcycles/Triumph/PreparedAssembly/RideBody.RideBody")));
 FrontWheel->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Vehicles/Motorcycles/Triumph/PreparedAssembly/FrontWheel.FrontWheel")));
 RearWheel->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Vehicles/Motorcycles/Triumph/PreparedAssembly/RearWheel.RearWheel")));
 if(auto* Sound=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/Vehicles/PrototypeMotorcycleEngine.PrototypeMotorcycleEngine"))) EngineAudio->SetSound(Sound);
 Kickstand->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Vehicles/Motorcycles/Triumph/PreparedAssembly/Kickstand.Kickstand")));
 bAudit=FParse::Param(FCommandLine::Get(),TEXT("MotorcycleAudit"));
 PhysicsBody->SetMassOverrideInKg(NAME_None,240.f,true);
 PhysicsBody->SetCenterOfMass(FVector(0,0,-10));
 UE_LOG(LogParadiseHeat,Display,TEXT("MOTORCYCLE_READY body=%s front=%s rear=%s"),*GetNameSafe(BodyMesh->GetStaticMesh()),*GetNameSafe(FrontWheel->GetStaticMesh()),*GetNameSafe(RearWheel->GetStaticMesh()));
}
void AParadiseHeatMotorcycle::SetupPlayerInputComponent(UInputComponent* Input)
{
 Super::SetupPlayerInputComponent(Input);
 Input->BindKey(EKeys::W,IE_Pressed,this,&AParadiseHeatMotorcycle::ArrowForwardPressed);
 Input->BindKey(EKeys::W,IE_Released,this,&AParadiseHeatMotorcycle::ArrowForwardReleased);
 Input->BindKey(EKeys::S,IE_Pressed,this,&AParadiseHeatMotorcycle::ArrowReversePressed);
 Input->BindKey(EKeys::S,IE_Released,this,&AParadiseHeatMotorcycle::ArrowReverseReleased);
 Input->BindKey(EKeys::A,IE_Pressed,this,&AParadiseHeatMotorcycle::ArrowLeftPressed);
 Input->BindKey(EKeys::A,IE_Released,this,&AParadiseHeatMotorcycle::ArrowLeftReleased);
 Input->BindKey(EKeys::D,IE_Pressed,this,&AParadiseHeatMotorcycle::ArrowRightPressed);
 Input->BindKey(EKeys::D,IE_Released,this,&AParadiseHeatMotorcycle::ArrowRightReleased);
 Input->BindKey(EKeys::SpaceBar,IE_Pressed,this,&AParadiseHeatMotorcycle::BrakePressed);
 Input->BindKey(EKeys::SpaceBar,IE_Released,this,&AParadiseHeatMotorcycle::BrakeReleased);
 Input->BindAxisKey(EKeys::MouseX,this,&AParadiseHeatMotorcycle::LookYaw);
 Input->BindAxisKey(EKeys::MouseY,this,&AParadiseHeatMotorcycle::LookPitch);
 Input->BindKey(EKeys::Up,IE_Pressed,this,&AParadiseHeatMotorcycle::ArrowForwardPressed);
 Input->BindKey(EKeys::Up,IE_Released,this,&AParadiseHeatMotorcycle::ArrowForwardReleased);
 Input->BindKey(EKeys::Down,IE_Pressed,this,&AParadiseHeatMotorcycle::ArrowReversePressed);
 Input->BindKey(EKeys::Down,IE_Released,this,&AParadiseHeatMotorcycle::ArrowReverseReleased);
 Input->BindKey(EKeys::Left,IE_Pressed,this,&AParadiseHeatMotorcycle::ArrowLeftPressed);
 Input->BindKey(EKeys::Left,IE_Released,this,&AParadiseHeatMotorcycle::ArrowLeftReleased);
 Input->BindKey(EKeys::Right,IE_Pressed,this,&AParadiseHeatMotorcycle::ArrowRightPressed);
 Input->BindKey(EKeys::Right,IE_Released,this,&AParadiseHeatMotorcycle::ArrowRightReleased);
}
void AParadiseHeatMotorcycle::InputForward(float V){ForwardInput=V;} void AParadiseHeatMotorcycle::InputReverse(float V){ReverseInput=V;}
void AParadiseHeatMotorcycle::InputLeft(float V){LeftInput=V;} void AParadiseHeatMotorcycle::InputRight(float V){RightInput=V;} void AParadiseHeatMotorcycle::InputBrake(float V){BrakeInput=V;}
void AParadiseHeatMotorcycle::ArrowForwardPressed(){ForwardInput=1.f;} void AParadiseHeatMotorcycle::ArrowForwardReleased(){ForwardInput=0.f;}
void AParadiseHeatMotorcycle::ArrowReversePressed(){ReverseInput=1.f;} void AParadiseHeatMotorcycle::ArrowReverseReleased(){ReverseInput=0.f;}
void AParadiseHeatMotorcycle::ArrowLeftPressed(){LeftInput=1.f;} void AParadiseHeatMotorcycle::ArrowLeftReleased(){LeftInput=0.f;}
void AParadiseHeatMotorcycle::ArrowRightPressed(){RightInput=1.f;} void AParadiseHeatMotorcycle::ArrowRightReleased(){RightInput=0.f;}
void AParadiseHeatMotorcycle::BrakePressed(){BrakeInput=1.f;} void AParadiseHeatMotorcycle::BrakeReleased(){BrakeInput=0.f;}
void AParadiseHeatMotorcycle::LookYaw(float V){if(FMath::Abs(V)>.01f)LastCameraLookTime=GetWorld()->GetTimeSeconds();if(APlayerController* PC=Cast<APlayerController>(GetController())) PC->AddYawInput(V);}
void AParadiseHeatMotorcycle::LookPitch(float V){if(FMath::Abs(V)>.01f)LastCameraLookTime=GetWorld()->GetTimeSeconds();if(APlayerController* PC=Cast<APlayerController>(GetController())) PC->AddPitchInput(-V);}
bool AParadiseHeatMotorcycle::IsAvailableFor(const APawn* Candidate) const { return Candidate&&!Candidate->ActorHasTag(TEXT("ImpactRagdoll"))&&!FallenRider.IsValid()&&GetWorld()->GetTimeSeconds()>MountAllowedAt&&!Rider.IsValid()&&FVector::DistSquared(Candidate->GetActorLocation(),GetActorLocation())<FMath::Square(300.f); }
void AParadiseHeatMotorcycle::Mount(APlayerController* PC,APawn* Character)
{
 if(!PC||!IsAvailableFor(Character)) return;
 ACharacter* Human=Cast<ACharacter>(Character);
 if(!Human||Human->GetMesh()->IsSimulatingPhysics()||!BuildRiderPose(Human))
 {
  UE_LOG(LogParadiseHeat,Error,TEXT("MOTORCYCLE_MOUNT_ABORT rider pose unavailable"));
  return;
 }
 Rider=Character;
 if(bCrashed)
 {
  SetActorLocationAndRotation(GetActorLocation()+FVector(0,0,65),FRotator(0,GetActorRotation().Yaw,0),false,nullptr,ETeleportType::TeleportPhysics);
  PhysicsBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
  PhysicsBody->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
 }
 bCrashed=false;
 PhysicsBody->SetBoxExtent(FVector(86,19,22));
 FeetDownBlend=1.f;LastPoseBlend=-1.f;
 MountAllowedAt=GetWorld()->GetTimeSeconds()+1.5f;
 Character->SetActorEnableCollision(false);
 Character->SetActorHiddenInGame(true);
 Human->GetCharacterMovement()->StopMovementImmediately();
 Human->GetCharacterMovement()->DisableMovement();
 RiderVisual->SetHiddenInGame(false);
 RiderVisual->SetVisibility(true,true);
 Kickstand->SetVisibility(false);
 ForwardInput=ReverseInput=LeftInput=RightInput=BrakeInput=0.f;
 PhysicsBody->SetSimulatePhysics(true);
 PhysicsBody->WakeAllRigidBodies();
 PC->Possess(this);
 PC->SetControlRotation(FRotator(-12,GetActorRotation().Yaw,0));
 if(PC->PlayerCameraManager){PC->PlayerCameraManager->ViewPitchMin=-55.f;PC->PlayerCameraManager->ViewPitchMax=-5.f;}
 EngineAudio->Play();
 UE_LOG(LogParadiseHeat,Display,TEXT("MOTORCYCLE_MOUNT rider=%s"),*GetNameSafe(Character));
}
void AParadiseHeatMotorcycle::Dismount(APlayerController* PC)
{
 APawn* Character=Rider.Get();if(!PC||!Character)return;
 Character->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
 Character->SetActorLocation(GetActorLocation()+GetActorRightVector()*210+FVector(0,0,80));Character->SetActorRotation(FRotator(0,GetActorRotation().Yaw,0));Character->SetActorHiddenInGame(false);Character->SetActorEnableCollision(true);
 if(ACharacter* RiderCharacter=Cast<ACharacter>(Character))
 {
  RiderCharacter->UnCrouch();
  if(UCharacterMovementComponent* Movement=RiderCharacter->GetCharacterMovement()) Movement->SetMovementMode(MOVE_Walking);
 }
 Rider.Reset();PC->Possess(Character);
 ForwardInput=ReverseInput=LeftInput=RightInput=BrakeInput=0.f;
 Kickstand->SetVisibility(true);
 RiderVisual->SetVisibility(false,true);PhysicsBody->SetSimulatePhysics(true);PhysicsBody->WakeAllRigidBodies();
 if(PC->PlayerCameraManager){PC->PlayerCameraManager->ViewPitchMin=-65.f;PC->PlayerCameraManager->ViewPitchMax=8.f;}
 EngineAudio->Stop();UE_LOG(LogParadiseHeat,Display,TEXT("MOTORCYCLE_DISMOUNT"));
}
// Pose from the reference hierarchy, never from a moving locomotion frame.
// Rotate entire descendant chains so parent/child transforms remain coherent.
bool AParadiseHeatMotorcycle::BuildRiderPose(ACharacter* Character)
{
 USkeletalMeshComponent* Source=Character->GetMesh();
 USkeletalMesh* Mesh=Source?Source->GetSkeletalMeshAsset():nullptr;
 if(!Mesh)return false;
 const FReferenceSkeleton& Ref=Mesh->GetRefSkeleton();
 const int32 Pelvis=Ref.FindBoneIndex(TEXT("pelvis"));
 if(Pelvis==INDEX_NONE)return false;
 TArray<FTransform> CS=Ref.GetRefBonePose();
 for(int32 I=0;I<CS.Num();++I)
  if(Ref.GetParentIndex(I)>=0) CS[I]=CS[I]*CS[Ref.GetParentIndex(I)];
 const FQuat Facing(FVector::UpVector,FMath::DegreesToRadians(-90.f));
 const FQuat Lean(FVector::RightVector,FMath::DegreesToRadians(24.f));
 const FQuat Frame=Lean*Facing;
 const FVector ReferencePelvis=CS[Pelvis].GetLocation();
 const FVector SeatPelvis(-27,0,27);
 for(auto& T:CS)
 {
  T.SetLocation(Frame.RotateVector(T.GetLocation()-ReferencePelvis)+SeatPelvis);
  T.SetRotation(Frame*T.GetRotation());
 }
 const TArray<FTransform> Base=CS;
 auto RotateChain=[&](int32 Joint,const FQuat& Delta)
 {
  const FVector Pivot=CS[Joint].GetLocation();
  for(int32 I=0;I<CS.Num();++I)
  {
   int32 P=I;
   while(P>=0&&P!=Joint)P=Ref.GetParentIndex(P);
   if(P!=Joint)continue;
   CS[I].SetLocation(Pivot+Delta.RotateVector(CS[I].GetLocation()-Pivot));
   CS[I].SetRotation((Delta*CS[I].GetRotation()).GetNormalized());
  }
 };
 auto Limb=[&](FName Upper,FName Lower,FName End,FVector Target,FVector Pole)->bool
 {
  const int32 A=Ref.FindBoneIndex(Upper),B=Ref.FindBoneIndex(Lower),C=Ref.FindBoneIndex(End);
  if(A<0||B<0||C<0)return false;
  const FVector Origin=CS[A].GetLocation();
  const float L1=FVector::Distance(Origin,CS[B].GetLocation());
  const float L2=FVector::Distance(CS[B].GetLocation(),CS[C].GetLocation());
  const FVector Direction=(Target-Origin).GetSafeNormal();
  const float D=FMath::Clamp(float(FVector::Distance(Origin,Target)),FMath::Abs(L1-L2)+.1f,L1+L2-.1f);
  const float Along=(L1*L1-L2*L2+D*D)/(2*D);
  const FVector Bend=(Pole-Direction*FVector::DotProduct(Pole,Direction)).GetSafeNormal();
  const FVector JointTarget=Origin+Direction*Along+Bend*FMath::Sqrt(FMath::Max(0.f,L1*L1-Along*Along));
  RotateChain(A,FQuat::FindBetweenVectors(CS[B].GetLocation()-Origin,JointTarget-Origin));
  RotateChain(B,FQuat::FindBetweenVectors(CS[C].GetLocation()-CS[B].GetLocation(),Target-CS[B].GetLocation()));
  // Restore the reference end orientation instead of inheriting the knee/elbow rotation.
  if(End.ToString().StartsWith(TEXT("foot")))
  {
   // Base includes torso lean: remove it for soles roughly parallel with the pegs.
   RotateChain(C,(Lean.Inverse()*Base[C].GetRotation())*CS[C].GetRotation().Inverse());
  }
  UE_LOG(LogParadiseHeat,Display,TEXT("RIDER_IK %s end=%s target=%s error=%.2f lengths=%.1f/%.1f"),
   *End.ToString(),*CS[C].GetLocation().ToString(),*Target.ToString(),FVector::Distance(CS[C].GetLocation(),Target),L1,L2);
  return true;
 };
 bool OK=true;
 for(int32 Side : {-1,1})
 {
  const FString S=Side<0?TEXT("_l"):TEXT("_r");
  OK &= Limb(FName(TEXT("thigh")+S),FName(TEXT("calf")+S),FName(TEXT("foot")+S),
   FMath::Lerp(FVector(-3,Side*25,-24),FVector(-12,Side*31,-43),FeetDownBlend),FVector(1,Side*.35f,0));
  OK &= Limb(FName(TEXT("upperarm")+S),FName(TEXT("lowerarm")+S),FName(TEXT("hand")+S),
   FVector(25,Side*34,40),FVector(-.15f,Side*1.f,-.3f));
 }
 if(!OK)return false;
 RiderVisual->SetSkinnedAssetAndUpdate(Mesh);
 for(int32 M=0;M<Source->GetNumMaterials();++M)RiderVisual->SetMaterial(M,Source->GetMaterial(M));
 RiderVisual->SetRelativeTransform(FTransform::Identity);
 RiderVisual->SetLeaderPoseComponent(nullptr);
 RiderVisual->bUseAttachParentBound=false;
 RiderVisual->SetBoundsScale(2.f);
 RiderVisual->BoneSpaceTransforms.SetNum(CS.Num());
 for(int32 I=0;I<CS.Num();++I)
 {
  const int32 Parent=Ref.GetParentIndex(I);
  RiderVisual->BoneSpaceTransforms[I]=Parent<0?CS[I]:CS[I].GetRelativeTransform(CS[Parent]);
 }
 RiderVisual->MarkRefreshTransformDirty();
 RiderVisual->RefreshBoneTransforms();
 RiderVisual->UpdateBounds();
 RiderVisual->MarkRenderTransformDirty();
 RiderVisual->MarkRenderDynamicDataDirty();
 UE_LOG(LogParadiseHeat,Display,TEXT("RIDER_POSE mesh=%s bones=%d bounds=%s"),*GetNameSafe(Mesh),CS.Num(),*RiderVisual->Bounds.Origin.ToString());
 return true;
}

void AParadiseHeatMotorcycle::OnCrashHit(UPrimitiveComponent* HitComponent,AActor* OtherActor,UPrimitiveComponent* OtherComponent,FVector NormalImpulse,const FHitResult& Hit)
{
 if(!IsMounted()||bCrashed||GetWorld()->GetTimeSeconds()<MountAllowedAt||OtherActor==Rider.Get())return;
 const FVector OtherVelocity=OtherComponent?OtherComponent->GetComponentVelocity():FVector::ZeroVector;
 const float ImpactSpeed=FMath::Abs(FVector::DotProduct(PreviousVelocity-OtherVelocity,Hit.ImpactNormal));
 if(ImpactSpeed<650.f||NormalImpulse.Size()<12000.f)return;
 ACharacter* Human=Cast<ACharacter>(Rider.Get());
 APlayerController* PC=Cast<APlayerController>(GetController());
 if(!Human||!PC)return;
 const FVector EjectVelocity=PreviousVelocity+FVector(0,0,280);
 Dismount(PC);
 CrashOrigin=Human->GetActorLocation();
 bCrashed=true;Kickstand->SetVisibility(false);
 // Suspension is disabled after a crash; the chassis must then enclose the
 // wheels and handlebars, not just the narrow central riding body.
 PhysicsBody->SetBoxExtent(FVector(94,38,53));
 FallenRider=Human;RecoverAt=GetWorld()->GetTimeSeconds()+4.f;MountAllowedAt=RecoverAt+3.f;
 Human->GetCharacterMovement()->DisableMovement();
 Human->SetActorLocation(GetActorLocation()+FVector(0,0,120),false,nullptr,ETeleportType::TeleportPhysics);
 auto* Mesh=Human->GetMesh();SavedMeshTransform=Mesh->GetRelativeTransform();
 Human->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Mesh->SetCollisionProfileName(TEXT("Ragdoll"));
 Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
 Mesh->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
 Mesh->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Block);
 Mesh->SetUseCCD(true);
 Mesh->SetAllBodiesSimulatePhysics(true);Mesh->SetSimulatePhysics(true);
 Mesh->SetAllPhysicsLinearVelocity(EjectVelocity);
 PhysicsBody->AddAngularImpulseInRadians(GetActorForwardVector()*5.f,NAME_None,true);
 UE_LOG(LogParadiseHeat,Display,TEXT("MOTORCYCLE_CRASH impact=%.1f physicsAsset=%s"),ImpactSpeed,*GetNameSafe(Mesh->GetPhysicsAsset()));
}

void AParadiseHeatMotorcycle::Tick(float Dt)
{
 Super::Tick(Dt);
 if(FallenRider.IsValid()&&GetWorld()->GetTimeSeconds()>RecoverAt)
 {
  ACharacter* Human=FallenRider.Get();auto* Mesh=Human->GetMesh();
  FVector Location=CrashOrigin;
  FCollisionQueryParams GroundQuery(SCENE_QUERY_STAT(CrashRecovery),false,Human);
  GroundQuery.AddIgnoredActor(this);
  FVector Candidate=Mesh->GetSocketLocation(TEXT("pelvis"));
  if(FVector::DistSquared2D(Candidate,CrashOrigin)>FMath::Square(1800.f))Candidate=CrashOrigin;
  bool bFound=false;
  for(int32 Attempt=0;Attempt<12;++Attempt)
  {
   const FVector Center=Attempt<6?Candidate:CrashOrigin;
   const FVector Offset=FRotator(0,(Attempt%6)*60.f,0).Vector()*float(Attempt%6)*90.f;
   const FVector Start=FVector(Center.X,Center.Y,CrashOrigin.Z+500.f)+Offset;
   FHitResult Ground;
   if(GetWorld()->LineTraceSingleByChannel(Ground,Start,Start-FVector(0,0,1200.f),ECC_Visibility,GroundQuery)&&Ground.ImpactNormal.Z>.7f)
   {
    const FVector Proposed=Ground.ImpactPoint+FVector(0,0,Human->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+5.f);
    if(!GetWorld()->OverlapBlockingTestByChannel(Proposed,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(Human->GetCapsuleComponent()->GetScaledCapsuleRadius(),Human->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),GroundQuery))
    {Location=Proposed;bFound=true;break;}
   }
  }
  if(!bFound){RecoverAt=GetWorld()->GetTimeSeconds()+.5f;return;}
  Mesh->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
  Mesh->SetAllBodiesSimulatePhysics(false);Mesh->SetSimulatePhysics(false);
  Mesh->AttachToComponent(Human->GetCapsuleComponent(),FAttachmentTransformRules::KeepRelativeTransform);
  Mesh->SetRelativeTransform(SavedMeshTransform);Mesh->SetCollisionProfileName(TEXT("CharacterMesh"));
  Human->SetActorLocation(Location,false,nullptr,ETeleportType::TeleportPhysics);
  Human->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
  Human->GetCharacterMovement()->StopMovementImmediately();
  Human->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
  FallenRider.Reset();
  UE_LOG(LogParadiseHeat,Display,TEXT("MOTORCYCLE_RECOVERY groundValidated=1 pos=%s"),*Location.ToString());
 }
 PreviousVelocity=PhysicsBody->GetPhysicsLinearVelocity();
 if(bCrashed)return;
 if(bAudit)
 {
  // Isolate regression inputs from any keys held in the foreground game window.
  ForwardInput=((AuditTime>13&&AuditTime<17)||(AuditTime>27&&AuditTime<30)||(AuditTime>39&&AuditTime<42))?1.f:0.f;
  ReverseInput=LeftInput=0;
  RightInput=(AuditTime>39&&AuditTime<42)?1.f:0.f;
  BrakeInput=ForwardInput>0?0.f:1.f;
  if(auto* PC=UGameplayStatics::GetPlayerController(this,0))
  {
   PC->SetIgnoreLookInput(true);
   if(AuditTime<13)
   {
    PC->SetControlRotation(FRotator(-8,GetActorRotation().Yaw-80,0));
    LastCameraLookTime=GetWorld()->GetTimeSeconds();
   }
  }
 }
 const float Throttle=Rider.IsValid()?ForwardInput:0.f;
 const float Steer=Rider.IsValid()?RightInput-LeftInput:0.f;
 const FVector Forward=GetActorForwardVector();
 const FVector Right=GetActorRightVector();
 const FVector Velocity=PhysicsBody->GetPhysicsLinearVelocity();
 const float Speed=FVector::DotProduct(Velocity,Forward);
 CurrentDriveSpeed=Speed;
 if(ACharacter* Human=Cast<ACharacter>(Rider.Get()))
 {
  FeetDownBlend=FMath::FInterpTo(FeetDownBlend,(FMath::Abs(Speed)<35.f||(ReverseInput>.1f&&Speed<35.f))?1.f:0.f,Dt,6.f);
  if(FMath::Abs(FeetDownBlend-LastPoseBlend)>.04f)
  {BuildRiderPose(Human);LastPoseBlend=FeetDownBlend;}
 }
 if(Rider.IsValid())
 {
  if(APlayerController* PC=Cast<APlayerController>(GetController()))
  {
   FRotator View=PC->GetControlRotation();
   if(FMath::Abs(Speed)>30&&GetWorld()->GetTimeSeconds()-LastCameraLookTime>2.f)
    View=FMath::RInterpTo(View,FRotator(-12,GetActorRotation().Yaw,0),Dt,2.8f);
   View.Pitch=FMath::Clamp(FRotator::NormalizeAxis(View.Pitch),-55.f,-5.f);
   View.Roll=0;
   PC->SetControlRotation(View);
  }
 }
 FCollisionQueryParams Query(SCENE_QUERY_STAT(BikeSuspension),false,this);
 if(Rider.IsValid())Query.AddIgnoredActor(Rider.Get());
 int32 Contacts=0;
 // Two real support points, one per wheel. Gravity stays enabled in every mode.
 for(int32 Wheel=0;Wheel<2;++Wheel)
 {
  const float X=Wheel==0?64.874f:-65.525f;
  const FVector MountPoint=GetActorTransform().TransformPosition(FVector(X,0,10));
  FHitResult Hit;
  const bool HitGround=GetWorld()->LineTraceSingleByChannel(Hit,MountPoint,MountPoint-FVector(0,0,105),ECC_Visibility,Query);
  if(HitGround&&Hit.ImpactNormal.Z>.35f)
  {
   const float SpringLength=FMath::Max(0.f,float(MountPoint.Z-Hit.ImpactPoint.Z)-27.f);
   const float Compression=43.f-SpringLength;
   if(Compression>0)
   {
    ++Contacts;
    const FVector PointVelocity=PhysicsBody->GetPhysicsLinearVelocityAtPoint(MountPoint);
    const float Support=FMath::Clamp(Compression*12000.f-PointVelocity.Z*2400.f,0.f,420000.f);
    PhysicsBody->AddForceAtLocation(FVector::UpVector*Support,MountPoint);
    const FVector WheelForward=FRotator(0,Wheel==0?Steer*26.f:0,0).RotateVector(Forward);
    const FVector WheelRight=FVector::CrossProduct(FVector::UpVector,WheelForward).GetSafeNormal();
    const float Lateral=FVector::DotProduct(PointVelocity,WheelRight);
    PhysicsBody->AddForceAtLocation(-WheelRight*FMath::Clamp(Lateral*2700.f,-Support*.95f,Support*.95f),MountPoint);
   }
  }
 }
 if(Contacts>0)
 {
  float Drive=Throttle*140000.f;
  if(FMath::Abs(Speed)>1800&&Drive*Speed>0)Drive=0;
  if(BrakeInput>.1f||!Rider.IsValid())
   Drive-=FMath::Clamp(Speed*6000.f,-230000.f,230000.f);
  else if(ReverseInput>.1f&&Throttle<.1f)
  {
   // Brake forward motion, then foot-paddle backward at at most 3 km/h.
   Drive=Speed>15.f?-FMath::Clamp(Speed*6000.f,0.f,230000.f):FMath::Clamp((-80.f-Speed)*1800.f,-28000.f,28000.f);
  }
  else if(FMath::Abs(Throttle)<.1f)Drive-=FMath::Clamp(Speed*160.f,-22000.f,22000.f);
  PhysicsBody->AddForce(Forward*Drive);
 }
 // Explicit balance assistance; this is not a claim of unassisted bike dynamics.
 const FVector Angular=PhysicsBody->GetPhysicsAngularVelocityInRadians();
 const float Lean=FMath::DegreesToRadians(-Steer*12.f*FMath::Clamp(FMath::Abs(Speed)/900.f,0.f,1.f));
 const FVector DesiredUp=FQuat(Forward,Lean).RotateVector(FVector::UpVector);
 FVector Acceleration=FVector::CrossProduct(GetActorUpVector(),DesiredUp)*55.f;
 Acceleration-=FVector(Angular.X,Angular.Y,0)*10.f;
 const float DesiredYaw=Contacts?Steer*FMath::Clamp(Speed/500.f,-1.f,1.f)*.65f:0.f;
 Acceleration.Z=(DesiredYaw-Angular.Z)*8.f;
 PhysicsBody->AddTorqueInRadians(Acceleration,NAME_None,true);
 WheelAngle=FMath::Fmod(WheelAngle+Speed*Dt/27.f*57.2958f,360.f);
 FrontWheel->SetRelativeRotation(FRotator(-WheelAngle,Steer*26.f,0));
 RearWheel->SetRelativeRotation(FRotator(-WheelAngle,0,0));
 if(EngineAudio&&EngineAudio->IsPlaying())
 {
  EngineAudio->SetPitchMultiplier(FMath::Clamp(.65f+FMath::Abs(Speed)/1500.f+FMath::Abs(Throttle)*.45f,.65f,2.15f));
  EngineAudio->SetVolumeMultiplier(.16f+FMath::Abs(Throttle)*.34f);
 }
 if(bAudit)TickAudit(Dt);
}

void AParadiseHeatMotorcycle::TickAudit(float Dt)
{
 AuditTime+=Dt;
 APlayerController* PC=UGameplayStatics::GetPlayerController(this,0);
 if(!PC)return;
 auto Snapshot=[&](const TCHAR* Name)
 {
  UE_LOG(LogParadiseHeat,Display,TEXT("BIKE_AUDIT %s t=%.1f pos=%s vel=%s mounted=%d riderVisible=%d standVisible=%d sim=%d bikeYaw=%.1f camera=%s"),Name,AuditTime,
   *GetActorLocation().ToString(),*GetVelocity().ToString(),IsMounted(),RiderVisual->IsVisible(),Kickstand->IsVisible(),PhysicsBody->IsSimulatingPhysics(),GetActorRotation().Yaw,*PC->GetControlRotation().ToString());
  FScreenshotRequest::RequestScreenshot(FPaths::ScreenShotDir()/(FString(TEXT("bike-"))+Name+TEXT(".png")),false,false);
 };
 if(AuditStage==0&&AuditTime>8)
 {
  APawn* Human=PC->GetPawn();
  if(!Human||Human==this)return;
  Human->SetActorLocation(GetActorLocation()+FVector(0,120,60),false,nullptr,ETeleportType::TeleportPhysics);
  Mount(PC,Human);
  PC->SetControlRotation(FRotator(-12,GetActorRotation().Yaw-80,0));
  AuditStart=GetActorLocation();++AuditStage;
 }
 else if(AuditStage==1&&AuditTime>13){Snapshot(TEXT("mounted"));ArrowForwardPressed();++AuditStage;}
 else if(AuditStage==2&&AuditTime>17){ArrowForwardReleased();BrakePressed();Snapshot(TEXT("drive"));++AuditStage;}
 else if(AuditStage==3&&AuditTime>21)
 {
  // Known clear roadway, elevated 2m: must fall and settle without a transform snap.
  SetActorLocation(FVector(0,0,250),false,nullptr,ETeleportType::TeleportPhysics);
  SetActorRotation(FRotator(0,0,0),ETeleportType::TeleportPhysics);
  PhysicsBody->SetPhysicsLinearVelocity(FVector::ZeroVector);
  PhysicsBody->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
  ++AuditStage;
 }
 else if(AuditStage==4&&AuditTime>25){Snapshot(TEXT("landed"));++AuditStage;}
 else if(AuditStage==5&&AuditTime>27)
 {
  // Cross a real 16cm sidewalk and 20cm curb into the road.
  SetActorLocation(FVector(-1200,-2000,68),false,nullptr,ETeleportType::TeleportPhysics);
  SetActorRotation(FRotator(0,0,0),ETeleportType::TeleportPhysics);
  BrakeReleased();ArrowForwardPressed();++AuditStage;
 }
 else if(AuditStage==6&&AuditTime>30){ArrowForwardReleased();BrakePressed();Snapshot(TEXT("curb"));++AuditStage;}
 else if(AuditStage==7&&AuditTime>33){Dismount(PC);Snapshot(TEXT("dismount"));++AuditStage;}
 else if(AuditStage==8&&AuditTime>34){Mount(PC,PC->GetPawn());PC->SetControlRotation(FRotator(-12,GetActorRotation().Yaw-80,0));++AuditStage;}
 else if(AuditStage==9&&AuditTime>37){Snapshot(TEXT("remount"));++AuditStage;}
 else if(AuditStage==10&&AuditTime>42){Snapshot(TEXT("turn"));++AuditStage;}
 else if(AuditStage==11&&AuditTime>45){Snapshot(TEXT("turn-settled"));++AuditStage;}
 else if(AuditStage==12&&AuditTime>47){GEngine->Exec(GetWorld(),TEXT("quit"));++AuditStage;}
}
