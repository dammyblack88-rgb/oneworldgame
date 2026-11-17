#pragma once

static bool PoseTrafficOccupant(UPoseableMeshComponent* RiderVisual, USkeletalMesh* Mesh, bool Bike, FVector WheelCenter=FVector(44,0,50), float SteeringDegrees=0.f, FVector WheelAxis=FVector(.889f,0,-.458f), bool FirstPerson=false)
{
 if(!Mesh)return false;
 const FReferenceSkeleton& Ref=Mesh->GetRefSkeleton();
 const int32 Pelvis=Ref.FindBoneIndex(TEXT("pelvis"));
 if(Pelvis==INDEX_NONE)return false;
 TArray<FTransform> CS=Ref.GetRefBonePose();
 for(int32 I=0;I<CS.Num();++I)
  if(Ref.GetParentIndex(I)>=0) CS[I]=CS[I]*CS[Ref.GetParentIndex(I)];
 const FQuat Facing(FVector::UpVector,FMath::DegreesToRadians(-90.f));
 const FQuat Lean(FVector::RightVector,FMath::DegreesToRadians(Bike?24.f:8.f));
 const FQuat Frame=Lean*Facing;
 const FVector ReferencePelvis=CS[Pelvis].GetLocation();
 const FVector SeatPelvis=Bike?FVector(-27,0,27):FVector::ZeroVector;
 for(auto& T:CS)
 {
  T.SetLocation(Frame.RotateVector(T.GetLocation()-ReferencePelvis)+SeatPelvis);
  T.SetRotation(Frame*T.GetRotation());
 }
 const TArray<FTransform> Base=CS;
 if(FirstPerson&&!Bike)
 {
  // First-person shoulders sit behind/below the camera. Hands still solve to
  // the same physical wheel targets; elbow seams remain outside the frustum.
  for(int32 I=0;I<CS.Num();++I)
  {
   int32 Ancestor=I;
   while(Ancestor>=0)
   {
    const FString Name=Ref.GetBoneName(Ancestor).ToString();
    if(Name==TEXT("upperarm_l")||Name==TEXT("upperarm_r")){CS[I].AddToTranslation(FVector(-20,0,-15));break;}
    Ancestor=Ref.GetParentIndex(Ancestor);
   }
  }
 }
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
  return true;
 };
 bool OK=true;
 for(int32 Side : {-1,1})
 {
  const FString S=Side<0?TEXT("_l"):TEXT("_r");
  OK &= Limb(FName(TEXT("thigh")+S),FName(TEXT("calf")+S),FName(TEXT("foot")+S),
   Bike?FVector(-3,Side*25,-24):FVector(42,Side*13,-12),FVector(1,Side*.35f,0));
  OK &= Limb(FName(TEXT("upperarm")+S),FName(TEXT("lowerarm")+S),FName(TEXT("hand")+S),
   Bike?FVector(25,Side*34,40):WheelCenter+FQuat(WheelAxis.GetSafeNormal(),FMath::DegreesToRadians(SteeringDegrees)).RotateVector(FVector(-8,Side*25,4)),Bike?FVector(-.15f,Side*1.f,-.3f):FVector(-1,Side*.15f,-2));
  if(!Bike)
  {
   // Grip the rim rather than leave the reference-pose fingers flat over the horn.
   const int32 Hand=Ref.FindBoneIndex(FName(TEXT("hand")+S));
   const int32 Middle=Ref.FindBoneIndex(FName(TEXT("middle_03")+S));
   const FQuat Steering(WheelAxis.GetSafeNormal(),FMath::DegreesToRadians(SteeringDegrees));
   if(Hand!=INDEX_NONE&&Middle!=INDEX_NONE)
    RotateChain(Hand,Steering*FQuat::FindBetweenNormals((CS[Middle].GetLocation()-CS[Hand].GetLocation()).GetSafeNormal(),FVector::ForwardVector));
   for(const FString Finger:{TEXT("index"),TEXT("middle"),TEXT("ring"),TEXT("pinky")})
   {
    for(int32 Joint=1;Joint<=2;++Joint)
    {
     const int32 A=Ref.FindBoneIndex(FName(FString::Printf(TEXT("%s_0%d%s"),*Finger,Joint,*S)));
     const int32 B=Ref.FindBoneIndex(FName(FString::Printf(TEXT("%s_0%d%s"),*Finger,Joint+1,*S)));
     if(A==INDEX_NONE||B==INDEX_NONE)continue;
     const FVector Direction=Steering.RotateVector(Joint==1?FVector(1,-Side*.35f,0).GetSafeNormal():FVector(0,-Side,0));
     RotateChain(A,FQuat::FindBetweenNormals((CS[B].GetLocation()-CS[A].GetLocation()).GetSafeNormal(),Direction));
    }
   }
  }
 }
 if(!OK)return false;
 if(RiderVisual->GetSkinnedAsset()!=Mesh)RiderVisual->SetSkinnedAssetAndUpdate(Mesh);
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
 return true;
}
