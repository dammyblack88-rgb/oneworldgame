// Component-space targets, solved as bone rotations to retain limb lengths.
static void AnimateCarEntry(UPoseableMeshComponent* Visual, const FReferenceSkeleton& Ref,
 const TArray<FTransform>& Standing, const TArray<FTransform>& Seated, float Age, FVector DoorHandle)
{
 if(Standing.Num()!=Ref.GetNum()||Seated.Num()!=Ref.GetNum())return;
 // Short entry only animates the door-opening arm. Preserve the idle body pose.
 TArray<FTransform> Pose=Standing;
 auto Rotate=[&](int32 Joint,FQuat Delta)
 {
  if(Joint<0)return;
  const FVector Pivot=Pose[Joint].GetLocation();
  for(int32 I=0;I<Pose.Num();++I)
  {
   int32 P=I;while(P>=0&&P!=Joint)P=Ref.GetParentIndex(P);
   if(P!=Joint)continue;
   Pose[I].SetLocation(Pivot+Delta.RotateVector(Pose[I].GetLocation()-Pivot));
   Pose[I].SetRotation((Delta*Pose[I].GetRotation()).GetNormalized());
  }
 };
 auto Solve=[&](const TCHAR* Upper,const TCHAR* Lower,const TCHAR* End,FVector Target,FVector Pole,FQuat EndRotation)
 {
  const int32 A=Ref.FindBoneIndex(Upper),B=Ref.FindBoneIndex(Lower),C=Ref.FindBoneIndex(End);
  if(A<0||B<0||C<0)return;
  const FVector Origin=Pose[A].GetLocation();
  const float L1=FVector::Distance(Origin,Pose[B].GetLocation()),L2=FVector::Distance(Pose[B].GetLocation(),Pose[C].GetLocation());
  const FVector Direction=(Target-Origin).GetSafeNormal();
  const float D=FMath::Clamp(float(FVector::Distance(Origin,Target)),FMath::Abs(L1-L2)+.1f,L1+L2-.1f);
  Target=Origin+Direction*D;
  const float Along=(L1*L1-L2*L2+D*D)/(2*D);
  const FVector Bend=(Pole-Direction*FVector::DotProduct(Pole,Direction)).GetSafeNormal();
  const FVector Knee=Origin+Direction*Along+Bend*FMath::Sqrt(FMath::Max(0.f,L1*L1-Along*Along));
  Rotate(A,FQuat::FindBetweenVectors(Pose[B].GetLocation()-Origin,Knee-Origin));
  Rotate(B,FQuat::FindBetweenVectors(Pose[C].GetLocation()-Pose[B].GetLocation(),Target-Pose[B].GetLocation()));
  Rotate(C,EndRotation*Pose[C].GetRotation().Inverse());
 };
 const int32 Hand=Ref.FindBoneIndex(TEXT("hand_l"));
 if(Hand>=0)
 {
  const float Weight=FMath::SmoothStep(.05f,.5f,Age);
  if(Weight>0.f)Solve(TEXT("upperarm_l"),TEXT("lowerarm_l"),TEXT("hand_l"),FMath::Lerp(Pose[Hand].GetLocation(),DoorHandle,Weight),
   FVector(-.4f,-1,-.5f),Pose[Hand].GetRotation());
 }
 for(int32 I=0;I<Pose.Num();++I)
 {
  const int32 P=Ref.GetParentIndex(I);
  Visual->BoneSpaceTransforms[I]=P<0?Pose[I]:Pose[I].GetRelativeTransform(Pose[P]);
 }
 Visual->MarkRefreshTransformDirty();Visual->RefreshBoneTransforms();Visual->UpdateBounds();Visual->MarkRenderDynamicDataDirty();
}
