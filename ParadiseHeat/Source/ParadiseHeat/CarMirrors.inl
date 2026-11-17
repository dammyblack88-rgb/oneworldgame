#include "Components/SceneCaptureComponent2D.h"
#include "Engine/SceneCapture2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Engine/TextureRenderTarget2D.h"

void AParadiseHeatPlayerController::CreateCarMirrors()
{
 RemoveCarMirrors();
 if(!DrivenCar)return;
 auto* Body=DrivenCar->FindComponentByClass<USkeletalMeshComponent>();
 auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Vehicles/Player/M_LiveCarMirror.M_LiveCarMirror"));
 if(!Body||!Material)return;
 const FString Name=GetNameSafe(Body->GetSkeletalMeshAsset());
 const bool SUV=Name.Contains(TEXT("SUV")),Truck=Name.Contains(TEXT("BoxTruck"));
 const FVector Positions[3]={SUV?FVector(36,0,153):FVector(34.2f,-4.7f,127.6f),Truck?FVector(162,-126,224):SUV?FVector(54.7f,-103,122):FVector(58.4f,-98.f,100.f),Truck?FVector(162,126,224):SUV?FVector(54.7f,103,122):FVector(58.4f,98.f,100.f)};
 for(int32 I=(Truck||SUV)?1:0;I<3;++I)
 {
  const FString Kind=Truck?TEXT("BoxTruck"):SUV?TEXT("SUV"):TEXT("Sedan");
  auto* Glass=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Vehicles/Player/Entry/SM_%s_Mirror%d.SM_%s_Mirror%d"),*Kind,I,*Kind,I));
  if(!Glass){UE_LOG(LogParadiseHeat,Error,TEXT("Missing fitted mirror %s %d"),*Kind,I);continue;}
  auto* Surface=NewObject<UStaticMeshComponent>(DrivenCar,*FString::Printf(TEXT("LiveMirrorSurface%d"),I));DrivenCar->AddInstanceComponent(Surface);Surface->SetupAttachment(Body);
  Surface->SetStaticMesh(Glass);Surface->SetCollisionEnabled(ECollisionEnabled::NoCollision);Surface->SetCastShadow(false);
  Surface->SetRelativeTransform(FTransform::Identity);
  auto* RT=NewObject<UTextureRenderTarget2D>(DrivenCar);RT->RenderTargetFormat=RTF_RGBA8;RT->ClearColor=FLinearColor::Black;RT->InitAutoFormat(I==0?384:256,I==0?128:160);RT->UpdateResourceImmediate(true);
  auto* MID=UMaterialInstanceDynamic::Create(Material,DrivenCar);MID->SetTextureParameterValue(TEXT("MirrorView"),RT);Surface->SetMaterial(0,MID);Surface->RegisterComponent();
  auto* Capture=GetWorld()->SpawnActor<ASceneCapture2D>();
  Capture->AttachToComponent(Body,FAttachmentTransformRules::KeepRelativeTransform);
  Capture->SetActorRelativeLocation(I==0?FVector(-245,0,SUV?130:105):Positions[I]+FVector(-4,I==1?-14:14,0));
  Capture->SetActorRelativeRotation(FRotator(-3,I==0?180:I==1?-155:155,0));
  auto* C=Capture->GetCaptureComponent2D();C->TextureTarget=RT;C->CaptureSource=ESceneCaptureSource::SCS_FinalColorLDR;C->FOVAngle=I==0?65:80;
  C->bCaptureEveryFrame=false;C->bCaptureOnMovement=false;C->bAlwaysPersistRenderingState=true;C->MaxViewDistanceOverride=7000;
  C->ShowFlags.SetMotionBlur(false);C->ShowFlags.SetTemporalAA(false);C->ShowFlags.SetDynamicShadows(false);
  C->PostProcessSettings.bOverride_AutoExposureMethod=true;C->PostProcessSettings.AutoExposureMethod=EAutoExposureMethod::AEM_Histogram;C->PostProcessSettings.bOverride_AutoExposureMinBrightness=true;C->PostProcessSettings.bOverride_AutoExposureMaxBrightness=true;C->PostProcessSettings.AutoExposureMinBrightness=10.5f;C->PostProcessSettings.AutoExposureMaxBrightness=10.5f;C->PostProcessSettings.bOverride_AutoExposureBias=true;C->PostProcessSettings.AutoExposureBias=0;
  C->PostProcessSettings.bOverride_MotionBlurAmount=true;C->PostProcessSettings.MotionBlurAmount=0;
  if(CockpitBody)C->HideComponent(CockpitBody);
  CarMirrorSurfaces.Add(Surface);CarMirrorCaptures.Add(Capture);
 }
 for(AActor* A:CarMirrorCaptures)if(auto* Capture=Cast<ASceneCapture2D>(A))
 {
  for(UStaticMeshComponent* Surface:CarMirrorSurfaces)Capture->GetCaptureComponent2D()->HideComponent(Surface);
  Capture->GetCaptureComponent2D()->CaptureScene();
 }
 NextMirrorUpdate=0;MirrorUpdateIndex=0;
 UE_LOG(LogParadiseHeat,Display,TEXT("LIVE_CAR_MIRRORS count=%d"),CarMirrorCaptures.Num());
}

void AParadiseHeatPlayerController::UpdateCarMirrors()
{
 if(CarMirrorCaptures.IsEmpty()||GetWorld()->GetTimeSeconds()<NextMirrorUpdate)return;
 NextMirrorUpdate=GetWorld()->GetTimeSeconds()+.045f;
 auto* Capture=Cast<ASceneCapture2D>(CarMirrorCaptures[MirrorUpdateIndex++%CarMirrorCaptures.Num()]);
 if(IsValid(Capture))
 {
  Capture->GetCaptureComponent2D()->CaptureScene();
  if(MirrorUpdateIndex==90&&FParse::Param(FCommandLine::Get(),TEXT("FullCabinAudit")))UKismetRenderingLibrary::ExportRenderTarget(this,Capture->GetCaptureComponent2D()->TextureTarget,TEXT("/private/tmp"),TEXT("mirror-capture.png"));
 }
}

void AParadiseHeatPlayerController::RemoveCarMirrors()
{
 for(AActor* A:CarMirrorCaptures)if(IsValid(A))A->Destroy();CarMirrorCaptures.Reset();
 for(UStaticMeshComponent* Surface:CarMirrorSurfaces)if(IsValid(Surface))Surface->DestroyComponent();CarMirrorSurfaces.Reset();
}
