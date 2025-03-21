// Fill out your copyright notice in the Description page of Project Settings.


#include "SlimeNavGridBuilder.h"
#include "Components/BoxComponent.h"
#include "Engine/OverlapResult.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"


DEFINE_LOG_CATEGORY(SlimeNAVGRID_LOG);

// Sets default values
ASlimeNavGridBuilder::ASlimeNavGridBuilder()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	VolumeBox = CreateDefaultSubobject<UBoxComponent>(TEXT("VolumeBox"));
	VolumeBox->BodyInstance.SetCollisionProfileName("Custom");
	VolumeBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	VolumeBox->BodyInstance.SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VolumeBox->SetWorldScale3D(FVector(50.f, 50.f, 50.f));
	RootComponent = VolumeBox;

	GridStepSize = 100.0f;
	bUseActorWhiteList = false;
	bUseActorBlackList = false;
	bAutoRemoveTracers = true;
	bAutoSaveGrid = true;

	TracerActorBP = ASlimeNavGridTracer::StaticClass();
	NavPointActorBP = ASlimeNavPoint::StaticClass();
	NavPointEdgeActorBP = ASlimeNavPointEdge::StaticClass();

	BounceNavDistance = 3.0f;
	TraceDistanceModificator = 1.5f;
	ClosePointsFilterModificator = 0.1f;
	ConnectionSphereRadiusModificator = 1.5f;
	TraceDistanceForEdgesModificator = 1.9f;
	EgdeDeviationModificator = 0.15f;
	TracersInVolumesCheckDistance = 100000.0f;
	bShouldTryToRemoveTracersEnclosedInVolumes = false;
}

// Called when the game starts or when spawned
void ASlimeNavGridBuilder::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ASlimeNavGridBuilder::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

int32 ASlimeNavGridBuilder::BuildGrid()
{
	/*//Get time start
	double StartTime = FPlatformTime::Seconds();
	
	UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Empty All"));
	EmptyAll();
	UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Spawn tracers"));
	SpawnTracers();

	if (bShouldTryToRemoveTracersEnclosedInVolumes) {
		UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("RemoveTracersClosedInVolumes"));
		RemoveTracersClosedInVolumes();
	}

	UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Trace from all tracers"));
	TraceFromAllTracers();
	
	if (bAutoRemoveTracers) {
		UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Remove all tracers"));
		RemoveAllTracers();
	}
	
	
	UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Nav Points Locations = %d"), NavPointsLocations.Num());

	UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Spawn nav points"));
	GLog->FlushThreadedLogs();
	SpawnNavPoints();
	UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Build Relations"));
	GLog->FlushThreadedLogs();
	BuildRelations();
	UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Build edge relations"));
	GLog->FlushThreadedLogs();
	BuildPossibleEdgeRelations();
	UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("End of build edge relations"));
	GLog->FlushThreadedLogs();

	UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("RemoveNoConnected"));
	RemoveNoConnected();

	if (bAutoSaveGrid) {
		UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Saving grid"));
		SaveGrid();
	}

	//DrawDebugRelations();
	UE_LOG(SlimeNAVGRID_LOG, Warning, TEXT("Grid has been build. Nav Points = %d"), NavPoints.Num());
	//print duration
	double EndTime = FPlatformTime::Seconds();
	double Duration = EndTime - StartTime;
	UE_LOG(SlimeNAVGRID_LOG, Warning, TEXT("Duration: %f"), Duration);*/

	// Start measuring total build time
	BuildStartTime = FPlatformTime::Seconds();

	// Begin the first step
	CurrentBuildStep = EGridBuildStep::EmptyAll;

	// Register a ticker that fires every frame to process the next step
	FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &ASlimeNavGridBuilder::TickBuildSteps),
		0.0f  // Tick every frame
	);
	
	
	return NavPoints.Num();
}

bool ASlimeNavGridBuilder::TickBuildSteps(float DeltaTime)
{
    switch (CurrentBuildStep)
    {
    case EGridBuildStep::EmptyAll:
    {
        UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Emptying All (Tracers + NavPoints)..."));
        EmptyAll();
        CurrentBuildStep = EGridBuildStep::SpawnTracers;
        break;
    }

    case EGridBuildStep::SpawnTracers:
    {
        UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Spawning tracers..."));
        SpawnTracers();
        CurrentBuildStep = (bShouldTryToRemoveTracersEnclosedInVolumes)
            ? EGridBuildStep::RemoveTracersEnclosed
            : EGridBuildStep::TraceFromTracers;
        break;
    }

    case EGridBuildStep::RemoveTracersEnclosed:
    {
        UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Removing enclosed tracers..."));
        RemoveTracersClosedInVolumes();
        CurrentBuildStep = EGridBuildStep::TraceFromTracers;
        break;
    }

    case EGridBuildStep::TraceFromTracers:
    {
        UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Tracing from all tracers..."));
        TraceFromAllTracers();
        CurrentBuildStep = bAutoRemoveTracers ? EGridBuildStep::RemoveAllTracers : EGridBuildStep::SpawnNavPoints;
        break;
    }

    case EGridBuildStep::RemoveAllTracers:
    {
        UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Removing all tracers..."));
        RemoveAllTracers();
        CurrentBuildStep = EGridBuildStep::SpawnNavPoints;
        break;
    }

    case EGridBuildStep::SpawnNavPoints:
    {
        UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Spawning nav points..."));
        SpawnNavPoints();
        CurrentBuildStep = EGridBuildStep::BuildRelations;
        break;
    }

    case EGridBuildStep::BuildRelations:
    {
        UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Building direct relations..."));
        BuildRelations();
        CurrentBuildStep = EGridBuildStep::BuildEdgeRelations;
        break;
    }

    case EGridBuildStep::BuildEdgeRelations:
    {
        UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Building edge relations..."));
        BuildPossibleEdgeRelations();
        CurrentBuildStep = EGridBuildStep::RemoveNoConnected;
        break;
    }

    case EGridBuildStep::RemoveNoConnected:
    {
        UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Removing disconnected points..."));
        RemoveNoConnected();
        CurrentBuildStep = bAutoSaveGrid ? EGridBuildStep::SaveGrid : EGridBuildStep::Done;
        break;
    }

    case EGridBuildStep::SaveGrid:
    {
        UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Saving grid..."));
        SaveGrid();
        CurrentBuildStep = EGridBuildStep::Done;
        break;
    }

    case EGridBuildStep::Done:
    {
        double EndTime = FPlatformTime::Seconds();
        UE_LOG(SlimeNAVGRID_LOG, Warning, TEXT("Grid build complete. Nav Points = %d"), NavPoints.Num());
        UE_LOG(SlimeNAVGRID_LOG, Warning, TEXT("Total duration: %f seconds"), EndTime - BuildStartTime);

        // Return false => remove this ticker from the engine so we stop ticking
        return false;
    }

    default:
        // Just in case
        return false;
    }

    // Return true => keep ticking on next frame
    return true;
}



void ASlimeNavGridBuilder::SpawnTracers()
{
	FVector Origin;
	FVector BoxExtent;
	GetActorBounds(false, Origin, BoxExtent);
	FVector GridStart = Origin - BoxExtent;
	FVector GridEnd = Origin + BoxExtent;
	

	//Create a Spawn Parameters struct
	FActorSpawnParameters SpawnParams;
	SpawnParams.bNoFail = false;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
	FRotator DefaultRotator = FRotator(0.0f, 0.0f, 0.0f);
	
	FVector TracerLocation;
	
	ASlimeNavGridTracer* Tracer = NULL;
	
	for (float x = GridStart.X; x < GridEnd.X; x = x + GridStepSize) {
		TracerLocation.X = x;
		for (float y = GridStart.Y; y < GridEnd.Y; y = y + GridStepSize) {
			TracerLocation.Y = y;
			for (float z = GridStart.Z; z < GridEnd.Z; z = z + GridStepSize) {
				TracerLocation.Z = z;
				Tracer = GetWorld()->SpawnActor<ASlimeNavGridTracer>(TracerActorBP, TracerLocation, DefaultRotator, SpawnParams);
				if (Tracer) {
					Tracers.Add(Tracer);
				}
			}
		}
		
	}
}

void ASlimeNavGridBuilder::RemoveTracersClosedInVolumes()
{
	FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), false, this);
	RV_TraceParams.bTraceComplex = false;
	RV_TraceParams.bReturnPhysicalMaterial = false;

	//ignore all tracers
	TArray<AActor*> ActorsToIgnore;
	for (int32 i = 0; i < Tracers.Num(); ++i) {
		AActor* Actor = Cast<AActor>(Tracers[i]);
		ActorsToIgnore.Add(Actor);
	}
	RV_TraceParams.AddIgnoredActors(ActorsToIgnore);

	float TraceDistance = TracersInVolumesCheckDistance;

	int32 RemovedNum = 0;

	for (int32 i = 0; i < Tracers.Num(); ++i) {
		ASlimeNavGridTracer* Tracer = Tracers[i];

		FVector StartLocation = Tracer->GetActorLocation();
		FVector EndLocation;

		TArray<AActor*> ActorsBumpedIn;

		for (int32 x = -1; x <= 1; x++) {
			for (int32 y = -1; y <= 1; y++) {
				for (int32 z = -1; z <= 1; z++) {
					FVector Direction = FVector(x, y, z);
					if (Direction.Size() == 1) {

						EndLocation = StartLocation + Direction * TraceDistance;
						FHitResult RV_Hit(ForceInit);

						GetWorld()->LineTraceSingleByChannel(
							RV_Hit,
							StartLocation,
							EndLocation,
							ECC_WorldStatic,
							RV_TraceParams
						);

						if (RV_Hit.bBlockingHit) {
							AActor* BlockingActor = RV_Hit.GetActor();
							ActorsBumpedIn.Add(BlockingActor);
						}
						
					}
				}
			}
		}

		// remove iterated tracer if there is the same actor around the tracer in 6 directions
		if (ActorsBumpedIn.Num() == 6) {
			AActor* FirstActor = ActorsBumpedIn[0];
			bool bFoundDifferent = false;
			for (AActor* ActorBumpedIn : ActorsBumpedIn)
			{
				if (ActorBumpedIn != FirstActor) {
					bFoundDifferent = true;
				}
			}
			if (!bFoundDifferent) {
				Tracers.Remove(Tracer);
				RemovedNum++;
			}
		}

	}

	UE_LOG(SlimeNAVGRID_LOG, Warning, TEXT("Removed enclosed tracers: %d"), RemovedNum);
}

void ASlimeNavGridBuilder::TraceFromAllTracers()
{
	FCollisionQueryParams RV_TraceParams = FCollisionQueryParams(FName(TEXT("RV_Trace")), false, this);
	RV_TraceParams.bTraceComplex = false;
	RV_TraceParams.bReturnPhysicalMaterial = false;

	TArray<AActor*> ActorsToIgnore;
	for (int32 i = 0; i < Tracers.Num(); ++i) {
		AActor* Actor = Cast<AActor>(Tracers[i]);
		ActorsToIgnore.Add(Actor);
	}
	RV_TraceParams.AddIgnoredActors(ActorsToIgnore);

	float TraceDistance = GridStepSize * TraceDistanceModificator;

	for (int32 i = 0; i < Tracers.Num(); ++i) {
		ASlimeNavGridTracer* Tracer = Tracers[i];

		FVector StartLocation = Tracer->GetActorLocation();
		FVector EndLocation;

		for (int32 x = -1; x <= 1; x++) {
			for (int32 y = -1; y <= 1; y++) {
				for (int32 z = -1; z <= 1; z++) {
					FVector Direction = FVector(x, y, z);
					if (Direction.Size() == 1) {
						
						EndLocation = StartLocation + Direction * TraceDistance;

						//Re-initialize hit info
						FHitResult RV_Hit(ForceInit);

						//call GetWorld() from within an actor extending class
						GetWorld()->LineTraceSingleByChannel(
							RV_Hit,        //result
							StartLocation,    //start
							EndLocation, //end
							ECC_GameTraceChannel5, //collision channel
							RV_TraceParams
						);

						AddNavPointByHitResult(RV_Hit);
					}
				}
			}
		}

		
	}
}

void ASlimeNavGridBuilder::AddNavPointByHitResult(FHitResult RV_Hit)
{
	if (RV_Hit.bBlockingHit) {

		AActor* BlockingActor = RV_Hit.GetActor();

		if (bUseActorWhiteList) {
			bool bFoundFromWhiteList = false;
			for (int32 i = 0; i < ActorsWhiteList.Num(); ++i) {
				if (ActorsWhiteList[i] == BlockingActor) {
					bFoundFromWhiteList = true;
					break;
				}
			}
			if (!bFoundFromWhiteList) {
				return;
			}
		}

		if (bUseActorBlackList) {
			for (AActor* BlackListedActor : ActorsBlackList) {
				if (BlockingActor == BlackListedActor) {
					return;
				}
			}
		}

		FVector NavPointLocation = RV_Hit.Location + RV_Hit.Normal * BounceNavDistance;
		bool bIsTooClose = false;

		for (int32 i = 0; i < NavPointsLocations.Num(); i++) {
			float Distance = (NavPointsLocations[i] - NavPointLocation).Size();
			if (Distance < GridStepSize * ClosePointsFilterModificator) {
				bIsTooClose = true;
				break;
			}
		}

		if (!bIsTooClose) {
			int32 PointIndex = NavPointsLocations.Add(NavPointLocation);
			NavPointsNormals.Add(PointIndex, RV_Hit.Normal);
		}
	}
}

void ASlimeNavGridBuilder::SpawnNavPoints()
{
	FActorSpawnParameters SpawnParams;
	SpawnParams.bNoFail = true;
	FRotator DefaultRotator = FRotator(0.0f, 0.0f, 0.0f);
	ASlimeNavPoint* NavPoint = NULL;

	for (int32 i = 0; i < NavPointsLocations.Num(); i++) {
		NavPoint = GetWorld()->SpawnActor<ASlimeNavPoint>(NavPointActorBP, NavPointsLocations[i], DefaultRotator, SpawnParams);
		if (NavPoint) {
			NavPoints.Add(NavPoint);
			FVector* Normal = NavPointsNormals.Find(i);
			if (Normal) {
				NavPoint->Normal = *Normal;
			}
		}
	}
}

void ASlimeNavGridBuilder::BuildRelations()
{
	FCollisionObjectQueryParams FCOQP = FCollisionObjectQueryParams(ECC_TO_BITFIELD(ECC_WorldDynamic));
	
	const FCollisionShape Sphere = FCollisionShape::MakeSphere(GridStepSize * ConnectionSphereRadiusModificator);
	

	ASlimeNavPoint* NavPoint = NULL;
	for (int32 i = 0; i < NavPoints.Num(); ++i) {
		NavPoint = NavPoints[i];
		FCollisionQueryParams FCQP = FCollisionQueryParams(FName(TEXT("RV_Trace")), false, this);
		FCQP.AddIgnoredActor(NavPoint);
		TArray < FOverlapResult > OutOverlaps;
		bool bIsOverlap = GetWorld()->OverlapMultiByObjectType(
			OutOverlaps,
			NavPoint->GetActorLocation(),
			FQuat::Identity, 
			FCOQP,
			Sphere,
			FCQP
		);

		if (bIsOverlap) {
			for (int32 j = 0; j < OutOverlaps.Num(); ++j) {
				FOverlapResult OverlapResult = OutOverlaps[j];
				AActor* OverlappedActor = OverlapResult.GetActor();
				if (OverlappedActor->IsA(ASlimeNavPoint::StaticClass())) {
					ASlimeNavPoint* OverlappedNavPoint = Cast<ASlimeNavPoint>(OverlappedActor);
					bool bIsVisible = CheckNavPointsVisibility(NavPoint, OverlappedNavPoint);
					if (bIsVisible) {
						NavPoint->Neighbors.AddUnique(OverlappedNavPoint);
						OverlappedNavPoint->Neighbors.AddUnique(NavPoint);
					} else {
						NavPoint->PossibleEdgeNeighbors.AddUnique(OverlappedNavPoint);
					}
					
				}
			}
		}
	}
}

void ASlimeNavGridBuilder::DrawDebugRelations()
{
	ASlimeNavPoint* NavPoint = NULL;
	ASlimeNavPoint* NeighborNavPoint = NULL;

	for (int32 i = 0; i < NavPoints.Num(); ++i) {
		NavPoint = NavPoints[i];

		FColor DrawColor = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f).ToFColor(true);
		FColor DrawColorNormal = FLinearColor(0.0f, 1.0f, 1.0f, 1.0f).ToFColor(true);
		float DrawDuration = 10.0f;
		bool DrawShadow = false;
		//DrawDebugString(GEngine->GetWorldFromContextObject(this), NavPoint->GetActorLocation(), *FString::Printf(TEXT("[%d] - [%d]"), NavPoint->Neighbors.Num(), NavPoint->PossibleEdgeNeighbors.Num()), NULL, DrawColor, DrawDuration, DrawShadow);

		DrawDebugLine(
			GetWorld(),
			NavPoint->GetActorLocation(),
			NavPoint->GetActorLocation() + NavPoint->Normal * 70.0f,
			DrawColorNormal,
			false,
			DrawDuration,
			0,
			DebugThickness
		);

		for (int32 j = 0; j < NavPoint->Neighbors.Num(); ++j) {
			NeighborNavPoint = NavPoint->Neighbors[j];
			DrawDebugLine(
				GetWorld(),
				NavPoint->GetActorLocation(),
				NeighborNavPoint->GetActorLocation(),
				DrawColor,
				false,
				DrawDuration,
				0,
				DebugThickness
			);
		}

	}
}

/** 
* Returns true if navpoints could see each other 
*/
bool ASlimeNavGridBuilder::CheckNavPointsVisibility(ASlimeNavPoint* NavPoint1, ASlimeNavPoint* NavPoint2)
{
	FHitResult OutHit;
	ECollisionChannel TraceChannel = ECollisionChannel::ECC_Visibility;

	FCollisionQueryParams TraceQueryParams = FCollisionQueryParams(FName(TEXT("RV_Trace_NavPoints")), false, this);
	TraceQueryParams.bTraceComplex = false;
	TraceQueryParams.bReturnPhysicalMaterial = false;
	TraceQueryParams.AddIgnoredActor(NavPoint1);

	bool bBlockingFound = GetWorld()->LineTraceSingleByChannel
	(
		OutHit,
		NavPoint1->GetActorLocation(),
		NavPoint2->GetActorLocation(),
		TraceChannel,
		TraceQueryParams
	);


	if (!bBlockingFound) {
		return false;
	}

	AActor* BlockedActor = OutHit.GetActor();
	if (!BlockedActor) {
		return false;
	}

	if (BlockedActor == NavPoint2) {
		return true;
	}

	return false;
}

/**
* Returns true if locations could see each other
*/
bool ASlimeNavGridBuilder::CheckNavPointCanSeeLocation(ASlimeNavPoint* NavPoint, FVector Location)
{
	FHitResult OutHit;
	ECollisionChannel TraceChannel = ECollisionChannel::ECC_Visibility;

	FCollisionQueryParams TraceQueryParams = FCollisionQueryParams(FName(TEXT("RV_Trace_Locations")), false, this);
	TraceQueryParams.bTraceComplex = false;
	TraceQueryParams.bReturnPhysicalMaterial = false;
	TraceQueryParams.AddIgnoredActor(NavPoint);

	bool bBlockingFound = GetWorld()->LineTraceSingleByChannel
	(
		OutHit,
		NavPoint->GetActorLocation(),
		Location,
		TraceChannel,
		TraceQueryParams
	);

	return !bBlockingFound;
}

/** 
* Finds nav points on egdes of boxes (i.e. from side to top) finding intersection of pack of ortogonal vectors from both points.
*/
void ASlimeNavGridBuilder::BuildPossibleEdgeRelations()
{
	ASlimeNavPoint* NavPoint = NULL;
	ASlimeNavPoint* PossibleNavPoint = NULL;
	for (int32 i = 0; i < NavPoints.Num(); ++i) {
		NavPoint = NavPoints[i];
		for (int32 j = 0; j < NavPoint->PossibleEdgeNeighbors.Num(); ++j) {
			PossibleNavPoint = NavPoint->PossibleEdgeNeighbors[j];
			for (int32 x1 = -1; x1 <= 1; x1++) {
				for (int32 y1 = -1; y1 <= 1; y1++) {
					for (int32 z1 = -1; z1 <= 1; z1++) {
						for (int32 x2 = -1; x2 <= 1; x2++) {
							for (int32 y2 = -1; y2 <= 1; y2++) {
								for (int32 z2 = -1; z2 <= 1; z2++) {
									FVector Direction1 = FVector(x1, y1, z1);
									FVector Direction2 = FVector(x2, y2, z2);
									bool bOnlyOneNonZero = (Direction1.Size() == 1 && Direction2.Size() == 1);
									bool bCorrespingValuesAreNotEqual = ((Direction1.GetAbs() - Direction2.GetAbs()).Size() != 0);
									if (bOnlyOneNonZero && bCorrespingValuesAreNotEqual) {
										FVector Start0 = NavPoint->GetActorLocation();
										FVector End0 = NavPoint->GetActorLocation() + Direction1 * GridStepSize * TraceDistanceForEdgesModificator;
										FVector Start1 = PossibleNavPoint->GetActorLocation();
										FVector End1 = PossibleNavPoint->GetActorLocation() + Direction2 * GridStepSize * TraceDistanceForEdgesModificator;
										FVector Intersection;
										bool bIntersect = GetLineLineIntersection(Start0, End0, Start1, End1, Intersection);
										if (bIntersect) {
											CheckAndAddIntersectionNavPointEdge(Intersection, NavPoint, PossibleNavPoint);
										}
									}
								}
							}
						}
					}
				}
			}
		}
		NavPoint->PossibleEdgeNeighbors.Empty();
	}
}

bool ASlimeNavGridBuilder::GetLineLineIntersection(FVector Start0, FVector End0, FVector Start1, FVector End1, FVector& Intersection)
{
	TMap<int32, FVector> v;
	v.Add(0, Start0);
	v.Add(1, End0);
	v.Add(2, Start1);
	v.Add(3, End1);

	float d0232 = Dmnop(&v, 0, 2, 3, 2);
	float d3210 = Dmnop(&v, 3, 2, 1, 0);
	float d3232 = Dmnop(&v, 3, 2, 3, 2);
	float mu = (d0232 * d3210 - Dmnop(&v, 0, 2, 1, 0)*d3232) / (Dmnop(&v, 1, 0, 1, 0) * Dmnop(&v, 3, 2, 3, 2) - Dmnop(&v, 3, 2, 1, 0) * Dmnop(&v, 3, 2, 1, 0));
    float NormalisedDistanceLine1 = (d0232 + mu * d3210) / d3232;
		 
	
	FVector ClosestPointLine0 = Start0 + mu * (End0 - Start0);
	FVector ClosestPointLine1 = Start1 + NormalisedDistanceLine1 * (End1 - Start1);

	float Distance = (ClosestPointLine0 - ClosestPointLine1).Size();

	bool bItersectionOnSegment1 = (NormalisedDistanceLine1 > 0);

	//@TODO: check if 3 points are collinear?
	
	if (Distance < GridStepSize * EgdeDeviationModificator && bItersectionOnSegment1) {
		Intersection = ClosestPointLine0;
		return true;
	}

	return false;
}

float ASlimeNavGridBuilder::Dmnop(const TMap<int32, FVector>* v, int32 m, int32 n, int32 o, int32 p)
{
	const FVector* vm = v->Find(m);
	const FVector* vn = v->Find(n);
	const FVector* vo = v->Find(o);
	const FVector* vp = v->Find(p);
	return (vm->X - vn->X) * (vo->X - vp->X) + (vm->Y -vn->Y) * (vo->Y - vp->Y) + (vm->Z - vn->Z) * (vo->Z - vp->Z);
}

void ASlimeNavGridBuilder::CheckAndAddIntersectionNavPointEdge(FVector Intersection, ASlimeNavPoint* NavPoint1, ASlimeNavPoint* NavPoint2)
{

	bool bFirstSee = CheckNavPointCanSeeLocation(NavPoint1, Intersection);
	if (!bFirstSee) {
		return;
	}
	bool bSecondSee = CheckNavPointCanSeeLocation(NavPoint2, Intersection);
	if (!bSecondSee) {
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.bNoFail = false;
	FRotator DefaultRotator = FRotator(0.0f, 0.0f, 0.0f);

	ASlimeNavPointEdge* NavPointEdge = GetWorld()->SpawnActor<ASlimeNavPointEdge>(NavPointEdgeActorBP, Intersection, DefaultRotator, SpawnParams);
	if (NavPointEdge) {
		NavPoints.Add(NavPointEdge);
		NavPoint1->Neighbors.Add(NavPointEdge);
		NavPoint2->Neighbors.Add(NavPointEdge);
		NavPointEdge->Neighbors.Add(NavPoint1);
		NavPointEdge->Neighbors.Add(NavPoint2);

		FVector Normal = NavPoint1->Normal + NavPoint2->Normal;
		Normal.Normalize();
		NavPointEdge->Normal = Normal;
	}
}

void ASlimeNavGridBuilder::RemoveAllTracers()
{
	for (int32 i = 0; i < Tracers.Num(); ++i) {
		Tracers[i]->Destroy();
	}
	Tracers.Empty();
}

void ASlimeNavGridBuilder::SaveGrid()
{
	TMap<int32, FVector> NavLocations;
	TMap<int32, FVector> NavNormals;
	TMap<int32, FSlimeNavRelations> NavRelations;

	ASlimeNavPoint* NavPoint = NULL;
	ASlimeNavPoint* NeighborNavPoint = NULL;

	for (int32 i = 0; i < NavPoints.Num(); ++i) {
		NavPoint = NavPoints[i];
		NavLocations.Add(i, NavPoint->GetActorLocation());
		NavNormals.Add(i, NavPoint->Normal);
		FSlimeNavRelations SlimeNavRelations;
		for (int32 j = 0; j < NavPoint->Neighbors.Num(); ++j) {
			int32 NeighborIndex = GetNavPointIndex(NavPoint->Neighbors[j]);
			SlimeNavRelations.Neighbors.Add(NeighborIndex);
		}
		NavRelations.Add(i, SlimeNavRelations);
	}

	//Get the current level name
	FString CurrentLevel = UGameplayStatics::GetCurrentLevelName(GetWorld(), true);
	USlimeNavGridSaveGame* SaveGameInstance = Cast<USlimeNavGridSaveGame>(UGameplayStatics::CreateSaveGameObject(USlimeNavGridSaveGame::StaticClass()));
	SaveGameInstance->NavLocations = NavLocations;
	SaveGameInstance->NavNormals = NavNormals;
	SaveGameInstance->NavRelations = NavRelations;
	SaveGameInstance->SaveSlotName = TEXT("SlimeNavGrid") + CurrentLevel;
	UGameplayStatics::SaveGameToSlot(SaveGameInstance, SaveGameInstance->SaveSlotName, SaveGameInstance->UserIndex);

	if (bSaveToContentFolder)
	{
		CopySaveGameToContent(SaveGameInstance->SaveSlotName);
	}
}

int32 ASlimeNavGridBuilder::GetNavPointIndex(ASlimeNavPoint* NavPoint)
{
	for (int32 i = 0; i < NavPoints.Num(); ++i) {
		if (NavPoints[i] == NavPoint) {
			return i;
		}
	}
	return -1;
}

void ASlimeNavGridBuilder::RemoveNoConnected()
{
	TArray<ASlimeNavPoint*> FilteredNavPoints;
	ASlimeNavPoint* NavPoint = NULL;

	for (int32 i = 0; i < NavPoints.Num(); ++i) {
		NavPoint = NavPoints[i];

		if (NavPoint->Neighbors.Num() > 1) {
			FilteredNavPoints.Add(NavPoint);
		} else {
			NavPoint->Destroy();
		}
	}

	NavPoints = FilteredNavPoints;
}

void ASlimeNavGridBuilder::RemoveAllNavPoints()
{
	ASlimeNavPoint* NavPoint = NULL;
	for (int32 i = 0; i < NavPoints.Num(); ++i) {
		NavPoint = NavPoints[i];
		NavPoint->Destroy();
	}
	NavPoints.Empty();
}

void ASlimeNavGridBuilder::EmptyAll()
{
	RemoveAllTracers();
	RemoveAllNavPoints();
	NavPointsNormals.Empty();
	NavPointsLocations.Empty();
}

bool ASlimeNavGridBuilder::CopySaveGameToContent(const FString& SaveSlotName)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    // Source file: Get the saved game file path
    FString SavedDir = FPaths::ProjectSavedDir() + TEXT("SaveGames/");
    FString SourceFile = SavedDir + SaveSlotName + TEXT(".sav");
    
    // Make sure source file exists
    if (!PlatformFile.FileExists(*SourceFile))
    {
        UE_LOG(SlimeNAVGRID_LOG, Error, TEXT("SaveGame file not found: %s"), *SourceFile);
        return false;
    }
    
    // Destination file: Create path in content directory
    FString ContentDir = FPaths::ProjectContentDir() + ContentFolderPath + TEXT("/");
    FString DestFile = ContentDir + SaveSlotName + TEXT(".sav");
    
    // Make sure destination directory exists
    if (!PlatformFile.DirectoryExists(*ContentDir))
    {
        if (!PlatformFile.CreateDirectoryTree(*ContentDir))
        {
            UE_LOG(SlimeNAVGRID_LOG, Error, TEXT("Failed to create directory: %s"), *ContentDir);
            return false;
        }
    }
    
    // Copy the file
    if (PlatformFile.CopyFile(*DestFile, *SourceFile))
    {
        UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Successfully copied SaveGame to content: %s"), *DestFile);
        return true;
    }
    else
    {
        UE_LOG(SlimeNAVGRID_LOG, Error, TEXT("Failed to copy SaveGame to content: %s"), *DestFile);
        return false;
    }
}

bool ASlimeNavGridBuilder::CopySaveGameFromContent(const FString& SaveSlotName)
{
      IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    
    // Ensure ContentFolderPath is valid
    if (ContentFolderPath.IsEmpty())
    {
        ContentFolderPath = TEXT("NavData");
    }
    
	// Try multiple possible source locations in order of likelihood
	TArray<FString> PossibleSourcePaths;
    
	// Path if we're in the editor not in PIE
	PossibleSourcePaths.Add(FPaths::Combine(FPaths::ProjectContentDir(), ContentFolderPath, SaveSlotName + TEXT(".sav")));
    
	// Path for PIE and Editor when testing
	PossibleSourcePaths.Add(FPaths::Combine(FPaths::ProjectDir(), TEXT("Content"), ContentFolderPath, SaveSlotName+TEXT(".sav")));
    
	// Path for packaged game
	if (FApp::IsGame() && !GIsEditor)
	{
		PossibleSourcePaths.Add(FPaths::Combine(FPaths::LaunchDir(), TEXT("Content"), ContentFolderPath, SaveSlotName+ TEXT(".sav")));
		PossibleSourcePaths.Add(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("../../../"), FApp::GetProjectName(), TEXT("Content"), ContentFolderPath, SaveSlotName+ TEXT(".sav")));
	}
	
    // Find the first valid source path
    FString ContentSourcePath;
    bool bFoundSource = false;
    
    for (const FString& Path : PossibleSourcePaths)
    {
        UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Checking path: %s"), *Path);
        if (PlatformFile.FileExists(*Path))
        {
            ContentSourcePath = Path;
            bFoundSource = true;
            UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Found content file at: %s"), *ContentSourcePath);
            break;
        }
    }
    
    if (!bFoundSource)
    {
        UE_LOG(SlimeNAVGRID_LOG, Warning, TEXT("Content SaveGame file not found. Checked paths:"));
        for (const FString& Path : PossibleSourcePaths)
        {
            UE_LOG(SlimeNAVGRID_LOG, Warning, TEXT("  - %s"), *Path);
        }
        return false;
    }
    
    // Safely get SaveGames directory
    FString SavedDir;
    
    if (GIsEditor)
    {
        // In editor (including PIE)
        SavedDir = FPaths::ProjectSavedDir() / TEXT("SaveGames/");
    }
    else
    {
        // Packaged game
        SavedDir = FPaths::ProjectSavedDir() / TEXT("SaveGames/");
        
        // Alternative path for packaged game if the above doesn't exist
        if (!PlatformFile.DirectoryExists(*SavedDir))
        {
            SavedDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"));
        }
    }
    
    // Make sure destination directory exists
    if (!PlatformFile.DirectoryExists(*SavedDir))
    {
        if (!PlatformFile.CreateDirectoryTree(*SavedDir))
        {
            UE_LOG(SlimeNAVGRID_LOG, Error, TEXT("Failed to create directory: %s"), *SavedDir);
            return false;
        }
    }
    
    FString DestFile = FPaths::Combine(SavedDir, SaveSlotName+ TEXT(".sav"));

	
    
    // Copy the file
    if (PlatformFile.CopyFile(*DestFile, *ContentSourcePath))
    {
        UE_LOG(SlimeNAVGRID_LOG, Log, TEXT("Successfully copied SaveGame from content to: %s"), *DestFile);
        return true;
    }
    else
    {
        UE_LOG(SlimeNAVGRID_LOG, Error, TEXT("Failed to copy SaveGame from content to: %s"), *DestFile);
        return false;
    }
}

